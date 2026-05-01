#include <SDL.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "iff.h"
#include "map.h"

#ifdef _WIN32
#include <windows.h>
#endif

enum {
  BASE_WIDTH = 320,
  BASE_HEIGHT = 224,
  WIDESCREEN_WIDTH = 400,
  FIXED_TICK_HZ = 50,
  GLOOM_MAX_ACTIVE_DOORS = 16,
  GLOOM_MAX_ACTIVE_ROTPOLYS = 32,
  GLOOM_MAX_ROTPOLY_ZONES = 32,
  GLOOM_TELEPORT_DELAY_TICKS = 12,
  GLOOM_AMIGA_FOCAL_SHIFT = 6,
  GLOOM_AMIGA_FOCAL = 1 << GLOOM_AMIGA_FOCAL_SHIFT,
  GLOOM_AMIGA_VIEW_COLUMNS = 106,
  GLOOM_AMIGA_VIEW_ROWS = 80,
  GLOOM_AMIGA_DARK_SHIFT = 7,
  GLOOM_AMIGA_MAX_Z = 16 << GLOOM_AMIGA_DARK_SHIFT,
  GLOOM_PLAYER_EYE_Y = -110,
  GLOOM_WALL_TOP_Y = -256,
  GLOOM_WALL_BOTTOM_Y = 0,
  GLOOM_PLAYER_MOVESPEED = 13,
  GLOOM_PLAYER_ROT_ACC = 0x20000,
  GLOOM_PLAYER_ROT_REV_ACC = 0x40000,
  GLOOM_PLAYER_ROT_SET_ACC = 0x20000,
  GLOOM_PLAYER_MAX_ROT_SPEED = 0x40000,
  GLOOM_PLAYER_BOUNCE_STEP = 20,
  GLOOM_PLAYER_UNBOUNCE_STEP = 30,
  GLOOM_PLAYER_BOB_HEIGHT = 20,
  GLOOM_MOUSE_LOOK_FIXED_PER_PIXEL = 0x2000
};

typedef struct {
  bool active;
  size_t zone_index;
  int16_t base_x1;
  int16_t base_z1;
  int16_t base_x2;
  int16_t base_z2;
  int16_t frac;
  int16_t frac_add;
} RuntimeDoor;

typedef struct {
  int16_t base_x;
  int16_t base_z;
  int16_t base_na;
  int16_t base_nb;
  int16_t delta_x;
  int16_t delta_z;
} RuntimeRotPolyVertex;

typedef struct {
  bool active;
  size_t first_zone_index;
  uint16_t zone_count;
  int16_t speed;
  int16_t rot;
  uint16_t flags;
  int16_t center_x;
  int16_t center_z;
  RuntimeRotPolyVertex vertices[GLOOM_MAX_ROTPOLY_ZONES];
} RuntimeRotPoly;

typedef struct {
  uint64_t tick_count;
  GloomMap map;
  int16_t min_x;
  int16_t max_x;
  int16_t min_z;
  int16_t max_z;
  float camera_x;
  float camera_z;
  float camera_y;
  float camera_angle;
  int32_t player_rot_fixed;
  int32_t player_rotspeed;
  int16_t player_bounce;
  int16_t player_radius;
  bool triggered_events[GLOOM_EVENT_COUNT + 1];
  bool teleport_active;
  uint8_t teleport_ticks;
  int16_t teleport_x;
  int16_t teleport_z;
  int16_t teleport_rotation;
  RuntimeDoor doors[GLOOM_MAX_ACTIVE_DOORS];
  RuntimeRotPoly rotpolys[GLOOM_MAX_ACTIVE_ROTPOLYS];
} AppState;

typedef struct {
  uint32_t parsed;
  uint32_t skipped;
  uint32_t failed;
} SweepStats;

typedef struct {
  SDL_Texture *texture;
  uint32_t *argb_pixels;
  int width;
  int height;
} PreviewAsset;

enum {
  GLOOM_TEXTURE_SCREENS = 8,
  GLOOM_TEXTURES_PER_SCREEN = 20,
  GLOOM_TEXTURE_WIDTH = 64,
  GLOOM_TEXTURE_HEIGHT = 64,
  GLOOM_TEXTURE_COLUMN_BYTES = 65,
  GLOOM_FLAT_TEXTURE_SIZE = 128,
  GLOOM_MAX_RENDER_ZONES = 2048,
  GLOOM_MAX_WALL_CANDIDATES = 2048,
  GLOOM_MAX_COLUMN_WALL_HITS = 128,
  GLOOM_MAX_DEBUG_SPRITES = 128,
  GLOOM_OBJECT_TYPE_COUNT = 24
};

typedef struct {
  bool loaded;
  char source_name[64];
  uint32_t palette[256];
  uint8_t *texels;
  uint8_t *column_flags;
  size_t texture_count;
} WallTextureScreen;

typedef struct {
  WallTextureScreen screens[GLOOM_TEXTURE_SCREENS];
  size_t loaded_count;
} WallTextureSet;

typedef struct {
  bool loaded;
  char source_name[64];
  uint32_t palette[256];
  uint8_t *texels;
} FlatTexture;

typedef struct {
  char tile_tag[32];
  FlatTexture floor;
  FlatTexture roof;
} FlatTextureSet;

typedef struct {
  int16_t handle_x;
  int16_t handle_y;
  int16_t width;
  int16_t height;
  uint32_t *argb_pixels;
} ObjectFrame;

typedef struct {
  bool loaded;
  bool eight_rotations;
  uint16_t scale;
  uint16_t fixed_frame;
  bool use_fixed_frame;
  uint16_t rotation_shift;
  uint16_t frame_count_per_rotation;
  size_t frame_count;
  char source_name[64];
  ObjectFrame *frames;
} ObjectVisual;

typedef struct {
  const char *asset_name;
  bool eight_rotations;
  uint16_t scale;
  uint16_t fixed_frame;
  bool use_fixed_frame;
} ObjectVisualDefinition;

typedef struct {
  ObjectVisual visuals[GLOOM_OBJECT_TYPE_COUNT];
} ObjectVisualSet;

typedef struct {
  int16_t x;
  int16_t z;
} GridOffset;

typedef struct {
  GridOffset *items;
  size_t count;
} GridOffsetSet;

typedef struct {
  size_t zone_index;
  float sort_depth;
} WallCandidate;

typedef struct {
  const GloomZone *zone;
  float vx1;
  float vz1;
  float vx2;
  float vz2;
  float sort_depth;
} SceneWall;

typedef struct {
  const SceneWall *wall;
  float depth;
  float wall_u;
} RayWallHit;

typedef struct {
  int16_t joyx;
  int16_t joyy;
  int16_t joys;
} PlayerControls;

static float clampf(float value, float lo, float hi);
static int32_t amiga_rotation_to_fixed(int16_t rotation);
static float amiga_rotation_to_radians(int16_t rotation);
static float player_rotation_fixed_to_radians(int32_t rotation_fixed);
static void update_player_camera_y(AppState *state);
static bool resolve_runtime_file_path(const char *input_path, char *out_path, size_t out_path_size);
static bool resolve_player_wall_collision(AppState *state, float *x, float *z);
static void update_rotpolys(AppState *state);
static void update_doors(AppState *state);
static void check_event_triggers(AppState *state);

static void compute_map_bounds(AppState *state) {
  size_t i = 0;

  if (state->map.zone_count == 0) {
    state->min_x = -128;
    state->max_x = 128;
    state->min_z = -128;
    state->max_z = 128;
    return;
  }

  state->min_x = state->max_x = state->map.zones[0].x1;
  state->min_z = state->max_z = state->map.zones[0].z1;

  for (i = 0; i < state->map.zone_count; ++i) {
    const GloomZone *z = &state->map.zones[i];

    if (z->x1 < state->min_x) state->min_x = z->x1;
    if (z->x2 < state->min_x) state->min_x = z->x2;
    if (z->x1 > state->max_x) state->max_x = z->x1;
    if (z->x2 > state->max_x) state->max_x = z->x2;

    if (z->z1 < state->min_z) state->min_z = z->z1;
    if (z->z2 < state->min_z) state->min_z = z->z2;
    if (z->z1 > state->max_z) state->max_z = z->z1;
    if (z->z2 > state->max_z) state->max_z = z->z2;
  }
}

static bool initialize_camera_from_map_spawn(AppState *state) {
  const GloomObjectSpawn *spawn = NULL;
  size_t i = 0;

  if (state == NULL) {
    return false;
  }

  for (i = 0; i < state->map.object_spawn_count; ++i) {
    const GloomObjectSpawn *candidate = &state->map.object_spawns[i];

    if (candidate->event_id == 1u && candidate->object_type == 0) {
      spawn = candidate;
      break;
    }
  }

  if (spawn == NULL) {
    for (i = 0; i < state->map.object_spawn_count; ++i) {
      const GloomObjectSpawn *candidate = &state->map.object_spawns[i];

      if (candidate->object_type == 0) {
        spawn = candidate;
        break;
      }
    }
  }

  if (spawn == NULL) {
    return false;
  }

  state->camera_x = (float)spawn->x;
  state->camera_z = (float)spawn->z;
  state->player_rot_fixed = amiga_rotation_to_fixed(spawn->rotation);
  state->player_rotspeed = 0;
  state->player_bounce = 0;
  state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
  update_player_camera_y(state);
  return true;
}

static void sweep_file(const char *path, SweepStats *stats) {
  GloomMap map;
  char error[256] = {0};

  memset(&map, 0, sizeof(map));

  if (gloom_map_load(path, &map, error, sizeof(error))) {
    stats->parsed += 1;
    gloom_map_free(&map);
  } else {
    stats->failed += 1;
    fprintf(stderr, "[FAIL] %s: %s\n", path, error[0] ? error : "unknown error");
  }
}

#ifdef _WIN32
static void sweep_directory(const char *dir, SweepStats *stats) {
  char pattern[MAX_PATH] = {0};
  WIN32_FIND_DATAA entry;
  HANDLE hfind = INVALID_HANDLE_VALUE;

  (void)snprintf(pattern, sizeof(pattern), "%s\\*", dir);
  hfind = FindFirstFileA(pattern, &entry);
  if (hfind == INVALID_HANDLE_VALUE) {
    fprintf(stderr, "[WARN] Could not read directory %s\n", dir);
    return;
  }

  do {
    char full_path[MAX_PATH] = {0};

    if (strcmp(entry.cFileName, ".") == 0 || strcmp(entry.cFileName, "..") == 0) {
      continue;
    }

    if ((entry.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
      continue;
    }

    (void)snprintf(full_path, sizeof(full_path), "%s\\%s", dir, entry.cFileName);
    sweep_file(full_path, stats);
  } while (FindNextFileA(hfind, &entry) != 0);

  FindClose(hfind);
}
#else
static void sweep_directory(const char *dir, SweepStats *stats) {
  (void)stats;
  fprintf(stderr, "[WARN] Sweep mode is only implemented for Windows in this bootstrap. Skipping %s\n", dir);
}
#endif

static uint32_t total_event_commands(const GloomMap *map) {
  uint32_t total = 0;
  size_t i = 0;

  for (i = 0; i < GLOOM_EVENT_COUNT; ++i) {
    total += map->events[i].command_count;
  }

  return total;
}

static int run_iff_info(const char *path) {
  GloomIffImage image;
  char resolved_path[1024] = {0};
  const char *load_path = path;
  char error[256] = {0};
  size_t i = 0;
  size_t preview_count = 0;

  memset(&image, 0, sizeof(image));

  if (resolve_runtime_file_path(path, resolved_path, sizeof(resolved_path))) {
    load_path = resolved_path;
  }

  if (!gloom_iff_load(load_path, &image, error, sizeof(error))) {
    fprintf(stderr, "IFF parse failed for %s: %s\n", load_path, error[0] ? error : "unknown error");
    return 1;
  }

  printf("IFF: %s\n", load_path);
    printf("form=%s bmhd=%s width=%u height=%u planes=%u compression=%u palette=%zu body=%zu decoded=%s pixels=%zu\n",
      image.form_type,
         image.has_bmhd ? "yes" : "no", (unsigned)image.width, (unsigned)image.height, (unsigned)image.planes,
      (unsigned)image.compression, image.palette_count, image.body_size, image.has_decoded_pixels ? "yes" : "no",
      image.pixel_count);

  preview_count = image.palette_count < 8u ? image.palette_count : 8u;
  if (preview_count > 0u) {
    printf("palette_preview:");
    for (i = 0; i < preview_count; ++i) {
      printf(" [%zu]=(%u,%u,%u)", i, (unsigned)image.palette[i].r, (unsigned)image.palette[i].g,
             (unsigned)image.palette[i].b);
    }
    printf("\n");
  }

  gloom_iff_free(&image);
  return 0;
}

static uint32_t palette_to_argb(const GloomIffImage *image, uint8_t index) {
  uint8_t r = 0;
  uint8_t g = 0;
  uint8_t b = 0;

  if (image->palette_count > 0u) {
    size_t palette_index = (size_t)index % image->palette_count;
    r = image->palette[palette_index].r;
    g = image->palette[palette_index].g;
    b = image->palette[palette_index].b;
  } else {
    uint32_t max_index = (1u << image->planes) - 1u;
    uint8_t value = max_index > 0u ? (uint8_t)(((uint32_t)index * 255u) / max_index) : 0u;
    r = value;
    g = value;
    b = value;
  }

  return 0xFF000000u | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
}

static uint16_t main_read_be16(const uint8_t *p) {
  return (uint16_t)(((uint16_t)p[0] << 8) | (uint16_t)p[1]);
}

static uint32_t main_read_be32(const uint8_t *p) {
  return ((uint32_t)p[0] << 24) | ((uint32_t)p[1] << 16) | ((uint32_t)p[2] << 8) | (uint32_t)p[3];
}

static bool read_binary_blob(const char *path, uint8_t **out_data, size_t *out_size) {
  FILE *f = NULL;
  uint8_t *buffer = NULL;
  long file_size = 0;
  size_t read_bytes = 0;

  if (path == NULL || out_data == NULL || out_size == NULL) {
    return false;
  }

  *out_data = NULL;
  *out_size = 0u;

  f = fopen(path, "rb");
  if (f == NULL) {
    return false;
  }

  if (fseek(f, 0, SEEK_END) != 0) {
    fclose(f);
    return false;
  }

  file_size = ftell(f);
  if (file_size < 0) {
    fclose(f);
    return false;
  }

  if (fseek(f, 0, SEEK_SET) != 0) {
    fclose(f);
    return false;
  }

  buffer = (uint8_t *)malloc((size_t)file_size);
  if (buffer == NULL && file_size > 0) {
    fclose(f);
    return false;
  }

  read_bytes = fread(buffer, 1, (size_t)file_size, f);
  fclose(f);

  if (read_bytes != (size_t)file_size) {
    free(buffer);
    return false;
  }

  *out_data = buffer;
  *out_size = (size_t)file_size;
  return true;
}

static bool file_exists_readable(const char *path) {
  FILE *f = fopen(path, "rb");
  if (f == NULL) {
    return false;
  }
  fclose(f);
  return true;
}

static void trim_trailing_slashes(char *path) {
  size_t len = 0;

  if (path == NULL) {
    return;
  }

  len = strlen(path);
  while (len > 0u && (path[len - 1u] == '/' || path[len - 1u] == '\\')) {
    path[len - 1u] = '\0';
    len -= 1u;
  }
}

static bool pop_path_component(char *path) {
  char *slash1 = NULL;
  char *slash2 = NULL;
  char *slash = NULL;

  if (path == NULL || path[0] == '\0') {
    return false;
  }

  trim_trailing_slashes(path);
  slash1 = strrchr(path, '/');
  slash2 = strrchr(path, '\\');
  slash = slash1;

  if (slash2 != NULL && (slash == NULL || slash2 > slash)) {
    slash = slash2;
  }

  if (slash == NULL) {
    return false;
  }

  *slash = '\0';
  return path[0] != '\0';
}

static bool get_executable_dir(char *out_dir, size_t out_dir_size) {
  char *base_path = NULL;

  if (out_dir == NULL || out_dir_size == 0u) {
    return false;
  }

  out_dir[0] = '\0';
  base_path = SDL_GetBasePath();
  if (base_path != NULL && base_path[0] != '\0') {
    (void)snprintf(out_dir, out_dir_size, "%s", base_path);
    SDL_free(base_path);
    trim_trailing_slashes(out_dir);
    return out_dir[0] != '\0';
  }

  if (base_path != NULL) {
    SDL_free(base_path);
  }

#ifdef _WIN32
  {
    char module_path[4096] = {0};
    DWORD len = GetModuleFileNameA(NULL, module_path, (DWORD)sizeof(module_path));
    size_t i = 0;

    if (len == 0u || len >= (DWORD)sizeof(module_path)) {
      return false;
    }

    for (i = (size_t)len; i > 0u; --i) {
      if (module_path[i - 1u] == '\\' || module_path[i - 1u] == '/') {
        module_path[i - 1u] = '\0';
        break;
      }
    }

    (void)snprintf(out_dir, out_dir_size, "%s", module_path);
    trim_trailing_slashes(out_dir);
    return out_dir[0] != '\0';
  }
#else
  return false;
#endif
}

static bool resolve_runtime_file_path(const char *input_path, char *out_path, size_t out_path_size) {
  char exe_dir[1024] = {0};
  char search_root[1024] = {0};
  char candidate[1280] = {0};
  size_t depth = 0;

  if (input_path == NULL || input_path[0] == '\0' || out_path == NULL || out_path_size == 0u) {
    return false;
  }

  if (file_exists_readable(input_path)) {
    (void)snprintf(out_path, out_path_size, "%s", input_path);
    return true;
  }

  if (!get_executable_dir(exe_dir, sizeof(exe_dir))) {
    return false;
  }

  (void)snprintf(search_root, sizeof(search_root), "%s", exe_dir);

  for (depth = 0; depth < 8u; ++depth) {
    (void)snprintf(candidate, sizeof(candidate), "%s/%s", search_root, input_path);
    if (file_exists_readable(candidate)) {
      (void)snprintf(out_path, out_path_size, "%s", candidate);
      return true;
    }

    if (!pop_path_component(search_root)) {
      break;
    }
  }

  return false;
}

static bool resolve_texture_screen_path(const char *name, char *out_path, size_t out_path_size) {
  const char *candidates[6] = {NULL, NULL, NULL, NULL, NULL, NULL};
  char path0[256] = {0};
  char path1[256] = {0};
  char path2[256] = {0};
  char path3[256] = {0};
  char path4[256] = {0};
  char path5[256] = {0};
  size_t i = 0;

  if (name == NULL || name[0] == '\0' || out_path == NULL || out_path_size == 0u) {
    return false;
  }

  (void)snprintf(path0, sizeof(path0), "%s", name);
  (void)snprintf(path1, sizeof(path1), "amiga/%s", name);
  (void)snprintf(path2, sizeof(path2), "amiga/txts/%s", name);
  (void)snprintf(path3, sizeof(path3), "amiga/data/txts/%s", name);
  (void)snprintf(path4, sizeof(path4), "amiga/ggfx/%s", name);
  (void)snprintf(path5, sizeof(path5), "amiga/data/ggfx/%s", name);

  candidates[0] = path0;
  candidates[1] = path1;
  candidates[2] = path2;
  candidates[3] = path3;
  candidates[4] = path4;
  candidates[5] = path5;

  for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    if (resolve_runtime_file_path(candidates[i], out_path, out_path_size)) {
      return true;
    }
  }

  return false;
}

