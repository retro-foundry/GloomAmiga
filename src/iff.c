#include "iff.h"

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

static bool in_bounds(size_t offset, size_t needed, size_t total) {
  return offset <= total && needed <= (total - offset);
}

static bool read_file_bytes(const char *path, uint8_t **out_data, size_t *out_size, char *error, size_t error_size) {
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

static bool checked_mul_size(size_t a, size_t b, size_t *out) {
  if (a > 0u && b > (SIZE_MAX / a)) {
    return false;
  }
  *out = a * b;
  return true;
}

static size_t ilbm_row_bytes(uint16_t width) {
  return (((size_t)width + 15u) / 16u) * 2u;
}

static bool decode_byterun1(const uint8_t *src, size_t src_size, uint8_t *dst, size_t dst_size, char *error,
                            size_t error_size) {
  size_t src_pos = 0;
  size_t dst_pos = 0;

  while (dst_pos < dst_size) {
    int8_t control = 0;

    if (src_pos >= src_size) {
      return set_error(error, error_size, "ByteRun1 source ended early (%zu/%zu)", dst_pos, dst_size);
    }

    control = (int8_t)src[src_pos++];

    if (control >= 0) {
      size_t count = (size_t)control + 1u;
      if (!in_bounds(src_pos, count, src_size) || !in_bounds(dst_pos, count, dst_size)) {
        return set_error(error, error_size, "ByteRun1 literal run is out of bounds");
      }
      memcpy(dst + dst_pos, src + src_pos, count);
      src_pos += count;
      dst_pos += count;
    } else if (control != -128) {
      size_t count = (size_t)(1 - (int)control);
      uint8_t value = 0;

      if (src_pos >= src_size || !in_bounds(dst_pos, count, dst_size)) {
        return set_error(error, error_size, "ByteRun1 repeated run is out of bounds");
      }

      value = src[src_pos++];
      memset(dst + dst_pos, value, count);
      dst_pos += count;
    }
  }

  return true;
}

static bool decode_ilbm_pixels(GloomIffImage *image, char *error, size_t error_size) {
  size_t row_bytes = 0;
  uint8_t mask_planes = 0;
  size_t planes_in_body = 0;
  size_t scanline_bytes = 0;
  size_t planar_size = 0;
  size_t pixel_count = 0;
  uint8_t *planar = NULL;
  uint8_t *pixels = NULL;
  size_t y = 0;

  if (strcmp(image->form_type, "ILBM") != 0) {
    return true;
  }

  if (!image->has_bmhd || image->body == NULL || image->body_size == 0u) {
    return true;
  }

  if (image->width == 0u || image->height == 0u) {
    return set_error(error, error_size, "ILBM image has invalid dimensions %ux%u", (unsigned)image->width,
                     (unsigned)image->height);
  }

  if (image->planes == 0u || image->planes > 8u) {
    return set_error(error, error_size, "ILBM bitplane count %u is not supported", (unsigned)image->planes);
  }

  if (image->masking > 2u) {
    return set_error(error, error_size, "ILBM masking mode %u is not supported", (unsigned)image->masking);
  }

  row_bytes = ilbm_row_bytes(image->width);
  mask_planes = image->masking == 1u ? 1u : 0u;
  planes_in_body = (size_t)image->planes + (size_t)mask_planes;

  if (!checked_mul_size(row_bytes, planes_in_body, &scanline_bytes) ||
      !checked_mul_size(scanline_bytes, (size_t)image->height, &planar_size) ||
      !checked_mul_size((size_t)image->width, (size_t)image->height, &pixel_count)) {
    return set_error(error, error_size, "ILBM image dimensions are too large to decode safely");
  }

  planar = (uint8_t *)malloc(planar_size);
  pixels = (uint8_t *)malloc(pixel_count);
  if (planar == NULL || pixels == NULL) {
    free(planar);
    free(pixels);
    return set_error(error, error_size, "Out of memory while decoding ILBM BODY");
  }

  if (image->compression == 0u) {
    if (image->body_size < planar_size) {
      free(planar);
      free(pixels);
      return set_error(error, error_size, "ILBM BODY is smaller than expected (%zu < %zu)", image->body_size,
                       planar_size);
    }
    memcpy(planar, image->body, planar_size);
  } else if (image->compression == 1u) {
    if (!decode_byterun1(image->body, image->body_size, planar, planar_size, error, error_size)) {
      free(planar);
      free(pixels);
      return false;
    }
  } else {
    free(planar);
    free(pixels);
    return set_error(error, error_size, "ILBM compression %u is not supported", (unsigned)image->compression);
  }

  for (y = 0; y < (size_t)image->height; ++y) {
    size_t x = 0;
    size_t row_base = y * scanline_bytes;

    for (x = 0; x < (size_t)image->width; ++x) {
      uint8_t index = 0;
      size_t plane = 0;

      for (plane = 0; plane < (size_t)image->planes; ++plane) {
        const uint8_t *plane_row = planar + row_base + (plane * row_bytes);
        uint8_t packed = plane_row[x >> 3u];
        uint8_t bit = (uint8_t)((packed >> (7u - (x & 7u))) & 1u);
        index |= (uint8_t)(bit << plane);
      }

      pixels[(y * (size_t)image->width) + x] = index;
    }
  }

  free(planar);
  free(image->pixels);
  image->pixels = pixels;
  image->pixel_count = pixel_count;
  image->has_decoded_pixels = true;
  return true;
}

void gloom_iff_free(GloomIffImage *image) {
  if (image == NULL) {
    return;
  }

  free(image->palette);
  free(image->body);
  free(image->pixels);
  memset(image, 0, sizeof(*image));
}

static bool parse_bmhd(GloomIffImage *image, const uint8_t *chunk_data, size_t chunk_size, char *error, size_t error_size) {
  if (chunk_size < 20u) {
    return set_error(error, error_size, "BMHD chunk too small (%zu)", chunk_size);
  }

  image->has_bmhd = true;
  image->width = read_be16(chunk_data + 0);
  image->height = read_be16(chunk_data + 2);
  image->planes = chunk_data[8];
  image->masking = chunk_data[9];
  image->compression = chunk_data[10];
  image->transparent_color = read_be16(chunk_data + 12);
  return true;
}

static bool parse_cmap(GloomIffImage *image, const uint8_t *chunk_data, size_t chunk_size, char *error, size_t error_size) {
  size_t color_count = 0;
  size_t i = 0;
  GloomRgb *palette = NULL;

  if ((chunk_size % 3u) != 0u) {
    return set_error(error, error_size, "CMAP chunk size %zu is not a multiple of 3", chunk_size);
  }

  color_count = chunk_size / 3u;
  palette = (GloomRgb *)malloc(color_count * sizeof(*palette));
  if (color_count > 0u && palette == NULL) {
    return set_error(error, error_size, "Out of memory while allocating CMAP (%zu colors)", color_count);
  }

  for (i = 0; i < color_count; ++i) {
    palette[i].r = chunk_data[i * 3u + 0u];
    palette[i].g = chunk_data[i * 3u + 1u];
    palette[i].b = chunk_data[i * 3u + 2u];
  }

  free(image->palette);
  image->palette = palette;
  image->palette_count = color_count;
  return true;
}

static bool parse_body(GloomIffImage *image, const uint8_t *chunk_data, size_t chunk_size, char *error, size_t error_size) {
  uint8_t *body = NULL;

  body = (uint8_t *)malloc(chunk_size);
  if (chunk_size > 0u && body == NULL) {
    return set_error(error, error_size, "Out of memory while allocating BODY (%zu bytes)", chunk_size);
  }

  if (chunk_size > 0u) {
    memcpy(body, chunk_data, chunk_size);
  }

  free(image->body);
  image->body = body;
  image->body_size = chunk_size;
  return true;
}

bool gloom_iff_load(const char *path, GloomIffImage *out_image, char *error, size_t error_size) {
  uint8_t *data = NULL;
  size_t total = 0;
  size_t form_size = 0;
  size_t form_end = 0;
  size_t offset = 0;

  if (out_image == NULL || path == NULL) {
    return set_error(error, error_size, "Invalid IFF load arguments");
  }

  memset(out_image, 0, sizeof(*out_image));

  if (!read_file_bytes(path, &data, &total, error, error_size)) {
    return false;
  }

  if (total < 12u) {
    free(data);
    return set_error(error, error_size, "IFF file too small (%zu bytes)", total);
  }

  if (memcmp(data, "FORM", 4u) != 0) {
    free(data);
    return set_error(error, error_size, "%s is not an IFF FORM", path);
  }

  form_size = (size_t)read_be32(data + 4);
  form_end = 8u + form_size;
  if (form_end > total) {
    free(data);
    return set_error(error, error_size, "IFF FORM size (%zu) exceeds file size (%zu)", form_end, total);
  }

  memcpy(out_image->form_type, data + 8, 4u);
  out_image->form_type[4] = '\0';

  offset = 12u;
  while (offset + 8u <= form_end) {
    const uint8_t *chunk = data + offset;
    size_t chunk_size = (size_t)read_be32(chunk + 4);
    size_t chunk_data_offset = offset + 8u;
    size_t chunk_padded_size = chunk_size + (chunk_size & 1u);

    if (!in_bounds(chunk_data_offset, chunk_padded_size, total) || (chunk_data_offset + chunk_padded_size) > form_end) {
      free(data);
      gloom_iff_free(out_image);
      return set_error(error, error_size, "IFF chunk is truncated at offset 0x%zx", offset);
    }

    if (memcmp(chunk, "BMHD", 4u) == 0) {
      if (!parse_bmhd(out_image, data + chunk_data_offset, chunk_size, error, error_size)) {
        free(data);
        gloom_iff_free(out_image);
        return false;
      }
    } else if (memcmp(chunk, "CMAP", 4u) == 0) {
      if (!parse_cmap(out_image, data + chunk_data_offset, chunk_size, error, error_size)) {
        free(data);
        gloom_iff_free(out_image);
        return false;
      }
    } else if (memcmp(chunk, "BODY", 4u) == 0) {
      if (!parse_body(out_image, data + chunk_data_offset, chunk_size, error, error_size)) {
        free(data);
        gloom_iff_free(out_image);
        return false;
      }
    }

    offset = chunk_data_offset + chunk_padded_size;
  }

  if (!decode_ilbm_pixels(out_image, error, error_size)) {
    free(data);
    gloom_iff_free(out_image);
    return false;
  }

  free(data);
  return true;
}
