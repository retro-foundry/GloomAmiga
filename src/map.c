#include "map.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static bool set_error(char *error, size_t error_size, const char *fmt, ...) {
  if (error != NULL && error_size > 0) {
    va_list args;
    va_start(args, fmt);
    (void)vsnprintf(error, error_size, fmt, args);
    va_end(args);
  }
  return false;
}

static uint16_t read_be16(const uint8_t *p) {
  return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t read_be32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static uint32_t rotl32(uint32_t value, uint32_t count) {
  count &= 31u;
  if (count == 0u) {
    return value;
  }
  return (value << count) | (value >> (32u - count));
}

static uint32_t rotr32(uint32_t value, uint32_t count) {
  count &= 31u;
  if (count == 0u) {
    return value;
  }
  return (value >> count) | (value << (32u - count));
}

typedef struct {
  const uint8_t *source_start;
  const uint8_t *source_cursor;
  uint32_t word;
  int bits_left;
} CrmBitReader;

typedef struct {
  uint16_t cmp_len[16];
  uint16_t cmp_dist[16];
  uint16_t add_len[16];
  uint16_t add_dist[16];
  uint16_t real[527];
  uint16_t bits_per_code[32];
} CrmTables;

static uint32_t crm_mask_for_bits(uint32_t bit_count) {
  if (bit_count >= 16u) {
    return 0xFFFFu;
  }
  return (1u << bit_count) - 1u;
}

static bool crm_read_be16_back(CrmBitReader *br, uint16_t *out, char *error, size_t error_size) {
  if (br->source_cursor < br->source_start || (size_t)(br->source_cursor - br->source_start) < 2u) {
    return set_error(error, error_size, "CrM stream truncated while reading trailing 16-bit word");
  }

  br->source_cursor -= 2;
  *out = read_be16(br->source_cursor);
  return true;
}

static bool crm_read_be32_back(CrmBitReader *br, uint32_t *out, char *error, size_t error_size) {
  if (br->source_cursor < br->source_start || (size_t)(br->source_cursor - br->source_start) < 4u) {
    return set_error(error, error_size, "CrM stream truncated while reading trailing 32-bit word");
  }

  br->source_cursor -= 4;
  *out = read_be32(br->source_cursor);
  return true;
}

static bool crm_bitreader_init(CrmBitReader *br, const uint8_t *source_floor, const uint8_t *source,
                               size_t source_size, char *error, size_t error_size) {
  uint16_t bits_in_last_word = 0;
  uint32_t trailing_word = 0;
  uint32_t rotate = 0;

  if (source_size < 6u) {
    return set_error(error, error_size, "CrM stream too small (%zu bytes)", source_size);
  }

  br->source_start = source_floor != NULL ? source_floor : source;
  br->source_cursor = source + source_size;
  br->word = 0;
  br->bits_left = 0;

  if (!crm_read_be16_back(br, &bits_in_last_word, error, error_size) ||
      !crm_read_be32_back(br, &trailing_word, error, error_size)) {
    return false;
  }

  if (bits_in_last_word > 16u) {
    return set_error(error, error_size, "CrM stream has invalid trailing bit count %u", (unsigned)bits_in_last_word);
  }

  rotate = 16u - bits_in_last_word;
  br->word = trailing_word >> rotate;
  br->bits_left = (int)bits_in_last_word;
  return true;
}

static bool crm_next_bit(CrmBitReader *br, uint32_t *out_bit, char *error, size_t error_size) {
  br->bits_left -= 1;

  if (br->bits_left == 0) {
    uint16_t old_low = (uint16_t)br->word;
    uint16_t next_word = 0;

    br->word >>= 1;
    br->word = rotl32(br->word, 16u);

    if (!crm_read_be16_back(br, &next_word, error, error_size)) {
      return false;
    }

    br->word = (br->word & 0xFFFF0000u) | (uint32_t)next_word;
    br->word = rotl32(br->word, 16u);
    br->bits_left = 16;
    *out_bit = (old_low & 1u) != 0u ? 1u : 0u;
    return true;
  }

  if (br->bits_left < 0) {
    return set_error(error, error_size, "CrM bitreader underflow");
  }

  *out_bit = br->word & 1u;
  br->word >>= 1;
  return true;
}

static bool crm_get_bits(CrmBitReader *br, uint32_t bit_count, uint32_t *out_bits, char *error, size_t error_size) {
  uint32_t bits = 0;

  if (bit_count == 0u || bit_count > 16u) {
    return set_error(error, error_size, "CrM invalid bit request (%u)", (unsigned)bit_count);
  }

  bits = (uint16_t)br->word;
  br->word >>= bit_count;
  br->bits_left -= (int)bit_count;

  if (br->bits_left <= 0) {
    uint16_t next_word = 0;

    br->bits_left += 16;
    br->word = rotr32(br->word, (uint32_t)br->bits_left);

    if (!crm_read_be16_back(br, &next_word, error, error_size)) {
      return false;
    }

    br->word = (br->word & 0xFFFF0000u) | (uint32_t)next_word;
    br->word = rotl32(br->word, (uint32_t)br->bits_left);
  }

  *out_bits = bits & crm_mask_for_bits(bit_count);
  return true;
}

static bool crm_write_byte(uint8_t **dst_cursor, uint8_t *dst_start, uint8_t value, char *error, size_t error_size) {
  if (*dst_cursor <= dst_start) {
    return set_error(error, error_size, "CrM destination underflow while writing byte");
  }

  *dst_cursor -= 1;
  **dst_cursor = value;
  return true;
}

static bool crm_read_ref_byte(uint8_t **ref_cursor, const uint8_t *dst_start, const uint8_t *dst_end, uint8_t *out,
                              char *error, size_t error_size) {
  if (*ref_cursor <= dst_start || *ref_cursor > dst_end) {
    return set_error(error, error_size, "CrM back-reference out of range");
  }

  *ref_cursor -= 1;
  *out = **ref_cursor;
  return true;
}

static bool crm_copy_ref_byte(uint8_t **dst_cursor, uint8_t *dst_start, const uint8_t *dst_end, uint8_t **ref_cursor,
                              char *error, size_t error_size) {
  uint8_t value = 0;

  if (!crm_read_ref_byte(ref_cursor, dst_start, dst_end, &value, error, error_size)) {
    return false;
  }

  return crm_write_byte(dst_cursor, dst_start, value, error, error_size);
}

static bool crm_fast_decrunch(const uint8_t *source_floor, const uint8_t *source, size_t source_size,
                              uint8_t *destination, size_t destination_size, char *error, size_t error_size) {
  CrmBitReader br;
  uint8_t *dst_cursor = destination + destination_size;
  const uint8_t *dst_end = destination + destination_size;

  if (!crm_bitreader_init(&br, source_floor, source, source_size, error, error_size)) {
    return false;
  }

  while (dst_cursor > destination) {
    uint32_t bit = 0;

    if (!crm_next_bit(&br, &bit, error, error_size)) {
      return false;
    }

    if (bit != 0u) {
      uint32_t literal = 0;

      if (!crm_get_bits(&br, 8u, &literal, error, error_size)) {
        return false;
      }

      if (!crm_write_byte(&dst_cursor, destination, (uint8_t)literal, error, error_size)) {
        return false;
      }
      continue;
    }

    {
      uint32_t run_bits = 0;
      uint32_t run_len = 0;
      uint32_t extra = 0;
      uint32_t distance_bits = 0;
      uint32_t distance_base = 0;
      uint32_t distance_extra = 0;
      uint32_t distance = 0;
      uint8_t *ref_cursor = NULL;
      uint32_t i = 0;

      if (!crm_next_bit(&br, &bit, error, error_size)) {
        return false;
      }
      if (bit == 0u) {
        run_bits = 1u;
        run_len = 1u;
      } else {
        if (!crm_next_bit(&br, &bit, error, error_size)) {
          return false;
        }
        if (bit == 0u) {
          run_bits = 2u;
          run_len = 3u;
        } else {
          if (!crm_next_bit(&br, &bit, error, error_size)) {
            return false;
          }
          if (bit == 0u) {
            run_bits = 4u;
            run_len = 7u;
          } else {
            run_bits = 8u;
            run_len = 0x17u;
          }
        }
      }

      if (!crm_get_bits(&br, run_bits, &extra, error, error_size)) {
        return false;
      }
      run_len += extra;

      if (run_len == 22u) {
        uint32_t literal_count_bits = 5u;

        if (!crm_next_bit(&br, &bit, error, error_size)) {
          return false;
        }
        if (bit == 0u) {
          literal_count_bits = 14u;
        }

        if (!crm_get_bits(&br, literal_count_bits, &extra, error, error_size)) {
          return false;
        }

        run_len = 14u + extra;

        for (i = 0; i <= run_len; ++i) {
          uint32_t literal = 0;
          if (!crm_get_bits(&br, 8u, &literal, error, error_size)) {
            return false;
          }
          if (!crm_write_byte(&dst_cursor, destination, (uint8_t)literal, error, error_size)) {
            return false;
          }
        }
        continue;
      }

      if (run_len >= 22u) {
        run_len -= 1u;
      }

      if (!crm_next_bit(&br, &bit, error, error_size)) {
        return false;
      }
      if (bit == 0u) {
        distance_bits = 9u;
        distance_base = 0x20u;
      } else {
        if (!crm_next_bit(&br, &bit, error, error_size)) {
          return false;
        }
        if (bit == 0u) {
          distance_bits = 5u;
          distance_base = 0u;
        } else {
          distance_bits = 14u;
          distance_base = 0x220u;
        }
      }

      if (!crm_get_bits(&br, distance_bits, &distance_extra, error, error_size)) {
        return false;
      }

      distance = distance_base + distance_extra;
      ref_cursor = dst_cursor + distance;

      for (i = 0; i <= run_len; ++i) {
        if (!crm_copy_ref_byte(&dst_cursor, destination, dst_end, &ref_cursor, error, error_size)) {
          return false;
        }
      }
    }
  }

  return dst_cursor == destination;
}

static bool crm_read_tab(CrmBitReader *br, uint16_t *bit_counts, uint16_t *real_table, size_t real_capacity,
                         uint32_t max_bits, char *error, size_t error_size) {
  uint32_t count = 0;
  uint32_t i = 0;
  uint32_t total_real = 0;

  if (!crm_get_bits(br, 4u, &count, error, error_size)) {
    return false;
  }

  if (count == 0u || count > 16u) {
    return set_error(error, error_size, "CrM2 table bit-count length %u is invalid", (unsigned)count);
  }

  for (i = 0; i < 16u; ++i) {
    bit_counts[i] = 0;
  }

  for (i = 0; i < count; ++i) {
    uint32_t bits = (i + 1u) <= max_bits ? (i + 1u) : max_bits;
    uint32_t value = 0;

    if (!crm_get_bits(br, bits, &value, error, error_size)) {
      return false;
    }

    bit_counts[i] = (uint16_t)value;
    total_real += value;
  }

  if ((size_t)total_real > real_capacity) {
    return set_error(error, error_size, "CrM2 table overflows real-table capacity (%u > %zu)", (unsigned)total_real,
                     real_capacity);
  }

  for (i = 0; i < total_real; ++i) {
    uint32_t value = 0;
    if (!crm_get_bits(br, max_bits, &value, error, error_size)) {
      return false;
    }
    real_table[i] = (uint16_t)value;
  }

  return true;
}

static void crm_calc_cmp_tab(const uint16_t *bit_counts, uint16_t *cmp, uint16_t *add) {
  uint32_t code_sum = 0;
  uint32_t running_total = 0;
  uint32_t previous_cmp = 0;
  size_t i = 0;

  memset(cmp, 0, 16u * sizeof(*cmp));
  memset(add, 0, 16u * sizeof(*add));

  for (i = 0; i < 15u; ++i) {
    int32_t add_value = (int32_t)running_total - (int32_t)(previous_cmp * 2u);

    add[i] = (uint16_t)add_value;
    running_total += bit_counts[i];
    code_sum += bit_counts[i];
    cmp[i] = (uint16_t)code_sum;
    previous_cmp = cmp[i];
    code_sum <<= 1;
  }
}

static bool crm_read_symbol(CrmBitReader *br, const uint16_t *cmp, const uint16_t *add, const uint16_t *real,
                            size_t real_capacity, int real_base_index, uint16_t *out_symbol, char *error,
                            size_t error_size) {
  uint32_t code = 0;
  size_t cmp_index = 0;

  while (cmp_index < 16u) {
    uint32_t bit = 0;

    if (!crm_next_bit(br, &bit, error, error_size)) {
      return false;
    }

    code = ((code << 1u) | bit) & 0xFFFFu;
    if ((uint32_t)cmp[cmp_index] > code) {
      uint16_t code16 = (uint16_t)code;
      int16_t byte_disp = 0;
      int real_index = 0;

      code16 = (uint16_t)(code16 + add[cmp_index]);
      byte_disp = (int16_t)(uint16_t)(code16 << 1u);
      real_index = real_base_index + (int)(byte_disp / 2);

      if (real_index < 0 || (size_t)real_index >= real_capacity) {
        return set_error(error, error_size, "CrM2 symbol index %d is out of range (%zu)", real_index,
                         real_capacity);
      }

      *out_symbol = real[real_index];
      return true;
    }

    cmp_index += 1u;
  }

  return set_error(error, error_size, "CrM2 symbol decode overflow");
}

static bool crm_lzh_decrunch(const uint8_t *source_floor, const uint8_t *source, size_t source_size,
                             uint8_t *destination, size_t destination_size, char *error, size_t error_size) {
  CrmBitReader br;
  CrmTables tables;
  uint8_t *dst_cursor = destination + destination_size;
  const uint8_t *dst_end = destination + destination_size;

  memset(&tables, 0, sizeof(tables));

  if (!crm_bitreader_init(&br, source_floor, source, source_size, error, error_size)) {
    return false;
  }

  while (true) {
    uint32_t block_count = 0;
    uint32_t i = 0;
    uint32_t continue_flag = 0;

    memset(tables.bits_per_code, 0, sizeof(tables.bits_per_code));

    if (!crm_read_tab(&br, &tables.bits_per_code[16], &tables.real[15], 512u, 9u, error, error_size)) {
      return false;
    }

    if (!crm_read_tab(&br, &tables.bits_per_code[0], &tables.real[0], 15u, 4u, error, error_size)) {
      return false;
    }

    crm_calc_cmp_tab(&tables.bits_per_code[16], tables.cmp_len, tables.add_len);
    crm_calc_cmp_tab(&tables.bits_per_code[0], tables.cmp_dist, tables.add_dist);

    if (!crm_get_bits(&br, 16u, &block_count, error, error_size)) {
      return false;
    }

    for (i = 0; i <= block_count; ++i) {
      uint16_t symbol = 0;

      if (!crm_read_symbol(&br, tables.cmp_len, tables.add_len, &tables.real[0], 527u, 15, &symbol, error,
                           error_size)) {
        return false;
      }

      if ((symbol & 0x100u) != 0u) {
        if (!crm_write_byte(&dst_cursor, destination, (uint8_t)symbol, error, error_size)) {
          return false;
        }
      } else {
        uint16_t distance_code = 0;
        uint32_t distance_extra_bits = 0;
        uint32_t distance_set_bit = 0;
        uint32_t distance = 0;
        uint32_t distance_word = 0;
        int32_t signed_distance = 0;
        uint8_t *ref_cursor = NULL;
        uint32_t j = 0;
        uint8_t trailing = 0;

        if (!crm_read_symbol(&br, tables.cmp_dist, tables.add_dist, &tables.real[0], 15u, 0, &distance_code, error,
                             error_size)) {
          return false;
        }

        distance_extra_bits = distance_code;
        distance_set_bit = distance_code;
        if (distance_extra_bits == 0u) {
          distance_extra_bits = 1u;
          distance_set_bit = 16u;
        }

        if (distance_extra_bits > 16u || distance_set_bit > 30u) {
          return set_error(error, error_size, "CrM2 distance code %u is invalid", (unsigned)distance_code);
        }

        if (!crm_get_bits(&br, distance_extra_bits, &distance, error, error_size)) {
          return false;
        }

        distance_word = distance;
        if (distance_set_bit < 16u) {
          distance_word |= (1u << distance_set_bit);
        }

        signed_distance = (int16_t)(uint16_t)distance_word;
        if (signed_distance < -1 || signed_distance + 1 > (int32_t)(dst_end - dst_cursor)) {
          return set_error(error, error_size,
                           "CrM back-reference out of range (code=%u bits=%u raw=0x%04x signed=%d written=%zu)",
                           (unsigned)distance_code, (unsigned)distance_extra_bits,
                           (unsigned)(uint16_t)distance_word, (int)signed_distance,
                           (size_t)(dst_end - dst_cursor));
        }

        ref_cursor = dst_cursor + signed_distance + 1;

        for (j = 0; j <= (uint32_t)symbol; ++j) {
          if (!crm_copy_ref_byte(&dst_cursor, destination, dst_end, &ref_cursor, error, error_size)) {
            return false;
          }
        }

        if (!crm_copy_ref_byte(&dst_cursor, destination, dst_end, &ref_cursor, error, error_size) ||
            !crm_read_ref_byte(&ref_cursor, destination, dst_end, &trailing, error, error_size) ||
            !crm_write_byte(&dst_cursor, destination, trailing, error, error_size)) {
          return false;
        }
      }
    }

    if (!crm_get_bits(&br, 1u, &continue_flag, error, error_size)) {
      return false;
    }

    if (continue_flag == 0u) {
      break;
    }
  }

  if (dst_cursor != destination) {
    return set_error(error, error_size, "CrM output size mismatch (%zu bytes short)",
                     (size_t)(dst_cursor - destination));
  }

  return true;
}

static bool maybe_decrunch_crm(uint8_t **io_data, size_t *io_size, char *error, size_t error_size) {
  uint8_t *data = *io_data;
  size_t total = *io_size;
  bool is_crm1 = false;
  bool is_crm2 = false;
  uint32_t destination_size = 0;
  uint32_t source_size = 0;
  uint8_t *decrunched = NULL;

  if (total < 14u) {
    return true;
  }

  is_crm1 = data[0] == 'C' && data[1] == 'r' && data[2] == 'M' && data[3] == '!';
  is_crm2 = data[0] == 'C' && data[1] == 'r' && data[2] == 'M' && data[3] == '2';

  if (!is_crm1 && !is_crm2) {
    return true;
  }

  destination_size = read_be32(data + 6);
  source_size = read_be32(data + 10);

  if (destination_size == 0u || source_size == 0u) {
    return set_error(error, error_size, "CrM header has invalid sizes (dest=%u src=%u)", (unsigned)destination_size,
                     (unsigned)source_size);
  }

  if ((size_t)14u + (size_t)source_size > total) {
    return set_error(error, error_size, "CrM payload is truncated (need %u bytes, have %zu)",
                     (unsigned)(14u + source_size), total);
  }

  decrunched = (uint8_t *)malloc((size_t)destination_size);
  if (decrunched == NULL) {
    return set_error(error, error_size, "Out of memory while allocating CrM destination (%u bytes)",
                     (unsigned)destination_size);
  }

  if (!(is_crm1 ? crm_fast_decrunch(data, data + 14, source_size, decrunched, destination_size, error, error_size)
                : crm_lzh_decrunch(data, data + 14, source_size, decrunched, destination_size, error, error_size))) {
    free(decrunched);
    return false;
  }

  free(data);
  *io_data = decrunched;
  *io_size = (size_t)destination_size;
  return true;
}

bool gloom_decrunch_crm_buffer(uint8_t **io_data, size_t *io_size, char *error, size_t error_size) {
  if (io_data == NULL || io_size == NULL || *io_data == NULL) {
    return set_error(error, error_size, "Invalid CrM buffer arguments");
  }

  return maybe_decrunch_crm(io_data, io_size, error, error_size);
}

static bool in_bounds(size_t offset, size_t needed, size_t total) {
  return offset <= total && needed <= (total - offset);
}

static bool read_file(const char *path, uint8_t **out_data, size_t *out_size, char *error, size_t error_size) {
  FILE *f = fopen(path, "rb");
  uint8_t *data = NULL;
  long file_len = 0;
  size_t read_len = 0;

  if (f == NULL) {
    return set_error(error, error_size, "Failed to open %s: %s", path, strerror(errno));
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return set_error(error, error_size, "Failed to seek end for %s", path);
  }

  file_len = ftell(f);
  if (file_len < 0) {
    fclose(f);
    return set_error(error, error_size, "Failed to determine size for %s", path);
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return set_error(error, error_size, "Failed to seek start for %s", path);
  }

  data = (uint8_t *)malloc((size_t)file_len);
  if (data == NULL) {
    fclose(f);
    return set_error(error, error_size, "Out of memory while reading %s", path);
  }

  read_len = fread(data, 1, (size_t)file_len, f);
  fclose(f);

  if (read_len != (size_t)file_len) {
    free(data);
    return set_error(error, error_size, "Failed to read all bytes from %s", path);
  }

  *out_data = data;
  *out_size = (size_t)file_len;
  return true;
}

static bool copy_blob(uint8_t **out_blob, size_t *out_blob_size, const uint8_t *src, size_t offset, size_t size, size_t total,
                      char *error, size_t error_size) {
  uint8_t *blob = NULL;

  if (!in_bounds(offset, size, total)) {
    return set_error(error, error_size, "Blob offset out of range (offset=%zu size=%zu total=%zu)", offset, size, total);
  }

  if (size == 0) {
    *out_blob = NULL;
    *out_blob_size = 0;
    return true;
  }

  blob = (uint8_t *)malloc(size);
  if (blob == NULL) {
    return set_error(error, error_size, "Out of memory while copying blob (%zu bytes)", size);
  }

  memcpy(blob, src + offset, size);
  *out_blob = blob;
  *out_blob_size = size;
  return true;
}

static bool parse_texture_names(GloomMap *map, const uint8_t *data, size_t total, char *error, size_t error_size) {
  size_t pos = map->header.texname_offset;
  size_t name_index = 0;

  for (name_index = 0; name_index < GLOOM_TEXTURE_NAME_COUNT; ++name_index) {
    size_t write_index = 0;
    bool done = false;

    while (pos < total) {
      uint8_t ch = data[pos++];
      if (ch == 0) {
        map->texture_names[name_index][write_index] = '\0';
        done = true;
        break;
      }
      if (write_index + 1 < sizeof(map->texture_names[name_index])) {
        map->texture_names[name_index][write_index++] = (char)ch;
      }
    }

    if (!done) {
      return set_error(error, error_size, "Texture name %zu is not null-terminated", name_index);
    }
  }

  return true;
}

static bool parse_grid_cells(GloomMap *map, char *error, size_t error_size) {
  size_t cell = 0;

  if (map->grid_blob_size < (size_t)GLOOM_GRID_CELL_COUNT * (size_t)GLOOM_GRID_CELL_SIZE_BYTES) {
    return set_error(error, error_size, "Grid section is too small (%zu bytes)", map->grid_blob_size);
  }

  for (cell = 0; cell < (size_t)GLOOM_GRID_CELL_COUNT; ++cell) {
    const uint8_t *entry = map->grid_blob + (cell * (size_t)GLOOM_GRID_CELL_SIZE_BYTES);

    map->wall_grid[cell].count_minus_one = (int16_t)read_be16(entry + 0);
    map->wall_grid[cell].ppnt_word_offset = read_be16(entry + 2);
    map->trigger_grid[cell].count_minus_one = (int16_t)read_be16(entry + 4);
    map->trigger_grid[cell].ppnt_word_offset = read_be16(entry + 6);
  }

  map->has_grid_cells = true;
  return true;
}

static bool parse_animations(GloomMap *map, const uint8_t *data, size_t total, char *error, size_t error_size) {
  size_t pos = map->header.anim_offset;
  size_t capacity = 0;
  size_t count = 0;
  GloomAnim *items = NULL;

  while (pos + 2 <= total && pos + 2 <= map->header.texname_offset) {
    uint16_t frame_count = read_be16(data + pos);
    pos += 2;

    if (frame_count == 0) {
      break;
    }

    if (!in_bounds(pos, 6, total) || pos + 6 > map->header.texname_offset) {
      free(items);
      return set_error(error, error_size, "Animation table truncated before texture names");
    }

    if (count == capacity) {
      size_t next_capacity = capacity == 0 ? 16 : capacity * 2;
      GloomAnim *next = (GloomAnim *)realloc(items, next_capacity * sizeof(*next));
      if (next == NULL) {
        free(items);
        return set_error(error, error_size, "Out of memory while growing animation list");
      }
      items = next;
      capacity = next_capacity;
    }

    items[count].frame_count = frame_count;
    items[count].first_frame = read_be16(data + pos + 0);
    items[count].delay = read_be16(data + pos + 2);
    items[count].current = read_be16(data + pos + 4);
    ++count;

    pos += 6;
  }

  map->animations = items;
  map->animation_count = count;
  return true;
}

static bool parse_zones(GloomMap *map, const uint8_t *data, size_t total, char *error, size_t error_size) {
  size_t zone_bytes = 0;
  size_t zone_count = 0;
  size_t i = 0;

  if (map->header.ppnt_offset < map->header.poly_offset) {
    return set_error(error, error_size, "ppnt_offset is before poly_offset");
  }

  zone_bytes = map->header.ppnt_offset - map->header.poly_offset;
  if ((zone_bytes % GLOOM_ZONE_SIZE_BYTES) != 0) {
    return set_error(error, error_size, "Zone section size %zu is not a multiple of %d", zone_bytes, GLOOM_ZONE_SIZE_BYTES);
  }

  zone_count = zone_bytes / GLOOM_ZONE_SIZE_BYTES;
  if (!in_bounds(map->header.poly_offset, zone_bytes, total)) {
    return set_error(error, error_size, "Zone section is out of file bounds");
  }

  map->zones = (GloomZone *)calloc(zone_count, sizeof(*map->zones));
  if (zone_count > 0 && map->zones == NULL) {
    return set_error(error, error_size, "Out of memory while allocating %zu zones", zone_count);
  }

  map->zone_count = zone_count;

  for (i = 0; i < zone_count; ++i) {
    const uint8_t *z = data + map->header.poly_offset + (i * GLOOM_ZONE_SIZE_BYTES);
    GloomZone *out = &map->zones[i];

    out->ztype = (int16_t)read_be16(z + 0);
    out->x1 = (int16_t)read_be16(z + 2);
    out->z1 = (int16_t)read_be16(z + 4);
    out->x2 = (int16_t)read_be16(z + 6);
    out->z2 = (int16_t)read_be16(z + 8);
    out->a = (int16_t)read_be16(z + 10);
    out->b = (int16_t)read_be16(z + 12);
    out->na = (int16_t)read_be16(z + 14);
    out->nb = (int16_t)read_be16(z + 16);
    out->ln = (int16_t)read_be16(z + 18);
    memcpy(out->textures, z + 20, 8);
    out->scale = (int16_t)read_be16(z + 28);
    out->event_id = (int16_t)read_be16(z + 30);
  }

  return true;
}

static bool append_object_spawn(GloomMap *map, size_t event_index, int16_t object_type, int16_t x, int16_t y,
                                int16_t z, int16_t rotation, char *error, size_t error_size) {
  GloomObjectSpawn *next = NULL;
  size_t new_count = 0;

  if (map == NULL) {
    return set_error(error, error_size, "Invalid map while appending object spawn");
  }

  new_count = map->object_spawn_count + 1u;
  next = (GloomObjectSpawn *)realloc(map->object_spawns, new_count * sizeof(*next));
  if (next == NULL) {
    return set_error(error, error_size, "Out of memory while appending object spawn");
  }

  map->object_spawns = next;
  map->object_spawns[map->object_spawn_count].object_type = object_type;
  map->object_spawns[map->object_spawn_count].x = x;
  map->object_spawns[map->object_spawn_count].y = y;
  map->object_spawns[map->object_spawn_count].z = z;
  map->object_spawns[map->object_spawn_count].rotation = rotation;
  map->object_spawns[map->object_spawn_count].event_id = (uint8_t)(event_index + 1u);
  map->object_spawn_count = new_count;

  return true;
}

static bool append_door_command(GloomMap *map, size_t event_index, uint16_t zone_index, char *error,
                                size_t error_size) {
  GloomDoorCommand *next = NULL;
  size_t new_count = 0;

  if (map == NULL) {
    return set_error(error, error_size, "Invalid map while appending door command");
  }
  if ((size_t)zone_index >= map->zone_count) {
    return set_error(error, error_size, "Door command references zone %u, but map has %zu zones",
                     (unsigned)zone_index, map->zone_count);
  }

  new_count = map->door_command_count + 1u;
  next = (GloomDoorCommand *)realloc(map->door_commands, new_count * sizeof(*next));
  if (next == NULL) {
    return set_error(error, error_size, "Out of memory while appending door command");
  }

  map->door_commands = next;
  map->door_commands[map->door_command_count].event_id = (uint8_t)(event_index + 1u);
  map->door_commands[map->door_command_count].zone_index = zone_index;
  map->door_command_count = new_count;
  return true;
}

static bool append_texture_change_command(GloomMap *map, size_t event_index, uint16_t zone_index,
                                          uint16_t texture_index, char *error, size_t error_size) {
  GloomTextureChangeCommand *next = NULL;
  size_t new_count = 0;

  if (map == NULL) {
    return set_error(error, error_size, "Invalid map while appending texture-change command");
  }
  if ((size_t)zone_index >= map->zone_count) {
    return set_error(error, error_size, "Texture-change command references zone %u, but map has %zu zones",
                     (unsigned)zone_index, map->zone_count);
  }

  if (texture_index > 255u) {
    return set_error(error, error_size, "Texture-change command uses out-of-range texture %u",
                     (unsigned)texture_index);
  }

  new_count = map->texture_change_command_count + 1u;
  next = (GloomTextureChangeCommand *)realloc(map->texture_change_commands, new_count * sizeof(*next));
  if (next == NULL) {
    return set_error(error, error_size, "Out of memory while appending texture-change command");
  }

  map->texture_change_commands = next;
  map->texture_change_commands[map->texture_change_command_count].event_id = (uint8_t)(event_index + 1u);
  map->texture_change_commands[map->texture_change_command_count].zone_index = zone_index;
  map->texture_change_commands[map->texture_change_command_count].texture_index = (uint8_t)texture_index;
  map->texture_change_command_count = new_count;
  return true;
}

static bool append_teleport_command(GloomMap *map, size_t event_index, int16_t x, int16_t control_word, int16_t z,
                                    int16_t rotation, char *error, size_t error_size) {
  GloomTeleportCommand *next = NULL;
  size_t new_count = 0;

  if (map == NULL) {
    return set_error(error, error_size, "Invalid map while appending teleport command");
  }

  new_count = map->teleport_command_count + 1u;
  next = (GloomTeleportCommand *)realloc(map->teleport_commands, new_count * sizeof(*next));
  if (next == NULL) {
    return set_error(error, error_size, "Out of memory while appending teleport command");
  }

  map->teleport_commands = next;
  map->teleport_commands[map->teleport_command_count].event_id = (uint8_t)(event_index + 1u);
  map->teleport_commands[map->teleport_command_count].x = x;
  map->teleport_commands[map->teleport_command_count].control_word = control_word;
  map->teleport_commands[map->teleport_command_count].z = z;
  map->teleport_commands[map->teleport_command_count].rotation = rotation;
  map->teleport_command_count = new_count;
  return true;
}

static bool append_rotpoly_command(GloomMap *map, size_t event_index, uint16_t first_zone_index, uint16_t zone_count,
                                   int16_t speed, uint16_t flags, char *error, size_t error_size) {
  GloomRotPolyCommand *next = NULL;
  size_t new_count = 0;
  size_t first = (size_t)first_zone_index;
  size_t count = (size_t)zone_count;
  size_t needed_count = (flags & 1u) != 0u ? count * 2u : count;

  if (map == NULL) {
    return set_error(error, error_size, "Invalid map while appending rotpoly command");
  }
  if (count == 0u || count > 32u) {
    return set_error(error, error_size, "Rotpoly command uses invalid zone count %u", (unsigned)zone_count);
  }
  if (first > map->zone_count || needed_count > map->zone_count - first) {
    return set_error(error, error_size,
                     "Rotpoly command references zones %u..%zu, but map has %zu zones",
                     (unsigned)first_zone_index, first + needed_count - 1u, map->zone_count);
  }

  new_count = map->rotpoly_command_count + 1u;
  next = (GloomRotPolyCommand *)realloc(map->rotpoly_commands, new_count * sizeof(*next));
  if (next == NULL) {
    return set_error(error, error_size, "Out of memory while appending rotpoly command");
  }

  map->rotpoly_commands = next;
  map->rotpoly_commands[map->rotpoly_command_count].event_id = (uint8_t)(event_index + 1u);
  map->rotpoly_commands[map->rotpoly_command_count].first_zone_index = first_zone_index;
  map->rotpoly_commands[map->rotpoly_command_count].zone_count = zone_count;
  map->rotpoly_commands[map->rotpoly_command_count].speed = speed;
  map->rotpoly_commands[map->rotpoly_command_count].flags = flags;
  map->rotpoly_command_count = new_count;
  return true;
}

static bool parse_event_stream(GloomMap *map, const uint8_t *data, size_t total, size_t start, size_t event_index,
                               GloomEventStats *stats, char *error, size_t error_size) {
  size_t pos = start;

  while (true) {
    uint16_t op = 0;

    if (!in_bounds(pos, 2, total)) {
      return set_error(error, error_size, "Event stream at 0x%zx is truncated while reading opcode", start);
    }

    op = read_be16(data + pos);
    pos += 2;

    if (op == 0) {
      break;
    }

    stats->command_count++;

    switch (op) {
      case 1:
        stats->add_monster_count++;
        if (!in_bounds(pos, 10, total)) {
          return set_error(error, error_size, "Event stream at 0x%zx truncated in add-monster payload", start);
        }

        if (!append_object_spawn(map, event_index, (int16_t)read_be16(data + pos + 0), (int16_t)read_be16(data + pos + 2),
                                 (int16_t)read_be16(data + pos + 4), (int16_t)read_be16(data + pos + 6),
                                 (int16_t)read_be16(data + pos + 8), error, error_size)) {
          return false;
        }

        pos += 10;
        break;

      case 2:
        stats->open_door_count++;
        if (!in_bounds(pos, 2, total)) {
          return set_error(error, error_size, "Event stream at 0x%zx truncated in open-door payload", start);
        }
        if (!append_door_command(map, event_index, read_be16(data + pos), error, error_size)) {
          return false;
        }
        pos += 2;
        break;

      case 3:
        stats->teleport_count++;
        if (!in_bounds(pos, 8, total)) {
          return set_error(error, error_size, "Event stream at 0x%zx truncated in teleport payload", start);
        }
        if (!append_teleport_command(map, event_index, (int16_t)read_be16(data + pos + 0),
                                     (int16_t)read_be16(data + pos + 2),
                                     (int16_t)read_be16(data + pos + 4),
                                     (int16_t)read_be16(data + pos + 6), error, error_size)) {
          return false;
        }
        pos += 8;
        break;

      case 4:
        stats->load_objects_count++;
        while (true) {
          int16_t object_id = 0;
          if (!in_bounds(pos, 2, total)) {
            return set_error(error, error_size, "Event stream at 0x%zx truncated in load-objects payload", start);
          }
          object_id = (int16_t)read_be16(data + pos);
          pos += 2;
          if (object_id == -1) {
            break;
          }
        }
        break;

      case 5:
        stats->texture_change_count++;
        if (!in_bounds(pos, 4, total)) {
          return set_error(error, error_size, "Event stream at 0x%zx truncated in texture-change payload", start);
        }
        if (!append_texture_change_command(map, event_index, read_be16(data + pos), read_be16(data + pos + 2),
                                           error, error_size)) {
          return false;
        }
        pos += 4;
        break;

      case 6:
        stats->rotate_poly_count++;
        if (!in_bounds(pos, 8, total)) {
          return set_error(error, error_size, "Event stream at 0x%zx truncated in rotate-poly payload", start);
        }
        if (!append_rotpoly_command(map, event_index, read_be16(data + pos + 0), read_be16(data + pos + 2),
                                    (int16_t)read_be16(data + pos + 4), read_be16(data + pos + 6),
                                    error, error_size)) {
          return false;
        }
        pos += 8;
        break;

      default:
        stats->unknown_count++;
        return set_error(error, error_size, "Unknown event opcode %u in stream at 0x%zx", (unsigned)op, start);
    }
  }

  return true;
}

static bool parse_events(GloomMap *map, const uint8_t *data, size_t total, char *error, size_t error_size) {
  size_t i = 0;

  for (i = 0; i < GLOOM_EVENT_COUNT; ++i) {
    uint32_t offset = map->header.event_offsets[i];
    map->events[i].offset = offset;

    if (offset >= total) {
      return set_error(error, error_size, "Event %zu offset 0x%08x is out of file bounds", i + 1, (unsigned)offset);
    }

    if (!parse_event_stream(map, data, total, offset, i, &map->events[i], error, error_size)) {
      return false;
    }
  }

  return true;
}

void gloom_map_free(GloomMap *map) {
  if (map == NULL) {
    return;
  }

  free(map->zones);
  free(map->animations);
  free(map->grid_blob);
  free(map->ppnt_blob);
  free(map->object_spawns);
  free(map->door_commands);
  free(map->texture_change_commands);
  free(map->teleport_commands);
  free(map->rotpoly_commands);

  memset(map, 0, sizeof(*map));
}

bool gloom_map_load(const char *path, GloomMap *out_map, char *error, size_t error_size) {
  uint8_t *data = NULL;
  size_t total = 0;
  size_t i = 0;

  if (out_map == NULL || path == NULL) {
    return set_error(error, error_size, "Invalid map load arguments");
  }

  memset(out_map, 0, sizeof(*out_map));

  if (!read_file(path, &data, &total, error, error_size)) {
    return false;
  }

  if (!maybe_decrunch_crm(&data, &total, error, error_size)) {
    free(data);
    return false;
  }

  if (total < GLOOM_HEADER_SIZE_BYTES) {
    free(data);
    return set_error(error, error_size, "Map file is too small to contain a full header");
  }

  out_map->header.grid_offset = read_be32(data + 0);
  out_map->header.poly_offset = read_be32(data + 4);
  out_map->header.ppnt_offset = read_be32(data + 8);
  out_map->header.anim_offset = read_be32(data + 12);
  out_map->header.texname_offset = read_be32(data + 16);

  for (i = 0; i < GLOOM_EVENT_COUNT; ++i) {
    out_map->header.event_offsets[i] = read_be32(data + 20 + (i * 4));
  }

  if (!(out_map->header.grid_offset < out_map->header.poly_offset &&
        out_map->header.poly_offset < out_map->header.ppnt_offset &&
        out_map->header.ppnt_offset < out_map->header.anim_offset &&
        out_map->header.anim_offset < out_map->header.texname_offset &&
        out_map->header.texname_offset < total)) {
    free(data);
    return set_error(error, error_size, "Map offsets are invalid or out of order");
  }

  out_map->file_size = total;

  if (!copy_blob(&out_map->grid_blob, &out_map->grid_blob_size, data, out_map->header.grid_offset,
                 out_map->header.poly_offset - out_map->header.grid_offset, total, error, error_size)) {
    free(data);
    gloom_map_free(out_map);
    return false;
  }

  if (!copy_blob(&out_map->ppnt_blob, &out_map->ppnt_blob_size, data, out_map->header.ppnt_offset,
                 out_map->header.anim_offset - out_map->header.ppnt_offset, total, error, error_size)) {
    free(data);
    gloom_map_free(out_map);
    return false;
  }

  if (!parse_grid_cells(out_map, error, error_size) ||
      !parse_zones(out_map, data, total, error, error_size) ||
      !parse_animations(out_map, data, total, error, error_size) ||
      !parse_texture_names(out_map, data, total, error, error_size) ||
      !parse_events(out_map, data, total, error, error_size)) {
    free(data);
    gloom_map_free(out_map);
    return false;
  }

  free(data);
  return true;
}