static bool resolve_object_asset_path(const char *name, char *out_path, size_t out_path_size) {
  const char *candidates[7] = {NULL, NULL, NULL, NULL, NULL, NULL, NULL};
  char path0[256] = {0};
  char path1[256] = {0};
  char path2[256] = {0};
  char path3[256] = {0};
  char path4[256] = {0};
  char path5[256] = {0};
  char path6[256] = {0};
  size_t i = 0u;

  if (name == NULL || name[0] == '\0' || out_path == NULL || out_path_size == 0u) {
    return false;
  }

  (void)snprintf(path0, sizeof(path0), "%s", name);
  (void)snprintf(path1, sizeof(path1), "amiga/%s", name);
  (void)snprintf(path2, sizeof(path2), "amiga/objs/%s", name);
  (void)snprintf(path3, sizeof(path3), "amiga/data/objs/%s", name);
  (void)snprintf(path4, sizeof(path4), "amiga/prog/objs/%s", name);
  (void)snprintf(path5, sizeof(path5), "objs/%s", name);
  (void)snprintf(path6, sizeof(path6), "data/objs/%s", name);

  candidates[0] = path0;
  candidates[1] = path1;
  candidates[2] = path2;
  candidates[3] = path3;
  candidates[4] = path4;
  candidates[5] = path5;
  candidates[6] = path6;

  for (i = 0u; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    if (resolve_runtime_file_path(candidates[i], out_path, out_path_size)) {
      return true;
    }
  }

  return false;
}

static bool load_player_collision_radius(int16_t *out_radius) {
  const char *candidates[2] = {"amiga/objs/player", "amiga/prog/objs/player"};
  size_t i = 0u;

  if (out_radius == NULL) {
    return false;
  }

  for (i = 0u; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    char resolved_path[1024] = {0};
    uint8_t *data = NULL;
    size_t data_size = 0u;
    char error[256] = {0};
    uint16_t radius = 0u;

    if (!resolve_runtime_file_path(candidates[i], resolved_path, sizeof(resolved_path))) {
      continue;
    }

    if (!read_binary_blob(resolved_path, &data, &data_size)) {
      continue;
    }

    if (!gloom_decrunch_crm_buffer(&data, &data_size, error, sizeof(error))) {
      fprintf(stderr, "Failed to decrunch player object %s: %s\n", resolved_path,
              error[0] ? error : "unknown error");
      free(data);
      return false;
    }

    if (data_size < 6u) {
      fprintf(stderr, "Player object %s is too small after decrunch (%zu bytes)\n", resolved_path, data_size);
      free(data);
      return false;
    }

    radius = main_read_be16(data + 4);
    free(data);

    if (radius == 0u || radius > 512u) {
      fprintf(stderr, "Player object %s has invalid collision radius %u\n", resolved_path, (unsigned)radius);
      return false;
    }

    *out_radius = (int16_t)radius;
    printf("Loaded player collision radius %u from %s\n", (unsigned)radius, resolved_path);
    return true;
  }

  fprintf(stderr, "Missing player object for collision radius\n");
  return false;
}

static void free_grid_offset_set(GridOffsetSet *set) {
  if (set == NULL) {
    return;
  }

  free(set->items);
  memset(set, 0, sizeof(*set));
}

static bool load_grid_offset_set(GridOffsetSet *set) {
  uint8_t *data = NULL;
  size_t file_size = 0;
  size_t count = 0;
  size_t i = 0;
  char resolved_path[1024] = {0};

  if (set == NULL) {
    return false;
  }

  free_grid_offset_set(set);

  if (!resolve_runtime_file_path("amiga/gridoffs4.bin", resolved_path, sizeof(resolved_path))) {
    fprintf(stderr, "[ERROR] Missing original grid search order amiga/gridoffs4.bin\n");
    return false;
  }

  if (!read_binary_blob(resolved_path, &data, &file_size) || file_size == 0u || (file_size % 4u) != 0u) {
    fprintf(stderr, "[ERROR] Failed to read grid search order %s\n", resolved_path);
    free(data);
    return false;
  }

  count = file_size / 4u;
  set->items = (GridOffset *)calloc(count, sizeof(*set->items));
  if (set->items == NULL) {
    fprintf(stderr, "[ERROR] Out of memory while loading %s\n", resolved_path);
    free(data);
    return false;
  }

  for (i = 0; i < count; ++i) {
    const uint8_t *entry = data + (i * 4u);
    set->items[i].x = (int16_t)main_read_be16(entry + 0);
    set->items[i].z = (int16_t)main_read_be16(entry + 2);
  }

  set->count = count;
  free(data);
  printf("Loaded %zu grid wall search offsets from %s\n", set->count, resolved_path);
  return true;
}

static void free_wall_texture_set(WallTextureSet *set) {
  size_t i = 0;

  if (set == NULL) {
    return;
  }

  for (i = 0; i < GLOOM_TEXTURE_SCREENS; ++i) {
    free(set->screens[i].texels);
    free(set->screens[i].column_flags);
  }

  memset(set, 0, sizeof(*set));
}

static void set_default_texture_palette(uint32_t palette[256]) {
  size_t i = 0;

  for (i = 0; i < 256u; ++i) {
    uint8_t v = (uint8_t)i;
    palette[i] = 0xFF000000u | ((uint32_t)v << 16u) | ((uint32_t)v << 8u) | (uint32_t)v;
  }
}

static void load_packed_palette(uint32_t palette[256], const uint8_t *data, size_t data_size, size_t palette_offset) {
  size_t color_count = 0u;
  size_t entry_offset = palette_offset + 2u;
  size_t color_index = 1u;

  if (palette == NULL || data == NULL || palette_offset + 2u > data_size) {
    return;
  }

  color_count = (size_t)main_read_be16(data + palette_offset);
  while (color_index <= color_count && color_index < 256u && entry_offset + 1u < data_size) {
    uint16_t packed = main_read_be16(data + entry_offset);

    if (packed != 0xFFFFu) {
      uint8_t r = (uint8_t)(((packed >> 8) & 0x0Fu) * 17u);
      uint8_t g = (uint8_t)(((packed >> 4) & 0x0Fu) * 17u);
      uint8_t b = (uint8_t)((packed & 0x0Fu) * 17u);
      palette[color_index] = 0xFF000000u | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
    }

    entry_offset += 2u;
    color_index += 1u;
  }
}

static bool load_wall_texture_screen(const char *path, const char *source_name, WallTextureScreen *out_screen) {
  uint8_t *data = NULL;
  size_t file_size = 0;
  uint8_t *texels = NULL;
  uint8_t *column_flags = NULL;
  uint32_t palette_offset = 0u;
  size_t texture_data_end = 0u;
  size_t available_textures = 0u;
  size_t texture_index = 0u;

  if (path == NULL || out_screen == NULL) {
    return false;
  }

  memset(out_screen, 0, sizeof(*out_screen));

  if (!read_binary_blob(path, &data, &file_size) || file_size < 4u) {
    free(data);
    return false;
  }

  texels = (uint8_t *)calloc((size_t)GLOOM_TEXTURES_PER_SCREEN * (size_t)GLOOM_TEXTURE_WIDTH *
                                 (size_t)GLOOM_TEXTURE_HEIGHT,
                             sizeof(*texels));
  if (texels == NULL) {
    free(data);
    return false;
  }

  column_flags = (uint8_t *)calloc((size_t)GLOOM_TEXTURES_PER_SCREEN * (size_t)GLOOM_TEXTURE_WIDTH,
                                   sizeof(*column_flags));
  if (column_flags == NULL) {
    free(texels);
    free(data);
    return false;
  }

  set_default_texture_palette(out_screen->palette);

  palette_offset = main_read_be32(data + 0);
  texture_data_end = file_size;
  if (palette_offset >= 4u && (size_t)palette_offset <= file_size) {
    texture_data_end = (size_t)palette_offset;
  }

  if (texture_data_end > 4u) {
    available_textures = (texture_data_end - 4u) / ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_COLUMN_BYTES);
    if (available_textures > GLOOM_TEXTURES_PER_SCREEN) {
      available_textures = GLOOM_TEXTURES_PER_SCREEN;
    }
  }

  for (texture_index = 0; texture_index < available_textures; ++texture_index) {
    size_t texture_offset = 4u + texture_index * ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_COLUMN_BYTES);
    size_t x = 0;

    if (texture_offset + ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_COLUMN_BYTES) > texture_data_end) {
      break;
    }

    for (x = 0; x < (size_t)GLOOM_TEXTURE_WIDTH; ++x) {
      size_t column_offset = texture_offset + x * (size_t)GLOOM_TEXTURE_COLUMN_BYTES;
      size_t y = 0;
      const uint8_t *column_pixels = data + column_offset + 1u;

      column_flags[(texture_index * (size_t)GLOOM_TEXTURE_WIDTH) + x] = data[column_offset];

      for (y = 0; y < (size_t)GLOOM_TEXTURE_HEIGHT; ++y) {
        size_t dst = texture_index * ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT) +
                     y * (size_t)GLOOM_TEXTURE_WIDTH + x;
        texels[dst] = column_pixels[y];
      }
    }
  }

  load_packed_palette(out_screen->palette, data, file_size, (size_t)palette_offset);

  out_screen->loaded = true;
  out_screen->texels = texels;
  out_screen->column_flags = column_flags;
  out_screen->texture_count = available_textures;
  (void)snprintf(out_screen->source_name, sizeof(out_screen->source_name), "%s", source_name != NULL ? source_name : path);

  free(data);
  return true;
}

static bool load_map_wall_textures(const GloomMap *map, WallTextureSet *set) {
  size_t i = 0;
  bool ok = true;

  if (map == NULL || set == NULL) {
    return false;
  }

  free_wall_texture_set(set);

  for (i = 0; i < GLOOM_TEXTURE_SCREENS; ++i) {
    const char *name = map->texture_names[i];
    char resolved_path[256] = {0};

    if (name[0] == '\0') {
      continue;
    }

    if (!resolve_texture_screen_path(name, resolved_path, sizeof(resolved_path))) {
      fprintf(stderr, "Missing texture screen '%s'\n", name);
      ok = false;
      continue;
    }

    if (!load_wall_texture_screen(resolved_path, name, &set->screens[i])) {
      fprintf(stderr, "Failed to decode texture screen %s (%s)\n", name, resolved_path);
      ok = false;
      continue;
    }

    set->loaded_count += 1u;
  }

  return ok && set->loaded_count > 0u;
}

