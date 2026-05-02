#!/usr/bin/env python3
"""Extract Gloom Amiga visual assets to PNG files under amiga/art."""

from __future__ import annotations

import argparse
import json
import math
import shutil
import subprocess
import tempfile
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[1]
AMIGA = ROOT / "amiga"
DEFAULT_OUT = AMIGA / "art"

TEXTURE_W = 64
TEXTURE_H = 64
TEXTURE_COLUMN_BYTES = 65
FLAT_SIZE = 128


def be16(data: bytes | bytearray, offset: int) -> int:
    return (data[offset] << 8) | data[offset + 1]


def be32(data: bytes | bytearray, offset: int) -> int:
    return (data[offset] << 24) | (data[offset + 1] << 16) | (data[offset + 2] << 8) | data[offset + 3]


def s16(value: int) -> int:
    value &= 0xFFFF
    return value - 0x10000 if value & 0x8000 else value


def rotl32(value: int, count: int) -> int:
    value &= 0xFFFFFFFF
    count &= 31
    if count == 0:
        return value
    return ((value << count) & 0xFFFFFFFF) | (value >> (32 - count))


def rotr32(value: int, count: int) -> int:
    value &= 0xFFFFFFFF
    count &= 31
    if count == 0:
        return value
    return (value >> count) | ((value << (32 - count)) & 0xFFFFFFFF)


def mask_for_bits(bit_count: int) -> int:
    return 0xFFFF if bit_count >= 16 else (1 << bit_count) - 1


class CrmError(RuntimeError):
    pass


class CrmBitReader:
    def __init__(self, data: bytes, source_floor: int, source_offset: int, source_size: int) -> None:
        if source_size < 6:
            raise CrmError(f"CrM stream too small ({source_size} bytes)")
        self.data = data
        self.source_start = source_floor
        self.source_cursor = source_offset + source_size
        self.word = 0
        self.bits_left = 0
        bits_in_last_word = self.read_be16_back()
        trailing_word = self.read_be32_back()
        if bits_in_last_word > 16:
            raise CrmError(f"CrM stream has invalid trailing bit count {bits_in_last_word}")
        self.word = (trailing_word >> (16 - bits_in_last_word)) & 0xFFFFFFFF
        self.bits_left = bits_in_last_word

    def read_be16_back(self) -> int:
        if self.source_cursor - self.source_start < 2:
            raise CrmError("CrM stream truncated while reading trailing 16-bit word")
        self.source_cursor -= 2
        return be16(self.data, self.source_cursor)

    def read_be32_back(self) -> int:
        if self.source_cursor - self.source_start < 4:
            raise CrmError("CrM stream truncated while reading trailing 32-bit word")
        self.source_cursor -= 4
        return be32(self.data, self.source_cursor)

    def next_bit(self) -> int:
        self.bits_left -= 1
        if self.bits_left == 0:
            old_low = self.word & 0xFFFF
            self.word >>= 1
            self.word = rotl32(self.word, 16)
            next_word = self.read_be16_back()
            self.word = (self.word & 0xFFFF0000) | next_word
            self.word = rotl32(self.word, 16)
            self.bits_left = 16
            return 1 if old_low & 1 else 0
        if self.bits_left < 0:
            raise CrmError("CrM bitreader underflow")
        bit = self.word & 1
        self.word >>= 1
        return bit

    def get_bits(self, bit_count: int) -> int:
        if bit_count <= 0 or bit_count > 16:
            raise CrmError(f"CrM invalid bit request ({bit_count})")
        bits = self.word & 0xFFFF
        self.word >>= bit_count
        self.bits_left -= bit_count
        if self.bits_left <= 0:
            self.bits_left += 16
            self.word = rotr32(self.word, self.bits_left)
            next_word = self.read_be16_back()
            self.word = (self.word & 0xFFFF0000) | next_word
            self.word = rotl32(self.word, self.bits_left)
        return bits & mask_for_bits(bit_count)


def crm_write_byte(dst: bytearray, cursor: int, value: int) -> int:
    if cursor <= 0:
        raise CrmError("CrM destination underflow while writing byte")
    cursor -= 1
    dst[cursor] = value & 0xFF
    return cursor


