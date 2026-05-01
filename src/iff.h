#ifndef GLOOM_IFF_H
#define GLOOM_IFF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct {
  uint8_t r;
  uint8_t g;
  uint8_t b;
} GloomRgb;

typedef struct {
  char form_type[5];
  bool has_bmhd;
  bool has_decoded_pixels;
  uint16_t width;
  uint16_t height;
  uint8_t planes;
  uint8_t masking;
  uint8_t compression;
  uint16_t transparent_color;

  GloomRgb *palette;
  size_t palette_count;

  uint8_t *body;
  size_t body_size;

  uint8_t *pixels;
  size_t pixel_count;
} GloomIffImage;

bool gloom_iff_load(const char *path, GloomIffImage *out_image, char *error, size_t error_size);
void gloom_iff_free(GloomIffImage *image);

#endif