static bool validate_map_wall_texture_references(const GloomMap *map, const WallTextureSet *set) {
  bool ok = true;
  size_t zone_index = 0;

  if (map == NULL || set == NULL) {
    return false;
  }

  for (zone_index = 0; zone_index < map->zone_count; ++zone_index) {
    const GloomZone *zone = &map->zones[zone_index];
    size_t slot = 0;

    for (slot = 0; slot < sizeof(zone->textures); ++slot) {
      uint8_t texture_index = zone->textures[slot];
      size_t screen_index = texture_index / (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
      size_t local_index = texture_index % (uint8_t)GLOOM_TEXTURES_PER_SCREEN;

      if (texture_index == 255u) {
        continue;
      }

      if (screen_index >= GLOOM_TEXTURE_SCREENS || !set->screens[screen_index].loaded) {
        fprintf(stderr, "Zone %zu references texture %u, but texture screen %zu is not loaded\n", zone_index,
                texture_index, screen_index);
        ok = false;
      } else if (local_index >= set->screens[screen_index].texture_count) {
        fprintf(stderr, "Zone %zu references texture %u, but screen %zu only has %zu textures\n", zone_index,
                texture_index, screen_index, set->screens[screen_index].texture_count);
        ok = false;
      }
    }
  }

  for (zone_index = 0; zone_index < map->texture_change_command_count; ++zone_index) {
    const GloomTextureChangeCommand *command = &map->texture_change_commands[zone_index];
    uint8_t texture_index = command->texture_index;
    size_t screen_index = texture_index / (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
    size_t local_index = texture_index % (uint8_t)GLOOM_TEXTURES_PER_SCREEN;

    if (screen_index >= GLOOM_TEXTURE_SCREENS || !set->screens[screen_index].loaded) {
      fprintf(stderr, "Event %u changes zone %u to texture %u, but texture screen %zu is not loaded\n",
              (unsigned)command->event_id, (unsigned)command->zone_index, texture_index, screen_index);
      ok = false;
    } else if (local_index >= set->screens[screen_index].texture_count) {
      fprintf(stderr, "Event %u changes zone %u to texture %u, but screen %zu only has %zu textures\n",
              (unsigned)command->event_id, (unsigned)command->zone_index, texture_index, screen_index,
              set->screens[screen_index].texture_count);
      ok = false;
    }
  }

  return ok;
}

static void free_flat_texture(FlatTexture *texture) {
  if (texture == NULL) {
    return;
  }

  free(texture->texels);
  memset(texture, 0, sizeof(*texture));
}

static void free_flat_texture_set(FlatTextureSet *set) {
  if (set == NULL) {
    return;
  }

  free_flat_texture(&set->floor);
  free_flat_texture(&set->roof);
  memset(set, 0, sizeof(*set));
}

static bool map_leaf_name(const char *path, char *out_name, size_t out_name_size) {
  const char *leaf = path;
  const char *p = path;
  const char *dot = NULL;
  size_t len = 0u;

  if (path == NULL || out_name == NULL || out_name_size == 0u) {
    return false;
  }

  while (*p != '\0') {
    if (*p == '/' || *p == '\\') {
      leaf = p + 1;
    }
    ++p;
  }

  dot = strrchr(leaf, '.');
  len = dot != NULL ? (size_t)(dot - leaf) : strlen(leaf);
  if (len == 0u || len >= out_name_size) {
    return false;
  }

  memcpy(out_name, leaf, len);
  out_name[len] = '\0';
  return true;
}

static bool copy_script_token(char *out, size_t out_size, const char *line, size_t line_len) {
  if (out == NULL || out_size == 0u || line == NULL || line_len == 0u) {
    return false;
  }

  while (line_len > 0u && (line[line_len - 1u] == '\r' || line[line_len - 1u] == ' ' || line[line_len - 1u] == '\t')) {
    line_len -= 1u;
  }

  if (line_len == 0u || line_len >= out_size) {
    return false;
  }

  memcpy(out, line, line_len);
  out[line_len] = '\0';
  return true;
}

static bool resolve_script_tile_tag_for_map(const char *map_path, char *out_tag, size_t out_tag_size) {
  const char *script_candidates[2] = {"amiga/misc/script", "amiga/data/misc/script"};
  char map_name[64] = {0};
  char current_tile[32] = {0};
  uint8_t *script = NULL;
  size_t script_size = 0u;
  size_t candidate_index = 0u;
  char error[256] = {0};

  if (!map_leaf_name(map_path, map_name, sizeof(map_name)) || out_tag == NULL || out_tag_size == 0u) {
    return false;
  }

  for (candidate_index = 0u; candidate_index < sizeof(script_candidates) / sizeof(script_candidates[0]); ++candidate_index) {
    char resolved_path[1024] = {0};

    if (!resolve_runtime_file_path(script_candidates[candidate_index], resolved_path, sizeof(resolved_path))) {
      continue;
    }

    if (!read_binary_blob(resolved_path, &script, &script_size)) {
      continue;
    }

    if (!gloom_decrunch_crm_buffer(&script, &script_size, error, sizeof(error))) {
      fprintf(stderr, "Failed to decrunch script %s: %s\n", resolved_path, error[0] ? error : "unknown error");
      free(script);
      return false;
    }

    break;
  }

  if (script == NULL || script_size == 0u) {
    fprintf(stderr, "Unable to load game script for tile selection\n");
    free(script);
    return false;
  }

  {
    size_t pos = 0u;

    while (pos < script_size) {
      size_t line_start = pos;
      size_t line_len = 0u;
      const char *line = NULL;

      while (pos < script_size && script[pos] != '\n') {
        pos += 1u;
      }

      line = (const char *)script + line_start;
      line_len = pos - line_start;
      if (pos < script_size && script[pos] == '\n') {
        pos += 1u;
      }

      while (line_len > 0u && (*line == ' ' || *line == '\t')) {
        line += 1;
        line_len -= 1u;
      }
      while (line_len > 0u && (line[line_len - 1u] == '\r' || line[line_len - 1u] == ' ' || line[line_len - 1u] == '\t')) {
        line_len -= 1u;
      }

      if (line_len > 5u && memcmp(line, "tile_", 5u) == 0) {
        if (!copy_script_token(current_tile, sizeof(current_tile), line + 5u, line_len - 5u)) {
          free(script);
          return false;
        }
      } else if (line_len > 5u && memcmp(line, "play_", 5u) == 0) {
        size_t play_len = line_len - 5u;

        if (play_len == strlen(map_name) && memcmp(line + 5u, map_name, play_len) == 0) {
          if (current_tile[0] == '\0') {
            fprintf(stderr, "Script play_%s has no preceding tile_ command\n", map_name);
            free(script);
            return false;
          }

          if (strlen(current_tile) >= out_tag_size) {
            free(script);
            return false;
          }

          (void)snprintf(out_tag, out_tag_size, "%s", current_tile);
          free(script);
          return true;
        }
      }
    }
  }

  fprintf(stderr, "No script tile tag found for map %s\n", map_name);
  free(script);
  return false;
}

static bool resolve_flat_texture_path(const char *kind, const char *tile_tag, char *out_path, size_t out_path_size) {
  const char *candidates[4] = {NULL, NULL, NULL, NULL};
  char path0[256] = {0};
  char path1[256] = {0};
  char path2[256] = {0};
  char path3[256] = {0};
  size_t i = 0u;

  if (kind == NULL || tile_tag == NULL || tile_tag[0] == '\0' || out_path == NULL || out_path_size == 0u) {
    return false;
  }

  (void)snprintf(path0, sizeof(path0), "txts/%s%s", kind, tile_tag);
  (void)snprintf(path1, sizeof(path1), "amiga/txts/%s%s", kind, tile_tag);
  (void)snprintf(path2, sizeof(path2), "amiga/data/txts/%s%s", kind, tile_tag);
  (void)snprintf(path3, sizeof(path3), "data/txts/%s%s", kind, tile_tag);

  candidates[0] = path0;
  candidates[1] = path1;
  candidates[2] = path2;
  candidates[3] = path3;

  for (i = 0u; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    if (resolve_runtime_file_path(candidates[i], out_path, out_path_size)) {
      return true;
    }
  }

  return false;
}

static bool load_flat_texture(const char *kind, const char *tile_tag, FlatTexture *out_texture) {
  enum { FLAT_TEXEL_BYTES = GLOOM_FLAT_TEXTURE_SIZE * GLOOM_FLAT_TEXTURE_SIZE };
  char resolved_path[1024] = {0};
  uint8_t *data = NULL;
  size_t data_size = 0u;
  char error[256] = {0};
  uint8_t *texels = NULL;

  if (kind == NULL || tile_tag == NULL || out_texture == NULL) {
    return false;
  }

  memset(out_texture, 0, sizeof(*out_texture));

  if (!resolve_flat_texture_path(kind, tile_tag, resolved_path, sizeof(resolved_path))) {
    fprintf(stderr, "Missing %s tile for script tile_%s\n", kind, tile_tag);
    return false;
  }

  if (!read_binary_blob(resolved_path, &data, &data_size)) {
    fprintf(stderr, "Failed to read %s tile %s\n", kind, resolved_path);
    return false;
  }

  if (!gloom_decrunch_crm_buffer(&data, &data_size, error, sizeof(error))) {
    fprintf(stderr, "Failed to decrunch %s tile %s: %s\n", kind, resolved_path, error[0] ? error : "unknown error");
    free(data);
    return false;
  }

  if (data_size < (size_t)FLAT_TEXEL_BYTES + 2u) {
    fprintf(stderr, "%s tile %s is too small after decrunch (%zu bytes)\n", kind, resolved_path, data_size);
    free(data);
    return false;
  }

  texels = (uint8_t *)malloc((size_t)FLAT_TEXEL_BYTES);
  if (texels == NULL) {
    free(data);
    return false;
  }

  memcpy(texels, data, (size_t)FLAT_TEXEL_BYTES);
  set_default_texture_palette(out_texture->palette);
  load_packed_palette(out_texture->palette, data, data_size, (size_t)FLAT_TEXEL_BYTES);

  out_texture->texels = texels;
  out_texture->loaded = true;
  (void)snprintf(out_texture->source_name, sizeof(out_texture->source_name), "%s", resolved_path);

  free(data);
  return true;
}

static bool load_flat_texture_set_for_map(const char *map_path, FlatTextureSet *set) {
  char tile_tag[32] = {0};

  if (map_path == NULL || set == NULL) {
    return false;
  }

  free_flat_texture_set(set);

  if (!resolve_script_tile_tag_for_map(map_path, tile_tag, sizeof(tile_tag))) {
    return false;
  }

  if (!load_flat_texture("floor", tile_tag, &set->floor)) {
    free_flat_texture_set(set);
    return false;
  }

  if (!load_flat_texture("roof", tile_tag, &set->roof)) {
    free_flat_texture_set(set);
    return false;
  }

  (void)snprintf(set->tile_tag, sizeof(set->tile_tag), "%s", tile_tag);
  printf("Loaded script tile_%s floor/roof textures\n", set->tile_tag);
  return true;
}

static uint8_t choose_zone_texture_index(const GloomZone *zone, uint64_t tick_count) {
  uint8_t sequence[8] = {0};
  size_t count = 0;
  size_t i = 0;

  if (zone == NULL) {
    return 0u;
  }

  for (i = 0; i < sizeof(zone->textures); ++i) {
    if (zone->textures[i] != 255u) {
      sequence[count++] = zone->textures[i];
    }
  }

  if (count == 0u) {
    return 0u;
  }
  if (count == 1u) {
    return sequence[0];
  }

  return sequence[(tick_count / 10u) % count];
}

static uint32_t sample_wall_texture_argb_ex(const WallTextureSet *set, uint8_t texture_index, float u, float v,
                                            bool *out_transparent_column) {
  size_t screen_index = texture_index / (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
  size_t local_index = texture_index % (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
  size_t tx = 0;
  size_t ty = 0;
  size_t offset = 0;
  size_t flag_offset = 0;
  uint8_t palette_index = 0;
  bool transparent_column = false;

  if (out_transparent_column != NULL) {
    *out_transparent_column = false;
  }

  if (set == NULL || screen_index >= GLOOM_TEXTURE_SCREENS) {
    return 0xFF9A9A9Au;
  }

  if (!set->screens[screen_index].loaded || set->screens[screen_index].texels == NULL ||
      local_index >= set->screens[screen_index].texture_count) {
    return 0xFF9A9A9Au;
  }

  u = clampf(u, 0.0f, 1.0f);
  v = clampf(v, 0.0f, 1.0f);
  tx = (size_t)(u * (float)(GLOOM_TEXTURE_WIDTH - 1));
  ty = (size_t)(v * (float)(GLOOM_TEXTURE_HEIGHT - 1));

  offset = local_index * ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT) +
           ty * (size_t)GLOOM_TEXTURE_WIDTH + tx;
  flag_offset = local_index * (size_t)GLOOM_TEXTURE_WIDTH + tx;
  palette_index = set->screens[screen_index].texels[offset];

  transparent_column =
      set->screens[screen_index].column_flags != NULL && set->screens[screen_index].column_flags[flag_offset] != 0u;
  if (out_transparent_column != NULL) {
    *out_transparent_column = transparent_column;
  }

  if (transparent_column && palette_index == 0u) {
    return 0x00000000u;
  }

  return set->screens[screen_index].palette[palette_index];
}

static uint32_t sample_wall_texture_argb(const WallTextureSet *set, uint8_t texture_index, float u, float v) {
  return sample_wall_texture_argb_ex(set, texture_index, u, v, NULL);
}

static uint32_t sample_zone_wall_texture_argb(const WallTextureSet *set, const GloomZone *zone, float wall_u,
                                              float wall_v) {
  float scaled_u = wall_u;
  float local_u = 0.0f;
  int tile_index = 0;
  int tile_base = 0;
  uint8_t texture_index = 0;

  if (zone == NULL) {
    return 0xFF9A9A9Au;
  }

  wall_u = clampf(wall_u, 0.0f, 1.0f);
  wall_v = clampf(wall_v, 0.0f, 1.0f);

  if (zone->scale > 0) {
    scaled_u = wall_u * (float)zone->scale;
  } else if (zone->scale < 0) {
    int shift = -zone->scale;
    float divisor = 1.0f;

    if (shift > 1) {
      divisor = (float)(1u << (uint32_t)(shift - 1));
    }
    scaled_u = wall_u / divisor;
  }

  tile_base = (int)scaled_u;
  local_u = scaled_u - (float)tile_base;
  if (local_u < 0.0f) {
    local_u += 1.0f;
    tile_base -= 1;
  }

  tile_index = tile_base & 7;
  texture_index = zone->textures[tile_index];
  return sample_wall_texture_argb(set, texture_index, local_u, wall_v);
}

static const ObjectVisualDefinition *object_visual_definitions(void) {
  static const ObjectVisualDefinition definitions[GLOOM_OBJECT_TYPE_COUNT] = {
      {"player", true, 0x0200u, 0u, false},    {"player", true, 0x0200u, 0u, false},
      {"tokens", false, 0x0200u, 2u, true},    {"bullet2.bin", false, 0x0200u, 0u, false},
      {"tokens", false, 0x0200u, 0u, true},    {"tokens", false, 0x0200u, 0u, true},
      {"tokens", false, 0x0200u, 1u, true},    {"tokens", false, 0x0200u, 2u, true},
      {"dragon", true, 0x0300u, 0u, false},    {"tokens", false, 0x0200u, 3u, true},
      {"marine", true, 0x0200u, 0u, false},    {"baldy", true, 0x0220u, 0u, false},
      {"terra", true, 0x0280u, 0u, false},     {"ghoul", true, 0x0200u, 0u, false},
      {"phantom", true, 0x0280u, 0u, false},   {"demon", true, 0x0380u, 0u, false},
      {"bullet1.bin", false, 0x0200u, 0u, false}, {"bullet2.bin", false, 0x0200u, 0u, false},
      {"bullet3.bin", false, 0x0200u, 0u, false}, {"bullet4.bin", false, 0x0200u, 0u, false},
      {"bullet5.bin", false, 0x0200u, 0u, false}, {"lizard", true, 0x0240u, 0u, false},
      {"deathhead", true, 0x0200u, 0u, false}, {"troll", true, 0x0240u, 0u, false},
  };

  return definitions;
}

static void free_object_visual(ObjectVisual *visual) {
  size_t i = 0u;

  if (visual == NULL) {
    return;
  }

  for (i = 0u; i < visual->frame_count; ++i) {
    free(visual->frames[i].argb_pixels);
  }

  free(visual->frames);
  memset(visual, 0, sizeof(*visual));
}

static void free_object_visual_set(ObjectVisualSet *set) {
  size_t i = 0u;

  if (set == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_OBJECT_TYPE_COUNT; ++i) {
    free_object_visual(&set->visuals[i]);
  }

  memset(set, 0, sizeof(*set));
}

static bool load_object_visual_from_asset(const char *asset_name, const ObjectVisualDefinition *definition,
                                          ObjectVisual *out_visual) {
  char resolved_path[1024] = {0};
  uint8_t *data = NULL;
  size_t data_size = 0u;
  char error[256] = {0};
  uint32_t palette_offset = 0u;
  uint32_t palette[256];
  uint16_t rotation_shift = 0u;
  uint16_t frames_per_rotation = 0u;
  uint16_t max_w = 0u;
  uint16_t max_h = 0u;
  size_t frame_count = 0u;
  ObjectFrame *frames = NULL;
  size_t frame_index = 0u;

  if (asset_name == NULL || definition == NULL || out_visual == NULL) {
    return false;
  }

  memset(out_visual, 0, sizeof(*out_visual));

  if (!resolve_object_asset_path(asset_name, resolved_path, sizeof(resolved_path))) {
    fprintf(stderr, "Missing object visual asset '%s'\n", asset_name);
    return false;
  }

  if (!read_binary_blob(resolved_path, &data, &data_size)) {
    fprintf(stderr, "Failed to read object visual asset %s\n", resolved_path);
    return false;
  }

  if (!gloom_decrunch_crm_buffer(&data, &data_size, error, sizeof(error))) {
    fprintf(stderr, "Failed to decrunch object visual asset %s: %s\n", resolved_path,
            error[0] ? error : "unknown error");
    free(data);
    return false;
  }

  if (data_size < 16u) {
    fprintf(stderr, "Object visual asset %s is too small after decrunch (%zu bytes)\n", resolved_path, data_size);
    free(data);
    return false;
  }

  rotation_shift = main_read_be16(data + 0);
  frames_per_rotation = main_read_be16(data + 2);
  max_w = main_read_be16(data + 4);
  max_h = main_read_be16(data + 6);
  palette_offset = main_read_be32(data + 8);

  if (rotation_shift > 4u || frames_per_rotation == 0u || max_w == 0u || max_h == 0u ||
      palette_offset >= data_size) {
    fprintf(stderr, "Object visual asset %s has invalid animation header\n", resolved_path);
    free(data);
    return false;
  }

  frame_count = (size_t)frames_per_rotation << rotation_shift;
  if (frame_count == 0u || 12u + frame_count * 4u > data_size) {
    fprintf(stderr, "Object visual asset %s has invalid frame table\n", resolved_path);
    free(data);
    return false;
  }

  frames = (ObjectFrame *)calloc(frame_count, sizeof(*frames));
  if (frames == NULL) {
    free(data);
    return false;
  }

  set_default_texture_palette(palette);
  load_packed_palette(palette, data, data_size, (size_t)palette_offset);

  for (frame_index = 0u; frame_index < frame_count; ++frame_index) {
    uint32_t frame_offset = main_read_be32(data + 12u + frame_index * 4u);
    int16_t handle_x = 0;
    int16_t handle_y = 0;
    int16_t width = 0;
    int16_t height = 0;
    uint32_t *pixels = NULL;
    size_t sx = 0u;

    if ((size_t)frame_offset + 8u > data_size) {
      fprintf(stderr, "Object visual asset %s frame %zu is out of bounds\n", resolved_path, frame_index);
      goto fail;
    }

    handle_x = (int16_t)main_read_be16(data + frame_offset + 0u);
    handle_y = (int16_t)main_read_be16(data + frame_offset + 2u);
    width = (int16_t)main_read_be16(data + frame_offset + 4u);
    height = (int16_t)main_read_be16(data + frame_offset + 6u);

    if (width <= 0 || height <= 0 || width > 512 || height > 512 ||
        (size_t)frame_offset + 8u + (size_t)width * (size_t)height > data_size) {
      fprintf(stderr, "Object visual asset %s frame %zu has invalid dimensions\n", resolved_path, frame_index);
      goto fail;
    }

    pixels = (uint32_t *)malloc((size_t)width * (size_t)height * sizeof(*pixels));
    if (pixels == NULL) {
      goto fail;
    }

    for (sx = 0u; sx < (size_t)width; ++sx) {
      size_t sy = 0u;

      for (sy = 0u; sy < (size_t)height; ++sy) {
        uint8_t palette_index = data[(size_t)frame_offset + 8u + sx * (size_t)height + sy];
        uint32_t argb = palette[palette_index];

        if (palette_index == 0u) {
          argb &= 0x00FFFFFFu;
        }

        pixels[sy * (size_t)width + sx] = argb;
      }
    }

    frames[frame_index].handle_x = handle_x;
    frames[frame_index].handle_y = handle_y;
    frames[frame_index].width = width;
    frames[frame_index].height = height;
    frames[frame_index].argb_pixels = pixels;
  }

  out_visual->loaded = true;
  out_visual->eight_rotations = definition->eight_rotations;
  out_visual->scale = definition->scale;
  out_visual->fixed_frame = definition->fixed_frame;
  out_visual->use_fixed_frame = definition->use_fixed_frame;
  out_visual->rotation_shift = rotation_shift;
  out_visual->frame_count_per_rotation = frames_per_rotation;
  out_visual->frame_count = frame_count;
  out_visual->frames = frames;
  (void)snprintf(out_visual->source_name, sizeof(out_visual->source_name), "%s", resolved_path);

  free(data);
  return true;

fail:
  for (frame_index = 0u; frame_index < frame_count; ++frame_index) {
    free(frames[frame_index].argb_pixels);
  }
  free(frames);
  free(data);
  return false;
}

static bool load_object_visual_set_for_map(const GloomMap *map, ObjectVisualSet *set) {
  const ObjectVisualDefinition *definitions = object_visual_definitions();
  uint8_t needed[GLOOM_OBJECT_TYPE_COUNT] = {0};
  size_t i = 0u;
  size_t loaded_count = 0u;

  if (map == NULL || set == NULL) {
    return false;
  }

  free_object_visual_set(set);

  for (i = 0u; i < map->object_spawn_count; ++i) {
    int16_t object_type = map->object_spawns[i].object_type;

    if (object_type >= 0 && object_type < (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
      needed[object_type] = 1u;
    }
  }

  for (i = 0u; i < GLOOM_OBJECT_TYPE_COUNT; ++i) {
    const char *asset_name = definitions[i].asset_name;

    if (needed[i] == 0u || asset_name == NULL) {
      continue;
    }

    if (!load_object_visual_from_asset(asset_name, &definitions[i], &set->visuals[i])) {
      free_object_visual_set(set);
      return false;
    }
    loaded_count += 1u;
  }

  printf("Loaded %zu object visual bindings from real object assets\n", loaded_count);
  return true;
}

static uint32_t sample_object_frame_argb(const ObjectFrame *frame, float u, float v) {
  int sx = 0;
  int sy = 0;

  if (frame == NULL || frame->argb_pixels == NULL || frame->width <= 0 || frame->height <= 0) {
    return 0u;
  }

  u = clampf(u, 0.0f, 1.0f);
  v = clampf(v, 0.0f, 1.0f);
  sx = (int)(u * (float)(frame->width - 1));
  sy = (int)(v * (float)(frame->height - 1));
  return frame->argb_pixels[(sy * frame->width) + sx];
}

static const ObjectFrame *select_object_visual_frame(const ObjectVisual *visual, const GloomObjectSpawn *spawn,
                                                     const AppState *state) {
  size_t frame_number = 0u;
  size_t rotation_count = 1u;
  size_t rotation_index = 0u;
  size_t frame_index = 0u;

  if (visual == NULL || spawn == NULL || state == NULL || !visual->loaded || visual->frames == NULL ||
      visual->frame_count == 0u) {
    return NULL;
  }

  if (visual->use_fixed_frame) {
    frame_number = visual->fixed_frame;
  } else if (visual->frame_count_per_rotation > 0u) {
    frame_number = (size_t)(state->tick_count / 8u) % (size_t)visual->frame_count_per_rotation;
  }

  if (visual->eight_rotations && visual->rotation_shift < 8u) {
    float dx = state->camera_x - (float)spawn->x;
    float dz = state->camera_z - (float)spawn->z;
    float angle = SDL_atan2f(dx, dz);
    int angle_unit = 0;
    int rotation_unit = 0;

    if (angle < 0.0f) {
      angle += 6.28318530718f;
    }

    rotation_count = (size_t)1u << visual->rotation_shift;
    angle_unit = ((int)((angle * 256.0f / 6.28318530718f) + 0.5f)) & 255;
    rotation_unit = (angle_unit + (int)(128u / rotation_count) - (int)spawn->rotation) & 255;
    rotation_index = (size_t)((rotation_unit * (int)rotation_count) >> 8);
    if (rotation_index >= rotation_count) {
      rotation_index = rotation_count - 1u;
    }
  }

  frame_index = frame_number * rotation_count + rotation_index;
  if (frame_index >= visual->frame_count) {
    frame_index %= visual->frame_count;
  }

  return &visual->frames[frame_index];
}

static void free_preview_asset(PreviewAsset *asset) {
  if (asset == NULL) {
    return;
  }

  if (asset->texture != NULL) {
    SDL_DestroyTexture(asset->texture);
  }
  free(asset->argb_pixels);
  memset(asset, 0, sizeof(*asset));
}

static bool load_iff_preview_asset(SDL_Renderer *renderer, const char *path, PreviewAsset *out_asset) {
  GloomIffImage image;
  char error[256] = {0};
  uint32_t *argb_pixels = NULL;
  SDL_Texture *texture = NULL;
  size_t i = 0;

  if (out_asset == NULL) {
    return false;
  }

  memset(out_asset, 0, sizeof(*out_asset));
  memset(&image, 0, sizeof(image));

  if (!gloom_iff_load(path, &image, error, sizeof(error))) {
    fprintf(stderr, "Texture preview load failed for %s: %s\n", path, error[0] ? error : "unknown error");
    return false;
  }

  if (!image.has_decoded_pixels || image.pixel_count == 0u) {
    fprintf(stderr, "Texture preview unavailable for %s: no decoded pixels\n", path);
    gloom_iff_free(&image);
    return false;
  }

  argb_pixels = (uint32_t *)malloc(image.pixel_count * sizeof(*argb_pixels));
  if (argb_pixels == NULL) {
    fprintf(stderr, "Out of memory while preparing texture preview\n");
    gloom_iff_free(&image);
    return false;
  }

  for (i = 0; i < image.pixel_count; ++i) {
    argb_pixels[i] = palette_to_argb(&image, image.pixels[i]);
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STATIC, (int)image.width,
                              (int)image.height);
  if (texture == NULL) {
    fprintf(stderr, "SDL_CreateTexture failed for preview: %s\n", SDL_GetError());
    free(argb_pixels);
    gloom_iff_free(&image);
    return false;
  }

  if (SDL_UpdateTexture(texture, NULL, argb_pixels, (int)image.width * (int)sizeof(uint32_t)) != 0) {
    fprintf(stderr, "SDL_UpdateTexture failed for preview: %s\n", SDL_GetError());
    SDL_DestroyTexture(texture);
    free(argb_pixels);
    gloom_iff_free(&image);
    return false;
  }

  (void)SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);
  out_asset->texture = texture;
  out_asset->argb_pixels = argb_pixels;
  out_asset->width = (int)image.width;
  out_asset->height = (int)image.height;

  gloom_iff_free(&image);
  return true;
}

static int run_iff_preview(const char *path) {
  GloomIffImage image;
  char resolved_path[1024] = {0};
  const char *load_path = path;
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  SDL_Texture *texture = NULL;
  uint32_t *argb_pixels = NULL;
  char error[256] = {0};
  int exit_code = 1;
  bool running = true;
  int window_w = 0;
  int window_h = 0;
  size_t i = 0;

  memset(&image, 0, sizeof(image));

  if (resolve_runtime_file_path(path, resolved_path, sizeof(resolved_path))) {
    load_path = resolved_path;
  }

  if (!gloom_iff_load(load_path, &image, error, sizeof(error))) {
    fprintf(stderr, "IFF parse failed for %s: %s\n", load_path, error[0] ? error : "unknown error");
    return 1;
  }

  if (!image.has_decoded_pixels || image.pixel_count == 0u) {
    fprintf(stderr, "IFF preview failed for %s: no decoded pixels available\n", path);
    goto cleanup;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    goto cleanup;
  }

  window_w = (int)image.width * 2;
  window_h = (int)image.height * 2;
  if (window_w < 320) window_w = 320;
  if (window_h < 240) window_h = 240;

  window = SDL_CreateWindow("Gloom IFF Preview", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_w, window_h,
                            SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    goto cleanup;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (renderer == NULL) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    goto cleanup;
  }

  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, (int)image.width,
                              (int)image.height);
  if (texture == NULL) {
    fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
    goto cleanup;
  }

  argb_pixels = (uint32_t *)malloc(image.pixel_count * sizeof(*argb_pixels));
  if (argb_pixels == NULL) {
    fprintf(stderr, "Out of memory while preparing preview image\n");
    goto cleanup;
  }

  for (i = 0; i < image.pixel_count; ++i) {
    argb_pixels[i] = palette_to_argb(&image, image.pixels[i]);
  }

  if (SDL_UpdateTexture(texture, NULL, argb_pixels, (int)image.width * (int)sizeof(uint32_t)) != 0) {
    fprintf(stderr, "SDL_UpdateTexture failed: %s\n", SDL_GetError());
    goto cleanup;
  }

  (void)SDL_RenderSetLogicalSize(renderer, (int)image.width, (int)image.height);

  while (running) {
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        running = false;
      }
    }

    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, texture, NULL, NULL);
    SDL_RenderPresent(renderer);
  }

  exit_code = 0;