def crm_read_ref_byte(dst: bytearray, ref_cursor: int) -> tuple[int, int]:
    if ref_cursor <= 0 or ref_cursor > len(dst):
        raise CrmError("CrM back-reference out of range")
    ref_cursor -= 1
    return ref_cursor, dst[ref_cursor]


def crm_copy_ref_byte(dst: bytearray, cursor: int, ref_cursor: int) -> tuple[int, int]:
    ref_cursor, value = crm_read_ref_byte(dst, ref_cursor)
    cursor = crm_write_byte(dst, cursor, value)
    return cursor, ref_cursor


def crm_fast_decrunch(data: bytes, source_size: int, destination_size: int) -> bytes:
    br = CrmBitReader(data, 0, 14, source_size)
    dst = bytearray(destination_size)
    cursor = destination_size

    while cursor > 0:
        if br.next_bit() != 0:
            cursor = crm_write_byte(dst, cursor, br.get_bits(8))
            continue

        bit = br.next_bit()
        if bit == 0:
            run_bits = 1
            run_len = 1
        else:
            bit = br.next_bit()
            if bit == 0:
                run_bits = 2
                run_len = 3
            else:
                bit = br.next_bit()
                if bit == 0:
                    run_bits = 4
                    run_len = 7
                else:
                    run_bits = 8
                    run_len = 0x17

        run_len += br.get_bits(run_bits)
        if run_len == 22:
            literal_count_bits = 14 if br.next_bit() == 0 else 5
            run_len = 14 + br.get_bits(literal_count_bits)
            for _ in range(run_len + 1):
                cursor = crm_write_byte(dst, cursor, br.get_bits(8))
            continue

        if run_len >= 22:
            run_len -= 1

        bit = br.next_bit()
        if bit == 0:
            distance_bits = 9
            distance_base = 0x20
        else:
            bit = br.next_bit()
            if bit == 0:
                distance_bits = 5
                distance_base = 0
            else:
                distance_bits = 14
                distance_base = 0x220

        distance = distance_base + br.get_bits(distance_bits)
        ref_cursor = cursor + distance
        for _ in range(run_len + 1):
            cursor, ref_cursor = crm_copy_ref_byte(dst, cursor, ref_cursor)

    return bytes(dst)


def crm_read_tab(
    br: CrmBitReader,
    bit_counts: list[int],
    bit_counts_offset: int,
    real: list[int],
    real_offset: int,
    real_capacity: int,
    max_bits: int,
) -> None:
    count = br.get_bits(4)
    if count == 0 or count > 16:
        raise CrmError(f"CrM2 table bit-count length {count} is invalid")

    for i in range(16):
        bit_counts[bit_counts_offset + i] = 0

    total_real = 0
    for i in range(count):
        bits = i + 1 if (i + 1) <= max_bits else max_bits
        value = br.get_bits(bits)
        bit_counts[bit_counts_offset + i] = value
        total_real += value

    if total_real > real_capacity:
        raise CrmError(f"CrM2 table overflows real-table capacity ({total_real} > {real_capacity})")

    for i in range(total_real):
        real[real_offset + i] = br.get_bits(max_bits)


def crm_calc_cmp_tab(bit_counts: list[int], offset: int) -> tuple[list[int], list[int]]:
    cmp_table = [0] * 16
    add_table = [0] * 16
    code_sum = 0
    running_total = 0
    previous_cmp = 0

    for i in range(15):
        add_table[i] = (running_total - (previous_cmp * 2)) & 0xFFFF
        running_total += bit_counts[offset + i]
        code_sum += bit_counts[offset + i]
        cmp_table[i] = code_sum & 0xFFFF
        previous_cmp = cmp_table[i]
        code_sum <<= 1

    return cmp_table, add_table


def crm_read_symbol(
    br: CrmBitReader,
    cmp_table: list[int],
    add_table: list[int],
    real: list[int],
    real_capacity: int,
    real_base_index: int,
) -> int:
    code = 0
    for cmp_index in range(16):
        code = ((code << 1) | br.next_bit()) & 0xFFFF
        if cmp_table[cmp_index] > code:
            code16 = (code + add_table[cmp_index]) & 0xFFFF
            byte_disp = s16((code16 << 1) & 0xFFFF)
            real_index = real_base_index + math.trunc(byte_disp / 2)
            if real_index < 0 or real_index >= real_capacity:
                raise CrmError(f"CrM2 symbol index {real_index} is out of range ({real_capacity})")
            return real[real_index]
    raise CrmError("CrM2 symbol decode overflow")


