#ifndef GLOOM_MAP_H
#define GLOOM_MAP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

enum {
  GLOOM_EVENT_COUNT = 24,
  GLOOM_TEXTURE_NAME_COUNT = 8,
  GLOOM_GRID_WIDTH = 32,
  GLOOM_GRID_HEIGHT = 32,
  GLOOM_GRID_CELL_COUNT = GLOOM_GRID_WIDTH * GLOOM_GRID_HEIGHT,
  GLOOM_GRID_CELL_SIZE_BYTES = 8,
  GLOOM_ZONE_SIZE_BYTES = 32,
  GLOOM_HEADER_SIZE_BYTES = 20 + (GLOOM_EVENT_COUNT * 4)
};

typedef struct {
  uint32_t grid_offset;
  uint32_t poly_offset;
  uint32_t ppnt_offset;
  uint32_t anim_offset;
  uint32_t texname_offset;
  uint32_t event_offsets[GLOOM_EVENT_COUNT];
} GloomMapHeader;

typedef struct {
  int16_t ztype;
  int16_t x1;
  int16_t z1;
  int16_t x2;
  int16_t z2;
  int16_t a;
  int16_t b;
  int16_t na;
  int16_t nb;
  int16_t ln;
  uint8_t textures[8];
  int16_t scale;
  int16_t event_id;
} GloomZone;

typedef struct {
  uint16_t frame_count;
  uint16_t first_frame;
  uint16_t delay;
  uint16_t current;
} GloomAnim;

typedef struct {
  int16_t count_minus_one;
  uint16_t ppnt_word_offset;
} GloomGridCell;

typedef struct {
  uint32_t offset;
  uint16_t command_count;
  uint16_t add_monster_count;
  uint16_t open_door_count;
  uint16_t teleport_count;
  uint16_t load_objects_count;
  uint16_t texture_change_count;
  uint16_t rotate_poly_count;
  uint16_t unknown_count;
} GloomEventStats;

typedef struct {
  int16_t object_type;
  int16_t x;
  int16_t y;
  int16_t z;
  int16_t rotation;
  uint8_t event_id;
} GloomObjectSpawn;

typedef struct {
  uint8_t event_id;
  uint16_t zone_index;
} GloomDoorCommand;

typedef struct {
  uint8_t event_id;
  uint16_t zone_index;
  uint8_t texture_index;
} GloomTextureChangeCommand;

typedef struct {
  uint8_t event_id;
  int16_t x;
  int16_t control_word;
  int16_t z;
  int16_t rotation;
} GloomTeleportCommand;

typedef struct {
  uint8_t event_id;
  uint16_t first_zone_index;
  uint16_t zone_count;
  int16_t speed;
  uint16_t flags;
} GloomRotPolyCommand;

typedef struct {
  GloomMapHeader header;

  GloomZone *zones;
  size_t zone_count;

  GloomAnim *animations;
  size_t animation_count;

  char texture_names[GLOOM_TEXTURE_NAME_COUNT][64];

  uint8_t *grid_blob;
  size_t grid_blob_size;

  GloomGridCell wall_grid[GLOOM_GRID_CELL_COUNT];
  GloomGridCell trigger_grid[GLOOM_GRID_CELL_COUNT];
  bool has_grid_cells;

  uint8_t *ppnt_blob;
  size_t ppnt_blob_size;

  GloomObjectSpawn *object_spawns;
  size_t object_spawn_count;

  GloomDoorCommand *door_commands;
  size_t door_command_count;

  GloomTextureChangeCommand *texture_change_commands;
  size_t texture_change_command_count;

  GloomTeleportCommand *teleport_commands;
  size_t teleport_command_count;

  GloomRotPolyCommand *rotpoly_commands;
  size_t rotpoly_command_count;

  GloomEventStats events[GLOOM_EVENT_COUNT];

  size_t file_size;
} GloomMap;

bool gloom_map_load(const char *path, GloomMap *out_map, char *error, size_t error_size);
void gloom_map_free(GloomMap *map);
bool gloom_decrunch_crm_buffer(uint8_t **io_data, size_t *io_size, char *error, size_t error_size);

#endif