cleanup:
  free(argb_pixels);
  if (texture != NULL) SDL_DestroyTexture(texture);
  if (renderer != NULL) SDL_DestroyRenderer(renderer);
  if (window != NULL) SDL_DestroyWindow(window);
  SDL_Quit();
  gloom_iff_free(&image);
  return exit_code;
}

static int16_t player_control_axis(bool negative_pressed, bool positive_pressed) {
  if (negative_pressed && !positive_pressed) {
    return -1;
  }
  if (positive_pressed && !negative_pressed) {
    return 1;
  }
  return 0;
}

static PlayerControls read_modern_player_controls(const uint8_t *keyboard) {
  PlayerControls controls;
  int16_t strafe = 0;
  int16_t turn = 0;

  memset(&controls, 0, sizeof(controls));
  if (keyboard == NULL) {
    return controls;
  }

  controls.joyy = player_control_axis(keyboard[SDL_SCANCODE_W] || keyboard[SDL_SCANCODE_UP],
                                      keyboard[SDL_SCANCODE_S] || keyboard[SDL_SCANCODE_DOWN]);
  strafe = player_control_axis(keyboard[SDL_SCANCODE_A], keyboard[SDL_SCANCODE_D]);
  turn = player_control_axis(keyboard[SDL_SCANCODE_LEFT] || keyboard[SDL_SCANCODE_Q],
                             keyboard[SDL_SCANCODE_RIGHT] || keyboard[SDL_SCANCODE_E]);

  if (strafe != 0) {
    controls.joyx = strafe;
    controls.joys = -1;
  } else {
    controls.joyx = turn;
  }

  return controls;
}

static int32_t wrap_player_rotation_fixed(int64_t rotation_fixed) {
  const int32_t period = 256 << 16;
  int64_t wrapped = rotation_fixed % (int64_t)period;

  if (wrapped < 0) {
    wrapped += period;
  }

  return (int32_t)wrapped;
}

static void update_player_rotation(AppState *state, const PlayerControls *controls, int mouse_dx) {
  int16_t joyx = 0;
  int16_t joys = 0;

  if (state == NULL) {
    return;
  }

  if (controls != NULL) {
    joyx = controls->joyx;
    joys = controls->joys;
  }

  if (joys == 0 && joyx != 0) {
    int32_t accel = GLOOM_PLAYER_ROT_ACC;

    if ((state->player_rotspeed < 0 && joyx > 0) || (state->player_rotspeed > 0 && joyx < 0)) {
      accel = GLOOM_PLAYER_ROT_REV_ACC;
    }
    if (joyx < 0) {
      accel = -accel;
    }

    state->player_rotspeed += accel;
    if (state->player_rotspeed > GLOOM_PLAYER_MAX_ROT_SPEED) {
      state->player_rotspeed = GLOOM_PLAYER_MAX_ROT_SPEED;
    } else if (state->player_rotspeed < -GLOOM_PLAYER_MAX_ROT_SPEED) {
      state->player_rotspeed = -GLOOM_PLAYER_MAX_ROT_SPEED;
    }
  } else if (state->player_rotspeed < 0) {
    state->player_rotspeed += GLOOM_PLAYER_ROT_SET_ACC;
    if (state->player_rotspeed > 0) {
      state->player_rotspeed = 0;
    }
  } else if (state->player_rotspeed > 0) {
    state->player_rotspeed -= GLOOM_PLAYER_ROT_SET_ACC;
    if (state->player_rotspeed < 0) {
      state->player_rotspeed = 0;
    }
  }

  state->player_rot_fixed =
      wrap_player_rotation_fixed((int64_t)state->player_rot_fixed + (int64_t)state->player_rotspeed +
                                 ((int64_t)mouse_dx * (int64_t)GLOOM_MOUSE_LOOK_FIXED_PER_PIXEL));
  state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
}

static void advance_player_bounce(AppState *state) {
  uint16_t bounce = 0u;

  if (state == NULL) {
    return;
  }

  bounce = (uint16_t)state->player_bounce;
  bounce = (uint16_t)(bounce + (uint16_t)GLOOM_PLAYER_BOUNCE_STEP);
  state->player_bounce = (int16_t)bounce;
}

static void settle_player_bounce(AppState *state) {
  uint16_t bounce = 0u;

  if (state == NULL || state->player_bounce == 0) {
    return;
  }

  bounce = (uint16_t)state->player_bounce;
  bounce = (uint16_t)(bounce + (uint16_t)GLOOM_PLAYER_UNBOUNCE_STEP);
  state->player_bounce = (int16_t)bounce;

  if ((bounce & 127u) < (uint16_t)GLOOM_PLAYER_UNBOUNCE_STEP) {
    state->player_bounce = 0;
  }
}

static void update_player_camera_y(AppState *state) {
  uint16_t phase = 0u;
  float bob = 0.0f;

  if (state == NULL) {
    return;
  }

  phase = (uint16_t)state->player_bounce & 255u;
  bob = SDL_sinf(((float)phase * 6.28318530718f) / 256.0f) * (float)GLOOM_PLAYER_BOB_HEIGHT;
  state->camera_y = (float)GLOOM_PLAYER_EYE_Y + bob;
}