def crm_lzh_decrunch(data: bytes, source_size: int, destination_size: int) -> bytes:
    br = CrmBitReader(data, 0, 14, source_size)
    dst = bytearray(destination_size)
    cursor = destination_size
    real = [0] * 527
    bits_per_code = [0] * 32

    while True:
        for i in range(32):
            bits_per_code[i] = 0

        crm_read_tab(br, bits_per_code, 16, real, 15, 512, 9)
        crm_read_tab(br, bits_per_code, 0, real, 0, 15, 4)
        cmp_len, add_len = crm_calc_cmp_tab(bits_per_code, 16)
        cmp_dist, add_dist = crm_calc_cmp_tab(bits_per_code, 0)
        block_count = br.get_bits(16)

        for _ in range(block_count + 1):
            symbol = crm_read_symbol(br, cmp_len, add_len, real, 527, 15)
            if symbol & 0x100:
                cursor = crm_write_byte(dst, cursor, symbol)
            else:
                distance_code = crm_read_symbol(br, cmp_dist, add_dist, real, 15, 0)
                distance_extra_bits = distance_code or 1
                distance_set_bit = distance_code or 16
                if distance_extra_bits > 16 or distance_set_bit > 30:
                    raise CrmError(f"CrM2 distance code {distance_code} is invalid")
                distance = br.get_bits(distance_extra_bits)
                distance_word = distance
                if distance_set_bit < 16:
                    distance_word |= 1 << distance_set_bit
                signed_distance = s16(distance_word)
                if signed_distance < -1 or signed_distance + 1 > (len(dst) - cursor):
                    raise CrmError(
                        "CrM back-reference out of range "
                        f"(code={distance_code} bits={distance_extra_bits} raw=0x{distance_word & 0xFFFF:04x})"
                    )
                ref_cursor = cursor + signed_distance + 1
                for _ in range(symbol + 1):
                    cursor, ref_cursor = crm_copy_ref_byte(dst, cursor, ref_cursor)
                cursor, ref_cursor = crm_copy_ref_byte(dst, cursor, ref_cursor)
                ref_cursor, trailing = crm_read_ref_byte(dst, ref_cursor)
                cursor = crm_write_byte(dst, cursor, trailing)

        if br.get_bits(1) == 0:
            break

    if cursor != 0:
        raise CrmError(f"CrM output size mismatch ({cursor} bytes short)")
    return bytes(dst)


def maybe_decrunch_crm(data: bytes) -> bytes:
    if len(data) < 14 or data[:4] not in (b"CrM!", b"CrM2"):
        return data
    destination_size = be32(data, 6)
    source_size = be32(data, 10)
    if destination_size == 0 or source_size == 0:
        raise CrmError(f"CrM header has invalid sizes dest={destination_size} src={source_size}")
    if 14 + source_size > len(data):
        raise CrmError(f"CrM payload is truncated need={14 + source_size} have={len(data)}")
    if data[:4] == b"CrM!":
        return crm_fast_decrunch(data, source_size, destination_size)
    return crm_lzh_decrunch(data, source_size, destination_size)


def sanitize_rel(path: Path) -> str:
    rel = path.relative_to(AMIGA)
    return "__".join(rel.parts).replace(":", "_")


def ensure_dir(path: Path) -> None:
    path.mkdir(parents=True, exist_ok=True)


def default_palette(alpha_zero: bool = False) -> list[tuple[int, int, int, int]]:
    palette = []
    for i in range(256):
        a = 0 if alpha_zero and i == 0 else 255
        palette.append((i, i, i, a))
    return palette