static void update_player_movement(AppState *state, const PlayerControls *controls) {
  float forward = 0.0f;
  float strafe = 0.0f;
  float sin_a = 0.0f;
  float cos_a = 0.0f;

  if (state == NULL || controls == NULL) {
    return;
  }

  if (controls->joyy == 0 && (controls->joys == 0 || controls->joyx == 0)) {
    settle_player_bounce(state);
    update_player_camera_y(state);
    return;
  }

  if (controls->joyy != 0) {
    forward = -(float)controls->joyy * (float)GLOOM_PLAYER_MOVESPEED;
  }
  if (controls->joys != 0 && controls->joyx != 0) {
    strafe = (float)controls->joyx * (float)GLOOM_PLAYER_MOVESPEED;
  }

  sin_a = SDL_sinf(state->camera_angle);
  cos_a = SDL_cosf(state->camera_angle);

  {
    float old_x = state->camera_x;
    float old_z = state->camera_z;
    float new_x = old_x + (sin_a * forward) + (cos_a * strafe);
    float new_z = old_z + (cos_a * forward) - (sin_a * strafe);

    if (resolve_player_wall_collision(state, &new_x, &new_z)) {
      state->camera_x = new_x;
      state->camera_z = new_z;
    } else {
      state->camera_x = old_x;
      state->camera_z = old_z;
    }
  }

  advance_player_bounce(state);
  update_player_camera_y(state);
}

static void update(AppState *state, const uint8_t *keyboard, int mouse_dx) {
  PlayerControls controls;

  state->tick_count += 1;
  update_rotpolys(state);
  update_doors(state);

  if (state->teleport_active) {
    if (state->teleport_ticks > 0u) {
      state->teleport_ticks -= 1u;
    }
    if (state->teleport_ticks == 0u) {
      state->camera_x = (float)state->teleport_x;
      state->camera_z = (float)state->teleport_z;
      state->player_rot_fixed = amiga_rotation_to_fixed(state->teleport_rotation);
      state->player_rotspeed = 0;
      state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
      state->teleport_active = false;
    } else {
      update_player_camera_y(state);
      return;
    }
  }

  {
    float corrected_x = state->camera_x;
    float corrected_z = state->camera_z;

    if (!resolve_player_wall_collision(state, &corrected_x, &corrected_z)) {
      return;
    }
    state->camera_x = corrected_x;
    state->camera_z = corrected_z;
  }

  check_event_triggers(state);

  controls = read_modern_player_controls(keyboard);
  update_player_rotation(state, &controls, mouse_dx);
  update_player_movement(state, &controls);
}

static void world_to_screen(const AppState *state, int x, int z, int rx, int ry, int rw, int rh, int *sx, int *sy) {
  float span_x = (float)(state->max_x - state->min_x);
  float span_z = (float)(state->max_z - state->min_z);
  float scale_x = 1.0f;
  float scale_z = 1.0f;
  float scale = 1.0f;
  float cx = 0.5f * ((float)state->min_x + (float)state->max_x);
  float cz = 0.5f * ((float)state->min_z + (float)state->max_z);
  float dx = (float)x - cx;
  float dz = (float)z - cz;
  float px = 0.0f;
  float py = 0.0f;

  if (span_x < 1.0f) span_x = 1.0f;
  if (span_z < 1.0f) span_z = 1.0f;

  scale_x = (float)(rw - 10) / span_x;
  scale_z = (float)(rh - 10) / span_z;
  scale = scale_x < scale_z ? scale_x : scale_z;
  if (scale < 0.01f) scale = 0.01f;

  px = (float)rx + 0.5f * (float)rw + dx * scale;
  py = (float)ry + 0.5f * (float)rh - dz * scale;

  *sx = (int)px;
  *sy = (int)py;
}

static void render_map_debug(SDL_Renderer *renderer, const AppState *state, int x, int y, int w, int h) {
  size_t i = 0;
  SDL_Rect panel = {x, y, w, h};

  SDL_SetRenderDrawColor(renderer, 14, 18, 24, 255);
  SDL_RenderFillRect(renderer, &panel);

  for (i = 0; i < state->map.zone_count; ++i) {
    const GloomZone *z = &state->map.zones[i];
    int sx1 = 0;
    int sy1 = 0;
    int sx2 = 0;
    int sy2 = 0;

    world_to_screen(state, z->x1, z->z1, x, y, w, h, &sx1, &sy1);
    world_to_screen(state, z->x2, z->z2, x, y, w, h, &sx2, &sy2);

    if (z->ztype == 1) {
      SDL_SetRenderDrawColor(renderer, 232, 157, 92, 255);
    } else if (z->ztype == 2) {
      SDL_SetRenderDrawColor(renderer, 120, 198, 137, 255);
    } else if (z->ztype == 3) {
      SDL_SetRenderDrawColor(renderer, 105, 165, 255, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 150, 150, 150, 255);
    }

    SDL_RenderDrawLine(renderer, sx1, sy1, sx2, sy2);
  }

  SDL_SetRenderDrawColor(renderer, 75, 90, 112, 255);
  SDL_RenderDrawRect(renderer, &panel);
}

static uint32_t preview_sample_argb(const PreviewAsset *preview, float u, float v) {
  int sx = 0;
  int sy = 0;

  if (preview == NULL || preview->argb_pixels == NULL || preview->width <= 0 || preview->height <= 0) {
    return 0xFFFFFFFFu;
  }

  if (u < 0.0f) u = 0.0f;
  if (u > 1.0f) u = 1.0f;
  if (v < 0.0f) v = 0.0f;
  if (v > 1.0f) v = 1.0f;

  sx = (int)(u * (float)(preview->width - 1));
  sy = (int)(v * (float)(preview->height - 1));
  return preview->argb_pixels[(sy * preview->width) + sx];
}

static float clampf(float value, float lo, float hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return value;
}

static int floor_to_int(float value) {
  int as_int = (int)value;

  if ((float)as_int > value) {
    as_int -= 1;
  }

  return as_int;
}

static int32_t amiga_arithmetic_shift_right_16(int64_t value) {
  if (value >= 0) {
    return (int32_t)(value >> 16);
  }

  return -(int32_t)(((-value) + 65535) >> 16);
}

static int16_t clamp_int16(int32_t value) {
  if (value < -32768) return -32768;
  if (value > 32767) return 32767;
  return (int16_t)value;
}

static int32_t amiga_rotation_to_fixed(int16_t rotation) {
  return (int32_t)((uint32_t)(rotation & 255) << 16u);
}

static float player_rotation_fixed_to_radians(int32_t rotation_fixed) {
  return ((float)rotation_fixed * 6.28318530718f) / (float)(256 << 16);
}

static float amiga_rotation_to_radians(int16_t rotation) {
  return player_rotation_fixed_to_radians(amiga_rotation_to_fixed(rotation));
}

static uint8_t amiga_depth_dark_index(float depth) {
  int z = floor_to_int(depth);
  int source_z = 0;
  int root = 0;
  int palette_index = 0;

  if (z < 0) {
    z = 0;
  }
  if (z >= GLOOM_AMIGA_MAX_Z) {
    z = GLOOM_AMIGA_MAX_Z - 1;
  }

  source_z = (GLOOM_AMIGA_MAX_Z - 1) - z;
  root = (int)SDL_sqrtf((float)(source_z << 3));
  palette_index = (root >> 3) ^ 15;
  if (palette_index < 0) palette_index = 0;
  if (palette_index > 15) palette_index = 15;

  return (uint8_t)palette_index;
}

static uint32_t apply_amiga_depth_argb(uint32_t argb, float depth) {
  uint8_t alpha = (uint8_t)(argb >> 24);
  int subtract = (int)amiga_depth_dark_index(depth) * 17;
  int r = (int)((argb >> 16) & 0xFFu) - subtract;
  int g = (int)((argb >> 8) & 0xFFu) - subtract;
  int b = (int)(argb & 0xFFu) - subtract;

  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  return ((uint32_t)alpha << 24u) | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
}

static bool clip_segment_halfspace(float *x1, float *z1, float *x2, float *z2, float a, float b) {
  float f1 = a * (*x1) + b * (*z1);
  float f2 = a * (*x2) + b * (*z2);

  if (f1 <= 0.0f && f2 <= 0.0f) {
    return true;
  }
  if (f1 > 0.0f && f2 > 0.0f) {
    return false;
  }

  {
    float t = f1 / (f1 - f2);
    float ix = *x1 + ((*x2 - *x1) * t);
    float iz = *z1 + ((*z2 - *z1) * t);

    if (f1 > 0.0f) {
      *x1 = ix;
      *z1 = iz;
    } else {
      *x2 = ix;
      *z2 = iz;
    }
  }

  return true;
}

static int compare_wall_candidates(const void *a, const void *b) {
  const WallCandidate *wa = (const WallCandidate *)a;
  const WallCandidate *wb = (const WallCandidate *)b;

  if (wa->sort_depth < wb->sort_depth) return -1;
  if (wa->sort_depth > wb->sort_depth) return 1;
  return 0;
}

static size_t collect_wall_candidates_from_grid(const GloomMap *map, const GridOffsetSet *grid_offsets, float cam_x,
                                                float cam_z, WallCandidate *out_candidates,
                                                size_t candidate_capacity) {
  uint8_t seen[GLOOM_MAX_RENDER_ZONES] = {0};
  int base_x = 0;
  int base_z = 0;
  size_t candidate_count = 0;
  size_t offset_index = 0;

  if (map == NULL || grid_offsets == NULL || grid_offsets->items == NULL || grid_offsets->count == 0u ||
      out_candidates == NULL || candidate_capacity == 0u || !map->has_grid_cells ||
      map->ppnt_blob == NULL || map->ppnt_blob_size < 2u) {
    return 0u;
  }

  if (map->zone_count > (size_t)GLOOM_MAX_RENDER_ZONES) {
    return 0u;
  }

  if (cam_x < 0.0f || cam_z < 0.0f) {
    return 0u;
  }

  base_x = (int)cam_x >> 8;
  base_z = (int)cam_z >> 8;
  if (base_x < 0 || base_x >= GLOOM_GRID_WIDTH || base_z < 0 || base_z >= GLOOM_GRID_HEIGHT) {
    return 0u;
  }

  for (offset_index = 0; offset_index < grid_offsets->count; ++offset_index) {
    int gx = base_x + grid_offsets->items[offset_index].x;
    int gz = base_z + grid_offsets->items[offset_index].z;
    size_t cell_index = 0;
    const GloomGridCell *cell = NULL;
    size_t poly_count = 0;
    size_t poly_index = 0;

    if (gx < 0 || gx >= GLOOM_GRID_WIDTH || gz < 0 || gz >= GLOOM_GRID_HEIGHT) {
      continue;
    }

    cell_index = (size_t)gz * (size_t)GLOOM_GRID_WIDTH + (size_t)gx;
    cell = &map->wall_grid[cell_index];
    if (cell->count_minus_one < 0) {
      continue;
    }

    poly_count = (size_t)cell->count_minus_one + 1u;
    if (((size_t)cell->ppnt_word_offset + poly_count) * 2u > map->ppnt_blob_size) {
      continue;
    }

    for (poly_index = 0; poly_index < poly_count; ++poly_index) {
      size_t word_offset = (size_t)cell->ppnt_word_offset + poly_index;
      size_t zone_index = (size_t)main_read_be16(map->ppnt_blob + (word_offset * 2u));
      const GloomZone *zone = NULL;
      WallCandidate *candidate = NULL;
      float mid_x = 0.0f;
      float mid_z = 0.0f;
      float dx = 0.0f;
      float dz = 0.0f;

      if (zone_index >= map->zone_count || zone_index >= (size_t)GLOOM_MAX_RENDER_ZONES || seen[zone_index]) {
        continue;
      }

      zone = &map->zones[zone_index];
      if (zone->event_id < 0) {
        continue;
      }

      if (candidate_count >= candidate_capacity) {
        return candidate_count;
      }

      seen[zone_index] = 1u;
      mid_x = 0.5f * ((float)zone->x1 + (float)zone->x2);
      mid_z = 0.5f * ((float)zone->z1 + (float)zone->z2);
      dx = mid_x - cam_x;
      dz = mid_z - cam_z;

      candidate = &out_candidates[candidate_count++];
      candidate->zone_index = zone_index;
      candidate->sort_depth = dx * dx + dz * dz;
    }
  }

  if (candidate_count > 1u) {
    qsort(out_candidates, candidate_count, sizeof(*out_candidates), compare_wall_candidates);
  }

  return candidate_count;
}

static bool append_wall_candidate_unique(const GloomMap *map, WallCandidate *candidates, size_t *candidate_count,
                                         size_t candidate_capacity, size_t zone_index, float cam_x, float cam_z) {
  const GloomZone *zone = NULL;
  float mid_x = 0.0f;
  float mid_z = 0.0f;
  float dx = 0.0f;
  float dz = 0.0f;
  size_t i = 0u;

  if (map == NULL || candidates == NULL || candidate_count == NULL || zone_index >= map->zone_count ||
      zone_index >= (size_t)GLOOM_MAX_RENDER_ZONES) {
    return false;
  }

  for (i = 0u; i < *candidate_count; ++i) {
    if (candidates[i].zone_index == zone_index) {
      return true;
    }
  }

  if (*candidate_count >= candidate_capacity) {
    return false;
  }

  zone = &map->zones[zone_index];
  if (zone->event_id < 0) {
    return true;
  }

  mid_x = 0.5f * ((float)zone->x1 + (float)zone->x2);
  mid_z = 0.5f * ((float)zone->z1 + (float)zone->z2);
  dx = mid_x - cam_x;
  dz = mid_z - cam_z;

  candidates[*candidate_count].zone_index = zone_index;
  candidates[*candidate_count].sort_depth = dx * dx + dz * dz;
  *candidate_count += 1u;
  return true;
}

static void append_runtime_rotpoly_wall_candidates(const AppState *state, WallCandidate *candidates,
                                                   size_t *candidate_count, size_t candidate_capacity, float cam_x,
                                                   float cam_z) {
  size_t i = 0u;

  if (state == NULL || candidates == NULL || candidate_count == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_ACTIVE_ROTPOLYS; ++i) {
    const RuntimeRotPoly *rotpoly = &state->rotpolys[i];
    size_t zone_index = 0u;

    if (!rotpoly->active || rotpoly->zone_count == 0u) {
      continue;
    }

    for (zone_index = 0u; zone_index < (size_t)rotpoly->zone_count; ++zone_index) {
      if (!append_wall_candidate_unique(&state->map, candidates, candidate_count, candidate_capacity,
                                        rotpoly->first_zone_index + zone_index, cam_x, cam_z)) {
        return;
      }
    }
  }
}

static uint16_t zone_fixed_projection_high_word(const GloomZone *zone, int16_t x, int16_t z, bool perpendicular) {
  int32_t dx = (int32_t)zone->x2 - (int32_t)x;
  int32_t dz = (int32_t)zone->z2 - (int32_t)z;
  int32_t ax = perpendicular ? (int32_t)zone->a : (int32_t)zone->na;
  int32_t bz = perpendicular ? (int32_t)zone->b : (int32_t)zone->nb;
  int64_t projected = ((int64_t)dx * ax) + ((int64_t)dz * bz);

  projected *= 2;
  if (perpendicular && projected < 0) {
    projected = -projected;
  }

  return (uint16_t)(((uint64_t)projected >> 16u) & 0xFFFFu);
}

static uint16_t zone_segment_distance(const GloomZone *zone, int16_t x, int16_t z) {
  uint16_t end_distance = 0u;

  if (zone == NULL) {
    return 0x3FFFu;
  }

  end_distance = zone_fixed_projection_high_word(zone, x, z, false);
  if (end_distance >= (uint16_t)zone->ln) {
    return 0x3FFFu;
  }

  return zone_fixed_projection_high_word(zone, x, z, true);
}

static bool consider_collision_zone(const GloomMap *map, size_t zone_index, int16_t x, int16_t z,
                                    uint8_t seen[GLOOM_MAX_RENDER_ZONES], uint16_t *closest_distance,
                                    size_t *closest_zone_index) {
  const GloomZone *zone = NULL;
  uint16_t distance = 0u;

  if (map == NULL || seen == NULL || closest_distance == NULL || closest_zone_index == NULL ||
      zone_index >= map->zone_count || zone_index >= (size_t)GLOOM_MAX_RENDER_ZONES || seen[zone_index]) {
    return true;
  }

  seen[zone_index] = 1u;
  zone = &map->zones[zone_index];
  distance = zone_segment_distance(zone, x, z);
  if (distance < *closest_distance) {
    *closest_distance = distance;
    *closest_zone_index = zone_index;
  }

  return true;
}

static bool find_player_wall_collision(const AppState *state, float x, float z, uint16_t *out_penetration,
                                       size_t *out_zone_index) {
  static const int offsets[9][2] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1},
                                   {-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
  uint8_t seen[GLOOM_MAX_RENDER_ZONES] = {0};
  uint16_t closest_distance = 0x3FFFu;
  size_t closest_zone_index = 0u;
  int world_x = 0;
  int world_z = 0;
  int base_x = 0;
  int base_z = 0;
  size_t offset_index = 0u;
  size_t rotpoly_index = 0u;

  if (state == NULL || out_penetration == NULL || out_zone_index == NULL || state->player_radius <= 0 ||
      !state->map.has_grid_cells || state->map.ppnt_blob == NULL || state->map.ppnt_blob_size < 2u ||
      state->map.zone_count > (size_t)GLOOM_MAX_RENDER_ZONES) {
    return false;
  }

  world_x = floor_to_int(x);
  world_z = floor_to_int(z);
  if (world_x < 0 || world_z < 0 || world_x > 32767 || world_z > 32767) {
    return false;
  }

  base_x = world_x >> 8;
  base_z = world_z >> 8;

  for (offset_index = 0u; offset_index < sizeof(offsets) / sizeof(offsets[0]); ++offset_index) {
    int gx = base_x + offsets[offset_index][0];
    int gz = base_z + offsets[offset_index][1];
    size_t cell_index = 0u;
    const GloomGridCell *cell = NULL;
    size_t poly_count = 0u;
    size_t poly_index = 0u;

    if (gx < 0 || gx >= GLOOM_GRID_WIDTH || gz < 0 || gz >= GLOOM_GRID_HEIGHT) {
      continue;
    }

    cell_index = (size_t)gz * (size_t)GLOOM_GRID_WIDTH + (size_t)gx;
    cell = &state->map.wall_grid[cell_index];
    if (cell->count_minus_one < 0) {
      continue;
    }

    poly_count = (size_t)cell->count_minus_one + 1u;
    if (((size_t)cell->ppnt_word_offset + poly_count) * 2u > state->map.ppnt_blob_size) {
      continue;
    }

    for (poly_index = 0u; poly_index < poly_count; ++poly_index) {
      size_t word_offset = (size_t)cell->ppnt_word_offset + poly_index;
      size_t zone_index = (size_t)main_read_be16(state->map.ppnt_blob + (word_offset * 2u));

      (void)consider_collision_zone(&state->map, zone_index, (int16_t)world_x, (int16_t)world_z, seen,
                                    &closest_distance, &closest_zone_index);
    }
  }

  for (rotpoly_index = 0u; rotpoly_index < GLOOM_MAX_ACTIVE_ROTPOLYS; ++rotpoly_index) {
    const RuntimeRotPoly *rotpoly = &state->rotpolys[rotpoly_index];
    size_t zone_offset = 0u;

    if (!rotpoly->active || rotpoly->zone_count == 0u) {
      continue;
    }

    for (zone_offset = 0u; zone_offset < (size_t)rotpoly->zone_count; ++zone_offset) {
      (void)consider_collision_zone(&state->map, rotpoly->first_zone_index + zone_offset, (int16_t)world_x,
                                    (int16_t)world_z, seen, &closest_distance, &closest_zone_index);
    }
  }

  if ((int32_t)closest_distance - (int32_t)state->player_radius >= 0) {
    return false;
  }

  *out_penetration = (uint16_t)((int32_t)state->player_radius - (int32_t)closest_distance);
  *out_zone_index = closest_zone_index;
  return true;
}

static bool resolve_player_wall_collision(AppState *state, float *x, float *z) {
  int attempt = 0;

  if (state == NULL || x == NULL || z == NULL) {
    return false;
  }

  for (attempt = 0; attempt < 3; ++attempt) {
    uint16_t penetration = 0u;
    size_t zone_index = 0u;
    const GloomZone *zone = NULL;
    float push_x = 0.0f;
    float push_z = 0.0f;

    if (!find_player_wall_collision(state, *x, *z, &penetration, &zone_index)) {
      return true;
    }

    if (attempt == 2 || zone_index >= state->map.zone_count) {
      return false;
    }

    zone = &state->map.zones[zone_index];
    push_x = ((float)penetration * (float)zone->a) / 32768.0f;
    push_z = ((float)penetration * (float)zone->b) / 32768.0f;
    *x -= push_x;
    *z -= push_z;
  }

  return false;
}

static void recalculate_zone_vectors(GloomZone *zone) {
  int32_t dx = 0;
  int32_t dz = 0;
  float length = 0.0f;
  int32_t na = 0;
  int32_t nb = 0;

  if (zone == NULL) {
    return;
  }

  dx = (int32_t)zone->x2 - (int32_t)zone->x1;
  dz = (int32_t)zone->z2 - (int32_t)zone->z1;
  length = SDL_sqrtf((float)((dx * dx) + (dz * dz)));
  if (length < 1.0f) {
    zone->na = 0;
    zone->nb = 0;
    zone->a = 0;
    zone->b = 0;
    return;
  }

  na = (int32_t)((float)dx * 32766.0f / length);
  nb = (int32_t)((float)dz * 32766.0f / length);
  zone->na = clamp_int16(na);
  zone->nb = clamp_int16(nb);
  zone->a = clamp_int16(-nb);
  zone->b = clamp_int16(na);
}

static void rotate_amiga_1024(int16_t x, int16_t z, int16_t rotation, int16_t *out_x, int16_t *out_z) {
  float angle = ((float)(rotation & 1023) * 6.28318530718f) / 1024.0f;
  int32_t cos_word = (int32_t)(SDL_cosf(angle) * 32766.0f);
  int32_t sin_word = (int32_t)(SDL_sinf(angle) * 32766.0f);
  int64_t new_x = (((int64_t)x * (int64_t)cos_word) + ((int64_t)z * (int64_t)-sin_word)) * 2;
  int64_t new_z = (((int64_t)x * (int64_t)sin_word) + ((int64_t)z * (int64_t)cos_word)) * 2;

  if (out_x != NULL) {
    *out_x = clamp_int16(amiga_arithmetic_shift_right_16(new_x));
  }
  if (out_z != NULL) {
    *out_z = clamp_int16(amiga_arithmetic_shift_right_16(new_z));
  }
}

static void start_runtime_door(AppState *state, uint16_t zone_index) {
  size_t i = 0u;
  RuntimeDoor *door = NULL;
  const GloomZone *zone = NULL;

  if (state == NULL || (size_t)zone_index >= state->map.zone_count) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_ACTIVE_DOORS; ++i) {
    if (state->doors[i].active && state->doors[i].zone_index == (size_t)zone_index) {
      return;
    }
  }

  for (i = 0u; i < GLOOM_MAX_ACTIVE_DOORS; ++i) {
    if (!state->doors[i].active) {
      door = &state->doors[i];
      break;
    }
  }

  if (door == NULL) {
    fprintf(stderr, "[WARN] Door command for zone %u ignored; active door list is full\n", (unsigned)zone_index);
    return;
  }

  zone = &state->map.zones[zone_index];
  memset(door, 0, sizeof(*door));
  door->active = true;
  door->zone_index = (size_t)zone_index;
  door->base_x1 = zone->x1;
  door->base_z1 = zone->z1;
  door->base_x2 = zone->x2;
  door->base_z2 = zone->z2;
  door->frac = 0;
  door->frac_add = 0x0100;
}

static void start_pending_teleport(AppState *state, const GloomTeleportCommand *command) {
  if (state == NULL || command == NULL || command->control_word != 0) {
    return;
  }

  state->teleport_active = true;
  state->teleport_ticks = GLOOM_TELEPORT_DELAY_TICKS;
  state->teleport_x = command->x;
  state->teleport_z = command->z;
  state->teleport_rotation = command->rotation;
}

static void start_runtime_rotpoly(AppState *state, const GloomRotPolyCommand *command) {
  RuntimeRotPoly *rotpoly = NULL;
  size_t i = 0u;
  size_t first_zone = 0u;
  size_t zone_count = 0u;

  if (state == NULL || command == NULL) {
    return;
  }

  first_zone = (size_t)command->first_zone_index;
  zone_count = (size_t)command->zone_count;
  if (zone_count == 0u || zone_count > (size_t)GLOOM_MAX_ROTPOLY_ZONES || first_zone >= state->map.zone_count ||
      zone_count > state->map.zone_count - first_zone) {
    return;
  }
  if ((command->flags & 1u) != 0u && zone_count * 2u > state->map.zone_count - first_zone) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_ACTIVE_ROTPOLYS; ++i) {
    if (!state->rotpolys[i].active) {
      rotpoly = &state->rotpolys[i];
      break;
    }
  }

  if (rotpoly == NULL) {
    fprintf(stderr, "[WARN] Rotpoly command for zone %u ignored; active rotpoly list is full\n",
            (unsigned)command->first_zone_index);
    return;
  }

  memset(rotpoly, 0, sizeof(*rotpoly));
  rotpoly->active = true;
  rotpoly->first_zone_index = first_zone;
  rotpoly->zone_count = command->zone_count;
  rotpoly->speed = command->speed;
  rotpoly->flags = command->flags;

  if ((rotpoly->flags & 1u) != 0u) {
    for (i = 0u; i < zone_count; ++i) {
      const GloomZone *source = &state->map.zones[first_zone + i];
      const GloomZone *target = &state->map.zones[first_zone + zone_count + i];
      RuntimeRotPolyVertex *vertex = &rotpoly->vertices[i];

      vertex->base_x = source->x1;
      vertex->base_z = source->z1;
      vertex->delta_x = (int16_t)((int32_t)target->x1 - (int32_t)source->x1);
      vertex->delta_z = (int16_t)((int32_t)target->z1 - (int32_t)source->z1);
    }
  } else {
    int32_t center_x = 0;
    int32_t center_z = 0;

    for (i = 0u; i < zone_count; ++i) {
      const GloomZone *zone = &state->map.zones[first_zone + i];

      center_x += zone->x1;
      center_z += zone->z1;
    }

    center_x /= (int32_t)zone_count;
    center_z /= (int32_t)zone_count;
    rotpoly->center_x = clamp_int16(center_x);
    rotpoly->center_z = clamp_int16(center_z);

    for (i = 0u; i < zone_count; ++i) {
      const GloomZone *zone = &state->map.zones[first_zone + i];
      RuntimeRotPolyVertex *vertex = &rotpoly->vertices[i];

      vertex->base_x = clamp_int16((int32_t)zone->x1 - (int32_t)rotpoly->center_x);
      vertex->base_z = clamp_int16((int32_t)zone->z1 - (int32_t)rotpoly->center_z);
      vertex->base_na = zone->na;
      vertex->base_nb = zone->nb;
    }
  }
}

static void execute_map_event(AppState *state, uint8_t event_id) {
  size_t i = 0u;

  if (state == NULL || event_id == 0u || event_id > (uint8_t)GLOOM_EVENT_COUNT) {
    return;
  }

  if (event_id < 19u && state->triggered_events[event_id]) {
    return;
  }

  if (event_id < 19u) {
    state->triggered_events[event_id] = true;
    for (i = 0u; i < state->map.zone_count; ++i) {
      if (state->map.zones[i].event_id == (int16_t)event_id) {
        state->map.zones[i].event_id = (int16_t)-state->map.zones[i].event_id;
      }
    }
  }

  for (i = 0u; i < state->map.texture_change_command_count; ++i) {
    const GloomTextureChangeCommand *command = &state->map.texture_change_commands[i];

    if (command->event_id == event_id && (size_t)command->zone_index < state->map.zone_count) {
      state->map.zones[command->zone_index].textures[0] = command->texture_index;
    }
  }

  for (i = 0u; i < state->map.teleport_command_count; ++i) {
    const GloomTeleportCommand *command = &state->map.teleport_commands[i];

    if (command->event_id == event_id) {
      start_pending_teleport(state, command);
    }
  }

  for (i = 0u; i < state->map.rotpoly_command_count; ++i) {
    const GloomRotPolyCommand *command = &state->map.rotpoly_commands[i];

    if (command->event_id == event_id) {
      start_runtime_rotpoly(state, command);
    }
  }

  for (i = 0u; i < state->map.door_command_count; ++i) {
    const GloomDoorCommand *command = &state->map.door_commands[i];

    if (command->event_id == event_id) {
      start_runtime_door(state, command->zone_index);
    }
  }
}

static void update_rotpolys(AppState *state) {
  size_t i = 0u;

  if (state == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_ACTIVE_ROTPOLYS; ++i) {
    RuntimeRotPoly *rotpoly = &state->rotpolys[i];
    size_t first_zone = 0u;
    size_t zone_count = 0u;
    int16_t new_x[GLOOM_MAX_ROTPOLY_ZONES];
    int16_t new_z[GLOOM_MAX_ROTPOLY_ZONES];
    size_t zone_index = 0u;

    if (!rotpoly->active || rotpoly->speed == 0) {
      continue;
    }

    first_zone = rotpoly->first_zone_index;
    zone_count = (size_t)rotpoly->zone_count;
    if (zone_count == 0u || zone_count > (size_t)GLOOM_MAX_ROTPOLY_ZONES || first_zone >= state->map.zone_count ||
        zone_count > state->map.zone_count - first_zone) {
      rotpoly->active = false;
      continue;
    }

    rotpoly->rot = (int16_t)((uint16_t)rotpoly->rot + (uint16_t)rotpoly->speed);

    if ((rotpoly->flags & 1u) != 0u) {
      int32_t frac = rotpoly->rot;

      if (frac <= 0) {
        frac = 0;
        rotpoly->speed = (int16_t)-rotpoly->speed;
      } else if (frac >= 0x4000) {
        frac = 0x4000;
        if ((rotpoly->flags & 2u) != 0u) {
          rotpoly->speed = (int16_t)-rotpoly->speed;
        } else {
          rotpoly->speed = 0;
        }
      }

      rotpoly->rot = (int16_t)frac;

      for (zone_index = 0u; zone_index < zone_count; ++zone_index) {
        const RuntimeRotPolyVertex *vertex = &rotpoly->vertices[zone_index];
        int32_t move_x =
            amiga_arithmetic_shift_right_16((int64_t)vertex->delta_x * (int64_t)frac * 4);
        int32_t move_z =
            amiga_arithmetic_shift_right_16((int64_t)vertex->delta_z * (int64_t)frac * 4);

        new_x[zone_index] = clamp_int16((int32_t)vertex->base_x + move_x);
        new_z[zone_index] = clamp_int16((int32_t)vertex->base_z + move_z);
      }
    } else {
      for (zone_index = 0u; zone_index < zone_count; ++zone_index) {
        const RuntimeRotPolyVertex *vertex = &rotpoly->vertices[zone_index];
        int16_t rel_x = 0;
        int16_t rel_z = 0;

        rotate_amiga_1024(vertex->base_x, vertex->base_z, rotpoly->rot, &rel_x, &rel_z);
        new_x[zone_index] = clamp_int16((int32_t)rotpoly->center_x + (int32_t)rel_x);
        new_z[zone_index] = clamp_int16((int32_t)rotpoly->center_z + (int32_t)rel_z);
      }
    }

    for (zone_index = 0u; zone_index < zone_count; ++zone_index) {
      size_t current_index = first_zone + zone_index;
      size_t previous_index = first_zone + ((zone_index == 0u) ? zone_count - 1u : zone_index - 1u);

      state->map.zones[current_index].x1 = new_x[zone_index];
      state->map.zones[current_index].z1 = new_z[zone_index];
      state->map.zones[previous_index].x2 = new_x[zone_index];
      state->map.zones[previous_index].z2 = new_z[zone_index];
    }

    if ((rotpoly->flags & 1u) != 0u) {
      for (zone_index = 0u; zone_index < zone_count; ++zone_index) {
        recalculate_zone_vectors(&state->map.zones[first_zone + zone_index]);
      }
    } else {
      for (zone_index = 0u; zone_index < zone_count; ++zone_index) {
        const RuntimeRotPolyVertex *vertex = &rotpoly->vertices[zone_index];
        GloomZone *zone = &state->map.zones[first_zone + zone_index];
        int16_t na = 0;
        int16_t nb = 0;

        rotate_amiga_1024(vertex->base_na, vertex->base_nb, rotpoly->rot, &na, &nb);
        zone->na = na;
        zone->nb = nb;
        zone->a = clamp_int16(-(int32_t)nb);
        zone->b = na;
      }
    }
  }
}