def load_packed_palette(data: bytes, palette_offset: int, alpha_zero: bool = False) -> list[tuple[int, int, int, int]]:
    palette = default_palette(alpha_zero)
    if palette_offset + 2 > len(data):
        return palette
    color_count = be16(data, palette_offset)
    entry = palette_offset + 2
    for color_index in range(1, min(color_count, 255) + 1):
        if entry + 2 > len(data):
            break
        packed = be16(data, entry)
        entry += 2
        if packed == 0xFFFF:
            continue
        r = ((packed >> 8) & 0x0F) * 17
        g = ((packed >> 4) & 0x0F) * 17
        b = (packed & 0x0F) * 17
        palette[color_index] = (r, g, b, 255)
    return palette


def load_raw_palette(data: bytes, color_count: int) -> list[tuple[int, int, int, int]]:
    palette = default_palette(False)
    if len(data) >= color_count * 4:
        for i in range(color_count):
            hi = be16(data, i * 4)
            lo = be16(data, i * 4 + 2)
            r = (((hi >> 8) & 0x0F) << 4) | ((lo >> 8) & 0x0F)
            g = (((hi >> 4) & 0x0F) << 4) | ((lo >> 4) & 0x0F)
            b = ((hi & 0x0F) << 4) | (lo & 0x0F)
            palette[i] = (r, g, b, 255)
    elif len(data) >= color_count * 2:
        for i in range(color_count):
            packed = be16(data, i * 2)
            palette[i] = (((packed >> 8) & 0x0F) * 17, ((packed >> 4) & 0x0F) * 17, (packed & 0x0F) * 17, 255)
    return palette


def find_raw_palette_for(path: Path, depth: int) -> list[tuple[int, int, int, int]]:
    color_count = min(1 << max(1, min(depth, 8)), 256)
    candidates = [
        path.with_suffix(".pal"),
        path.parent / f"{path.name}.pal",
        path.parent / f"{path.stem}.pal",
    ]
    for candidate in candidates:
        if not candidate.exists() or candidate == path:
            continue
        try:
            return load_raw_palette(maybe_decrunch_crm(candidate.read_bytes()), color_count)
        except Exception:
            continue
    return default_palette(False)


def image_from_indices(
    width: int,
    height: int,
    get_index,
    palette: list[tuple[int, int, int, int]],
) -> Image.Image:
    image = Image.new("RGBA", (width, height))
    px = image.load()
    for y in range(height):
        for x in range(width):
            px[x, y] = palette[get_index(x, y) & 0xFF]
    return image