static void update_doors(AppState *state) {
  size_t i = 0u;

  if (state == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_ACTIVE_DOORS; ++i) {
    RuntimeDoor *door = &state->doors[i];
    GloomZone *zone = NULL;
    int32_t frac = 0;
    int32_t width_x = 0;
    int32_t width_z = 0;
    int32_t move_x = 0;
    int32_t move_z = 0;

    if (!door->active || door->zone_index >= state->map.zone_count) {
      continue;
    }

    zone = &state->map.zones[door->zone_index];
    frac = (int32_t)door->frac + (int32_t)door->frac_add;
    if (frac < 0) {
      frac = 0;
    }
    if (frac > 0x4000) {
      frac = 0x4000;
    }

    door->frac = (int16_t)frac;
    zone->event_id = (int16_t)(uint16_t)(frac * 2);

    width_x = (int32_t)door->base_x2 - (int32_t)door->base_x1;
    width_z = (int32_t)door->base_z2 - (int32_t)door->base_z1;
    move_x = amiga_arithmetic_shift_right_16((int64_t)width_x * (int64_t)frac * 4);
    move_z = amiga_arithmetic_shift_right_16((int64_t)width_z * (int64_t)frac * 4);

    zone->x1 = (int16_t)((int32_t)door->base_x1 - move_x);
    zone->z1 = (int16_t)((int32_t)door->base_z1 - move_z);
    zone->x2 = (int16_t)((int32_t)zone->x1 + width_x);
    zone->z2 = (int16_t)((int32_t)zone->z1 + width_z);

    if (frac == 0 || frac == 0x4000) {
      door->active = false;
    }
  }
}

static void check_event_triggers(AppState *state) {
  static const int offsets[9][2] = {{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1},
                                   {-1, -1}, {1, -1}, {-1, 1}, {1, 1}};
  uint8_t seen[GLOOM_MAX_RENDER_ZONES] = {0};
  int cam_x = 0;
  int cam_z = 0;
  int base_x = 0;
  int base_z = 0;
  size_t offset_index = 0u;

  if (state == NULL || state->player_radius <= 0 || !state->map.has_grid_cells || state->map.ppnt_blob == NULL ||
      state->map.ppnt_blob_size < 2u || state->map.zone_count > (size_t)GLOOM_MAX_RENDER_ZONES) {
    return;
  }

  cam_x = floor_to_int(state->camera_x);
  cam_z = floor_to_int(state->camera_z);
  if (cam_x < 0 || cam_z < 0 || cam_x > 32767 || cam_z > 32767) {
    return;
  }

  base_x = cam_x >> 8;
  base_z = cam_z >> 8;
  if (base_x < 0 || base_x >= GLOOM_GRID_WIDTH || base_z < 0 || base_z >= GLOOM_GRID_HEIGHT) {
    return;
  }

  for (offset_index = 0u; offset_index < sizeof(offsets) / sizeof(offsets[0]); ++offset_index) {
    int gx = base_x + offsets[offset_index][0];
    int gz = base_z + offsets[offset_index][1];
    size_t cell_index = 0u;
    const GloomGridCell *cell = NULL;
    size_t poly_count = 0u;
    size_t poly_index = 0u;

    if (gx < 0 || gx >= GLOOM_GRID_WIDTH || gz < 0 || gz >= GLOOM_GRID_HEIGHT) {
      continue;
    }

    cell_index = (size_t)gz * (size_t)GLOOM_GRID_WIDTH + (size_t)gx;
    cell = &state->map.trigger_grid[cell_index];
    if (cell->count_minus_one < 0) {
      continue;
    }

    poly_count = (size_t)cell->count_minus_one + 1u;
    if (((size_t)cell->ppnt_word_offset + poly_count) * 2u > state->map.ppnt_blob_size) {
      continue;
    }

    for (poly_index = 0u; poly_index < poly_count; ++poly_index) {
      size_t word_offset = (size_t)cell->ppnt_word_offset + poly_index;
      size_t zone_index = (size_t)main_read_be16(state->map.ppnt_blob + (word_offset * 2u));
      GloomZone *zone = NULL;
      uint16_t distance = 0u;
      uint8_t event_id = 0u;

      if (zone_index >= state->map.zone_count || zone_index >= (size_t)GLOOM_MAX_RENDER_ZONES || seen[zone_index]) {
        continue;
      }

      seen[zone_index] = 1u;
      zone = &state->map.zones[zone_index];
      if (zone->event_id <= 0 || zone->event_id > (int16_t)GLOOM_EVENT_COUNT) {
        continue;
      }

      distance = zone_segment_distance(zone, (int16_t)cam_x, (int16_t)cam_z);
      if ((int32_t)distance - (int32_t)state->player_radius >= 0) {
        continue;
      }

      event_id = (uint8_t)zone->event_id;
      execute_map_event(state, event_id);
    }
  }
}

static uint32_t sample_flat_texture_argb(const FlatTexture *texture, float world_x, float world_z) {
  int tx = 0;
  int tz = 0;
  uint8_t palette_index = 0;

  if (texture == NULL || !texture->loaded || texture->texels == NULL) {
    return 0xFF000000u;
  }

  tx = floor_to_int(world_x) & (GLOOM_FLAT_TEXTURE_SIZE - 1);
  tz = floor_to_int(world_z) & (GLOOM_FLAT_TEXTURE_SIZE - 1);
  palette_index = texture->texels[(tx * GLOOM_FLAT_TEXTURE_SIZE) + tz];
  return texture->palette[palette_index];
}

static float projection_focal_for_viewport(int w, int h) {
  float scale_x = (float)w / (float)GLOOM_AMIGA_VIEW_COLUMNS;
  float scale_y = (float)h / (float)GLOOM_AMIGA_VIEW_ROWS;
  float scale = scale_x > scale_y ? scale_x : scale_y;

  if (scale < 0.01f) {
    scale = 0.01f;
  }

  return (float)GLOOM_AMIGA_FOCAL * scale;
}

static void render_flat_textures(SDL_Renderer *renderer, const AppState *state, const FlatTextureSet *flats, int x,
                                 int y, int w, int h, float focal, float far_depth) {
  float eye_height = 0.0f;
  float ceiling_delta = 0.0f;
  float view_cos = 0.0f;
  float view_sin = 0.0f;
  int row = 0;

  if (renderer == NULL || state == NULL || flats == NULL || !flats->floor.loaded || !flats->roof.loaded) {
    return;
  }

  eye_height = -state->camera_y;
  ceiling_delta = 255.0f + state->camera_y;
  view_cos = SDL_cosf(state->camera_angle);
  view_sin = SDL_sinf(state->camera_angle);

  if (eye_height < 1.0f) {
    eye_height = 1.0f;
  }
  if (ceiling_delta < 1.0f) {
    ceiling_delta = 1.0f;
  }

  for (row = 0; row < h; ++row) {
    float centered_y = (float)(row - (h / 2));
    const FlatTexture *texture = NULL;
    float plane_distance = 0.0f;
    float depth = 0.0f;
    int col = 0;

    if (centered_y > 0.5f) {
      texture = &flats->floor;
      plane_distance = eye_height;
      depth = (plane_distance * focal) / centered_y;
    } else if (centered_y < -0.5f) {
      texture = &flats->roof;
      plane_distance = ceiling_delta;
      depth = (plane_distance * focal) / -centered_y;
    } else {
      continue;
    }

    if (depth < 1.0f || depth >= far_depth) {
      continue;
    }

    for (col = 0; col < w; ++col) {
      float ray_x = ((float)(col - (w / 2))) / focal;
      float view_x = ray_x * depth;
      float world_x = state->camera_x + (view_x * view_cos) + (depth * view_sin);
      float world_z = state->camera_z - (view_x * view_sin) + (depth * view_cos);
      uint32_t argb = sample_flat_texture_argb(texture, world_x, world_z);
      uint8_t r = 0u;
      uint8_t g = 0u;
      uint8_t b = 0u;

      argb = apply_amiga_depth_argb(argb, depth);
      r = (uint8_t)(argb >> 16);
      g = (uint8_t)(argb >> 8);
      b = (uint8_t)argb;

      SDL_SetRenderDrawColor(renderer, r, g, b, 255);
      SDL_RenderDrawPoint(renderer, x + col, y + row);
    }
  }
}

static void render_wall_debug(SDL_Renderer *renderer, const AppState *state, const GridOffsetSet *grid_offsets,
                              const WallTextureSet *wall_textures, const FlatTextureSet *flats,
                              const ObjectVisualSet *object_visuals, int x, int y, int w, int h) {
  typedef struct {
    float screen_x;
    float screen_y;
    float screen_w;
    float screen_h;
    float depth;
    const ObjectFrame *frame;
  } DebugSprite;

  float near_plane = 1.0f;
  float view_angle = state->camera_angle;
  float cam_x = state->camera_x;
  float cam_z = state->camera_z;
  float view_cos = SDL_cosf(view_angle);
  float view_sin = SDL_sinf(view_angle);
  float focal = projection_focal_for_viewport(w, h);
  float frustum_ratio = ((float)w * 0.5f) / focal;
  float far_depth = (float)GLOOM_AMIGA_MAX_Z;
  float *depth_buffer = NULL;
  float *sprite_depth_buffer = NULL;
  uint8_t *filled_pixels = NULL;
  DebugSprite debug_sprites[GLOOM_MAX_DEBUG_SPRITES];
  WallCandidate wall_candidates[GLOOM_MAX_WALL_CANDIDATES];
  SceneWall scene_walls[GLOOM_MAX_WALL_CANDIDATES];
  size_t wall_candidate_count = 0;
  size_t scene_wall_count = 0;
  size_t sprite_write = 0;
  size_t i = 0;
  int horizon_y = y + (h / 2);

  if (w <= 0 || h <= 0) {
    return;
  }

  if (wall_textures == NULL || wall_textures->loaded_count == 0u) {
    return;
  }

  depth_buffer = (float *)malloc((size_t)w * sizeof(*depth_buffer));
  sprite_depth_buffer = (float *)malloc((size_t)w * sizeof(*sprite_depth_buffer));
  filled_pixels = (uint8_t *)malloc((size_t)h);
  if (depth_buffer == NULL || sprite_depth_buffer == NULL || filled_pixels == NULL) {
    free(depth_buffer);
    free(sprite_depth_buffer);
    free(filled_pixels);
    return;
  }

  wall_candidate_count =
      collect_wall_candidates_from_grid(&state->map, grid_offsets, cam_x, cam_z, wall_candidates,
                                        sizeof(wall_candidates) / sizeof(wall_candidates[0]));
  append_runtime_rotpoly_wall_candidates(state, wall_candidates, &wall_candidate_count,
                                         sizeof(wall_candidates) / sizeof(wall_candidates[0]), cam_x, cam_z);
  if (wall_candidate_count > 1u) {
    qsort(wall_candidates, wall_candidate_count, sizeof(wall_candidates[0]), compare_wall_candidates);
  }

  render_flat_textures(renderer, state, flats, x, y, w, h, focal, far_depth);

  for (i = 0; i < (size_t)w; ++i) {
    depth_buffer[i] = 1.0e9f;
    sprite_depth_buffer[i] = 1.0e9f;
  }

  for (i = 0; i < wall_candidate_count; ++i) {
    const GloomZone *z = &state->map.zones[wall_candidates[i].zone_index];
    float x1 = (float)z->x1 - cam_x;
    float z1 = (float)z->z1 - cam_z;
    float x2 = (float)z->x2 - cam_x;
    float z2 = (float)z->z2 - cam_z;
    float vx1 = x1 * view_cos - z1 * view_sin;
    float vz1 = x1 * view_sin + z1 * view_cos;
    float vx2 = x2 * view_cos - z2 * view_sin;
    float vz2 = x2 * view_sin + z2 * view_cos;
    float face_a = vz1 - vz2;
    float face_b = vx2 - vx1;
    float face_c = vx1 * face_a + vz1 * face_b;
    SceneWall *scene_wall = NULL;

    if (vz1 <= 0.0f && vz2 <= 0.0f) {
      continue;
    }

    if (vz1 >= far_depth && vz2 >= far_depth) {
      continue;
    }

    if (face_c < 0.0f) {
      continue;
    }

    if (scene_wall_count >= sizeof(scene_walls) / sizeof(scene_walls[0])) {
      break;
    }

    scene_wall = &scene_walls[scene_wall_count++];
    scene_wall->zone = z;
    scene_wall->vx1 = vx1;
    scene_wall->vz1 = vz1;
    scene_wall->vx2 = vx2;
    scene_wall->vz2 = vz2;
    scene_wall->sort_depth = wall_candidates[i].sort_depth;
  }

  for (i = 0; i < (size_t)w; ++i) {
    int screen_x = x + (int)i;
    float ray_x = ((float)((int)i - (w / 2))) / focal;
    RayWallHit hits[GLOOM_MAX_COLUMN_WALL_HITS];
    size_t hit_count = 0u;
    size_t wall_index = 0;
    size_t hit_index = 0;

    memset(filled_pixels, 0, (size_t)h);

    for (wall_index = 0; wall_index < scene_wall_count; ++wall_index) {
      const SceneWall *wall = &scene_walls[wall_index];
      float sx = wall->vx2 - wall->vx1;
      float sz = wall->vz2 - wall->vz1;
      float denom = ray_x * sz - sx;
      float depth = 0.0f;
      float wall_u = 0.0f;

      if (SDL_fabsf(denom) < 0.0001f) {
        continue;
      }

      depth = (wall->vx1 * sz - wall->vz1 * sx) / denom;
      if (depth < near_plane || depth >= far_depth) {
        continue;
      }

      wall_u = (wall->vx1 - wall->vz1 * ray_x) / denom;
      if (wall_u < -0.0005f || wall_u > 1.0005f) {
        continue;
      }

      if (wall->zone->event_id > 0) {
        float open_threshold = (float)(uint16_t)wall->zone->event_id / 32768.0f;

        if (wall_u < open_threshold) {
          continue;
        }
      }

      {
        RayWallHit hit;
        size_t insert_at = hit_count;

        hit.wall = wall;
        hit.depth = depth;
        hit.wall_u = clampf(wall_u, 0.0f, 1.0f);

        while (insert_at > 0u && hits[insert_at - 1u].depth > hit.depth) {
          if (insert_at < (size_t)GLOOM_MAX_COLUMN_WALL_HITS) {
            hits[insert_at] = hits[insert_at - 1u];
          }
          insert_at -= 1u;
        }

        if (insert_at < (size_t)GLOOM_MAX_COLUMN_WALL_HITS) {
          hits[insert_at] = hit;
          if (hit_count < (size_t)GLOOM_MAX_COLUMN_WALL_HITS) {
            hit_count += 1u;
          }
        }
      }
    }

    for (hit_index = 0u; hit_index < hit_count; ++hit_index) {
      const RayWallHit *hit = &hits[hit_index];
      float wall_top =
          (float)horizon_y + (((float)GLOOM_WALL_TOP_Y - state->camera_y) * focal) / hit->depth;
      float wall_bottom =
          (float)horizon_y + (((float)GLOOM_WALL_BOTTOM_Y - state->camera_y) * focal) / hit->depth;
      float wall_height = wall_bottom - wall_top;
      int y0 = (int)wall_top;
      int y1 = (int)wall_bottom;
      int draw_y = 0;

      if (wall_height < 1.0f) {
        float center_y = (wall_top + wall_bottom) * 0.5f;
        wall_top = center_y - 0.5f;
        wall_bottom = center_y + 0.5f;
        wall_height = wall_bottom - wall_top;
        y0 = (int)wall_top;
        y1 = (int)wall_bottom;
      }

      if (y0 < y) y0 = y;
      if (y1 > y + h - 1) y1 = y + h - 1;
      if (y0 > y1 || wall_height <= 0.0f) {
        continue;
      }

      for (draw_y = y0; draw_y <= y1; ++draw_y) {
        float wall_v = ((float)draw_y + 0.5f - wall_top) / wall_height;
        uint32_t argb = 0u;
        uint8_t alpha = 0u;
        uint8_t r = 0u;
        uint8_t g = 0u;
        uint8_t b = 0u;

        if (filled_pixels[draw_y - y] != 0u) {
          continue;
        }

        argb = sample_zone_wall_texture_argb(wall_textures, hit->wall->zone, hit->wall_u, wall_v);
        alpha = (uint8_t)(argb >> 24);

        if (alpha == 0u) {
          continue;
        }

        argb = apply_amiga_depth_argb(argb, hit->depth);
        r = (uint8_t)(argb >> 16);
        g = (uint8_t)(argb >> 8);
        b = (uint8_t)argb;
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawPoint(renderer, screen_x, draw_y);
        filled_pixels[draw_y - y] = 1u;

        if (hit->depth < depth_buffer[i]) {
          depth_buffer[i] = hit->depth;
        }
      }
    }
  }

  if (state->map.object_spawn_count > 0u && object_visuals != NULL) {
    for (i = 0; i < state->map.object_spawn_count && sprite_write < GLOOM_MAX_DEBUG_SPRITES; ++i) {
      const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
      const ObjectVisual *visual = NULL;
      const ObjectFrame *frame = NULL;
      float wx = (float)spawn->x;
      float wz = (float)spawn->z;
      float dx = wx - cam_x;
      float dz = wz - cam_z;
      float svx = dx * view_cos - dz * view_sin;
      float svz = dx * view_sin + dz * view_cos;
      float camera_y = state->camera_y;
      float object_y = 0.0f;
      float scale = 1.0f;
      float top_view_y = 0.0f;
      float left_view_x = 0.0f;

      if (spawn->event_id == 1u && spawn->object_type == 0) {
        continue;
      }

      if (spawn->object_type < 0 || spawn->object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
        continue;
      }

      visual = &object_visuals->visuals[spawn->object_type];
      if (!visual->loaded) {
        continue;
      }

      frame = select_object_visual_frame(visual, spawn, state);
      if (frame == NULL || frame->width <= 0 || frame->height <= 0) {
        continue;
      }

      if (svz >= near_plane && svz < far_depth && SDL_fabsf(svx) <= svz * frustum_ratio * 1.1f) {
        DebugSprite *sp = &debug_sprites[sprite_write];

        object_y = (float)spawn->y - camera_y;
        scale = (float)visual->scale / 256.0f;
        top_view_y = object_y - ((float)frame->handle_y * scale);
        left_view_x = svx - ((float)frame->handle_x * scale);

        sp->screen_x = (float)x + (float)w * 0.5f + (left_view_x / svz) * focal;
        sp->screen_y = (float)horizon_y + (top_view_y / svz) * focal;
        sp->screen_w = ((float)frame->width * scale * focal) / svz;
        sp->screen_h = ((float)frame->height * scale * focal) / svz;
        sp->depth = svz;
        sp->frame = frame;

        if (sp->screen_w >= 1.0f && sp->screen_h >= 1.0f) {
          sprite_write += 1u;
        }
      }
    }
  }

  for (i = 0; i < (size_t)w; ++i) {
    sprite_depth_buffer[i] = depth_buffer[i];
  }

  if (sprite_write > 1u) {
    size_t a = 0;
    for (a = 0; a + 1u < sprite_write; ++a) {
      size_t b = 0;
      for (b = a + 1u; b < sprite_write; ++b) {
        if (debug_sprites[a].depth < debug_sprites[b].depth) {
          DebugSprite tmp = debug_sprites[a];
          debug_sprites[a] = debug_sprites[b];
          debug_sprites[b] = tmp;
        }
      }
    }
  }

  for (i = 0; i < sprite_write; ++i) {
    const DebugSprite *sp = &debug_sprites[i];
    const ObjectFrame *frame = sp->frame;
    int x0 = 0;
    int x1 = 0;
    int y0 = 0;
    int y1 = 0;
    int screen_x = 0;

    if (frame == NULL || sp->screen_w < 1.0f || sp->screen_h < 1.0f) {
      continue;
    }

    x0 = (int)sp->screen_x;
    x1 = (int)(sp->screen_x + sp->screen_w);
    y0 = (int)sp->screen_y;
    y1 = (int)(sp->screen_y + sp->screen_h);

    if (x1 < x || x0 > x + w - 1 || y1 < y || y0 > y + h - 1) {
      continue;
    }

    if (x0 < x) x0 = x;
    if (x1 > x + w - 1) x1 = x + w - 1;
    if (y0 < y) y0 = y;
    if (y1 > y + h - 1) y1 = y + h - 1;

    for (screen_x = x0; screen_x <= x1; ++screen_x) {
      int rel_x = screen_x - x;
      int draw_y = 0;
      float tu = ((float)screen_x + 0.5f - sp->screen_x) / sp->screen_w;
      bool column_drawn = false;

      if (sp->depth >= sprite_depth_buffer[rel_x]) {
        continue;
      }

      for (draw_y = y0; draw_y <= y1; ++draw_y) {
        float tv = ((float)draw_y + 0.5f - sp->screen_y) / sp->screen_h;
        uint32_t argb = 0;
        uint8_t alpha = 0;
        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;

        argb = sample_object_frame_argb(frame, tu, tv);

        alpha = (uint8_t)(argb >> 24);
        r = (uint8_t)(argb >> 16);
        g = (uint8_t)(argb >> 8);
        b = (uint8_t)argb;

        if (alpha == 0u) {
          continue;
        }

        argb = apply_amiga_depth_argb(argb, sp->depth);
        r = (uint8_t)(argb >> 16);
        g = (uint8_t)(argb >> 8);
        b = (uint8_t)argb;

        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        SDL_RenderDrawPoint(renderer, screen_x, draw_y);
        column_drawn = true;
      }

      if (column_drawn) {
        sprite_depth_buffer[rel_x] = sp->depth;
      }
    }
  }

  free(filled_pixels);
  free(sprite_depth_buffer);
  free(depth_buffer);
}

static void render_texture_preview(SDL_Renderer *renderer, const PreviewAsset *preview, int x, int y, int w, int h) {
  SDL_Rect panel = {x, y, w, h};

  SDL_SetRenderDrawColor(renderer, 16, 20, 28, 255);
  SDL_RenderFillRect(renderer, &panel);

  if (preview != NULL && preview->texture != NULL && preview->width > 0 && preview->height > 0) {
    float fit_w = (float)(w - 8);
    float fit_h = (float)(h - 8);
    float scale_x = fit_w / (float)preview->width;
    float scale_y = fit_h / (float)preview->height;
    float scale = scale_x < scale_y ? scale_x : scale_y;
    int dst_w = (int)((float)preview->width * scale);
    int dst_h = (int)((float)preview->height * scale);
    SDL_Rect dst = {x + (w - dst_w) / 2, y + (h - dst_h) / 2, dst_w, dst_h};

    if (dst_w < 1) dst.w = 1;
    if (dst_h < 1) dst.h = 1;
    SDL_RenderCopy(renderer, preview->texture, NULL, &dst);
  } else {
    SDL_SetRenderDrawColor(renderer, 90, 90, 110, 255);
    SDL_RenderDrawLine(renderer, x + 6, y + 6, x + w - 6, y + h - 6);
    SDL_RenderDrawLine(renderer, x + w - 6, y + 6, x + 6, y + h - 6);
  }

  SDL_SetRenderDrawColor(renderer, 75, 90, 112, 255);
  SDL_RenderDrawRect(renderer, &panel);
}

static void draw_bar(SDL_Renderer *renderer, int x, int y, int width, int height, uint32_t value, uint32_t max_value,
                     SDL_Color color) {
  SDL_Rect bg = {x, y, width, height};
  SDL_Rect fg = {x, y, 0, height};

  SDL_SetRenderDrawColor(renderer, 28, 34, 44, 255);
  SDL_RenderFillRect(renderer, &bg);

  if (max_value > 0 && value > 0) {
    fg.w = (int)((uint64_t)width * (uint64_t)value / (uint64_t)max_value);
    if (fg.w < 1) {
      fg.w = 1;
    }
    if (fg.w > width) {
      fg.w = width;
    }

    SDL_SetRenderDrawColor(renderer, color.r, color.g, color.b, color.a);
    SDL_RenderFillRect(renderer, &fg);
  }

  SDL_SetRenderDrawColor(renderer, 65, 78, 98, 255);
  SDL_RenderDrawRect(renderer, &bg);
}

static void render(SDL_Renderer *renderer, const AppState *state, const GridOffsetSet *grid_offsets,
                   const WallTextureSet *wall_textures, const FlatTextureSet *flat_textures,
                   const ObjectVisualSet *object_visuals, int render_width, int render_height) {
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);

  render_wall_debug(renderer, state, grid_offsets, wall_textures, flat_textures, object_visuals, 0, 0, render_width,
                    render_height);

  SDL_RenderPresent(renderer);
}

static void update_render_dimensions(SDL_Renderer *renderer, bool classic_viewport, int *render_width,
                                     int *render_height) {
  int target_width = BASE_WIDTH;
  int target_height = BASE_HEIGHT;

  if (renderer == NULL || render_width == NULL || render_height == NULL) {
    return;
  }

  if (!classic_viewport) {
    int output_width = 0;
    int output_height = 0;

    if (SDL_GetRendererOutputSize(renderer, &output_width, &output_height) == 0 && output_width > 0 &&
        output_height > 0) {
      int64_t scaled_width = ((int64_t)output_width * (int64_t)BASE_HEIGHT) + (output_height / 2);
      target_width = (int)(scaled_width / output_height);
      if (target_width < 1) {
        target_width = 1;
      }
    } else {
      target_width = WIDESCREEN_WIDTH;
    }
  }

  if (*render_width != target_width || *render_height != target_height) {
    *render_width = target_width;
    *render_height = target_height;
    (void)SDL_RenderSetLogicalSize(renderer, target_width, target_height);
  }
}

int main(int argc, char **argv) {
  const char *map_path = "amiga/maps/map1_1";
  const char *resolved_map_path = map_path;
  char map_path_buffer[1024] = {0};
  char error[256] = {0};
  SDL_Window *window = NULL;
  SDL_Renderer *renderer = NULL;
  GridOffsetSet grid_offsets;
  WallTextureSet wall_textures;
  FlatTextureSet flat_textures;
  ObjectVisualSet object_visuals;
  uint64_t perf_frequency = 0;
  uint64_t perf_prev = 0;
  double accumulator = 0.0;
  double title_timer = 0.0;
  const double dt = 1.0 / (double)FIXED_TICK_HZ;
  bool running = true;
  int mouse_dx_accum = 0;
  int render_width = WIDESCREEN_WIDTH;
  int render_height = BASE_HEIGHT;
  bool classic_viewport = false;
  AppState state;

  memset(&grid_offsets, 0, sizeof(grid_offsets));
  memset(&wall_textures, 0, sizeof(wall_textures));
  memset(&flat_textures, 0, sizeof(flat_textures));
  memset(&object_visuals, 0, sizeof(object_visuals));

  if (argc > 1 && strcmp(argv[1], "--iff-info") == 0) {
    const char *iff_path = argc > 2 ? argv[2] : "amiga/combat.iff";
    return run_iff_info(iff_path);
  }

  if (argc > 1 && strcmp(argv[1], "--iff-preview") == 0) {
    const char *iff_path = argc > 2 ? argv[2] : "amiga/combat.iff";
    return run_iff_preview(iff_path);
  }

  if (argc > 1 && strcmp(argv[1], "--sweep") == 0) {
    SweepStats stats;
    int argi = 0;

    memset(&stats, 0, sizeof(stats));

    if (argc > 2) {
      for (argi = 2; argi < argc; ++argi) {
        sweep_directory(argv[argi], &stats);
      }
    } else {
      sweep_directory("amiga/maps", &stats);
      sweep_directory("amiga/data/maps", &stats);
    }

    printf("Sweep complete: parsed=%u skipped=%u failed=%u\n", stats.parsed, stats.skipped, stats.failed);
    return stats.failed == 0 ? 0 : 1;
  }

  if (argc > 1) {
    int argi = 0;

    for (argi = 1; argi < argc; ++argi) {
      if (strcmp(argv[argi], "--classic") == 0) {
        classic_viewport = true;
        render_width = BASE_WIDTH;
      } else if (strcmp(argv[argi], "--widescreen") == 0) {
        classic_viewport = false;
        render_width = WIDESCREEN_WIDTH;
      } else {
        map_path = argv[argi];
      }
    }
  }

  if (resolve_runtime_file_path(map_path, map_path_buffer, sizeof(map_path_buffer))) {
    resolved_map_path = map_path_buffer;
  }

  memset(&state, 0, sizeof(state));

  if (!gloom_map_load(resolved_map_path, &state.map, error, sizeof(error))) {
    fprintf(stderr, "Map parse failed: %s\n", error[0] ? error : "unknown error");
    return 1;
  }

  printf("Loaded %s\n", resolved_map_path);
  printf("zones=%zu animations=%zu grid_bytes=%zu ppnt_bytes=%zu event_commands=%u object_spawns=%zu\n",
         state.map.zone_count, state.map.animation_count, state.map.grid_blob_size, state.map.ppnt_blob_size,
         total_event_commands(&state.map), state.map.object_spawn_count);

  if (!load_map_wall_textures(&state.map, &wall_textures)) {
    fprintf(stderr, "No map texture screens decoded from map texture names\n");
    gloom_map_free(&state.map);
    return 1;
  }
  printf("Loaded %zu map texture screens from map names\n", wall_textures.loaded_count);

  if (!validate_map_wall_texture_references(&state.map, &wall_textures)) {
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  if (!load_flat_texture_set_for_map(resolved_map_path, &flat_textures)) {
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  if (!load_player_collision_radius(&state.player_radius)) {
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  if (!load_object_visual_set_for_map(&state.map, &object_visuals)) {
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  compute_map_bounds(&state);

  if (!initialize_camera_from_map_spawn(&state)) {
    fprintf(stderr, "No player spawn found in map event data\n");
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
  printf("Camera spawn: x=%.0f z=%.0f angle=%.3f\n", state.camera_x, state.camera_z, state.camera_angle);

  if (!load_grid_offset_set(&grid_offsets)) {
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  window = SDL_CreateWindow("Gloom PC Port", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, render_width * 4,
                            render_height * 4, SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    SDL_Quit();
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (renderer == NULL) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (renderer == NULL) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  (void)SDL_RenderSetIntegerScale(renderer, classic_viewport ? SDL_TRUE : SDL_FALSE);
  (void)SDL_RenderSetLogicalSize(renderer, render_width, render_height);
  update_render_dimensions(renderer, classic_viewport, &render_width, &render_height);
  (void)SDL_SetRelativeMouseMode(SDL_TRUE);
  (void)SDL_ShowCursor(SDL_DISABLE);

  perf_frequency = SDL_GetPerformanceFrequency();
  perf_prev = SDL_GetPerformanceCounter();

  while (running) {
    SDL_Event event;
    uint64_t perf_now = SDL_GetPerformanceCounter();
    double elapsed = (double)(perf_now - perf_prev) / (double)perf_frequency;
    perf_prev = perf_now;

    if (elapsed > 0.25) {
      elapsed = 0.25;
    }

    accumulator += elapsed;
    title_timer += elapsed;

    while (SDL_PollEvent(&event) != 0) {
      if (event.type == SDL_QUIT) {
        running = false;
      }
      if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        running = false;
      }
      if (event.type == SDL_MOUSEMOTION) {
        mouse_dx_accum += event.motion.xrel;
      }
    }

    while (accumulator >= dt) {
      const uint8_t *keyboard = SDL_GetKeyboardState(NULL);
      int mouse_dx = mouse_dx_accum;
      mouse_dx_accum = 0;

      update(&state, keyboard, mouse_dx);
      accumulator -= dt;
    }

    if (title_timer >= 1.0) {
      char title[160];
      (void)snprintf(title, sizeof(title), "Gloom PC Port | x=%.0f z=%.0f angle=%.2f | zones=%zu", state.camera_x,
                     state.camera_z, state.camera_angle, state.map.zone_count);
      SDL_SetWindowTitle(window, title);
      title_timer = 0.0;
    }

    update_render_dimensions(renderer, classic_viewport, &render_width, &render_height);
    render(renderer, &state, &grid_offsets, &wall_textures, &flat_textures, &object_visuals, render_width,
           render_height);
  }

  free_grid_offset_set(&grid_offsets);
  free_object_visual_set(&object_visuals);
  free_flat_texture_set(&flat_textures);
  free_wall_texture_set(&wall_textures);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDL_Quit();
  gloom_map_free(&state.map);

  return 0;
}