def save_sheet(frames: list[Image.Image], out_path: Path, columns: int = 8, pad: int = 2) -> None:
    if not frames:
        return
    columns = max(1, min(columns, len(frames)))
    cell_w = max(frame.width for frame in frames)
    cell_h = max(frame.height for frame in frames)
    rows = (len(frames) + columns - 1) // columns
    sheet = Image.new("RGBA", (columns * cell_w + (columns - 1) * pad, rows * cell_h + (rows - 1) * pad), (0, 0, 0, 0))
    for i, frame in enumerate(frames):
        x = (i % columns) * (cell_w + pad)
        y = (i // columns) * (cell_h + pad)
        sheet.alpha_composite(frame, (x, y))
    ensure_dir(out_path.parent)
    sheet.save(out_path)


def try_decode_object_visual(path: Path, data: bytes, out_root: Path) -> list[Path]:
    if len(data) < 16:
        return []
    rotation_shift = be16(data, 0)
    frames_per_rotation = be16(data, 2)
    max_w = be16(data, 4)
    max_h = be16(data, 6)
    palette_offset = be32(data, 8)
    if (
        rotation_shift > 8
        or frames_per_rotation == 0
        or max_w == 0
        or max_h == 0
        or max_w > 512
        or max_h > 512
        or palette_offset >= len(data)
    ):
        return []

    frame_count = frames_per_rotation << rotation_shift
    table_end = 12 + frame_count * 4
    if frame_count <= 0 or frame_count > 1024 or table_end > len(data):
        return []

    frames: list[Image.Image] = []
    palette = load_packed_palette(data, palette_offset, alpha_zero=True)
    for i in range(frame_count):
        frame_offset = be32(data, 12 + i * 4)
        if frame_offset + 8 > len(data):
            return []
        handle_x = s16(be16(data, frame_offset))
        handle_y = s16(be16(data, frame_offset + 2))
        width = s16(be16(data, frame_offset + 4))
        height = s16(be16(data, frame_offset + 6))
        del handle_x, handle_y
        if width <= 0 or height <= 0 or width > 512 or height > 512:
            return []
        pixel_start = frame_offset + 8
        pixel_end = pixel_start + width * height
        if pixel_end > len(data) or pixel_start >= palette_offset:
            return []
        image = Image.new("RGBA", (width, height))
        px = image.load()
        for x in range(width):
            column = pixel_start + x * height
            for y in range(height):
                px[x, y] = palette[data[column + y]]
        frames.append(image)

    out_dir = out_root / "objects" / sanitize_rel(path)
    ensure_dir(out_dir)
    written = []
    for i, frame in enumerate(frames):
        frame_path = out_dir / f"frame_{i:04d}.png"
        frame.save(frame_path)
        written.append(frame_path)
    sheet_path = out_dir / "_sheet.png"
    save_sheet(frames, sheet_path, columns=8)
    written.append(sheet_path)
    return written


def write_blit_frames(path: Path, frames: list[Image.Image], out_root: Path, variant: str) -> list[Path]:
    out_dir = out_root / variant / sanitize_rel(path)
    ensure_dir(out_dir)
    written = []
    for i, frame in enumerate(frames):
        frame_path = out_dir / f"shape_{i:04d}.png"
        frame.save(frame_path)
        written.append(frame_path)
    sheet_path = out_dir / "_sheet.png"
    save_sheet(frames, sheet_path, columns=16)
    written.append(sheet_path)
    return written


def try_decode_makeblit1_table(path: Path, data: bytes, out_root: Path) -> list[Path]:
    if len(data) < 16:
        return []
    palette_offset = be32(data, 0)
    first_shape_offset = be32(data, 4)
    if first_shape_offset < 8 or palette_offset >= len(data) or first_shape_offset > palette_offset:
        return []
    if (first_shape_offset - 4) % 4 != 0:
        return []
    shape_count = (first_shape_offset - 4) // 4
    if shape_count <= 0 or shape_count > 512:
        return []

    color_count = be16(data, palette_offset) if palette_offset + 2 <= len(data) else 1
    plane_count = max(1, min(8, math.ceil(math.log2(max(2, color_count + 1)))))
    palette = load_packed_palette(data, palette_offset, alpha_zero=True)
    frames: list[Image.Image] = []

    for i in range(shape_count):
        shape_offset = be32(data, 4 + i * 4)
        shape_limit = be32(data, 4 + (i + 1) * 4) if i + 1 < shape_count else palette_offset
        if shape_offset + 8 > shape_limit or shape_limit > palette_offset:
            return []
        mask_offset = be32(data, shape_offset)
        blit_size = be16(data, shape_offset + 6)
        rows = blit_size >> 6
        words = blit_size & 0x3F
        if words < 2 or rows == 0 or rows % plane_count != 0:
            return []
        source_words = words - 1
        row_stride = source_words * 2
        source_size = rows * row_stride
        width = source_words * 16
        height = rows // plane_count
        if (
            shape_offset + 8 + source_size > shape_limit
            or mask_offset < 8 + source_size
            or shape_offset + mask_offset + source_size > shape_limit
            or width <= 0
            or height <= 0
        ):
            return []

        image = Image.new("RGBA", (width, height))
        px = image.load()
        for y in range(height):
            for x in range(width):
                palette_index = 0
                covered = False
                word_offset = (x // 16) * 2
                bit_shift = 15 - (x % 16)
                for plane in range(plane_count):
                    row = y * plane_count + plane
                    src_word = be16(data, shape_offset + 8 + row * row_stride + word_offset)
                    mask_word = be16(data, shape_offset + mask_offset + row * row_stride + word_offset)
                    if (src_word >> bit_shift) & 1:
                        palette_index |= 1 << plane
                    if (mask_word >> bit_shift) & 1:
                        covered = True
                px[x, y] = palette[palette_index] if covered else (0, 0, 0, 0)
        frames.append(image)

    return write_blit_frames(path, frames, out_root, "blit")


def try_decode_makeblit2_table(path: Path, data: bytes, out_root: Path) -> list[Path]:
    if len(data) < 16:
        return []
    palette_offset = be32(data, 0)
    first_shape_offset = be32(data, 4)
    if first_shape_offset < 8 or palette_offset >= len(data) or first_shape_offset > palette_offset:
        return []
    if (first_shape_offset - 4) % 4 != 0:
        return []
    shape_count = (first_shape_offset - 4) // 4
    if shape_count <= 0 or shape_count > 512:
        return []

    palette = load_packed_palette(data, palette_offset, alpha_zero=True)
    frames: list[Image.Image] = []

    for i in range(shape_count):
        shape_offset = be32(data, 4 + i * 4)
        shape_limit = be32(data, 4 + (i + 1) * 4) if i + 1 < shape_count else palette_offset
        if shape_offset + 10 > shape_limit or shape_limit > palette_offset:
            return []
        mask_offset = be32(data, shape_offset)
        blit_size = be16(data, shape_offset + 6)
        height = blit_size >> 6
        words = blit_size & 0x3F
        plane_count = be16(data, shape_offset + 8)
        if words < 2 or height == 0 or plane_count <= 0 or plane_count > 8:
            return []
        source_words = words - 1
        row_stride = source_words * 2
        width = source_words * 16
        source_size = plane_count * height * row_stride
        mask_size = height * row_stride
        if (
            shape_offset + 10 + source_size > shape_limit
            or mask_offset < 10 + source_size
            or shape_offset + mask_offset + mask_size > shape_limit
            or width <= 0
            or height <= 0
        ):
            return []

        image = Image.new("RGBA", (width, height))
        px = image.load()
        for y in range(height):
            for x in range(width):
                palette_index = 0
                word_offset = (x // 16) * 2
                bit_shift = 15 - (x % 16)
                mask_word = be16(data, shape_offset + mask_offset + y * row_stride + word_offset)
                if ((mask_word >> bit_shift) & 1) == 0:
                    px[x, y] = (0, 0, 0, 0)
                    continue
                for plane in range(plane_count):
                    src_word = be16(data, shape_offset + 10 + (plane * height + y) * row_stride + word_offset)
                    if (src_word >> bit_shift) & 1:
                        palette_index |= 1 << plane
                px[x, y] = palette[palette_index]
        frames.append(image)

    return write_blit_frames(path, frames, out_root, "blit2")


def try_decode_blit_table(path: Path, data: bytes, out_root: Path) -> list[Path]:
    written = try_decode_makeblit1_table(path, data, out_root)
    if written:
        return written
    return try_decode_makeblit2_table(path, data, out_root)


def try_decode_wall_textures(path: Path, data: bytes, out_root: Path) -> list[Path]:
    rel_lower = str(path.relative_to(AMIGA)).replace("\\", "/").lower()
    if "/txts/" not in f"/{rel_lower}" and "/ggfx/" not in f"/{rel_lower}":
        return []
    if "floor" in path.name.lower() or "roof" in path.name.lower():
        return []
    if len(data) < 4:
        return []
    palette_offset = be32(data, 0)
    texture_data_end = palette_offset if 4 <= palette_offset <= len(data) else len(data)
    available = (texture_data_end - 4) // (TEXTURE_W * TEXTURE_COLUMN_BYTES) if texture_data_end > 4 else 0
    if available <= 0 or available > 64:
        return []
    palette = load_packed_palette(data, palette_offset, alpha_zero=False)

    frames: list[Image.Image] = []
    for texture_index in range(available):
        base = 4 + texture_index * TEXTURE_W * TEXTURE_COLUMN_BYTES
        image = Image.new("RGBA", (TEXTURE_W, TEXTURE_H))
        px = image.load()
        for x in range(TEXTURE_W):
            column = base + x * TEXTURE_COLUMN_BYTES
            transparent_column = data[column] != 0
            for y in range(TEXTURE_H):
                index = data[column + 1 + y]
                px[x, y] = (0, 0, 0, 0) if transparent_column and index == 0 else palette[index]
        frames.append(image)

    out_dir = out_root / "wall_textures" / sanitize_rel(path)
    ensure_dir(out_dir)
    written = []
    for i, frame in enumerate(frames):
        texture_path = out_dir / f"texture_{i:02d}.png"
        frame.save(texture_path)
        written.append(texture_path)
    sheet_path = out_dir / "_sheet.png"
    save_sheet(frames, sheet_path, columns=5)
    written.append(sheet_path)
    return written


def try_decode_flat(path: Path, data: bytes, out_root: Path) -> list[Path]:
    name = path.name.lower()
    rel_lower = str(path.relative_to(AMIGA)).replace("\\", "/").lower()
    if ("floor" not in name and "roof" not in name) or ("/txts/" not in f"/{rel_lower}" and "/ggfx/" not in f"/{rel_lower}"):
        return []
    texel_bytes = FLAT_SIZE * FLAT_SIZE
    if len(data) < texel_bytes + 2:
        return []
    palette = load_packed_palette(data, texel_bytes, alpha_zero=False)
    image = Image.new("RGBA", (FLAT_SIZE, FLAT_SIZE))
    px = image.load()
    for x in range(FLAT_SIZE):
        for y in range(FLAT_SIZE):
            px[x, y] = palette[data[x * FLAT_SIZE + y]]
    out_path = out_root / "flats" / f"{sanitize_rel(path)}.png"
    ensure_dir(out_path.parent)
    image.save(out_path)
    return [out_path]


def decode_byterun_row(data: bytes, pos: int, row_bytes: int) -> tuple[bytes, int] | None:
    out = bytearray()
    while len(out) < row_bytes:
        if pos >= len(data):
            return None
        n = data[pos]
        pos += 1
        if n >= 128:
            n -= 256
        if n >= 0:
            count = n + 1
            if pos + count > len(data):
                return None
            out.extend(data[pos : pos + count])
            pos += count
        elif n == -128:
            continue
        else:
            count = -n + 1
            if pos >= len(data):
                return None
            out.extend([data[pos]] * count)
            pos += 1
    return bytes(out[:row_bytes]), pos


def try_decode_trimmed_picture(path: Path, data: bytes, out_root: Path) -> list[Path]:
    if len(data) < 16:
        return []
    width = be16(data, 0)
    height = be16(data, 2)
    depth = be16(data, 4)
    if width <= 0 or width > 640 or height <= 0 or height > 512 or depth <= 0 or depth > 8:
        return []
    row_bytes = (width + 7) // 8
    pos = 12
    palette = find_raw_palette_for(path, depth)
    image = Image.new("RGBA", (width, height))
    px = image.load()

    for y in range(height):
        rows = []
        for _ in range(depth):
            decoded = decode_byterun_row(data, pos, row_bytes)
            if decoded is None:
                return []
            row, pos = decoded
            rows.append(row)
        for x in range(width):
            bit = 7 - (x & 7)
            byte_index = x >> 3
            palette_index = 0
            for plane in range(depth):
                if (rows[plane][byte_index] >> bit) & 1:
                    palette_index |= 1 << plane
            px[x, y] = palette[palette_index]

    out_path = out_root / "trimmed_pictures" / f"{sanitize_rel(path)}.png"
    ensure_dir(out_path.parent)
    image.save(out_path)
    return [out_path]


def try_decode_chk(path: Path, data: bytes, out_root: Path) -> list[Path]:
    if path.suffix.lower() != ".chk" or len(data) < 16:
        return []
    width = be16(data, 4)
    height = be16(data, 6)
    candidates = [(width, height), (width + 1, height + 1)]
    chosen = None
    for w, h in candidates:
        if w > 0 and h > 0 and 8 + w * h == len(data):
            chosen = (w, h)
            break
    if chosen is None:
        return []
    w, h = chosen
    palette = find_raw_palette_for(path.with_suffix(""), 8)
    image = Image.new("RGBA", (w, h))
    px = image.load()
    base = 8
    for x in range(w):
        for y in range(h):
            px[x, y] = palette[data[base + x * h + y]]
    out_path = out_root / "chunky" / f"{sanitize_rel(path)}.png"
    ensure_dir(out_path.parent)
    image.save(out_path)
    return [out_path]


def run_ffmpeg_form(path: Path, source_data: bytes | None, out_root: Path, ffmpeg: str) -> list[Path]:
    out_dir = out_root / "form" / sanitize_rel(path)
    ensure_dir(out_dir)
    pattern = out_dir / "frame_%04d.png"

    with tempfile.TemporaryDirectory(prefix="gloom_art_") as tmp_name:
        input_path = path
        if source_data is not None:
            input_path = Path(tmp_name) / "input.iff"
            input_path.write_bytes(source_data)
        result = subprocess.run(
            [ffmpeg, "-y", "-hide_banner", "-loglevel", "error", "-i", str(input_path), str(pattern)],
            cwd=ROOT,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
    if result.returncode != 0:
        return []

    frames = sorted(out_dir.glob("frame_*.png"))
    if not frames:
        return []
    images = [Image.open(frame).convert("RGBA") for frame in frames]
    sheet_path = out_dir / "_sheet.png"
    save_sheet(images, sheet_path, columns=5)
    for image in images:
        image.close()
    return frames + [sheet_path]


def should_try_data_decoders(path: Path) -> bool:
    if path.suffix.lower() in {".pal", ".info", ".bb2", ".s", ".bak", ".old", ".md", ".txt", ".opts", ".prefs"}:
        return False
    rel = str(path.relative_to(AMIGA)).replace("\\", "/").lower()
    if "/maps/" in f"/{rel}" or "/sfxs/" in f"/{rel}" or rel.startswith("prog/sfxs/"):
        return False
    return True


def extract_all(out_root: Path) -> dict:
    ffmpeg = shutil.which("ffmpeg")
    ensure_dir(out_root)

    manifest: dict = {
        "output_root": str(out_root.relative_to(ROOT)),
        "decoded_sources": [],
        "skipped_sources": [],
        "errors": [],
    }

    for path in sorted(AMIGA.rglob("*")):
        if not path.is_file() or out_root in path.parents or path == out_root:
            continue

        rel = str(path.relative_to(AMIGA)).replace("\\", "/")
        raw = path.read_bytes()
        decoded_any = False

        form_offset = raw.find(b"FORM")
        if ffmpeg and form_offset >= 0:
            form_data = raw[form_offset:] if form_offset > 0 else None
            written = run_ffmpeg_form(path, form_data, out_root, ffmpeg)
            if written:
                manifest["decoded_sources"].append({"source": rel, "format": "FORM/IFF/ANIM", "png_count": len(written)})
                decoded_any = True

        if should_try_data_decoders(path):
            try:
                data = maybe_decrunch_crm(raw)
            except Exception as exc:
                manifest["errors"].append({"source": rel, "error": f"CrM decrunch failed: {exc}"})
                data = raw

            decoders = [
                ("flat", try_decode_flat),
                ("wall_texture", try_decode_wall_textures),
                ("object_visual", try_decode_object_visual),
                ("blit_table", try_decode_blit_table),
                ("trimmed_picture", try_decode_trimmed_picture),
                ("chunky_chk", try_decode_chk),
            ]
            for format_name, decoder in decoders:
                try:
                    written = decoder(path, data, out_root)
                except Exception as exc:
                    manifest["errors"].append({"source": rel, "format": format_name, "error": str(exc)})
                    written = []
                if written:
                    manifest["decoded_sources"].append({"source": rel, "format": format_name, "png_count": len(written)})
                    decoded_any = True
                    break

        if not decoded_any and path.suffix.lower() not in {".pal"}:
            manifest["skipped_sources"].append(rel)

    manifest_path = out_root / "manifest.json"
    manifest_path.write_text(json.dumps(manifest, indent=2), encoding="utf-8")
    return manifest


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT, help="Output directory, default amiga/art")
    args = parser.parse_args()

    out_root = args.out
    if not out_root.is_absolute():
        out_root = (ROOT / out_root).resolve()
    out_root.relative_to(ROOT)

    manifest = extract_all(out_root)
    png_count = sum(item["png_count"] for item in manifest["decoded_sources"])
    print(f"Decoded {len(manifest['decoded_sources'])} source assets into {png_count} PNG files")
    print(f"Wrote {out_root / 'manifest.json'}")
    if manifest["errors"]:
        print(f"Decoder warnings/errors: {len(manifest['errors'])}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
