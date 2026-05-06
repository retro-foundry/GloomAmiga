#ifdef GLOOM_DOS_SDL3
#include "sdl_dos_compat.h"
#else
#include <SDL.h>
#endif

#if defined(GLOOM_ENABLE_MENU_MUSIC) && GLOOM_ENABLE_MENU_MUSIC && !defined(GLOOM_DOS_SDL3)
#include <SDL_mixer.h>
#define GLOOM_USE_SDL_MIXER 1
#else
#define GLOOM_USE_SDL_MIXER 0
#endif

#if defined(GLOOM_DOS_SDL3) && defined(GLOOM_DOS_MENU_MUSIC)
#include <xmp.h>
#define GLOOM_USE_DOS_MENU_MUSIC 1
#else
#define GLOOM_USE_DOS_MENU_MUSIC 0
#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "iff.h"
#include "map.h"

#ifdef GLOOM_BINARY_VERSION_MAJOR
#define GLOOM_RUNTIME_IS_BINARY 1
#else
#define GLOOM_RUNTIME_IS_BINARY 0
#endif

#if GLOOM_RUNTIME_IS_BINARY || defined(__EMSCRIPTEN__)
#define GLOOM_RUNTIME_HAS_AUTOSAVE 1
#else
#define GLOOM_RUNTIME_HAS_AUTOSAVE 0
#endif

#ifdef GLOOM_DOS_SDL3
static void dos_logf(const char *fmt, ...) {
  FILE *file = fopen("GLOOM.LOG", "a");
  va_list args;

  if (file == NULL || fmt == NULL) {
    if (file != NULL) {
      fclose(file);
    }
    return;
  }

  va_start(args, fmt);
  vfprintf(file, fmt, args);
  va_end(args);
  fputc('\n', file);
  fclose(file);
}
#else
static void dos_logf(const char *fmt, ...) {
  (void)fmt;
}
#endif

static const char *runtime_title(void) {
#if GLOOM_RUNTIME_IS_BINARY
  static char title[32];

  if (title[0] == '\0') {
    (void)snprintf(title, sizeof(title), "Gloom %d.%d.%d", GLOOM_BINARY_VERSION_MAJOR,
                   GLOOM_BINARY_VERSION_MINOR, GLOOM_BINARY_VERSION_PATCH);
  }
  return title;
#else
  return "Gloom With Friends";
#endif
}

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#ifdef _WIN32
#include <windows.h>
#endif

enum {
  BASE_WIDTH = 320,
  BASE_HEIGHT = 224,
  DOS_LOWEST_HEIGHT = 200,
  WIDESCREEN_WIDTH = 400,
  DEFAULT_WINDOW_WIDTH = 1280,
  DEFAULT_WINDOW_HEIGHT = 720,
  GLOOM_AMIGA_GAME_TICK_HZ = 25,
#ifdef GLOOM_DOS_SDL3
  FIXED_TICK_HZ = 25,
  DOS_EVENT_POLL_INTERVAL_FRAMES = 1,
#else
  FIXED_TICK_HZ = 60,
#endif
  GLOOM_TEXTURE_SCREENS = 8,
  GLOOM_TEXTURES_PER_SCREEN = 20,
  GLOOM_TOTAL_WALL_TEXTURES = GLOOM_TEXTURE_SCREENS * GLOOM_TEXTURES_PER_SCREEN,
  GLOOM_TEXTURE_WIDTH = 64,
  GLOOM_TEXTURE_HEIGHT = 64,
  GLOOM_TEXTURE_COLUMN_BYTES = 65,
  GLOOM_MAX_ACTIVE_DOORS = 16,
  GLOOM_MAX_ACTIVE_ROTPOLYS = 32,
  GLOOM_MAX_ROTPOLY_ZONES = 32,
  GLOOM_LEVEL_EXIT_EVENT_ID = 24,
  GLOOM_LEVEL_COMPLETE_FINISHED = 3,
  GLOOM_COMBAT_ROUND_FINISHED = 4,
  GLOOM_AMIGA_FOCAL_SHIFT = 7,
  GLOOM_AMIGA_FOCAL = 1 << GLOOM_AMIGA_FOCAL_SHIFT,
  /* The PC renderer scales from its established 106x80 viewport reference; keep it
     separate from the Amiga internal focshft value above to avoid FOV drift. */
  GLOOM_PC_VIEW_FOCAL_SHIFT = 6,
  GLOOM_PC_VIEW_FOCAL = 1 << GLOOM_PC_VIEW_FOCAL_SHIFT,
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
  GLOOM_PLAYER_FRAME_SPEED_FIXED = 0x6000,
  GLOOM_PLAYER_BOB_HEIGHT = 20,
  GLOOM_PLAYER_INITIAL_HEALTH = 25,
  GLOOM_PLAYER_CONTACT_DAMAGE = 1,
  GLOOM_PLAYER_RED_PAL_TIMER = 2,
  GLOOM_PLAYER_DEATH_ROT_STEP = 4,
  GLOOM_PLAYER_DEATH_EYE_STEP = 4,
  GLOOM_PLAYER_DEATH_EYE_CLAMP = -32,
  GLOOM_PLAYER_DEAD_DELAY = 63,
  GLOOM_PLAYER_RESTART_INVINCIBLE_TICKS = 75,
  GLOOM_PLAYER_MEGA_OVERKILL = 875,
  GLOOM_MOUSE_LOOK_FIXED_PER_PIXEL = 0x2000,
  GLOOM_WEAPON_COUNT = 5,
  GLOOM_MAX_RUNTIME_PROJECTILES = 64,
  GLOOM_MAX_RUNTIME_SPARKS = 192,
  GLOOM_MAX_RUNTIME_BLOOD = 128,
  GLOOM_MAX_RUNTIME_CHUNKS = 256,
  GLOOM_MAX_RUNTIME_GORE = 128,
  GLOOM_MAX_RUNTIME_OBJECTS = 1024,
  GLOOM_PLAYER_INITIAL_RELOAD = 5,
  GLOOM_KEYBOARD_HELD_FIRE_REPEAT_TICKS = 5,
  GLOOM_PLAYER_FIRE_Y = -60,
  GLOOM_PROJECTILE_RADIUS = 32,
  GLOOM_PROJECTILE_BARREL_FORWARD = (GLOOM_PROJECTILE_RADIUS * 5) / 2,
  GLOOM_PLAYER_PROJECTILE_RENDER_MIN_DISTANCE = GLOOM_PROJECTILE_RADIUS * 3,
  GLOOM_PICKUP_RADIUS = 48,
  GLOOM_HUD_FONT_PLANE_COUNT = 7,
  GLOOM_HUD_WEAPON_FIRST_CHAR = 39,
  GLOOM_HUD_LIFE_CHAR = 44,
  GLOOM_HUD_HEALTH_CHAR = 45,
  GLOOM_HUD_CLEAR_CHAR = 46,
  GLOOM_HUD_PANEL_CHAR = 47,
  GLOOM_HUD_STATUS_HEIGHT = 13,
  GLOOM_HUD_WEAPON_X = 55,
  GLOOM_HUD_WEAPON_Y = 2,
  GLOOM_HUD_WEAPON_SPACING = 10,
  GLOOM_HUD_LIVES_X = 218,
  GLOOM_HUD_LIVES_Y = 2,
  GLOOM_HUD_LIVES_SPACING = 8,
  GLOOM_HUD_LIVES_RIGHT_ALIGN_COUNT = 6,
  GLOOM_HUD_HEALTH_X = 267,
  GLOOM_HUD_HEALTH_Y = 3,
  GLOOM_HUD_HEALTH_SPACING = 2,
  GLOOM_GUN_IDLE_FRAME = 0,
  GLOOM_GUN_RECOIL_FRAME = 1,
  GLOOM_GUN_MUZZLE_FIRST_FRAME = 2,
  GLOOM_GUN_MUZZLE_FRAME_COUNT = 3,
  GLOOM_GUN_FLASH_AMIGA_TICKS = 4,
  GLOOM_GUN_LOWER_OFFSET = 30,
  GLOOM_GUN_MUZZLE_LOWER_OFFSET = 0,
  GLOOM_GUN_RECOIL_BACK_OFFSET = 7,
  GLOOM_GUN_SIDE_BOB = 6,
  GLOOM_GUN_SIDE_LIFT = 4,
  GLOOM_PLAYER_INITIAL_LIVES = 3,
  GLOOM_AUDIO_CHANNEL_COUNT = 4,
  GLOOM_PAULA_PAL_CLOCK_HZ = 3546895,
  GLOOM_UI_BEEP_HZ = 880,
  GLOOM_UI_BEEP_MS = 45,
  GLOOM_UI_SELECT_BEEP_HZ = 1320,
  GLOOM_UI_SELECT_BEEP_MS = 55,
  GLOOM_MENU_SELECT_PAUSE_MS = 120,
  GLOOM_MENU_BIG_FONT_WIDTH = 8,
  GLOOM_MENU_BIG_FONT_HEIGHT = 10,
  GLOOM_MENU_FLASH_TICKS = 13,
  GLOOM_GAMEPAD_COUNT = 2,
  GLOOM_GAMEPAD_MOVE_DEADZONE = 8000,
  GLOOM_GAMEPAD_LOOK_DEADZONE = 7000,
  GLOOM_GAMEPAD_MOVE_THRESHOLD = 7000,
  GLOOM_GAMEPAD_TRIGGER_THRESHOLD = 12000,
  GLOOM_GAMEPAD_LOOK_AXIS_TO_MOUSE_DIV = 1500,
  GLOOM_GAMEPAD_LOOK_MIN_DELTA_AXIS = 12000,
  GLOOM_WEBRTC_INPUT_LEFT = 1,
  GLOOM_WEBRTC_INPUT_RIGHT = 2,
  GLOOM_WEBRTC_INPUT_FORWARD = 4,
  GLOOM_WEBRTC_INPUT_BACK = 8,
  GLOOM_WEBRTC_INPUT_STRAFE_LEFT = 16,
  GLOOM_WEBRTC_INPUT_STRAFE_RIGHT = 32,
  GLOOM_WEBRTC_INPUT_FIRE = 64
};

enum {
  GLOOM_PLAYER_DEATH_NONE = 0,
  GLOOM_PLAYER_DEATH_FALLING,
  GLOOM_PLAYER_DEATH_DEAD_DELAY,
  GLOOM_PLAYER_DEATH_WAIT_RESTART,
  GLOOM_PLAYER_DEATH_INVINCIBLE,
  GLOOM_PLAYER_DEATH_OUT_OF_LIVES,
  GLOOM_PLAYER_DEATH_GAME_OVER
};

enum {
  GLOOM_SFX_SHOOT = 0,
  GLOOM_SFX_SHOOT2,
  GLOOM_SFX_SHOOT3,
  GLOOM_SFX_SHOOT4,
  GLOOM_SFX_SHOOT5,
  GLOOM_SFX_GRUNT,
  GLOOM_SFX_GRUNT2,
  GLOOM_SFX_GRUNT3,
  GLOOM_SFX_GRUNT4,
  GLOOM_SFX_TOKEN,
  GLOOM_SFX_DOOR,
  GLOOM_SFX_FOOTSTEP,
  GLOOM_SFX_DIE,
  GLOOM_SFX_SPLAT,
  GLOOM_SFX_TELEPORT,
  GLOOM_SFX_GHOUL,
  GLOOM_SFX_LIZARD,
  GLOOM_SFX_LIZHIT,
  GLOOM_SFX_TROLLMAD,
  GLOOM_SFX_TROLLHIT,
  GLOOM_SFX_ROBOT,
  GLOOM_SFX_ROBODIE,
  GLOOM_SFX_DRAGON,
  GLOOM_SFX_COUNT
};

typedef struct {
  const char *symbol;
  const char *path;
} SfxDefinition;

typedef struct {
  bool loaded;
  char source_path[256];
  uint16_t period;
  uint16_t length_words;
  uint32_t sample_count;
  double sample_rate;
  int8_t *samples;
} SfxSample;

typedef struct {
  bool active;
  bool queued;
  uint8_t queued_ticks;
  int sfx_id;
  int queued_sfx_id;
  int volume;
  int queued_volume;
  int priority;
  int queued_priority;
  uint8_t busy_passes;
  double position;
  double increment;
#ifdef GLOOM_DOS_SDL3
  uint32_t position_fp;
  uint32_t increment_fp;
#endif
} AudioChannel;

typedef struct {
  bool initialized;
  SDL_AudioDeviceID device;
#ifdef GLOOM_DOS_SDL3
  SDL_AudioStream *stream;
  uint8_t *mix_buffer;
  int mix_buffer_size;
  int dos_audio_chunk_size;
  int dos_audio_ring_bytes;
  int dos_audio_target_bytes;
#if GLOOM_USE_DOS_MENU_MUSIC
  xmp_context music_context;
  uint8_t *music_buffer;
  int music_buffer_size;
  int music_format_flags;
  bool music_loaded;
  bool music_playing;
#endif
#endif
  SDL_AudioSpec obtained;
  SfxSample samples[GLOOM_SFX_COUNT];
  AudioChannel channels[GLOOM_AUDIO_CHANNEL_COUNT];
  uint8_t last_grunt;
  uint32_t ui_beep_remaining;
  uint32_t ui_beep_total;
  double ui_beep_phase;
  double ui_beep_phase_step;
  float ui_beep_volume;
#ifdef GLOOM_DOS_SDL3
  uint32_t ui_beep_phase_fp;
  uint32_t ui_beep_phase_step_fp;
  int ui_beep_volume_s16;
#endif
} AudioSystem;

typedef struct {
  bool active;
  size_t zone_index;
  int16_t base_x1;
  int16_t base_z1;
  int16_t base_x2;
  int16_t base_z2;
  float frac;
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
  float rot;
  uint16_t flags;
  int16_t center_x;
  int16_t center_z;
  RuntimeRotPolyVertex vertices[GLOOM_MAX_ROTPOLY_ZONES];
} RuntimeRotPoly;

typedef struct {
  bool active;
  uint8_t weapon_index;
  bool barrel_origin;
  bool enemy;
  bool homing;
  uint8_t player_owner_index;
  uint8_t bounce_count;
  float x;
  float y;
  float z;
  float player_origin_x;
  float player_origin_z;
  float vx;
  float vz;
  float frame_phase;
  int16_t hitpoints;
  int16_t damage;
} RuntimeProjectile;

typedef struct {
  bool active;
  uint8_t weapon_index;
  float x;
  float y;
  float z;
  float vx;
  float vy;
  float vz;
  float lifetime;
  uint16_t frame_index;
} RuntimeSpark;

typedef struct {
  bool active;
  bool soul;
  float x;
  float y;
  float z;
  float vx;
  float vy;
  float vz;
  uint16_t color_mask;
} RuntimeBlood;

typedef struct {
  bool active;
  int16_t object_type;
  uint16_t frame_number;
  uint16_t scale;
  int16_t radius;
  float x;
  float y;
  float z;
  float vx;
  float vy;
  float vz;
} RuntimeChunk;

typedef struct {
  bool active;
  int16_t object_type;
  uint16_t frame_number;
  float x;
  float z;
} RuntimeGore;

typedef struct {
  bool active;
  bool enemy;
  float x;
  float y;
  float z;
  float vx;
  float vz;
  int16_t rotation;
  int16_t old_rotation;
  int16_t rot_speed;
  uint32_t frame_fixed;
  int32_t frame_speed_fixed;
  int16_t hitpoints;
  int16_t damage;
  int16_t radius;
  float move_speed;
  float delay;
  float delay2;
  float hurt_pause;
  float pause_delay;
  float bounce_phase;
  float render_y_offset;
  uint8_t logic_state;
  uint8_t old_logic_state;
  uint8_t contact_was_hit_player;
} RuntimeObjectState;

typedef struct {
  float camera_x;
  float camera_z;
  float camera_y;
  float camera_angle;
  int32_t player_rot_fixed;
  int32_t player_rotspeed;
  float player_bounce;
  uint32_t player_frame_fixed;
  float player_eye_y;
  int16_t player_hitpoints;
  int16_t player_lives;
  bool player_dead;
  uint8_t player_death_phase;
  float player_death_timer;
  uint8_t player_weapon;
  uint8_t player_reload;
  uint8_t player_bouncy_bullets;
  float player_reload_counter;
  float player_keyboard_fire_repeat_counter;
  float player_weapon_flash;
  float player_palette_timer;
  float player_mega_timer;
  float player_invisible_timer;
  float player_thermo_timer;
  float player_hyper_timer;
  bool player_last_fire;
  int16_t player_pixsize;
  int16_t player_pixsizeadd;
  float player_pixsize_accum;
  int16_t player_respawn_x;
  int16_t player_respawn_z;
  int16_t player_respawn_rotation;
} RuntimePlayerState;

typedef struct {
  uint64_t tick_count;
  GloomMap map;
  uint8_t wall_texture_remap[GLOOM_TOTAL_WALL_TEXTURES];
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
  float player_bounce;
  uint32_t player_frame_fixed;
  float player_eye_y;
  int16_t player_hitpoints;
  int16_t player_lives;
  bool player_dead;
  uint8_t player_death_phase;
  float player_death_timer;
  uint8_t player_weapon;
  uint8_t player_reload;
  uint8_t player_bouncy_bullets;
  float player_reload_counter;
  float player_keyboard_fire_repeat_counter;
  float player_weapon_flash;
  float player_palette_timer;
  float player_mega_timer;
  float player_invisible_timer;
  float player_thermo_timer;
  float player_hyper_timer;
  bool player_last_fire;
  bool barrel_projectile_origin;
  int16_t player_radius;
  uint32_t rng_state;
  bool triggered_events[GLOOM_EVENT_COUNT + 1];
  bool teleport_active;
  uint8_t violence_mode;
  uint16_t gore_write_index;
  int16_t player_pixsize;
  int16_t player_pixsizeadd;
  float player_pixsize_accum;
  int16_t player_respawn_x;
  int16_t player_respawn_z;
  int16_t player_respawn_rotation;
  bool two_player_mode;
  bool combat_mode;
  uint8_t combat_series;
  int16_t combat_start_lives;
  uint8_t combat_maps[8];
  int16_t combat_maps_left;
  uint8_t active_player_index;
  int16_t active_other_player_lives;
  RuntimePlayerState player2;
  int16_t teleport_x;
  int16_t teleport_z;
  int16_t teleport_rotation;
  int16_t finished;
  int16_t finished2;
  bool level_transition_reported;
  bool lock_teleport_reported;
  RuntimeDoor doors[GLOOM_MAX_ACTIVE_DOORS];
  RuntimeRotPoly rotpolys[GLOOM_MAX_ACTIVE_ROTPOLYS];
  RuntimeProjectile projectiles[GLOOM_MAX_RUNTIME_PROJECTILES];
  RuntimeSpark sparks[GLOOM_MAX_RUNTIME_SPARKS];
  RuntimeBlood blood[GLOOM_MAX_RUNTIME_BLOOD];
  RuntimeChunk chunks[GLOOM_MAX_RUNTIME_CHUNKS];
  RuntimeGore gore[GLOOM_MAX_RUNTIME_GORE];
  RuntimeObjectState objects[GLOOM_MAX_RUNTIME_OBJECTS];
  float dragon_dead_delay;
  bool dragon_finished_reported;
  bool death_suck_active;
  uint16_t death_sucker_index;
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

typedef struct {
  SDL_Texture *texture;
#ifdef GLOOM_DOS_SDL3
  SDL_Palette *index_palette;
  uint8_t *index_pixels;
  uint8_t *index_coverage;
  uint8_t *last_frame_indices;
#endif
  uint32_t *pixels;
  uint32_t *last_frame_pixels;
  float *depth_buffer;
  float *sprite_depth_buffer;
#ifdef GLOOM_DOS_SDL3
  int32_t *depth_buffer_int;
  int32_t *sprite_depth_buffer_int;
#endif
  uint32_t *filled_stamps;
  uint32_t filled_stamp;
#ifdef GLOOM_DOS_SDL3
  int index_pitch_pixels;
#endif
  int pitch_pixels;
  int width;
  int height;
} RenderFramebuffer;

typedef struct {
  char picture[64];
  char text[192];
} ScriptCompletion;

typedef struct {
  char picture[64];
  char text[192];
} ScriptLevelIntro;

typedef enum {
  SCRIPT_PLAY_NEXT_ERROR = 0,
  SCRIPT_PLAY_NEXT_MAP = 1,
  SCRIPT_PLAY_NEXT_DONE = 2
} ScriptPlayNextResult;

typedef enum {
  PAUSE_MENU_QUIT = -1,
  PAUSE_MENU_CONTINUE = 0,
  PAUSE_MENU_MAIN_MENU = 1,
  PAUSE_MENU_LOAD_GAME = 2
} PauseMenuResult;

typedef enum {
  GLOOM_GAME_MODE_ONE_PLAYER = 0,
  GLOOM_GAME_MODE_TWO_PLAYER = 1,
  GLOOM_GAME_MODE_COMBAT = 2,
  GLOOM_GAME_MODE_LOAD_ONE_PLAYER = 3,
  GLOOM_GAME_MODE_LOAD_TWO_PLAYER = 4
} GloomGameMode;

typedef enum {
  GLOOM_CONTROL_KEYBOARD = 0,
  GLOOM_CONTROL_GAMEPAD_1 = 1,
  GLOOM_CONTROL_GAMEPAD_2 = 2
} GloomControlSource;

typedef struct {
  GloomControlSource player1;
  GloomControlSource player2;
} RuntimeControlConfig;

typedef struct {
  bool has_resolution;
  int resolution_width;
  int resolution_height;
  bool has_gameplay_viewport;
  int gameplay_viewport_width;
  int gameplay_viewport_height;
  int gameplay_pixel_width;
  int gameplay_pixel_height;
} RuntimeDisplayIniConfig;

typedef struct {
  bool active;
  int source_width;
  int source_height;
  int pixel_width;
  int pixel_height;
  int dst_x;
  int dst_y;
  int dst_width;
  int dst_height;
} RuntimeGameplayViewport;

static RuntimeGameplayViewport runtime_gameplay_viewport_for_render_target(int render_width, int render_height);

typedef struct {
  uint32_t *argb_pixels;
#ifdef GLOOM_DOS_SDL3
  uint8_t *indexed_pixels;
  uint8_t *coverage;
#endif
  int width;
  int height;
} HudGlyph;

typedef struct {
  bool loaded;
  char source_name[64];
  size_t glyph_count;
  HudGlyph *glyphs;
#ifdef GLOOM_DOS_SDL3
  uint32_t palette[256];
  uint8_t palette_used[256];
#endif
} HudFont;

typedef struct {
  bool loaded;
  uint32_t *pixels;
#ifdef GLOOM_DOS_SDL3
  uint8_t *indices;
  uint32_t palette[256];
  uint8_t palette_used[256];
#endif
  int width;
  int height;
} MenuImage;

typedef struct {
  MenuImage title;
  MenuImage black_magic_logo;
  HudFont big_font;
} MenuAssets;

enum {
  GLOOM_FLAT_TEXTURE_SIZE = 128,
  GLOOM_MAX_RENDER_ZONES = 2048,
  GLOOM_MAX_WALL_CANDIDATES = 2048,
  GLOOM_MAX_COLUMN_WALL_HITS = 128,
  GLOOM_MAX_DEBUG_SPRITES = 512,
  GLOOM_OBJECT_TYPE_COUNT = 24
};

enum {
  GLOOM_OBJECT_TYPE_DRAGON = 8,
  GLOOM_OBJECT_TYPE_BOUNCY = 9,
  GLOOM_OBJECT_TYPE_MARINE = 10,
  GLOOM_OBJECT_TYPE_BALDY = 11,
  GLOOM_OBJECT_TYPE_TERRA = 12,
  GLOOM_OBJECT_TYPE_GHOUL = 13,
  GLOOM_OBJECT_TYPE_PHANTOM = 14,
  GLOOM_OBJECT_TYPE_DEMON = 15,
  GLOOM_OBJECT_TYPE_WEAPON1 = 16,
  GLOOM_OBJECT_TYPE_WEAPON2 = 17,
  GLOOM_OBJECT_TYPE_WEAPON3 = 18,
  GLOOM_OBJECT_TYPE_WEAPON4 = 19,
  GLOOM_OBJECT_TYPE_WEAPON5 = 20,
  GLOOM_OBJECT_TYPE_LIZARD = 21,
  GLOOM_OBJECT_TYPE_DEATHHEAD = 22,
  GLOOM_OBJECT_TYPE_TROLL = 23
};

enum {
  GLOOM_OBJECT_LOGIC_NORMAL = 0,
  GLOOM_OBJECT_LOGIC_BALDYCHARGE = 1,
  GLOOM_OBJECT_LOGIC_BALDYPUNCH = 2,
  GLOOM_OBJECT_LOGIC_TERRAFIRE = 3,
  GLOOM_OBJECT_LOGIC_DEMONPAUSE = 4,
  GLOOM_OBJECT_LOGIC_DEATHCHARGE = 5,
  GLOOM_OBJECT_LOGIC_DEATHSUCK = 6
};

typedef struct {
  bool loaded;
  char source_name[64];
  uint32_t palette[256];
  uint32_t shaded_palette[16][256];
#ifdef GLOOM_DOS_SDL3
  uint8_t shaded_indices[16][256];
  uint8_t palette_used[256];
  uint8_t *column_texels;
  uint8_t *column_shaded_texels;
#endif
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
  uint32_t shaded_palette[16][256];
#ifdef GLOOM_DOS_SDL3
  uint8_t shaded_indices[16][256];
  uint8_t palette_used[256];
  uint8_t *shaded_texels;
#endif
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
#ifdef GLOOM_DOS_SDL3
  uint8_t *indexed_pixels;
#endif
} ObjectFrame;

typedef struct {
  bool loaded;
  bool eight_rotations;
  uint16_t scale;
  uint16_t fixed_frame;
  bool use_fixed_frame;
  uint16_t rotation_shift;
  uint16_t frame_count_per_rotation;
  uint16_t collision_radius;
  size_t frame_count;
  char source_name[64];
#ifdef GLOOM_DOS_SDL3
  uint32_t shaded_palette[16][256];
  uint8_t shaded_indices[16][256];
  uint8_t palette_used[256];
#endif
  ObjectFrame *frames;
} ObjectVisual;

typedef struct {
  const char *asset_name;
  bool eight_rotations;
  uint16_t scale;
  uint16_t fixed_frame;
  bool use_fixed_frame;
} ObjectVisualDefinition;

typedef struct ObjectVisualSet {
  ObjectVisual visuals[GLOOM_OBJECT_TYPE_COUNT];
  ObjectVisual chunks[GLOOM_OBJECT_TYPE_COUNT];
} ObjectVisualSet;

typedef struct {
  ObjectVisual bullets[GLOOM_WEAPON_COUNT];
  ObjectVisual sparks[GLOOM_WEAPON_COUNT];
  ObjectVisual gun;
} WeaponVisualSet;

typedef struct {
  const char *bullet_asset;
  const char *spark_asset;
  int16_t hitpoints;
  int16_t damage;
  int16_t speed;
  uint16_t spark_frame_count;
} WeaponDefinition;

enum {
  GLOOM_OBJECT_DIE_NONE = 0,
  GLOOM_OBJECT_DIE_BLOWOBJECT = 1,
  GLOOM_OBJECT_DIE_BLOWQUICK = 2,
  GLOOM_OBJECT_DIE_BLOWDRAGON = 3,
  GLOOM_OBJECT_DIE_BLOWDEATH = 4,
  GLOOM_OBJECT_DIE_BLOWTERRA = 5
};

enum {
  GLOOM_VIOLENCE_MEATY = 0,
  GLOOM_VIOLENCE_MESSY = 1,
  GLOOM_VIOLENCE_MEATY_MESSY = 2
};

typedef struct {
  int16_t hitpoints;
  int16_t damage;
  float move_speed;
  int16_t base_delay;
  int16_t delay_range;
  uint32_t initial_frame_fixed;
  uint32_t frame_speed_fixed;
  int16_t hurt_pause_ticks;
  int16_t guts_y;
  uint16_t blood_color_mask;
  uint8_t death_routine;
  uint8_t ranged_weapon;
  int16_t ranged_damage;
  float projectile_speed;
  bool uses_fire1;
} ObjectCombatDefinition;

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
  int column_min;
  int column_max;
} SceneWall;

typedef struct {
  const SceneWall *wall;
  float depth;
  float wall_u;
} RayWallHit;

#ifdef GLOOM_DOS_SDL3
typedef struct {
  const GloomZone *zone;
  int32_t vx1;
  int32_t vz1;
  int32_t vx2;
  int32_t vz2;
  int32_t sz;
  int column_min;
  int column_max;
} DosSceneWall;

typedef struct {
  const DosSceneWall *wall;
  int32_t depth;
  int32_t wall_u;
  bool fast_wall_column;
  bool transparent_wall_column;
  const uint8_t *wall_texels;
  const uint8_t *wall_shaded_texels;
  const uint8_t *wall_palette;
  size_t wall_texel_base;
} DosRayWallHit;
#endif

typedef struct {
  int16_t joyx;
  int16_t joyy;
  int16_t joys;
  int16_t turn;
  bool fire;
  bool keyboard_fire_repeat;
} PlayerControls;

typedef struct {
  float screen_x;
  float screen_y;
  float screen_w;
  float screen_h;
  float depth;
#ifdef GLOOM_DOS_SDL3
  int32_t screen_x_fixed;
  int32_t screen_y_fixed;
  int32_t screen_w_fixed;
  int32_t screen_h_fixed;
  int32_t depth_int;
#endif
  int source_pad;
  const ObjectFrame *frame;
#ifdef GLOOM_DOS_SDL3
  const uint8_t (*shaded_indices)[256];
#endif
} DebugSprite;

typedef struct {
  SDL_GameController *controller;
  SDL_JoystickID instance_id;
} RuntimeGamepad;

typedef struct {
  bool enabled;
  uint32_t target_frames;
  uint32_t frames;
  uint64_t update_steps;
  uint64_t wall_candidates;
  uint64_t sprites;
  double frame_ms;
  double events_ms;
  double update_ms;
  double interpolate_ms;
  double render_ms;
  double render_lock_clear_ms;
  double render_flat_ms;
  double render_wall_ms;
  double render_wall_search_ms;
  double render_wall_draw_ms;
  double render_sprites_ms;
  double render_weapon_ms;
  double render_hud_ms;
  double render_red_ms;
  double render_present_ms;
} RuntimeProfiler;

static float clampf(float value, float lo, float hi);
#ifdef GLOOM_DOS_SDL3
static int32_t dos_float_to_fixed16(float value);
static inline int dos_fixed16_floor_to_int(int32_t value);
static inline int32_t dos_div_fixed16(int64_t numerator_fixed, int32_t denominator_fixed);
static inline int32_t dos_fixed16_clamp_unit(int32_t value);
#endif
static int32_t amiga_rotation_to_fixed(int16_t rotation);
static float amiga_rotation_to_radians(int16_t rotation);
static int16_t player_rotation_fixed_to_amiga(int32_t rotation_fixed);
static float player_rotation_fixed_to_radians(int32_t rotation_fixed);
static void apply_mouse_look(AppState *state, int mouse_dx);
static void update_player_camera_y(AppState *state);
static void capture_primary_player_state(const AppState *state, RuntimePlayerState *out_player);
static void apply_primary_player_state(AppState *state, const RuntimePlayerState *player);
static bool player_can_take_damage(const AppState *state);
static void start_player_death(AppState *state);
static void update_player_death(AppState *state, const PlayerControls *controls);
static void spawn_runtime_gore(AppState *state, const RuntimeChunk *chunk);
static bool resolve_runtime_file_path(const char *input_path, char *out_path, size_t out_path_size);
static bool find_wall_collision_radius(const AppState *state, float x, float z, int16_t radius,
                                       uint16_t *out_penetration, size_t *out_zone_index);
static bool resolve_wall_collision_radius(AppState *state, float *x, float *z, int16_t radius);
static bool resolve_player_wall_collision(AppState *state, float *x, float *z);
static void runtime_object_dir(int16_t rotation, float *out_x, float *out_z);
static const ObjectVisualDefinition *object_visual_definitions(void);
static int16_t object_type_initial_hitpoints(int16_t object_type);
static int16_t object_type_damage(int16_t object_type);
static float object_type_move_speed(int16_t object_type);
static const ObjectCombatDefinition *object_type_combat_definition(int16_t object_type);
static bool object_type_is_enemy(int16_t object_type);
static bool object_type_is_pickup(int16_t object_type);
static bool object_type_is_player(int16_t object_type);
static int runtime_object_collides_player_index(const AppState *state, const RuntimeObjectState *object);
static void initialize_runtime_objects(AppState *state);
static uint16_t runtime_random_word(AppState *state);
static bool spawn_enemy_projectile_params(AppState *state, const RuntimeObjectState *object, uint8_t weapon_index,
                                          int16_t hitpoints, int16_t damage, float speed, float y_offset);
static bool spawn_enemy_projectile_params_ex(AppState *state, const RuntimeObjectState *object, uint8_t weapon_index,
                                             int16_t hitpoints, int16_t damage, float speed, float y_offset,
                                             bool homing);
static bool audio_load_sfx_bank(AudioSystem *audio);
static bool audio_start(AudioSystem *audio);
static void audio_shutdown(AudioSystem *audio);
#if GLOOM_USE_SDL_MIXER
static bool menu_music_load(void);
#endif
static void menu_music_start(void);
static void menu_music_stop(void);
static void audio_play_sfx(int sfx_id, int volume, int priority);
static void audio_vblank_tick(void);
static void audio_pause_output(AudioSystem *audio, bool paused, bool clear_channels);
#ifdef GLOOM_DOS_SDL3
extern int SDL_DOSSoundBlasterGetQueuedAudioSize(SDL_AudioDeviceID devid);
extern int SDL_DOSSoundBlasterGetAudioBufferSize(SDL_AudioDeviceID devid);
extern int SDL_DOSSoundBlasterGetAudioChunkSize(SDL_AudioDeviceID devid);
extern bool SDL_DOSSoundBlasterQueueAudio(SDL_AudioDeviceID devid, const void *buffer, int len);
extern void SDL_DOSSoundBlasterClearQueuedAudio(SDL_AudioDeviceID devid);
#endif
static void gamepad_init(void);
static void gamepad_shutdown(void);
static void gamepad_handle_device_added(int device_index);
static void gamepad_handle_device_removed(SDL_JoystickID instance_id);
static PlayerControls read_gamepad_player_controls(size_t gamepad_index, unsigned player_index);
static PlayerControls read_webrtc_player2_controls(void);
static int gamepad_look_delta(size_t gamepad_index);
static bool gamepad_button_down(size_t gamepad_index, SDL_GameControllerButton button);
static bool gamepad_menu_activate_event(const SDL_Event *event);
static bool gamepad_menu_back_event(const SDL_Event *event);
static void gamepad_handle_event(const SDL_Event *event);
static int control_source_gamepad_index(GloomControlSource source);
static const char *control_source_menu_name(GloomControlSource source);
static void normalize_control_config(RuntimeControlConfig *config);
static void cycle_control_config(RuntimeControlConfig *config, unsigned player_index);
static bool player1_receives_keyboard_input(void);
static bool set_runtime_mouse_capture(SDL_Window *window, bool captured);
static void runtime_force_cursor_visible(void);
void runtime_emscripten_install_pointer_lock_listener(void);
static bool runtime_emscripten_consume_pointer_lock_lost(void);
static void runtime_emscripten_discard_pointer_lock_lost(void);
void runtime_emscripten_canvas_cursor_default(void);
void runtime_emscripten_install_fullscreen_listeners(void);
void runtime_emscripten_canvas_fullscreen_toggle(void);
void runtime_emscripten_hide_loading_overlay(void);
static bool runtime_emscripten_consume_fullscreen_resize(void);
#ifndef GLOOM_DOS_SDL3
static void select_aspect_720p_resolution(int *io_width, int *io_height);
#endif
static void initialize_combat_rotation_state(AppState *state, uint8_t series, int16_t start_lives);
static bool resolve_next_combat_map(AppState *state, char *out_map_path, size_t out_map_path_size);
static void initialize_wall_texture_remap(AppState *state);
static void update_wall_animations(AppState *state);
static void update_rotpolys(AppState *state);
static void update_doors(AppState *state);
static void check_event_triggers(AppState *state);
static void update_with_controls(AppState *state, const PlayerControls *controls, const PlayerControls *controls2,
                                 const ObjectVisualSet *object_visuals);

static AudioSystem g_audio;
#if GLOOM_USE_SDL_MIXER
static Mix_Music *g_menu_music;
static int g_menu_music_volume_percent = 100;
#endif
static RuntimeGamepad g_gamepads[GLOOM_GAMEPAD_COUNT];
static RuntimeControlConfig g_control_config = {GLOOM_CONTROL_KEYBOARD, GLOOM_CONTROL_GAMEPAD_1};
static bool g_webrtc_guest_connected = false;
static uint32_t g_webrtc_guest_player2_input = 0u;
static bool g_runtime_mouse_capture_active = false;
#ifdef GLOOM_DOS_SDL3
static bool g_dos_mouse_absolute_capture = false;
static bool g_dos_mouse_absolute_valid = false;
static float g_dos_mouse_absolute_x = 0.0f;
#endif
static RuntimeProfiler g_profiler;
static bool g_depth_tables_initialized = false;
static uint8_t g_depth_dark_indices[GLOOM_AMIGA_MAX_Z];
static uint8_t g_depth_subtract_values[GLOOM_AMIGA_MAX_Z];
#ifdef GLOOM_DOS_SDL3
static bool g_dos_index_palette_initialized = false;
static bool g_dos_index_palette_dirty = false;
static SDL_Color g_dos_index_palette_colors[256];
static uint32_t g_dos_index_palette_argb[256];
static uint8_t g_dos_rgb444_to_index[4096];
static SDL_Color g_dos_game_index_palette_colors[256];
static uint32_t g_dos_game_index_palette_argb[256];
static uint8_t g_dos_game_rgb444_to_index[4096];
static uint8_t g_dos_red_palette_indices[256];
static bool g_dos_menu_palette_active = false;
#endif

static uint64_t sim_ticks_to_amiga_ticks(uint64_t ticks) {
  return (ticks * (uint64_t)GLOOM_AMIGA_GAME_TICK_HZ) / (uint64_t)FIXED_TICK_HZ;
}

static uint32_t elapsed_amiga_ticks_for_sim_tick(uint64_t tick_count) {
  uint64_t current = 0u;
  uint64_t previous = 0u;

  if (tick_count == 0u) {
    return 0u;
  }

  current = sim_ticks_to_amiga_ticks(tick_count);
  previous = sim_ticks_to_amiga_ticks(tick_count - 1u);
  return (uint32_t)(current - previous);
}

static void initialize_wall_texture_remap(AppState *state) {
  size_t i = 0u;

  if (state == NULL) {
    return;
  }

  for (i = 0u; i < (size_t)GLOOM_TOTAL_WALL_TEXTURES; ++i) {
    state->wall_texture_remap[i] = (uint8_t)i;
  }
}

static uint8_t remap_wall_texture_index(const AppState *state, uint8_t texture_index) {
  if (state == NULL || texture_index >= (uint8_t)GLOOM_TOTAL_WALL_TEXTURES) {
    return texture_index;
  }

  return state->wall_texture_remap[texture_index];
}

static void advance_wall_animations_amiga_tick(AppState *state) {
  size_t anim_index = 0u;

  if (state == NULL || state->map.animations == NULL) {
    return;
  }

  for (anim_index = 0u; anim_index < state->map.animation_count; ++anim_index) {
    GloomAnim *anim = &state->map.animations[anim_index];
    uint16_t first_frame = anim->first_frame;
    uint16_t frame_count = anim->frame_count;
    int16_t current = (int16_t)(uint16_t)(anim->current - 1u);
    uint8_t first_texture = 0u;
    size_t frame_index = 0u;

    if (frame_count < 2u || first_frame >= (uint16_t)GLOOM_TOTAL_WALL_TEXTURES ||
        frame_count > (uint16_t)GLOOM_TOTAL_WALL_TEXTURES - first_frame) {
      continue;
    }

    anim->current = (uint16_t)current;
    if (current > 0) {
      continue;
    }

    anim->current = anim->delay;
    first_texture = state->wall_texture_remap[first_frame];
    for (frame_index = 0u; frame_index + 1u < (size_t)frame_count; ++frame_index) {
      state->wall_texture_remap[first_frame + frame_index] =
          state->wall_texture_remap[first_frame + frame_index + 1u];
    }
    state->wall_texture_remap[first_frame + (size_t)frame_count - 1u] = first_texture;
  }
}

static void update_wall_animations(AppState *state) {
  uint32_t tick_count = 0u;
  uint32_t i = 0u;

  if (state == NULL) {
    return;
  }

  tick_count = elapsed_amiga_ticks_for_sim_tick(state->tick_count);
  for (i = 0u; i < tick_count; ++i) {
    advance_wall_animations_amiga_tick(state);
  }
}

static void player_redpal(AppState *state) {
  if (state == NULL) {
    return;
  }

  state->player_palette_timer = (float)GLOOM_PLAYER_RED_PAL_TIMER;
}

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
  const GloomObjectSpawn *spawn2 = NULL;
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

  if (state->two_player_mode) {
    bool seen_first_player = false;

    for (i = 0; i < state->map.object_spawn_count; ++i) {
      const GloomObjectSpawn *candidate = &state->map.object_spawns[i];

      if (candidate->event_id == 1u && candidate->object_type == 1) {
        spawn2 = candidate;
        break;
      }
    }

    for (i = 0; i < state->map.object_spawn_count; ++i) {
      const GloomObjectSpawn *candidate = &state->map.object_spawns[i];

      if (spawn2 != NULL) {
        break;
      }
      if (candidate->object_type != 0) {
        continue;
      }
      if (candidate == spawn) {
        seen_first_player = true;
        continue;
      }
      if (candidate->event_id == 1u || seen_first_player) {
        spawn2 = candidate;
        break;
      }
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
  if (spawn2 == NULL) {
    spawn2 = spawn;
  }

  initialize_wall_texture_remap(state);
  state->camera_x = (float)spawn->x;
  state->camera_z = (float)spawn->z;
  state->player_respawn_x = spawn->x;
  state->player_respawn_z = spawn->z;
  state->player_respawn_rotation = spawn->rotation;
  state->player_rot_fixed = amiga_rotation_to_fixed(spawn->rotation);
  state->player_rotspeed = 0;
  state->player_bounce = 0;
  state->player_frame_fixed = 0u;
  state->player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  state->player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state->player_lives = GLOOM_PLAYER_INITIAL_LIVES;
  state->player_dead = false;
  state->player_death_phase = GLOOM_PLAYER_DEATH_NONE;
  state->player_death_timer = 0.0f;
  state->player_weapon = 0u;
  state->player_reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
  state->player_bouncy_bullets = 0u;
  state->player_reload_counter = 0.0f;
  state->player_weapon_flash = 0.0f;
  state->player_palette_timer = 0.0f;
  state->player_mega_timer = 0.0f;
  state->player_invisible_timer = 0.0f;
  state->player_thermo_timer = 0.0f;
  state->player_hyper_timer = 0.0f;
  state->player_last_fire = false;
  state->teleport_active = false;
  state->player_pixsize = 0;
  state->player_pixsizeadd = 0;
  state->player_pixsize_accum = 0.0f;
  state->finished = 0;
  state->finished2 = 0;
  state->level_transition_reported = false;
  state->lock_teleport_reported = false;
  if (state->combat_mode && state->combat_start_lives > 0) {
    state->player_lives = state->combat_start_lives;
  }
  state->rng_state = 0x47524F4Fu ^ ((uint32_t)(uint16_t)spawn->x << 16u) ^ (uint32_t)(uint16_t)spawn->z;
  state->violence_mode = GLOOM_VIOLENCE_MEATY_MESSY;
  state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
  update_player_camera_y(state);
  memset(&state->player2, 0, sizeof(state->player2));
  state->player2.camera_x = (float)spawn2->x;
  state->player2.camera_z = (float)spawn2->z;
  state->player2.player_respawn_x = spawn2->x;
  state->player2.player_respawn_z = spawn2->z;
  state->player2.player_respawn_rotation = spawn2->rotation;
  state->player2.player_rot_fixed = amiga_rotation_to_fixed(spawn2->rotation);
  state->player2.camera_angle = player_rotation_fixed_to_radians(state->player2.player_rot_fixed);
  state->player2.player_frame_fixed = 0u;
  state->player2.player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  state->player2.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state->player2.player_lives = GLOOM_PLAYER_INITIAL_LIVES;
  if (state->combat_mode && state->combat_start_lives > 0) {
    state->player2.player_lives = state->combat_start_lives;
  }
  state->player2.player_reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
  state->player2.camera_y = state->player2.player_eye_y;
  return true;
}

static void initialize_runtime_objects(AppState *state) {
  size_t i = 0u;
  size_t count = 0u;
  const ObjectVisualDefinition *visual_definitions = object_visual_definitions();

  if (state == NULL) {
    return;
  }

  memset(state->objects, 0, sizeof(state->objects));
  count = state->map.object_spawn_count;
  if (count > (size_t)GLOOM_MAX_RUNTIME_OBJECTS) {
    count = (size_t)GLOOM_MAX_RUNTIME_OBJECTS;
  }

  for (i = 0u; i < count; ++i) {
    const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
    RuntimeObjectState *object = &state->objects[i];
    const ObjectCombatDefinition *combat = object_type_combat_definition(spawn->object_type);
    bool is_pickup = object_type_is_pickup(spawn->object_type);
    int16_t hitpoints = object_type_initial_hitpoints(spawn->object_type);
    uint16_t scale = 0x0200u;

    if (object_type_is_player(spawn->object_type)) {
      continue;
    }

    if (hitpoints <= 0 && !is_pickup) {
      continue;
    }

    if (spawn->object_type >= 0 && spawn->object_type < (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
      scale = visual_definitions[spawn->object_type].scale;
    }

    object->active = true;
    object->enemy = object_type_is_enemy(spawn->object_type);
    object->x = (float)spawn->x;
    object->y = (float)spawn->y;
    object->z = (float)spawn->z;
    object->rotation = spawn->rotation;
    object->old_rotation = spawn->rotation;
    if (spawn->object_type == GLOOM_OBJECT_TYPE_DRAGON) {
      object->rot_speed = -1;
    } else if (spawn->object_type != GLOOM_OBJECT_TYPE_DEATHHEAD) {
      object->rot_speed = 3;
    }
    if (combat != NULL) {
      object->frame_fixed = combat->initial_frame_fixed;
      object->frame_speed_fixed = (int32_t)combat->frame_speed_fixed;
    }
    object->hitpoints = hitpoints;
    object->damage = object_type_damage(spawn->object_type);
    object->radius = (int16_t)((uint32_t)48u * (uint32_t)scale / 0x0200u);
    if (object->radius < 24) {
      object->radius = 24;
    }
    object->move_speed = object_type_move_speed(spawn->object_type);
    if (combat != NULL && combat->delay_range > 0u) {
      object->delay = (float)combat->base_delay + (float)(runtime_random_word(state) % combat->delay_range);
    }
  }
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

static const SfxDefinition *sfx_definitions(void) {
  static const SfxDefinition definitions[GLOOM_SFX_COUNT] = {
      {"shootsfx", "amiga/sfxs/shoot.bin"},       {"shootsfx2", "amiga/sfxs/shoot2.bin"},
      {"shootsfx3", "amiga/sfxs/shoot3.bin"},     {"shootsfx4", "amiga/sfxs/shoot4.bin"},
      {"shootsfx5", "amiga/sfxs/shoot5.bin"},     {"gruntsfx", "amiga/sfxs/grunt.bin"},
      {"gruntsfx2", "amiga/sfxs/grunt2.bin"},     {"gruntsfx3", "amiga/sfxs/grunt3.bin"},
      {"gruntsfx4", "amiga/sfxs/grunt4.bin"},     {"tokensfx", "amiga/sfxs/token.bin"},
      {"doorsfx", "amiga/sfxs/door.bin"},         {"footstepsfx", "amiga/sfxs/footstep.bin"},
      {"diesfx", "amiga/sfxs/die.bin"},           {"splatsfx", "amiga/sfxs/splat.bin"},
      {"telesfx", "amiga/sfxs/teleport.bin"},     {"ghoulsfx", "amiga/sfxs/ghoul.bin"},
      {"lizsfx", "amiga/sfxs/lizard.bin"},        {"lizhitsfx", "amiga/sfxs/lizhit.bin"},
      {"trollsfx", "amiga/sfxs/trollmad.bin"},    {"trollhitsfx", "amiga/sfxs/trollhit.bin"},
      {"robotsfx", "amiga/sfxs/robot.bin"},       {"robodiesfx", "amiga/sfxs/robodie.bin"},
      {"dragonsfx", "amiga/sfxs/dragon.bin"},
  };

  return definitions;
}

static int weapon_sfx_id(uint8_t weapon_index) {
  static const int ids[GLOOM_WEAPON_COUNT] = {
      GLOOM_SFX_SHOOT3, GLOOM_SFX_SHOOT5, GLOOM_SFX_SHOOT, GLOOM_SFX_SHOOT4, GLOOM_SFX_SHOOT5,
  };

  if (weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
    return -1;
  }
  return ids[weapon_index];
}

static void audio_free_sfx_bank(AudioSystem *audio) {
  size_t i = 0u;

  if (audio == NULL) {
    return;
  }

  for (i = 0u; i < (size_t)GLOOM_SFX_COUNT; ++i) {
    free(audio->samples[i].samples);
    memset(&audio->samples[i], 0, sizeof(audio->samples[i]));
  }
}

static bool audio_load_one_sfx(AudioSystem *audio, int sfx_id, const SfxDefinition *definition) {
  uint8_t *data = NULL;
  size_t size = 0u;
  char resolved_path[1024] = {0};
  uint16_t period = 0u;
  uint16_t length_words = 0u;
  size_t expected_size = 0u;

  if (audio == NULL || definition == NULL || sfx_id < 0 || sfx_id >= GLOOM_SFX_COUNT) {
    return false;
  }

  if (!resolve_runtime_file_path(definition->path, resolved_path, sizeof(resolved_path))) {
    fprintf(stderr, "Cannot load original SFX %s: missing '%s'\n", definition->symbol, definition->path);
    return false;
  }

  if (!read_binary_blob(resolved_path, &data, &size)) {
    fprintf(stderr, "Cannot load original SFX %s: failed reading '%s'\n", definition->symbol, resolved_path);
    return false;
  }

  if (size < 6u) {
    fprintf(stderr, "Cannot load original SFX %s: '%s' is too short for Paula period/length/data\n",
            definition->symbol, resolved_path);
    free(data);
    return false;
  }

  period = main_read_be16(data);
  length_words = main_read_be16(data + 2u);
  expected_size = 4u + (size_t)length_words * 2u;
  if (period == 0u || length_words == 0u || expected_size != size) {
    fprintf(stderr,
            "Cannot load original SFX %s: '%s' header mismatch period=%u length_words=%u expected_bytes=%zu actual_bytes=%zu\n",
            definition->symbol, resolved_path, (unsigned)period, (unsigned)length_words, expected_size, size);
    free(data);
    return false;
  }

  audio->samples[sfx_id].samples = (int8_t *)malloc((size_t)length_words * 2u);
  if (audio->samples[sfx_id].samples == NULL) {
    fprintf(stderr, "Out of memory while loading original SFX %s from '%s'\n", definition->symbol, resolved_path);
    free(data);
    return false;
  }

  memcpy(audio->samples[sfx_id].samples, data + 4u, (size_t)length_words * 2u);
  (void)snprintf(audio->samples[sfx_id].source_path, sizeof(audio->samples[sfx_id].source_path), "%s", resolved_path);
  audio->samples[sfx_id].period = period;
  audio->samples[sfx_id].length_words = length_words;
  audio->samples[sfx_id].sample_count = (uint32_t)((size_t)length_words * 2u);
  audio->samples[sfx_id].sample_rate = (double)GLOOM_PAULA_PAL_CLOCK_HZ / (double)period;
  audio->samples[sfx_id].loaded = true;
  free(data);
  return true;
}

static bool audio_load_sfx_bank(AudioSystem *audio) {
  const SfxDefinition *definitions = sfx_definitions();
  size_t i = 0u;

  if (audio == NULL) {
    return false;
  }

  audio_free_sfx_bank(audio);
  for (i = 0u; i < (size_t)GLOOM_SFX_COUNT; ++i) {
    if (!audio_load_one_sfx(audio, (int)i, &definitions[i])) {
      audio_free_sfx_bank(audio);
      return false;
    }
  }
  return true;
}

static void audio_mix_sfx_float(AudioSystem *audio, Uint8 *stream, int len, bool clear_stream) {
  float *out = (float *)stream;
  int frames = len / (int)(sizeof(float) * 2u);
  int frame = 0;
  static const double pi = 3.14159265358979323846;

  if (clear_stream) {
    memset(stream, 0, (size_t)len);
  }
  if (audio == NULL) {
    return;
  }

  for (frame = 0; frame < frames; ++frame) {
    float left = clear_stream ? 0.0f : out[frame * 2];
    float right = clear_stream ? 0.0f : out[(frame * 2) + 1];
    int channel_index = 0;

    for (channel_index = 0; channel_index < GLOOM_AUDIO_CHANNEL_COUNT; ++channel_index) {
      AudioChannel *channel = &audio->channels[channel_index];
      const SfxSample *sample = NULL;
      int sample_index = 0;
      float value = 0.0f;
      float scaled = 0.0f;

      if (!channel->active || channel->sfx_id < 0 || channel->sfx_id >= GLOOM_SFX_COUNT) {
        continue;
      }

      sample = &audio->samples[channel->sfx_id];
      sample_index = (int)channel->position;
      if (!sample->loaded || sample->samples == NULL || sample_index < 0 ||
          sample_index >= (int)sample->sample_count) {
        channel->active = false;
        continue;
      }

      if (channel->busy_passes > 0u) {
        value = (float)sample->samples[sample_index] / 128.0f;
        scaled = value * ((float)channel->volume / 64.0f);
        if (channel_index == 1 || channel_index == 2) {
          right += scaled;
        } else {
          left += scaled;
        }
      }

      channel->position += channel->increment;
      if (channel->position >= (double)sample->sample_count) {
        channel->active = false;
      }
    }

    if (audio->ui_beep_remaining > 0u && audio->ui_beep_total > 0u) {
      float envelope = (float)audio->ui_beep_remaining / (float)audio->ui_beep_total;
      float value = SDL_sinf((float)audio->ui_beep_phase) * audio->ui_beep_volume * envelope;

      left += value;
      right += value;
      audio->ui_beep_phase += audio->ui_beep_phase_step;
      if (audio->ui_beep_phase >= pi * 2.0) {
        audio->ui_beep_phase -= pi * 2.0;
      }
      audio->ui_beep_remaining -= 1u;
    }

    out[frame * 2] = clampf(left, -1.0f, 1.0f);
    out[frame * 2 + 1] = clampf(right, -1.0f, 1.0f);
  }
}

static void audio_callback(void *userdata, Uint8 *stream, int len) {
  audio_mix_sfx_float((AudioSystem *)userdata, stream, len, true);
}

#if GLOOM_USE_SDL_MIXER
static void audio_mixer_postmix(void *userdata, Uint8 *stream, int len) {
  audio_mix_sfx_float((AudioSystem *)userdata, stream, len, false);
}
#endif

static bool audio_output_ready(const AudioSystem *audio) {
  if (audio == NULL || !audio->initialized) {
    return false;
  }
#ifdef GLOOM_DOS_SDL3
  return audio->stream != NULL;
#else
#if GLOOM_USE_SDL_MIXER
  return audio->obtained.freq > 0 && audio->obtained.channels == 2;
#else
  return audio->device != 0u;
#endif
#endif
}

static void audio_lock_output(AudioSystem *audio) {
  if (audio == NULL) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (audio->stream != NULL) {
    (void)SDL_LockAudioStream(audio->stream);
  }
#else
#if GLOOM_USE_SDL_MIXER
  SDL_LockAudio();
#else
  if (audio->device != 0u) {
    SDL_LockAudioDevice(audio->device);
  }
#endif
#endif
}

static void audio_unlock_output(AudioSystem *audio) {
  if (audio == NULL) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (audio->stream != NULL) {
    (void)SDL_UnlockAudioStream(audio->stream);
  }
#else
#if GLOOM_USE_SDL_MIXER
  SDL_UnlockAudio();
#else
  if (audio->device != 0u) {
    SDL_UnlockAudioDevice(audio->device);
  }
#endif
#endif
}

#ifdef GLOOM_DOS_SDL3
enum {
  GLOOM_DOS_AUDIO_TARGET_CHUNKS = 6
};

static int16_t audio_clamp_s16(int value) {
  if (value < -32768) {
    return -32768;
  }
  if (value > 32767) {
    return 32767;
  }
  return (int16_t)value;
}

static void audio_dos_callback_s16(void *userdata, Uint8 *stream, int len) {
  AudioSystem *audio = (AudioSystem *)userdata;
  int16_t *out = (int16_t *)stream;
  int frames = len / (int)(sizeof(int16_t) * 2u);
  int frame = 0;

  memset(stream, 0, (size_t)len);
  if (audio == NULL) {
    return;
  }

  for (frame = 0; frame < frames; ++frame) {
    int left = 0;
    int right = 0;
    int channel_index = 0;

    for (channel_index = 0; channel_index < GLOOM_AUDIO_CHANNEL_COUNT; ++channel_index) {
      AudioChannel *channel = &audio->channels[channel_index];
      const SfxSample *sample = NULL;
      uint32_t sample_index = 0u;
      int scaled = 0;

      if (!channel->active || channel->sfx_id < 0 || channel->sfx_id >= GLOOM_SFX_COUNT) {
        continue;
      }

      sample = &audio->samples[channel->sfx_id];
      sample_index = channel->position_fp >> 16;
      if (!sample->loaded || sample->samples == NULL || sample_index >= sample->sample_count) {
        channel->active = false;
        continue;
      }

      if (channel->busy_passes > 0u) {
        scaled = (int)sample->samples[sample_index] * channel->volume * 4;
        if (channel_index == 1 || channel_index == 2) {
          right += scaled;
        } else {
          left += scaled;
        }
      }

      channel->position_fp += channel->increment_fp;
      if ((channel->position_fp >> 16) >= sample->sample_count) {
        channel->active = false;
      }
    }

    if (audio->ui_beep_remaining > 0u && audio->ui_beep_total > 0u) {
      int value = (audio->ui_beep_phase_fp & 0x80000000u) != 0u ? -audio->ui_beep_volume_s16
                                                                : audio->ui_beep_volume_s16;
      value = (int)(((int64_t)value * (int64_t)audio->ui_beep_remaining) / (int64_t)audio->ui_beep_total);
      left += value;
      right += value;
      audio->ui_beep_phase_fp += audio->ui_beep_phase_step_fp;
      audio->ui_beep_remaining -= 1u;
    }

    out[frame * 2] = audio_clamp_s16(left);
    out[(frame * 2) + 1] = audio_clamp_s16(right);
  }
}

static uint8_t audio_clamp_u8(int value) {
  if (value < 0) {
    return 0u;
  }
  if (value > 255) {
    return 255u;
  }
  return (uint8_t)value;
}

static void audio_dos_callback_u8(void *userdata, Uint8 *stream, int len, int channels) {
  AudioSystem *audio = (AudioSystem *)userdata;
  int frames = channels > 0 ? len / channels : 0;
  int frame = 0;

  if (channels != 1 && channels != 2) {
    memset(stream, 128, (size_t)len);
    return;
  }
  memset(stream, 128, (size_t)len);
  if (audio == NULL) {
    return;
  }

  for (frame = 0; frame < frames; ++frame) {
    int left = 0;
    int right = 0;
    int channel_index = 0;

    for (channel_index = 0; channel_index < GLOOM_AUDIO_CHANNEL_COUNT; ++channel_index) {
      AudioChannel *channel = &audio->channels[channel_index];
      const SfxSample *sample = NULL;
      uint32_t sample_index = 0u;
      int scaled = 0;

      if (!channel->active || channel->sfx_id < 0 || channel->sfx_id >= GLOOM_SFX_COUNT) {
        continue;
      }

      sample = &audio->samples[channel->sfx_id];
      sample_index = channel->position_fp >> 16;
      if (!sample->loaded || sample->samples == NULL || sample_index >= sample->sample_count) {
        channel->active = false;
        continue;
      }

      if (channel->busy_passes > 0u) {
        scaled = ((int)sample->samples[sample_index] * channel->volume) / 64;
        if (channel_index == 1 || channel_index == 2) {
          right += scaled;
        } else {
          left += scaled;
        }
      }

      channel->position_fp += channel->increment_fp;
      if ((channel->position_fp >> 16) >= sample->sample_count) {
        channel->active = false;
      }
    }

    if (audio->ui_beep_remaining > 0u && audio->ui_beep_total > 0u) {
      int value = (audio->ui_beep_phase_fp & 0x80000000u) != 0u ? -audio->ui_beep_volume_s16
                                                                : audio->ui_beep_volume_s16;
      value = (int)(((int64_t)value * (int64_t)audio->ui_beep_remaining) / (int64_t)audio->ui_beep_total);
      value /= 256;
      left += value;
      right += value;
      audio->ui_beep_phase_fp += audio->ui_beep_phase_step_fp;
      audio->ui_beep_remaining -= 1u;
    }

    if (channels == 1) {
      stream[frame] = audio_clamp_u8(128 + ((left + right) / 2));
    } else {
      stream[frame * 2] = audio_clamp_u8(128 + left);
      stream[(frame * 2) + 1] = audio_clamp_u8(128 + right);
    }
  }
}

static bool audio_dos_has_active_output(const AudioSystem *audio) {
  int i = 0;

  if (audio == NULL) {
    return false;
  }
  if (audio->ui_beep_remaining > 0u) {
    return true;
  }
#if GLOOM_USE_DOS_MENU_MUSIC
  if (audio->music_playing) {
    return true;
  }
#endif
  for (i = 0; i < GLOOM_AUDIO_CHANNEL_COUNT; ++i) {
    if (audio->channels[i].active || audio->channels[i].queued) {
      return true;
    }
  }
  return false;
}

#if GLOOM_USE_DOS_MENU_MUSIC
static void audio_dos_mix_menu_music(AudioSystem *audio, uint8_t *stream, int len) {
  const int volume = 40;
  int rc = 0;
  int i = 0;

  if (audio == NULL || stream == NULL || len <= 0 || !audio->music_playing || audio->music_context == NULL) {
    return;
  }
  if (audio->music_buffer_size < len) {
    uint8_t *new_buffer = (uint8_t *)realloc(audio->music_buffer, (size_t)len);
    if (new_buffer == NULL) {
      audio->music_playing = false;
      dos_logf("DOS menu music: out of memory resizing mix buffer to %d bytes", len);
      return;
    }
    audio->music_buffer = new_buffer;
    audio->music_buffer_size = len;
  }
  rc = xmp_play_buffer(audio->music_context, audio->music_buffer, len, 0);
  if (rc != 0) {
    xmp_restart_module(audio->music_context);
    rc = xmp_play_buffer(audio->music_context, audio->music_buffer, len, 0);
    if (rc != 0) {
      audio->music_playing = false;
      dos_logf("DOS menu music: xmp_play_buffer failed rc=%d", rc);
      return;
    }
  }

  if (audio->obtained.format == SDL_AUDIO_U8) {
    for (i = 0; i < len; ++i) {
      int mixed = (int)stream[i] + ((((int)audio->music_buffer[i] - 128) * volume) / 64);
      stream[i] = audio_clamp_u8(mixed);
    }
  } else {
    int16_t *dst = (int16_t *)stream;
    const int16_t *src = (const int16_t *)audio->music_buffer;
    int samples = len / (int)sizeof(int16_t);

    for (i = 0; i < samples; ++i) {
      dst[i] = audio_clamp_s16((int)dst[i] + (((int)src[i] * volume) / 64));
    }
  }
}
#endif

static void audio_dos_mix_device_format(AudioSystem *audio, uint8_t *stream, int len) {
  if (audio == NULL || stream == NULL || len <= 0) {
    return;
  }
  if (audio->obtained.format == SDL_AUDIO_U8) {
    audio_dos_callback_u8(audio, stream, len, audio->obtained.channels);
  } else {
    audio_dos_callback_s16(audio, stream, len);
  }
#if GLOOM_USE_DOS_MENU_MUSIC
  audio_dos_mix_menu_music(audio, stream, len);
#endif
}

static void audio_dos_pump(AudioSystem *audio) {
  int queued = 0;
  int buffer_size = 0;
  int chunk_size = 0;

  if (audio == NULL || !audio->initialized || audio->device == 0u || audio->mix_buffer == NULL ||
      audio->mix_buffer_size <= 0 || !audio_dos_has_active_output(audio)) {
    return;
  }
  if (audio->stream != NULL && SDL_AudioStreamDevicePaused(audio->stream)) {
    return;
  }

  queued = SDL_DOSSoundBlasterGetQueuedAudioSize(audio->device);
  buffer_size = audio->dos_audio_target_bytes > 0 ? audio->dos_audio_target_bytes
                                                  : SDL_DOSSoundBlasterGetAudioBufferSize(audio->device);
  chunk_size = audio->dos_audio_chunk_size > 0 ? audio->dos_audio_chunk_size
                                               : SDL_DOSSoundBlasterGetAudioChunkSize(audio->device);
  if (queued < 0 || buffer_size <= 0 || chunk_size <= 0 || chunk_size > audio->mix_buffer_size) {
    return;
  }

  while (queued + chunk_size <= buffer_size && audio_dos_has_active_output(audio)) {
    audio_dos_mix_device_format(audio, audio->mix_buffer, chunk_size);
    if (!SDL_DOSSoundBlasterQueueAudio(audio->device, audio->mix_buffer, chunk_size)) {
      dos_logf("DOS Sound Blaster: manual queue failed: %s", SDL_GetError());
      break;
    }
    queued += chunk_size;
  }
}

static void audio_dos_stream_callback(void *userdata, SDL_AudioStream *stream, int additional_amount,
                                      int total_amount) {
  AudioSystem *audio = (AudioSystem *)userdata;
  int remaining = additional_amount;
  const int frame_bytes = (int)(sizeof(int16_t) * 2u);

  (void)total_amount;
  if (audio == NULL || stream == NULL || audio->mix_buffer == NULL || audio->mix_buffer_size < frame_bytes) {
    return;
  }

  while (remaining > 0) {
    int bytes = remaining;

    if (bytes > audio->mix_buffer_size) {
      bytes = audio->mix_buffer_size;
    }
    bytes -= bytes % frame_bytes;
    if (bytes <= 0) {
      break;
    }

    audio_dos_callback_s16(audio, audio->mix_buffer, bytes);
    if (!SDL_PutAudioStreamData(stream, audio->mix_buffer, bytes)) {
      dos_logf("DOS Sound Blaster: SDL_PutAudioStreamData failed: %s", SDL_GetError());
      break;
    }
    remaining -= bytes;
  }
}
#endif

static bool audio_start(AudioSystem *audio) {
#ifdef GLOOM_DOS_SDL3
  SDL_AudioSpec desired;
  int sample_frames = 0;

  if (audio == NULL) {
    return false;
  }

  memset(&desired, 0, sizeof(desired));
  desired.freq = 22050;
  desired.format = SDL_AUDIO_S16LE;
  desired.channels = 2;

  audio->mix_buffer_size = 8192;
  audio->mix_buffer = (uint8_t *)malloc((size_t)audio->mix_buffer_size);
  if (audio->mix_buffer == NULL) {
    fprintf(stderr, "Out of memory while preparing DOS Sound Blaster Paula mixer buffer\n");
    return false;
  }

  SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "soundblaster");
  SDL_SetHint("SDL_DOS_MANUAL_AUDIO_FREQ", "22050");
  SDL_SetHint("SDL_DOS_MANUAL_AUDIO_PUMP", "1");
  audio->stream =
      SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &desired, audio_dos_stream_callback, audio);
  if (audio->stream == NULL) {
    fprintf(stderr, "SDL_OpenAudioDeviceStream failed for DOS Sound Blaster Paula SFX: %s\n", SDL_GetError());
    free(audio->mix_buffer);
    audio->mix_buffer = NULL;
    audio->mix_buffer_size = 0;
    return false;
  }

  audio->device = SDL_GetAudioStreamDevice(audio->stream);
  if (!SDL_GetAudioDeviceFormat(audio->device, &audio->obtained, &sample_frames)) {
    fprintf(stderr, "SDL_GetAudioDeviceFormat failed for DOS Sound Blaster Paula SFX: %s\n", SDL_GetError());
    SDL_DestroyAudioStream(audio->stream);
    audio->stream = NULL;
    audio->device = 0u;
    free(audio->mix_buffer);
    audio->mix_buffer = NULL;
    audio->mix_buffer_size = 0;
    return false;
  }
  if (!((audio->obtained.format == SDL_AUDIO_S16LE && audio->obtained.channels == 2) ||
        (audio->obtained.format == SDL_AUDIO_U8 &&
         (audio->obtained.channels == 1 || audio->obtained.channels == 2)))) {
    fprintf(stderr, "DOS Sound Blaster returned unsupported Paula SFX format/channels\n");
    SDL_DestroyAudioStream(audio->stream);
    audio->stream = NULL;
    audio->device = 0u;
    free(audio->mix_buffer);
    audio->mix_buffer = NULL;
    audio->mix_buffer_size = 0;
    return false;
  }
  audio->dos_audio_chunk_size = SDL_DOSSoundBlasterGetAudioChunkSize(audio->device);
  audio->dos_audio_ring_bytes = SDL_DOSSoundBlasterGetAudioBufferSize(audio->device);
  if (audio->dos_audio_chunk_size <= 0 || audio->dos_audio_ring_bytes <= 0) {
    fprintf(stderr, "DOS Sound Blaster manual pump did not expose an audio ring buffer\n");
    SDL_DestroyAudioStream(audio->stream);
    audio->stream = NULL;
    audio->device = 0u;
    free(audio->mix_buffer);
    audio->mix_buffer = NULL;
    audio->mix_buffer_size = 0;
    return false;
  }
  audio->dos_audio_target_bytes = audio->dos_audio_chunk_size * GLOOM_DOS_AUDIO_TARGET_CHUNKS;
  if (audio->dos_audio_target_bytes > audio->dos_audio_ring_bytes) {
    audio->dos_audio_target_bytes = audio->dos_audio_ring_bytes;
  }
  if (audio->dos_audio_chunk_size > audio->mix_buffer_size) {
    uint8_t *new_buffer = (uint8_t *)realloc(audio->mix_buffer, (size_t)audio->dos_audio_chunk_size);
    if (new_buffer == NULL) {
      fprintf(stderr, "Out of memory while resizing DOS Sound Blaster Paula mixer buffer\n");
      SDL_DestroyAudioStream(audio->stream);
      audio->stream = NULL;
      audio->device = 0u;
      free(audio->mix_buffer);
      audio->mix_buffer = NULL;
      audio->mix_buffer_size = 0;
      return false;
    }
    audio->mix_buffer = new_buffer;
    audio->mix_buffer_size = audio->dos_audio_chunk_size;
  }
  audio->initialized = true;
  dos_logf("DOS Sound Blaster: opened manual Paula SFX pump at %d Hz format=0x%x channels=%d sample_frames=%d "
           "chunk=%d ring=%d target=%d device=%lu",
           audio->obtained.freq, (unsigned)audio->obtained.format, audio->obtained.channels, sample_frames,
           audio->dos_audio_chunk_size, audio->dos_audio_ring_bytes, audio->dos_audio_target_bytes,
           (unsigned long)audio->device);
  if (!SDL_ResumeAudioStreamDevice(audio->stream)) {
    fprintf(stderr, "SDL_ResumeAudioStreamDevice failed for DOS Sound Blaster Paula SFX: %s\n", SDL_GetError());
    SDL_DestroyAudioStream(audio->stream);
    audio->stream = NULL;
    audio->device = 0u;
    audio->initialized = false;
    free(audio->mix_buffer);
    audio->mix_buffer = NULL;
    audio->mix_buffer_size = 0;
    return false;
  }
  return true;
#else
  if (audio == NULL) {
    return false;
  }

#if GLOOM_USE_SDL_MIXER
  int mixer_freq = 0;
  Uint16 mixer_format = 0;
  int mixer_channels = 0;

  if ((Mix_Init(MIX_INIT_MOD) & MIX_INIT_MOD) == 0) {
    fprintf(stderr, "SDL_mixer MOD decoder unavailable for Amiga title music: %s\n", Mix_GetError());
    return false;
  }
  if (Mix_OpenAudio(48000, AUDIO_F32SYS, 2, 1024) != 0) {
    fprintf(stderr, "Mix_OpenAudio failed while opening shared menu music/Paula SFX output: %s\n", Mix_GetError());
    Mix_Quit();
    return false;
  }
  if (Mix_QuerySpec(&mixer_freq, &mixer_format, &mixer_channels) == 0 ||
      mixer_format != AUDIO_F32SYS || mixer_channels != 2) {
    fprintf(stderr, "SDL_mixer returned unsupported format/channels for shared menu music/Paula SFX output\n");
    Mix_CloseAudio();
    Mix_Quit();
    return false;
  }
  audio->obtained.freq = mixer_freq;
  audio->obtained.format = mixer_format;
  audio->obtained.channels = (Uint8)mixer_channels;
  Mix_SetPostMix(audio_mixer_postmix, audio);
  audio->initialized = true;
  if (!menu_music_load()) {
    audio->initialized = false;
    Mix_SetPostMix(NULL, NULL);
    Mix_CloseAudio();
    Mix_Quit();
    return false;
  }
  return true;
#else
  SDL_AudioSpec desired;

  memset(&desired, 0, sizeof(desired));
  desired.freq = 48000;
  desired.format = AUDIO_F32SYS;
  desired.channels = 2;
  desired.samples = 1024;
  desired.callback = audio_callback;
  desired.userdata = audio;

  audio->device = SDL_OpenAudioDevice(NULL, 0, &desired, &audio->obtained, 0);
  if (audio->device == 0u) {
    fprintf(stderr, "SDL_OpenAudioDevice failed while porting Paula SFX: %s\n", SDL_GetError());
    return false;
  }

  if (audio->obtained.format != AUDIO_F32SYS || audio->obtained.channels != 2) {
    fprintf(stderr, "SDL_OpenAudioDevice returned unsupported format/channels for Paula SFX port\n");
    SDL_CloseAudioDevice(audio->device);
    audio->device = 0u;
    return false;
  }

  audio->initialized = true;
  SDL_PauseAudioDevice(audio->device, 0);
  return true;
#endif
#endif
}

static void audio_shutdown(AudioSystem *audio) {
  if (audio == NULL) {
    return;
  }

#ifdef GLOOM_DOS_SDL3
  menu_music_stop();
#if GLOOM_USE_DOS_MENU_MUSIC
  if (audio->music_context != NULL) {
    if (audio->music_loaded) {
      xmp_end_player(audio->music_context);
    }
    if (audio->music_loaded) {
      xmp_release_module(audio->music_context);
    }
    xmp_free_context(audio->music_context);
    audio->music_context = NULL;
  }
  free(audio->music_buffer);
  audio->music_buffer = NULL;
  audio->music_buffer_size = 0;
  audio->music_format_flags = 0;
  audio->music_loaded = false;
  audio->music_playing = false;
#endif
  if (audio->stream != NULL) {
    SDL_DestroyAudioStream(audio->stream);
    audio->stream = NULL;
  }
  free(audio->mix_buffer);
  audio->mix_buffer = NULL;
  audio->mix_buffer_size = 0;
  audio->dos_audio_chunk_size = 0;
  audio->dos_audio_ring_bytes = 0;
  audio->dos_audio_target_bytes = 0;
  audio->device = 0u;
#else
#if GLOOM_USE_SDL_MIXER
  menu_music_stop();
  if (g_menu_music != NULL) {
    Mix_FreeMusic(g_menu_music);
    g_menu_music = NULL;
  }
  Mix_SetPostMix(NULL, NULL);
  Mix_CloseAudio();
  Mix_Quit();
#else
  if (audio->device != 0u) {
    SDL_CloseAudioDevice(audio->device);
    audio->device = 0u;
  }
#endif
#endif
  audio->initialized = false;
  audio_free_sfx_bank(audio);
}

#if GLOOM_USE_SDL_MIXER
static bool menu_music_load(void) {
  char path[512];

  if (g_menu_music != NULL) {
    return true;
  }
  /* Port of amiga/gloom2.s titlemed load/start path: the title MED is sfxs/med1. */
  if (!resolve_runtime_file_path("amiga/sfxs/med1", path, sizeof(path))) {
    fprintf(stderr, "Unable to find Amiga title music asset amiga/sfxs/med1\n");
    return false;
  }
  g_menu_music = Mix_LoadMUS(path);
  if (g_menu_music == NULL) {
    fprintf(stderr, "Mix_LoadMUS failed for Amiga title music %s: %s\n", path, Mix_GetError());
    return false;
  }
  Mix_VolumeMusic((MIX_MAX_VOLUME * g_menu_music_volume_percent) / 100);
  return true;
}
#endif

#if GLOOM_USE_DOS_MENU_MUSIC
static bool menu_music_load_dos(AudioSystem *audio) {
  char path[512];
  int flags = 0;

  if (audio == NULL || !audio_output_ready(audio)) {
    return false;
  }
  if (audio->music_loaded) {
    return true;
  }
  if (audio->music_context == NULL) {
    audio->music_context = xmp_create_context();
    if (audio->music_context == NULL) {
      dos_logf("DOS menu music: xmp_create_context failed");
      return false;
    }
  }
  /* Port of amiga/gloom2.s titlemed load/start path: the title MED is sfxs/med1. */
  if (!resolve_runtime_file_path("amiga/sfxs/med1", path, sizeof(path))) {
    dos_logf("DOS menu music: unable to find amiga/sfxs/med1");
    return false;
  }
  if (xmp_load_module(audio->music_context, path) != 0) {
    dos_logf("DOS menu music: xmp_load_module failed for %s", path);
    return false;
  }
  audio->music_loaded = true;
  if (audio->obtained.format == SDL_AUDIO_U8) {
    flags |= XMP_FORMAT_8BIT | XMP_FORMAT_UNSIGNED;
  }
  if (audio->obtained.channels == 1) {
    flags |= XMP_FORMAT_MONO;
  }
  audio->music_format_flags = flags;
  if (xmp_start_player(audio->music_context, audio->obtained.freq, flags) != 0) {
    dos_logf("DOS menu music: xmp_start_player failed at %d Hz flags=0x%x", audio->obtained.freq, flags);
    xmp_release_module(audio->music_context);
    audio->music_loaded = false;
    return false;
  }
  (void)xmp_set_player(audio->music_context, XMP_PLAYER_FLAGS, XMP_FLAGS_A500);
  (void)xmp_set_player(audio->music_context, XMP_PLAYER_INTERP, XMP_INTERP_NEAREST);
  (void)xmp_set_player(audio->music_context, XMP_PLAYER_VOLUME, 80);
  dos_logf("DOS menu music: started amiga/sfxs/med1 at %d Hz flags=0x%x", audio->obtained.freq, flags);
  return true;
}
#endif

static void menu_music_start(void) {
#if GLOOM_USE_SDL_MIXER
  if (!audio_output_ready(&g_audio) || !menu_music_load()) {
    return;
  }
  if (Mix_PlayingMusic() == 0) {
    if (Mix_PlayMusic(g_menu_music, -1) != 0) {
      fprintf(stderr, "Mix_PlayMusic failed for Amiga title music: %s\n", Mix_GetError());
    }
  } else if (Mix_PausedMusic() != 0) {
    Mix_ResumeMusic();
  }
#elif GLOOM_USE_DOS_MENU_MUSIC
  if (menu_music_load_dos(&g_audio)) {
    g_audio.music_playing = true;
    audio_dos_pump(&g_audio);
  }
#endif
}

static void menu_music_stop(void) {
#if GLOOM_USE_SDL_MIXER
  if (Mix_PlayingMusic() != 0 || Mix_PausedMusic() != 0) {
    Mix_HaltMusic();
  }
#elif GLOOM_USE_DOS_MENU_MUSIC
  if (g_audio.music_context != NULL && g_audio.music_playing) {
    xmp_restart_module(g_audio.music_context);
  }
  g_audio.music_playing = false;
#endif
}

static void audio_start_channel_locked(AudioSystem *audio, AudioChannel *channel, int sfx_id, int volume, int priority) {
  const SfxSample *sample = NULL;

  if (audio == NULL || channel == NULL || sfx_id < 0 || sfx_id >= GLOOM_SFX_COUNT) {
    return;
  }

  sample = &audio->samples[sfx_id];
  if (!sample->loaded || sample->samples == NULL || sample->sample_count == 0u) {
    return;
  }

  memset(channel, 0, sizeof(*channel));
  channel->active = true;
  channel->sfx_id = sfx_id;
  channel->volume = volume < 0 ? 0 : (volume > 64 ? 64 : volume);
  channel->priority = priority;
  channel->busy_passes = 1u;
  channel->position = 0.0;
  channel->increment = sample->sample_rate / (double)audio->obtained.freq;
#ifdef GLOOM_DOS_SDL3
  channel->position_fp = 0u;
  channel->increment_fp = (uint32_t)((sample->sample_rate * 65536.0 / (double)audio->obtained.freq) + 0.5);
  if (channel->increment_fp == 0u) {
    channel->increment_fp = 1u;
  }
#endif
}

static void audio_play_sfx(int sfx_id, int volume, int priority) {
  AudioSystem *audio = &g_audio;
  int i = 0;

#ifdef GLOOM_DOS_SDL3
  if (!audio->initialized || audio->stream == NULL || sfx_id < 0 || sfx_id >= GLOOM_SFX_COUNT) {
    return;
  }
#else
  if (!audio_output_ready(audio) || sfx_id < 0 || sfx_id >= GLOOM_SFX_COUNT) {
    return;
  }
#endif

  audio_lock_output(audio);
  for (i = 0; i < GLOOM_AUDIO_CHANNEL_COUNT; ++i) {
    AudioChannel *channel = &audio->channels[i];

    if (!channel->active && !channel->queued) {
      audio_start_channel_locked(audio, channel, sfx_id, volume, priority);
      audio_unlock_output(audio);
      return;
    }
  }

  for (i = 0; i < GLOOM_AUDIO_CHANNEL_COUNT; ++i) {
    AudioChannel *channel = &audio->channels[i];

    if (priority > channel->priority) {
      channel->active = false;
      channel->queued = true;
      channel->queued_ticks = 1u;
      channel->queued_sfx_id = sfx_id;
      channel->queued_volume = volume;
      channel->queued_priority = priority;
      audio_unlock_output(audio);
      return;
    }
  }
  audio_unlock_output(audio);
}

static void audio_play_ui_beep(int hz, int duration_ms, float volume) {
  AudioSystem *audio = &g_audio;
  uint32_t total = 0u;

#ifdef GLOOM_DOS_SDL3
  if (!audio->initialized || audio->stream == NULL || audio->obtained.freq <= 0) {
    return;
  }
#else
  if (!audio_output_ready(audio) || audio->obtained.freq <= 0) {
    return;
  }
#endif

  total = (uint32_t)((audio->obtained.freq * duration_ms) / 1000);
  if (total == 0u) {
    total = 1u;
  }

  audio_lock_output(audio);
  audio->ui_beep_total = total;
  audio->ui_beep_remaining = total;
  audio->ui_beep_phase = 0.0;
  audio->ui_beep_phase_step =
      (2.0 * 3.14159265358979323846 * (double)hz) / (double)audio->obtained.freq;
  audio->ui_beep_volume = volume;
#ifdef GLOOM_DOS_SDL3
  audio->ui_beep_phase_fp = 0u;
  audio->ui_beep_phase_step_fp =
      audio->obtained.freq > 0 ? (uint32_t)(((uint64_t)(unsigned)hz << 32) / (uint32_t)audio->obtained.freq) : 0u;
  audio->ui_beep_volume_s16 = (int)(volume * 32767.0f);
#endif
  audio_unlock_output(audio);
}

static void audio_play_ui_move(void) {
  audio_play_ui_beep(GLOOM_UI_BEEP_HZ, GLOOM_UI_BEEP_MS, 0.12f);
}

static void audio_play_ui_activate(void) {
  audio_play_ui_beep(GLOOM_UI_SELECT_BEEP_HZ, GLOOM_UI_SELECT_BEEP_MS, 0.22f);
}

static void audio_play_ui_back(void) {
  audio_play_ui_beep(GLOOM_UI_BEEP_HZ, GLOOM_UI_BEEP_MS, 0.12f);
}

static void menu_pause_after_selection(void) {
  uint32_t start_tick = SDL_GetTicks();

  while (SDL_GetTicks() - start_tick < (uint32_t)GLOOM_MENU_SELECT_PAUSE_MS) {
#ifdef GLOOM_DOS_SDL3
    audio_dos_pump(&g_audio);
#endif
    SDL_Delay(5);
  }
}

static void audio_vblank_tick(void) {
  AudioSystem *audio = &g_audio;
  int i = 0;

  if (!audio_output_ready(audio)) {
    return;
  }

  audio_lock_output(audio);
  for (i = 0; i < GLOOM_AUDIO_CHANNEL_COUNT; ++i) {
    AudioChannel *channel = &audio->channels[i];

    if (!channel->queued) {
      continue;
    }
    if (channel->queued_ticks > 0u) {
      channel->queued_ticks -= 1u;
    }
    if (channel->queued_ticks == 0u) {
      int sfx_id = channel->queued_sfx_id;
      int volume = channel->queued_volume;
      int priority = channel->queued_priority;

      channel->queued = false;
      audio_start_channel_locked(audio, channel, sfx_id, volume, priority);
    }
  }
  audio_unlock_output(audio);
}

static void audio_pause_output(AudioSystem *audio, bool paused, bool clear_channels) {
  if (audio == NULL || !audio->initialized) {
    return;
  }
  if (!audio_output_ready(audio)) {
    return;
  }

  if (clear_channels) {
    audio_lock_output(audio);
    memset(audio->channels, 0, sizeof(audio->channels));
    audio->ui_beep_remaining = 0u;
    audio->ui_beep_total = 0u;
    audio->ui_beep_phase = 0.0;
    audio->ui_beep_phase_step = 0.0;
    audio->ui_beep_volume = 0.0f;
#ifdef GLOOM_DOS_SDL3
    if (audio->stream != NULL) {
      audio_unlock_output(audio);
      (void)SDL_ClearAudioStream(audio->stream);
      SDL_DOSSoundBlasterClearQueuedAudio(audio->device);
    }
#else
    audio_unlock_output(audio);
#endif
  }
#ifdef GLOOM_DOS_SDL3
  if (audio->stream != NULL) {
    if (paused) {
      (void)SDL_PauseAudioStreamDevice(audio->stream);
    } else {
      (void)SDL_ResumeAudioStreamDevice(audio->stream);
    }
  }
#else
#if GLOOM_USE_SDL_MIXER
  (void)paused;
#else
  SDL_PauseAudioDevice(audio->device, paused ? 1 : 0);
#endif
#endif
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
  (void)snprintf(path1, sizeof(path1), "amiga/objs/%s", name);
  (void)snprintf(path2, sizeof(path2), "amiga/data/objs/%s", name);
  (void)snprintf(path3, sizeof(path3), "amiga/prog/objs/%s", name);
  (void)snprintf(path4, sizeof(path4), "objs/%s", name);
  (void)snprintf(path5, sizeof(path5), "data/objs/%s", name);
  (void)snprintf(path6, sizeof(path6), "amiga/%s", name);

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

static bool resolve_hud_font_path(char *out_path, size_t out_path_size) {
  const char *candidates[8] = {"amiga/misc/smallfont2.bin", "amiga/prog/misc/smallfont2.bin", "misc/smallfont2.bin",
                               "data/misc/smallfont2.bin",
#ifdef GLOOM_DOS_SDL3
                               "amiga/misc/smallf2.bin", "amiga/prog/misc/smallf2.bin", "misc/smallf2.bin",
                               "data/misc/smallf2.bin"
#else
                               NULL, NULL, NULL, NULL
#endif
  };
  size_t i = 0u;

  if (out_path == NULL || out_path_size == 0u) {
    return false;
  }

  for (i = 0u; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    if (candidates[i] == NULL) {
      continue;
    }
    if (resolve_runtime_file_path(candidates[i], out_path, out_path_size)) {
      return true;
    }
  }

  return false;
}

static bool resolve_menu_big_font_path(char *out_path, size_t out_path_size) {
  const char *candidates[4] = {"amiga/misc/bigfont2.bin", "amiga/prog/misc/bigfont.bin", "misc/bigfont2.bin",
                               "data/misc/bigfont2.bin"};
  size_t i = 0u;

  if (out_path == NULL || out_path_size == 0u) {
    return false;
  }

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
  const char *candidates[] = {
      "amiga/gridoffs4.bin",
#ifdef GLOOM_DOS_SDL3
      "amiga/gridoff4.bin",
#endif
  };
  uint8_t *data = NULL;
  size_t file_size = 0;
  size_t count = 0;
  size_t i = 0;
  size_t candidate_index = 0u;
  char resolved_path[1024] = {0};

  if (set == NULL) {
    return false;
  }

  free_grid_offset_set(set);

  for (candidate_index = 0u; candidate_index < sizeof(candidates) / sizeof(candidates[0]); ++candidate_index) {
    if (resolve_runtime_file_path(candidates[candidate_index], resolved_path, sizeof(resolved_path))) {
      break;
    }
  }

  if (candidate_index == sizeof(candidates) / sizeof(candidates[0])) {
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
#ifdef GLOOM_DOS_SDL3
    free(set->screens[i].column_texels);
    free(set->screens[i].column_shaded_texels);
#endif
  }

  memset(set, 0, sizeof(*set));
}

static bool clone_wall_texture_screen(const WallTextureScreen *source, WallTextureScreen *destination) {
  size_t texel_count = 0u;
  size_t flag_count = 0u;

  if (source == NULL || destination == NULL || !source->loaded || source->texels == NULL ||
      source->texture_count == 0u) {
    return false;
  }

  free(destination->texels);
  free(destination->column_flags);
#ifdef GLOOM_DOS_SDL3
  free(destination->column_texels);
  free(destination->column_shaded_texels);
#endif
  *destination = *source;
  destination->texels = NULL;
  destination->column_flags = NULL;
#ifdef GLOOM_DOS_SDL3
  destination->column_texels = NULL;
  destination->column_shaded_texels = NULL;
#endif

  texel_count = source->texture_count * (size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT;
  flag_count = source->texture_count * (size_t)GLOOM_TEXTURE_WIDTH;

  destination->texels = (uint8_t *)malloc(texel_count * sizeof(*destination->texels));
  if (destination->texels == NULL) {
    memset(destination, 0, sizeof(*destination));
    return false;
  }
  memcpy(destination->texels, source->texels, texel_count * sizeof(*destination->texels));

#ifdef GLOOM_DOS_SDL3
  if (source->column_texels != NULL) {
    destination->column_texels = (uint8_t *)malloc(texel_count * sizeof(*destination->column_texels));
    if (destination->column_texels == NULL) {
      free(destination->texels);
      memset(destination, 0, sizeof(*destination));
      return false;
    }
    memcpy(destination->column_texels, source->column_texels, texel_count * sizeof(*destination->column_texels));
  }
  if (source->column_shaded_texels != NULL) {
    size_t shaded_texel_count = 16u * texel_count;

    destination->column_shaded_texels =
        (uint8_t *)malloc(shaded_texel_count * sizeof(*destination->column_shaded_texels));
    if (destination->column_shaded_texels == NULL) {
      free(destination->texels);
      free(destination->column_texels);
      memset(destination, 0, sizeof(*destination));
      return false;
    }
    memcpy(destination->column_shaded_texels, source->column_shaded_texels,
           shaded_texel_count * sizeof(*destination->column_shaded_texels));
  }
#endif

  if (source->column_flags != NULL) {
    destination->column_flags = (uint8_t *)malloc(flag_count * sizeof(*destination->column_flags));
    if (destination->column_flags == NULL) {
      free(destination->texels);
#ifdef GLOOM_DOS_SDL3
      free(destination->column_texels);
      free(destination->column_shaded_texels);
#endif
      memset(destination, 0, sizeof(*destination));
      return false;
    }
    memcpy(destination->column_flags, source->column_flags, flag_count * sizeof(*destination->column_flags));
  }

  return true;
}

static void set_default_texture_palette(uint32_t palette[256]) {
  size_t i = 0;

  for (i = 0; i < 256u; ++i) {
    uint8_t v = (uint8_t)i;
    palette[i] = 0xFF000000u | ((uint32_t)v << 16u) | ((uint32_t)v << 8u) | (uint32_t)v;
  }
}

static size_t load_packed_palette_with_used(uint32_t palette[256], const uint8_t *data, size_t data_size,
                                            size_t palette_offset, uint8_t palette_used[256]) {
  size_t color_count = 0u;
  size_t entry_offset = palette_offset + 2u;
  size_t color_index = 1u;

  if (palette_used != NULL) {
    memset(palette_used, 0, 256u * sizeof(*palette_used));
    palette_used[0] = 1u;
  }

  if (palette == NULL || data == NULL || palette_offset + 2u > data_size) {
    return 1u;
  }

  color_count = (size_t)main_read_be16(data + palette_offset);
  while (color_index <= color_count && color_index < 256u && entry_offset + 1u < data_size) {
    uint16_t packed = main_read_be16(data + entry_offset);

    if (packed != 0xFFFFu) {
      uint8_t r = (uint8_t)(((packed >> 8) & 0x0Fu) * 17u);
      uint8_t g = (uint8_t)(((packed >> 4) & 0x0Fu) * 17u);
      uint8_t b = (uint8_t)((packed & 0x0Fu) * 17u);
      palette[color_index] = 0xFF000000u | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
      if (palette_used != NULL) {
        palette_used[color_index] = 1u;
      }
    }

    entry_offset += 2u;
    color_index += 1u;
  }

  return color_index > 0u ? color_index : 1u;
}

static void load_packed_palette(uint32_t palette[256], const uint8_t *data, size_t data_size, size_t palette_offset) {
  (void)load_packed_palette_with_used(palette, data, data_size, palette_offset, NULL);
}

static uint32_t shade_palette_argb(uint32_t argb, int subtract) {
  uint8_t alpha = (uint8_t)(argb >> 24);
  int r = (int)((argb >> 16) & 0xFFu) - subtract;
  int g = (int)((argb >> 8) & 0xFFu) - subtract;
  int b = (int)(argb & 0xFFu) - subtract;

  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  return ((uint32_t)alpha << 24u) | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
}

static void build_shaded_palette(uint32_t shaded_palette[16][256], const uint32_t palette[256]) {
  size_t shade = 0u;

  if (shaded_palette == NULL || palette == NULL) {
    return;
  }

  for (shade = 0u; shade < 16u; ++shade) {
    int subtract = (int)shade * 17;
    size_t i = 0u;

    for (i = 0u; i < 256u; ++i) {
      shaded_palette[shade][i] = shade_palette_argb(palette[i], subtract);
    }
  }
}

#ifdef GLOOM_DOS_SDL3
static uint32_t dos_opaque_argb(uint32_t argb) {
  return 0xFF000000u | (argb & 0x00FFFFFFu);
}

static void dos_set_index_palette_entry(size_t index, uint32_t argb) {
  uint8_t r = (uint8_t)((argb >> 16u) & 0xFFu);
  uint8_t g = (uint8_t)((argb >> 8u) & 0xFFu);
  uint8_t b = (uint8_t)(argb & 0xFFu);

  if (index >= 256u) {
    return;
  }

  g_dos_index_palette_colors[index].r = r;
  g_dos_index_palette_colors[index].g = g;
  g_dos_index_palette_colors[index].b = b;
  g_dos_index_palette_colors[index].a = 255u;
  g_dos_index_palette_argb[index] = 0xFF000000u | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
}

static uint16_t dos_argb_to_rgb444(uint32_t argb) {
  uint32_t r = (((argb >> 16u) & 0xFFu) + 8u) / 17u;
  uint32_t g = (((argb >> 8u) & 0xFFu) + 8u) / 17u;
  uint32_t b = ((argb & 0xFFu) + 8u) / 17u;

  if (r > 15u) r = 15u;
  if (g > 15u) g = 15u;
  if (b > 15u) b = 15u;
  return (uint16_t)((r << 8u) | (g << 4u) | b);
}

static void dos_rebuild_rgb444_to_nearest_palette(uint8_t remap[4096], const uint32_t palette[256],
                                                  const uint8_t palette_used[256]) {
  uint8_t used_indices[256];
  size_t used_count = 0u;
  size_t i = 0u;
  uint16_t rgb = 0u;

  if (remap == NULL || palette == NULL) {
    return;
  }

  for (i = 0u; i < 256u; ++i) {
    if (palette_used == NULL || palette_used[i]) {
      used_indices[used_count++] = (uint8_t)i;
    }
  }
  if (used_count == 0u) {
    used_indices[used_count++] = 0u;
  }

  for (rgb = 0u; rgb < 4096u; ++rgb) {
    int r = (int)(((rgb >> 8u) & 0xFu) * 17u);
    int g = (int)(((rgb >> 4u) & 0xFu) * 17u);
    int b = (int)((rgb & 0xFu) * 17u);
    int best_distance = 1 << 30;
    uint8_t best_index = used_indices[0];
    size_t used_index = 0u;

    for (used_index = 0u; used_index < used_count; ++used_index) {
      uint8_t candidate = used_indices[used_index];
      uint32_t argb = palette[candidate];
      int dr = r - (int)((argb >> 16u) & 0xFFu);
      int dg = g - (int)((argb >> 8u) & 0xFFu);
      int db = b - (int)(argb & 0xFFu);
      int distance = (dr * dr) + (dg * dg) + (db * db);

      if (distance < best_distance) {
        best_distance = distance;
        best_index = candidate;
        if (distance == 0) {
          break;
        }
      }
    }

    remap[rgb] = best_index;
  }
}

static void dos_fail_missing_original_palette_table(const char *path) {
  dos_logf("Missing original AGA palette/remap table %s required by amiga/gloom2.s calcpalettes", path);
  fprintf(stderr, "Missing original AGA palette/remap table %s required by amiga/gloom2.s calcpalettes\n", path);
  exit(1);
}

static bool dos_read_original_table_candidates(const char *const *candidates, size_t candidate_count,
                                               uint8_t **out_data, size_t *out_size) {
  size_t i = 0u;

  if (candidates == NULL || out_data == NULL || out_size == NULL) {
    return false;
  }

  for (i = 0u; i < candidate_count; ++i) {
    char resolved_path[1024] = {0};

    if (candidates[i] == NULL) {
      continue;
    }
    if (resolve_runtime_file_path(candidates[i], resolved_path, sizeof(resolved_path)) &&
        read_binary_blob(resolved_path, out_data, out_size)) {
      return true;
    }
  }

  return false;
}

static void initialize_dos_index_palette_tables(void) {
  const char *palette_candidates[4] = {"amiga/misc/PAL8.BIN", "misc/PAL8.BIN", "amiga/misc/palette_8",
                                       "misc/palette_8"};
  const char *remap_candidates[4] = {"amiga/misc/RMAP8.BIN", "misc/RMAP8.BIN", "amiga/misc/remap_8",
                                     "misc/remap_8"};
  uint8_t *palette_data = NULL;
  uint8_t *remap_data = NULL;
  size_t palette_size = 0u;
  size_t remap_size = 0u;
  size_t i = 0u;

  if (g_dos_index_palette_initialized) {
    return;
  }

  if (!dos_read_original_table_candidates(palette_candidates, sizeof(palette_candidates) / sizeof(palette_candidates[0]),
                                          &palette_data, &palette_size) ||
      palette_size < 1024u) {
    free(palette_data);
    dos_fail_missing_original_palette_table("amiga/misc/palette_8");
  }
  if (!dos_read_original_table_candidates(remap_candidates, sizeof(remap_candidates) / sizeof(remap_candidates[0]),
                                          &remap_data, &remap_size) ||
      remap_size < 4096u) {
    free(palette_data);
    free(remap_data);
    dos_fail_missing_original_palette_table("amiga/misc/remap_8");
  }

  for (i = 0u; i < 256u; ++i) {
    const uint8_t *entry = palette_data + (i * 4u);
    uint16_t hi = main_read_be16(entry);
    uint16_t lo = main_read_be16(entry + 2u);
    uint8_t r = (uint8_t)(((hi & 0x0F00u) >> 4u) | ((lo & 0x0F00u) >> 8u));
    uint8_t g = (uint8_t)((hi & 0x00F0u) | ((lo & 0x00F0u) >> 4u));
    uint8_t b = (uint8_t)(((hi & 0x000Fu) << 4u) | (lo & 0x000Fu));

    dos_set_index_palette_entry(i, 0xFF000000u | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b);
    g_dos_red_palette_indices[i] = 0u;
  }
  memcpy(g_dos_rgb444_to_index, remap_data, 4096u);
  memcpy(g_dos_game_index_palette_colors, g_dos_index_palette_colors, sizeof(g_dos_game_index_palette_colors));
  memcpy(g_dos_game_index_palette_argb, g_dos_index_palette_argb, sizeof(g_dos_game_index_palette_argb));
  memcpy(g_dos_game_rgb444_to_index, g_dos_rgb444_to_index, sizeof(g_dos_game_rgb444_to_index));

  g_dos_index_palette_dirty = true;
  g_dos_index_palette_initialized = true;
  free(palette_data);
  free(remap_data);
  dos_logf("DOS indexed framebuffer: loaded original AGA misc/palette_8 and misc/remap_8");
}

static void dos_restore_game_index_palette_tables(void) {
  initialize_dos_index_palette_tables();
  memcpy(g_dos_index_palette_colors, g_dos_game_index_palette_colors, sizeof(g_dos_index_palette_colors));
  memcpy(g_dos_index_palette_argb, g_dos_game_index_palette_argb, sizeof(g_dos_index_palette_argb));
  memcpy(g_dos_rgb444_to_index, g_dos_game_rgb444_to_index, sizeof(g_dos_rgb444_to_index));
  g_dos_index_palette_dirty = true;
  g_dos_menu_palette_active = false;
}

static uint8_t dos_argb_to_index(uint32_t argb) {
  if ((uint8_t)(argb >> 24u) == 0u) {
    return 0u;
  }
  initialize_dos_index_palette_tables();
  return g_dos_rgb444_to_index[dos_argb_to_rgb444(argb)];
}

static uint8_t dos_argb_to_opaque_index(uint32_t argb) {
  return dos_argb_to_index(dos_opaque_argb(argb));
}

static void dos_register_source_palette(const uint32_t palette[256], const uint8_t palette_used[256]) {
  (void)palette;
  (void)palette_used;
}

static uint32_t dos_original_shade_palette_argb(uint32_t argb, int shade) {
  int r = (int)(((argb >> 16u) & 0xFFu) / 17u) - shade;
  int g = (int)(((argb >> 8u) & 0xFFu) / 17u) - shade;
  int b = (int)((argb & 0xFFu) / 17u) - shade;

  if (r <= 0) r = 1;
  if (g < 0) g = 0;
  if (b < 0) b = 0;
  if (r > 15) r = 15;
  if (g > 15) g = 15;
  if (b > 15) b = 15;

  return 0xFF000000u | ((uint32_t)(r * 17) << 16u) | ((uint32_t)(g * 17) << 8u) | (uint32_t)(b * 17);
}

static void dos_rebuild_red_palette_indices(void) {
  size_t i = 0u;

  initialize_dos_index_palette_tables();
  g_dos_red_palette_indices[0] = 0u;
  for (i = 1u; i < 256u; ++i) {
    uint32_t argb = g_dos_index_palette_argb[i];
    int r = (int)((argb >> 16u) & 0xFFu);
    int g = (int)((argb >> 8u) & 0xFFu);
    int b = (int)(argb & 0xFFu);
    int red = (r + g + b + (16 * 17)) / 3;

    if (red > 255) {
      red = 255;
    }
    g_dos_red_palette_indices[i] = dos_argb_to_opaque_index(0xFF000000u | ((uint32_t)red << 16u));
  }
}

static bool attach_dos_index_palette(SDL_Texture *texture, SDL_Palette **out_palette) {
  SDL_Palette *palette = NULL;

  if (texture == NULL || out_palette == NULL) {
    return false;
  }

  initialize_dos_index_palette_tables();
  palette = SDL_CreatePalette(256);
  if (palette == NULL) {
    fprintf(stderr, "Cannot create DOS indexed framebuffer palette: %s\n", SDL_GetError());
    return false;
  }
  if (!SDL_SetPaletteColors(palette, g_dos_index_palette_colors, 0, 256)) {
    fprintf(stderr, "Cannot populate DOS indexed framebuffer palette: %s\n", SDL_GetError());
    SDL_DestroyPalette(palette);
    return false;
  }
  if (!SDL_SetTexturePalette(texture, palette)) {
    fprintf(stderr, "Cannot attach DOS indexed framebuffer palette: %s\n", SDL_GetError());
    SDL_DestroyPalette(palette);
    return false;
  }

  *out_palette = palette;
  return true;
}

static void apply_dos_index_palette_to_window(SDL_Window *window) {
  SDL_Surface *surface = NULL;
  SDL_Palette *palette = NULL;

  if (window == NULL) {
    return;
  }

  initialize_dos_index_palette_tables();
  surface = SDL_GetWindowSurface(window);
  if (surface == NULL) {
    dos_logf("DOS indexed framebuffer: SDL_GetWindowSurface failed: %s", SDL_GetError());
    return;
  }
  if (surface->format != SDL_PIXELFORMAT_INDEX8) {
    dos_logf("DOS indexed framebuffer: window surface format is 0x%lx, not INDEX8", (unsigned long)surface->format);
    return;
  }

  palette = SDL_GetSurfacePalette(surface);
  if (palette == NULL) {
    palette = SDL_CreateSurfacePalette(surface);
  }
  if (palette == NULL) {
    dos_logf("DOS indexed framebuffer: cannot get/create window surface palette: %s", SDL_GetError());
    return;
  }
  if (!SDL_SetPaletteColors(palette, g_dos_index_palette_colors, 0, 256)) {
    dos_logf("DOS indexed framebuffer: cannot set window surface palette: %s", SDL_GetError());
    return;
  }
  (void)SDL_SetWindowSurfaceVSync(window, 0);
}

static void apply_dos_current_index_palette(RenderFramebuffer *framebuffer, SDL_Window *window) {
  if (framebuffer != NULL && framebuffer->index_palette != NULL) {
    if (!SDL_SetPaletteColors(framebuffer->index_palette, g_dos_index_palette_colors, 0, 256)) {
      dos_logf("DOS indexed framebuffer: cannot refresh texture palette: %s", SDL_GetError());
    }
  }
  apply_dos_index_palette_to_window(window);
  g_dos_index_palette_dirty = false;
}

static void refresh_dos_index_palette(RenderFramebuffer *framebuffer, SDL_Window *window) {
  dos_restore_game_index_palette_tables();
  dos_rebuild_red_palette_indices();
  apply_dos_current_index_palette(framebuffer, window);
}

static void configure_dos_index_display_mode(SDL_Window *window, int render_width, int render_height) {
  SDL_DisplayID display_id = 0;
  SDL_DisplayMode **modes = NULL;
  const SDL_DisplayMode *best = NULL;
  int mode_count = 0;
  int i = 0;

  if (window == NULL || render_width <= 0 || render_height <= 0) {
    return;
  }

  display_id = SDL_GetDisplayForWindow(window);
  modes = SDL_GetFullscreenDisplayModes(display_id, &mode_count);
  if (modes == NULL || mode_count <= 0) {
    dos_logf("DOS indexed framebuffer: no fullscreen modes found: %s", SDL_GetError());
    SDL_free(modes);
    return;
  }

  for (i = 0; i < mode_count; ++i) {
    const SDL_DisplayMode *mode = modes[i];
    int mode_area = 0;
    int best_area = 0;

    if (mode == NULL || mode->format != SDL_PIXELFORMAT_INDEX8 || mode->w < render_width ||
        mode->h < render_height) {
      continue;
    }
    if (best == NULL) {
      best = mode;
      continue;
    }
    mode_area = mode->w * mode->h;
    best_area = best->w * best->h;
    if (mode_area < best_area || (mode_area == best_area && mode->w < best->w)) {
      best = mode;
    }
  }

  if (best == NULL) {
    for (i = 0; i < mode_count; ++i) {
      const SDL_DisplayMode *mode = modes[i];

      if (mode != NULL && mode->format == SDL_PIXELFORMAT_INDEX8) {
        best = mode;
        break;
      }
    }
  }

  if (best == NULL) {
    dos_logf("DOS indexed framebuffer: no INDEX8 fullscreen mode available");
    SDL_free(modes);
    return;
  }

  if (!SDL_SetWindowFullscreenMode(window, best)) {
    dos_logf("DOS indexed framebuffer: failed to select %dx%d INDEX8 mode: %s", best->w, best->h, SDL_GetError());
    SDL_free(modes);
    return;
  }
  dos_logf("DOS indexed framebuffer: selected %dx%d INDEX8 display mode", best->w, best->h);
  if (!SDL_SetWindowFullscreen(window, true)) {
    const char *error = SDL_GetError();

    if (error != NULL && error[0] != '\0') {
      dos_logf("DOS indexed framebuffer: fullscreen request returned false: %s", error);
    }
  }
  SDL_free(modes);
}

static void build_dos_shaded_palette_indices(uint8_t shaded_indices[16][256], const uint32_t palette[256],
                                             const uint8_t palette_used[256]) {
  size_t shade = 0u;

  if (shaded_indices == NULL || palette == NULL) {
    return;
  }

  dos_register_source_palette(palette, palette_used);
  for (shade = 0u; shade < 16u; ++shade) {
    size_t i = 0u;

    for (i = 0u; i < 256u; ++i) {
      if (palette_used != NULL && !palette_used[i]) {
        shaded_indices[shade][i] = 0u;
      } else {
        uint32_t argb = i == 0u ? 0xFFFFFFFFu : palette[i];

        shaded_indices[shade][i] = dos_argb_to_index(dos_original_shade_palette_argb(argb, (int)shade));
      }
    }
  }
  dos_rebuild_red_palette_indices();
}

static void rebuild_dos_object_visual_indices(ObjectVisual *visual) {
  if (visual == NULL || !visual->loaded) {
    return;
  }
  build_dos_shaded_palette_indices(visual->shaded_indices, visual->shaded_palette[0], visual->palette_used);
}

static bool rebuild_dos_flat_texture_indices(FlatTexture *texture) {
  enum { FLAT_TEXEL_BYTES = GLOOM_FLAT_TEXTURE_SIZE * GLOOM_FLAT_TEXTURE_SIZE };
  size_t shade = 0u;

  if (texture == NULL || !texture->loaded || texture->texels == NULL) {
    return true;
  }

  build_dos_shaded_palette_indices(texture->shaded_indices, texture->palette, texture->palette_used);
  if (texture->shaded_texels == NULL) {
    texture->shaded_texels = (uint8_t *)malloc((size_t)16u * (size_t)FLAT_TEXEL_BYTES);
    if (texture->shaded_texels == NULL) {
      return false;
    }
  }

  for (shade = 0u; shade < 16u; ++shade) {
    uint8_t *dst = texture->shaded_texels + (shade * (size_t)FLAT_TEXEL_BYTES);
    const uint8_t *palette = texture->shaded_indices[shade];
    size_t i = 0u;

    for (i = 0u; i < (size_t)FLAT_TEXEL_BYTES; ++i) {
      dst[i] = palette[texture->texels[i]];
    }
  }

  return true;
}

static bool rebuild_dos_wall_texture_screen_indices(WallTextureScreen *screen) {
  size_t texel_count = 0u;
  size_t shade = 0u;

  if (screen == NULL || !screen->loaded || screen->column_texels == NULL || screen->texture_count == 0u) {
    return true;
  }

  build_dos_shaded_palette_indices(screen->shaded_indices, screen->palette, screen->palette_used);
  texel_count = screen->texture_count * (size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT;
  if (screen->column_shaded_texels == NULL) {
    screen->column_shaded_texels = (uint8_t *)malloc(16u * texel_count * sizeof(*screen->column_shaded_texels));
    if (screen->column_shaded_texels == NULL) {
      return false;
    }
  }

  for (shade = 0u; shade < 16u; ++shade) {
    uint8_t *dst = screen->column_shaded_texels + (shade * texel_count);
    const uint8_t *palette = screen->shaded_indices[shade];
    size_t i = 0u;

    for (i = 0u; i < texel_count; ++i) {
      dst[i] = palette[screen->column_texels[i]];
    }
  }

  return true;
}

static void rebuild_dos_level_index_mappings(WallTextureSet *wall_textures, FlatTextureSet *flat_textures,
                                             ObjectVisualSet *object_visuals) {
  size_t i = 0u;

  if (wall_textures != NULL) {
    for (i = 0u; i < GLOOM_TEXTURE_SCREENS; ++i) {
      WallTextureScreen *screen = &wall_textures->screens[i];

      if (screen->loaded) {
        dos_register_source_palette(screen->palette, screen->palette_used);
      }
    }
  }
  if (flat_textures != NULL) {
    if (flat_textures->floor.loaded) {
      dos_register_source_palette(flat_textures->floor.palette, flat_textures->floor.palette_used);
    }
    if (flat_textures->roof.loaded) {
      dos_register_source_palette(flat_textures->roof.palette, flat_textures->roof.palette_used);
    }
  }
  if (object_visuals != NULL) {
    for (i = 0u; i < GLOOM_OBJECT_TYPE_COUNT; ++i) {
      if (object_visuals->visuals[i].loaded) {
        dos_register_source_palette(object_visuals->visuals[i].shaded_palette[0], object_visuals->visuals[i].palette_used);
      }
      if (object_visuals->chunks[i].loaded) {
        dos_register_source_palette(object_visuals->chunks[i].shaded_palette[0], object_visuals->chunks[i].palette_used);
      }
    }
  }

  if (wall_textures != NULL) {
    for (i = 0u; i < GLOOM_TEXTURE_SCREENS; ++i) {
      WallTextureScreen *screen = &wall_textures->screens[i];

      if (screen->loaded) {
        (void)rebuild_dos_wall_texture_screen_indices(screen);
      }
    }
  }
  if (flat_textures != NULL) {
    if (flat_textures->floor.loaded) {
      (void)rebuild_dos_flat_texture_indices(&flat_textures->floor);
    }
    if (flat_textures->roof.loaded) {
      (void)rebuild_dos_flat_texture_indices(&flat_textures->roof);
    }
  }
  if (object_visuals != NULL) {
    for (i = 0u; i < GLOOM_OBJECT_TYPE_COUNT; ++i) {
      rebuild_dos_object_visual_indices(&object_visuals->visuals[i]);
      rebuild_dos_object_visual_indices(&object_visuals->chunks[i]);
    }
  }
}

static void rebuild_dos_weapon_index_mappings(WeaponVisualSet *weapon_visuals) {
  size_t i = 0u;

  if (weapon_visuals == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_WEAPON_COUNT; ++i) {
    if (weapon_visuals->bullets[i].loaded) {
      dos_register_source_palette(weapon_visuals->bullets[i].shaded_palette[0], weapon_visuals->bullets[i].palette_used);
    }
    if (weapon_visuals->sparks[i].loaded) {
      dos_register_source_palette(weapon_visuals->sparks[i].shaded_palette[0], weapon_visuals->sparks[i].palette_used);
    }
  }
  if (weapon_visuals->gun.loaded) {
    dos_register_source_palette(weapon_visuals->gun.shaded_palette[0], weapon_visuals->gun.palette_used);
  }

  for (i = 0u; i < GLOOM_WEAPON_COUNT; ++i) {
    rebuild_dos_object_visual_indices(&weapon_visuals->bullets[i]);
    rebuild_dos_object_visual_indices(&weapon_visuals->sparks[i]);
  }
  rebuild_dos_object_visual_indices(&weapon_visuals->gun);
}

static void rebuild_dos_runtime_index_mappings(WallTextureSet *wall_textures, FlatTextureSet *flat_textures,
                                               ObjectVisualSet *object_visuals, WeaponVisualSet *weapon_visuals) {
  rebuild_dos_level_index_mappings(wall_textures, flat_textures, object_visuals);
  rebuild_dos_weapon_index_mappings(weapon_visuals);
  dos_logf("DOS indexed framebuffer: using original AGA misc/palette_8 and misc/remap_8 tables");
}

static void register_dos_hud_font_palette(const HudFont *font) {
  (void)font;
}

static void build_dos_menu_display_palette(const MenuImage *image, const MenuImage *extra_image, const HudFont *font,
                                           uint32_t palette[256], uint8_t palette_used[256]) {
  size_t i = 0u;
  size_t extra_slot = 128u;

  if (palette == NULL || palette_used == NULL) {
    return;
  }

  memset(palette_used, 0, 256u * sizeof(*palette_used));
  for (i = 0u; i < 256u; ++i) {
    palette[i] = 0xFF000000u;
  }
  if (image != NULL) {
    for (i = 0u; i < 256u; ++i) {
      if (image->palette_used[i]) {
        palette[i] = image->palette[i];
        palette_used[i] = 1u;
      }
    }
  }
  if (extra_image != NULL) {
    for (i = 0u; i < 256u && extra_slot < 256u; ++i) {
      if (!extra_image->palette_used[i]) {
        continue;
      }
      palette[extra_slot] = extra_image->palette[i];
      palette_used[extra_slot] = 1u;
      extra_slot += 1u;
    }
  }
  if (font != NULL) {
    for (i = 0u; i < 4u; ++i) {
      palette[i] = i == 0u ? 0xFF000000u : font->palette[i];
      palette_used[i] = 1u;
    }
  }
  palette_used[0] = 1u;
}

static void apply_dos_menu_display_palette(RenderFramebuffer *framebuffer, const MenuImage *image,
                                           const MenuImage *extra_image,
                                           const HudFont *font) {
  uint32_t palette[256];
  uint8_t palette_used[256];
  size_t i = 0u;

  if (framebuffer == NULL) {
    return;
  }

  initialize_dos_index_palette_tables();
  build_dos_menu_display_palette(image, extra_image, font, palette, palette_used);
  for (i = 0u; i < 256u; ++i) {
    dos_set_index_palette_entry(i, palette[i]);
  }
  dos_rebuild_rgb444_to_nearest_palette(g_dos_rgb444_to_index, g_dos_index_palette_argb, palette_used);
  g_dos_index_palette_dirty = true;
  g_dos_menu_palette_active = true;
  apply_dos_current_index_palette(framebuffer, gloom_dos_window);
}
#endif

static void free_hud_font(HudFont *font) {
  size_t i = 0u;

  if (font == NULL) {
    return;
  }

  for (i = 0u; i < font->glyph_count; ++i) {
    free(font->glyphs[i].argb_pixels);
#ifdef GLOOM_DOS_SDL3
    free(font->glyphs[i].indexed_pixels);
    free(font->glyphs[i].coverage);
#endif
  }

  free(font->glyphs);
  memset(font, 0, sizeof(*font));
}

static bool load_hud_panel_shapes(HudFont *font, const char *resolved_path, const uint8_t *data, size_t data_size) {
  uint32_t first_shape_offset = 0u;
  uint32_t palette_offset = 0u;
  size_t shape_count = 0u;
  HudGlyph *glyphs = NULL;
  uint32_t palette[256];
  size_t shape_index = 0u;

  if (font == NULL || resolved_path == NULL || data == NULL || data_size < 16u) {
    return false;
  }

  first_shape_offset = main_read_be32(data + 12u);
  palette_offset = main_read_be32(data + 8u);
  if (first_shape_offset < 16u || first_shape_offset > palette_offset || palette_offset >= data_size ||
      ((first_shape_offset - 12u) % 4u) != 0u) {
    fprintf(stderr, "HUD panel %s has invalid showstats shape table offsets\n", resolved_path);
    return false;
  }

  shape_count = (size_t)(first_shape_offset - 12u) / 4u;
  if (shape_count <= (size_t)GLOOM_HUD_PANEL_CHAR) {
    fprintf(stderr, "HUD panel %s is missing showstats/predrawall shapes 39-47\n", resolved_path);
    return false;
  }

  glyphs = (HudGlyph *)calloc(shape_count, sizeof(*glyphs));
  if (glyphs == NULL) {
    return false;
  }

  set_default_texture_palette(palette);
  load_packed_palette(palette, data, data_size, (size_t)palette_offset);

  for (shape_index = 0u; shape_index < shape_count; ++shape_index) {
    uint32_t shape_offset = main_read_be32(data + 12u + shape_index * 4u);
    uint32_t shape_limit = shape_index + 1u < shape_count ? main_read_be32(data + 12u + (shape_index + 1u) * 4u)
                                                          : palette_offset;
    int16_t width = 0;
    int16_t height = 0;
    uint32_t *pixels = NULL;
    int sx = 0;

    if ((size_t)shape_offset + 8u > (size_t)shape_limit || shape_limit > palette_offset || shape_limit > data_size) {
      fprintf(stderr, "HUD panel %s shape %zu is out of bounds\n", resolved_path, shape_index);
      goto fail;
    }

    width = (int16_t)main_read_be16(data + shape_offset + 4u);
    height = (int16_t)main_read_be16(data + shape_offset + 6u);
    if (width <= 0 || height <= 0 || (size_t)shape_offset + 8u + (size_t)width * (size_t)height > shape_limit) {
      fprintf(stderr, "HUD panel %s shape %zu has invalid chunky dimensions\n", resolved_path, shape_index);
      goto fail;
    }

    pixels = (uint32_t *)calloc((size_t)width * (size_t)height, sizeof(*pixels));
    if (pixels == NULL) {
      goto fail;
    }

    for (sx = 0; sx < width; ++sx) {
      int sy = 0;

      for (sy = 0; sy < height; ++sy) {
        uint8_t palette_index = data[(size_t)shape_offset + 8u + (size_t)sx * (size_t)height + (size_t)sy];

        pixels[(size_t)sy * (size_t)width + (size_t)sx] =
            palette_index == 0u && shape_index != (size_t)GLOOM_HUD_PANEL_CHAR ? 0x00000000u : palette[palette_index];
      }
    }

    glyphs[shape_index].argb_pixels = pixels;
    glyphs[shape_index].width = width;
    glyphs[shape_index].height = height;
  }

  font->loaded = true;
  font->glyph_count = shape_count;
  font->glyphs = glyphs;
  (void)snprintf(font->source_name, sizeof(font->source_name), "%s", resolved_path);
  printf("Loaded %zu HUD panel shapes from original showstats panel asset\n", shape_count);
  return true;

fail:
  for (shape_index = 0u; shape_index < shape_count; ++shape_index) {
    free(glyphs[shape_index].argb_pixels);
  }
  free(glyphs);
  return false;
}

static bool load_hud_font(HudFont *font) {
  char resolved_path[1024] = {0};
  uint8_t *data = NULL;
  size_t data_size = 0u;
  char error[256] = {0};
  uint32_t palette_offset = 0u;
  uint32_t first_glyph_offset = 0u;
  size_t glyph_count = 0u;
  HudGlyph *glyphs = NULL;
  uint32_t palette[256];
  size_t glyph_index = 0u;

  if (font == NULL) {
    return false;
  }

  free_hud_font(font);

  if (!resolve_hud_font_path(resolved_path, sizeof(resolved_path))) {
    fprintf(stderr, "Missing original HUD panel font amiga/misc/smallfont2.bin\n");
    return false;
  }

  if (!read_binary_blob(resolved_path, &data, &data_size)) {
    fprintf(stderr, "Failed to read HUD font %s\n", resolved_path);
    return false;
  }

  if (!gloom_decrunch_crm_buffer(&data, &data_size, error, sizeof(error))) {
    fprintf(stderr, "Failed to decrunch HUD font %s: %s\n", resolved_path, error[0] ? error : "unknown error");
    free(data);
    return false;
  }

  if (data_size < 12u) {
    fprintf(stderr, "HUD font %s is too small after decrunch (%zu bytes)\n", resolved_path, data_size);
    free(data);
    return false;
  }

  palette_offset = main_read_be32(data + 0u);
  first_glyph_offset = main_read_be32(data + 4u);
  if (first_glyph_offset < 8u || first_glyph_offset > palette_offset || palette_offset >= data_size ||
      ((first_glyph_offset - 4u) % 4u) != 0u) {
    bool panel_loaded = load_hud_panel_shapes(font, resolved_path, data, data_size);

    free(data);
    return panel_loaded;
  }

  glyph_count = (size_t)(first_glyph_offset - 4u) / 4u;
  if (glyph_count <= (size_t)GLOOM_HUD_PANEL_CHAR) {
    fprintf(stderr, "HUD font %s is missing showstats glyphs\n", resolved_path);
    free(data);
    return false;
  }

  glyphs = (HudGlyph *)calloc(glyph_count, sizeof(*glyphs));
  if (glyphs == NULL) {
    free(data);
    return false;
  }

  set_default_texture_palette(palette);
  load_packed_palette(palette, data, data_size, (size_t)palette_offset);

  for (glyph_index = 0u; glyph_index < glyph_count; ++glyph_index) {
    uint32_t glyph_offset = main_read_be32(data + 4u + glyph_index * 4u);
    uint32_t glyph_limit = glyph_index + 1u < glyph_count ? main_read_be32(data + 4u + (glyph_index + 1u) * 4u)
                                                          : palette_offset;
    uint32_t mask_offset = 0u;
    uint16_t blit_width_words = 0u;
    uint16_t blit_size = 0u;
    uint16_t blit_rows = 0u;
    uint16_t blit_words = 0u;
    size_t source_words = 0u;
    size_t row_stride = 0u;
    size_t source_size = 0u;
    int pixel_width = 0;
    int pixel_height = 0;
    uint32_t *pixels = NULL;
    int py = 0;

    if (glyph_offset + 10u > glyph_limit || glyph_limit > palette_offset || glyph_limit > data_size) {
      fprintf(stderr, "HUD font %s glyph %zu is out of bounds\n", resolved_path, glyph_index);
      goto fail;
    }

    mask_offset = main_read_be32(data + glyph_offset + 0u);
    blit_width_words = main_read_be16(data + glyph_offset + 4u);
    blit_size = main_read_be16(data + glyph_offset + 6u);
    blit_rows = (uint16_t)(blit_size >> 6u);
    blit_words = (uint16_t)(blit_size & 0x3Fu);

    if (blit_width_words == 0u || blit_words < 2u || blit_rows == 0u ||
        (blit_rows % GLOOM_HUD_FONT_PLANE_COUNT) != 0u) {
      fprintf(stderr, "HUD font %s glyph %zu has invalid blitter dimensions\n", resolved_path, glyph_index);
      goto fail;
    }

    source_words = (size_t)blit_words - 1u;
    row_stride = source_words * 2u;
    source_size = (size_t)blit_rows * row_stride;
    pixel_width = (int)(source_words * 16u);
    pixel_height = (int)((size_t)blit_rows / (size_t)GLOOM_HUD_FONT_PLANE_COUNT);

    if (glyph_offset + 8u + source_size > glyph_limit || mask_offset < 8u + source_size ||
        glyph_offset + mask_offset + source_size > glyph_limit || pixel_width <= 0 || pixel_height <= 0) {
      fprintf(stderr, "HUD font %s glyph %zu has invalid source/mask layout\n", resolved_path, glyph_index);
      goto fail;
    }

    pixels = (uint32_t *)malloc((size_t)pixel_width * (size_t)pixel_height * sizeof(*pixels));
    if (pixels == NULL) {
      goto fail;
    }

    for (py = 0; py < pixel_height; ++py) {
      int px = 0;

      for (px = 0; px < pixel_width; ++px) {
        uint8_t palette_index = 0u;
        bool covered = false;
        int plane = 0;
        size_t word_offset = (size_t)(px / 16) * 2u;
        unsigned bit_shift = (unsigned)(15 - (px % 16));

        for (plane = 0; plane < GLOOM_HUD_FONT_PLANE_COUNT; ++plane) {
          size_t row = (size_t)py * (size_t)GLOOM_HUD_FONT_PLANE_COUNT + (size_t)plane;
          size_t src_offset = (size_t)glyph_offset + 8u + row * row_stride + word_offset;
          size_t mask_src_offset = (size_t)glyph_offset + (size_t)mask_offset + row * row_stride + word_offset;
          uint16_t src_word = main_read_be16(data + src_offset);
          uint16_t mask_word = main_read_be16(data + mask_src_offset);

          if (((src_word >> bit_shift) & 1u) != 0u) {
            palette_index = (uint8_t)(palette_index | (uint8_t)(1u << (unsigned)plane));
          }
          if (((mask_word >> bit_shift) & 1u) != 0u) {
            covered = true;
          }
        }

        pixels[(size_t)py * (size_t)pixel_width + (size_t)px] = covered ? palette[palette_index] : 0x00000000u;
      }
    }

    glyphs[glyph_index].argb_pixels = pixels;
    glyphs[glyph_index].width = pixel_width;
    glyphs[glyph_index].height = pixel_height;
  }

  font->loaded = true;
  font->glyph_count = glyph_count;
  font->glyphs = glyphs;
  (void)snprintf(font->source_name, sizeof(font->source_name), "%s", resolved_path);

  free(data);
  printf("Loaded %zu HUD glyphs from original smallfont shape table\n", glyph_count);
  return true;

fail:
  for (glyph_index = 0u; glyph_index < glyph_count; ++glyph_index) {
    free(glyphs[glyph_index].argb_pixels);
  }
  free(glyphs);
  free(data);
  return false;
}

static bool load_menu_big_font(HudFont *font) {
  char resolved_path[1024] = {0};
  uint8_t *data = NULL;
  size_t data_size = 0u;
  char error[256] = {0};
  uint32_t palette_offset = 0u;
  uint32_t first_glyph_offset = 0u;
  size_t glyph_count = 0u;
  HudGlyph *glyphs = NULL;
  uint32_t palette[256];
#ifdef GLOOM_DOS_SDL3
  uint8_t palette_used[256];
#endif
  size_t glyph_index = 0u;

  if (font == NULL) {
    return false;
  }

  free_hud_font(font);

  dos_logf("DOS checkpoint: menu big font resolve begin");
  if (!resolve_menu_big_font_path(resolved_path, sizeof(resolved_path))) {
    fprintf(stderr, "Missing original menu font amiga/misc/bigfont2.bin\n");
    return false;
  }
  dos_logf("DOS checkpoint: menu big font resolved %s", resolved_path);
  if (!read_binary_blob(resolved_path, &data, &data_size)) {
    fprintf(stderr, "Failed to read menu font %s\n", resolved_path);
    return false;
  }
  dos_logf("DOS checkpoint: menu big font read %zu bytes", data_size);
  if (!gloom_decrunch_crm_buffer(&data, &data_size, error, sizeof(error))) {
    fprintf(stderr, "Failed to decrunch menu font %s: %s\n", resolved_path, error[0] ? error : "unknown error");
    free(data);
    return false;
  }
  dos_logf("DOS checkpoint: menu big font decrunched %zu bytes", data_size);
  if (data_size < 12u) {
    fprintf(stderr, "Menu font %s is too small after decrunch (%zu bytes)\n", resolved_path, data_size);
    free(data);
    return false;
  }

  palette_offset = main_read_be32(data + 0u);
  first_glyph_offset = main_read_be32(data + 4u);
  if (first_glyph_offset < 8u || first_glyph_offset > palette_offset || palette_offset >= data_size ||
      ((first_glyph_offset - 4u) % 4u) != 0u) {
    fprintf(stderr, "Menu font %s has invalid original font shape table\n", resolved_path);
    free(data);
    return false;
  }

  glyph_count = (size_t)(first_glyph_offset - 4u) / 4u;
  glyphs = (HudGlyph *)calloc(glyph_count, sizeof(*glyphs));
  if (glyphs == NULL) {
    free(data);
    return false;
  }

  set_default_texture_palette(palette);
#ifdef GLOOM_DOS_SDL3
  (void)load_packed_palette_with_used(palette, data, data_size, (size_t)palette_offset, palette_used);
#else
  load_packed_palette(palette, data, data_size, (size_t)palette_offset);
#endif

  for (glyph_index = 0u; glyph_index < glyph_count; ++glyph_index) {
    uint32_t glyph_offset = main_read_be32(data + 4u + glyph_index * 4u);
    uint32_t glyph_limit = glyph_index + 1u < glyph_count ? main_read_be32(data + 4u + (glyph_index + 1u) * 4u)
                                                          : palette_offset;
    uint32_t mask_offset = 0u;
    uint16_t blit_size = 0u;
    uint16_t plane_count = 0u;
    uint16_t blit_rows = 0u;
    uint16_t blit_words = 0u;
    size_t source_words = 0u;
    size_t row_stride = 0u;
    size_t source_size = 0u;
    int pixel_width = 0;
    int pixel_height = 0;
    uint32_t *pixels = NULL;
#ifdef GLOOM_DOS_SDL3
    uint8_t *indexed_pixels = NULL;
    uint8_t *coverage = NULL;
#endif
    int py = 0;

    if (glyph_offset + 8u > glyph_limit || glyph_limit > palette_offset || glyph_limit > data_size) {
      fprintf(stderr, "Menu font %s glyph %zu is out of bounds\n", resolved_path, glyph_index);
      goto fail;
    }

    mask_offset = main_read_be32(data + glyph_offset + 0u);
    blit_size = main_read_be16(data + glyph_offset + 6u);
    plane_count = main_read_be16(data + glyph_offset + 8u);
    blit_rows = (uint16_t)(blit_size >> 6u);
    blit_words = (uint16_t)(blit_size & 0x3Fu);
    if (blit_words < 2u || blit_rows == 0u || plane_count == 0u || plane_count > GLOOM_HUD_FONT_PLANE_COUNT) {
      fprintf(stderr, "Menu font %s glyph %zu has invalid blitter dimensions rows=%u words=%u planes=%u\n",
              resolved_path, glyph_index, (unsigned)blit_rows, (unsigned)blit_words, (unsigned)plane_count);
      goto fail;
    }

    source_words = (size_t)blit_words - 1u;
    row_stride = source_words * 2u;
    source_size = (size_t)plane_count * (size_t)blit_rows * row_stride;
    pixel_width = (int)(source_words * 16u);
    pixel_height = (int)blit_rows;
    if (glyph_offset + 10u + source_size > glyph_limit || mask_offset < 10u + source_size ||
        glyph_offset + mask_offset + source_size > glyph_limit || pixel_width <= 0 || pixel_height <= 0) {
      fprintf(stderr, "Menu font %s glyph %zu has invalid source/mask layout\n", resolved_path, glyph_index);
      goto fail;
    }

    pixels = (uint32_t *)malloc((size_t)pixel_width * (size_t)pixel_height * sizeof(*pixels));
    if (pixels == NULL) {
      goto fail;
    }
#ifdef GLOOM_DOS_SDL3
    indexed_pixels = (uint8_t *)malloc((size_t)pixel_width * (size_t)pixel_height * sizeof(*indexed_pixels));
    coverage = (uint8_t *)malloc((size_t)pixel_width * (size_t)pixel_height * sizeof(*coverage));
    if (indexed_pixels == NULL || coverage == NULL) {
      free(pixels);
      free(indexed_pixels);
      free(coverage);
      goto fail;
    }
#endif

    for (py = 0; py < pixel_height; ++py) {
      int px = 0;

      for (px = 0; px < pixel_width; ++px) {
        uint8_t palette_index = 0u;
        bool covered = false;
        int plane = 0;
        size_t word_offset = (size_t)(px / 16) * 2u;
        unsigned bit_shift = (unsigned)(15 - (px % 16));

        for (plane = 0; plane < (int)plane_count; ++plane) {
          size_t row = ((size_t)plane * (size_t)pixel_height) + (size_t)py;
          size_t src_offset = (size_t)glyph_offset + 10u + row * row_stride + word_offset;
          size_t mask_src_offset = (size_t)glyph_offset + (size_t)mask_offset + row * row_stride + word_offset;
          uint16_t src_word = main_read_be16(data + src_offset);
          uint16_t mask_word = main_read_be16(data + mask_src_offset);

          if (((src_word >> bit_shift) & 1u) != 0u) {
            palette_index = (uint8_t)(palette_index | (uint8_t)(1u << (unsigned)plane));
          }
          if (((mask_word >> bit_shift) & 1u) != 0u) {
            covered = true;
          }
        }

        {
          size_t dst_index = (size_t)py * (size_t)pixel_width + (size_t)px;

          pixels[dst_index] = covered ? palette[palette_index] : 0x00000000u;
#ifdef GLOOM_DOS_SDL3
          indexed_pixels[dst_index] = palette_index;
          coverage[dst_index] = covered ? 1u : 0u;
#endif
        }
      }
    }

    glyphs[glyph_index].argb_pixels = pixels;
#ifdef GLOOM_DOS_SDL3
    glyphs[glyph_index].indexed_pixels = indexed_pixels;
    glyphs[glyph_index].coverage = coverage;
#endif
    glyphs[glyph_index].width = pixel_width;
    glyphs[glyph_index].height = pixel_height;
  }

  font->loaded = true;
  font->glyph_count = glyph_count;
  font->glyphs = glyphs;
#ifdef GLOOM_DOS_SDL3
  memcpy(font->palette, palette, sizeof(font->palette));
  memcpy(font->palette_used, palette_used, sizeof(font->palette_used));
#endif
  (void)snprintf(font->source_name, sizeof(font->source_name), "%s", resolved_path);
  free(data);
  dos_logf("DOS checkpoint: menu big font loaded %zu glyphs", glyph_count);
  printf("Loaded %zu menu glyphs from original bigfont asset\n", glyph_count);
  return true;

fail:
  for (glyph_index = 0u; glyph_index < glyph_count; ++glyph_index) {
    free(glyphs[glyph_index].argb_pixels);
#ifdef GLOOM_DOS_SDL3
    free(glyphs[glyph_index].indexed_pixels);
    free(glyphs[glyph_index].coverage);
#endif
  }
  free(glyphs);
  free(data);
  return false;
}

static bool load_wall_texture_screen(const char *path, const char *source_name, WallTextureScreen *out_screen) {
  uint8_t *data = NULL;
  size_t file_size = 0;
  uint8_t *texels = NULL;
#ifdef GLOOM_DOS_SDL3
  uint8_t *column_texels = NULL;
#endif
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

#ifdef GLOOM_DOS_SDL3
  column_texels = (uint8_t *)calloc((size_t)GLOOM_TEXTURES_PER_SCREEN * (size_t)GLOOM_TEXTURE_WIDTH *
                                       (size_t)GLOOM_TEXTURE_HEIGHT,
                                   sizeof(*column_texels));
  if (column_texels == NULL) {
    free(texels);
    free(data);
    return false;
  }
#endif

  column_flags = (uint8_t *)calloc((size_t)GLOOM_TEXTURES_PER_SCREEN * (size_t)GLOOM_TEXTURE_WIDTH,
                                   sizeof(*column_flags));
  if (column_flags == NULL) {
#ifdef GLOOM_DOS_SDL3
    free(column_texels);
#endif
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
#ifdef GLOOM_DOS_SDL3
        column_texels[texture_index * ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT) +
                      x * (size_t)GLOOM_TEXTURE_HEIGHT + y] = column_pixels[y];
#endif
      }
    }
  }

#ifdef GLOOM_DOS_SDL3
  (void)load_packed_palette_with_used(out_screen->palette, data, file_size, (size_t)palette_offset,
                                      out_screen->palette_used);
#else
  load_packed_palette(out_screen->palette, data, file_size, (size_t)palette_offset);
#endif
  build_shaded_palette(out_screen->shaded_palette, out_screen->palette);
#ifdef GLOOM_DOS_SDL3
  build_dos_shaded_palette_indices(out_screen->shaded_indices, out_screen->palette, out_screen->palette_used);
#endif

  out_screen->loaded = true;
  out_screen->texels = texels;
#ifdef GLOOM_DOS_SDL3
  out_screen->column_texels = column_texels;
#endif
  out_screen->column_flags = column_flags;
  out_screen->texture_count = available_textures;
  (void)snprintf(out_screen->source_name, sizeof(out_screen->source_name), "%s", source_name != NULL ? source_name : path);
#ifdef GLOOM_DOS_SDL3
  if (!rebuild_dos_wall_texture_screen_indices(out_screen)) {
    free(data);
    free(out_screen->texels);
    free(out_screen->column_flags);
    free(out_screen->column_texels);
    free(out_screen->column_shaded_texels);
    memset(out_screen, 0, sizeof(*out_screen));
    return false;
  }
#endif

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
    size_t target_screen = set->loaded_count;

    if (name[0] == '\0') {
      continue;
    }
    if (target_screen >= GLOOM_TEXTURE_SCREENS) {
      fprintf(stderr, "Map references more than %u compact texture screens\n", (unsigned)GLOOM_TEXTURE_SCREENS);
      ok = false;
      continue;
    }

    if (!resolve_texture_screen_path(name, resolved_path, sizeof(resolved_path))) {
      fprintf(stderr, "Missing texture screen '%s'\n", name);
      ok = false;
      continue;
    }

    /*
     * amiga/gloom2.s: loadtxts only appends 20 entries to the global textures
     * pointer table for non-empty names; empty texture-name slots do not reserve a
     * 20-texture gap.
     */
    if (!load_wall_texture_screen(resolved_path, name, &set->screens[target_screen])) {
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
  size_t anim_index = 0u;

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

  for (anim_index = 0u; anim_index < map->animation_count; ++anim_index) {
    const GloomAnim *anim = &map->animations[anim_index];
    uint16_t frame_offset = 0u;

    if (anim->frame_count < 2u) {
      fprintf(stderr, "Animation %zu has %u frame(s), but amiga/gloom2.s doanims expects at least 2\n",
              anim_index, (unsigned)anim->frame_count);
      ok = false;
      continue;
    }

    if (anim->first_frame >= (uint16_t)GLOOM_TOTAL_WALL_TEXTURES ||
        anim->frame_count > (uint16_t)GLOOM_TOTAL_WALL_TEXTURES - anim->first_frame) {
      fprintf(stderr, "Animation %zu references texture range %u..%u outside the original textures table\n",
              anim_index, (unsigned)anim->first_frame,
              (unsigned)(anim->first_frame + anim->frame_count - 1u));
      ok = false;
      continue;
    }

    for (frame_offset = 0u; frame_offset < anim->frame_count; ++frame_offset) {
      uint16_t texture_index = (uint16_t)(anim->first_frame + frame_offset);
      size_t screen_index = texture_index / (uint16_t)GLOOM_TEXTURES_PER_SCREEN;
      size_t local_index = texture_index % (uint16_t)GLOOM_TEXTURES_PER_SCREEN;

      if (screen_index >= GLOOM_TEXTURE_SCREENS || !set->screens[screen_index].loaded) {
        fprintf(stderr, "Animation %zu references texture %u, but texture screen %zu is not loaded\n", anim_index,
                (unsigned)texture_index, screen_index);
        ok = false;
      } else if (local_index >= set->screens[screen_index].texture_count) {
        fprintf(stderr, "Animation %zu references texture %u, but screen %zu only has %zu textures\n", anim_index,
                (unsigned)texture_index, screen_index, set->screens[screen_index].texture_count);
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

static bool preserve_stale_wall_texture_references(const GloomMap *map, WallTextureSet *set,
                                                   const WallTextureSet *previous_set) {
  bool ok = true;
  size_t zone_index = 0u;

  if (map == NULL || set == NULL || previous_set == NULL) {
    return true;
  }

  for (zone_index = 0u; zone_index < map->zone_count; ++zone_index) {
    const GloomZone *zone = &map->zones[zone_index];
    size_t slot = 0u;

    for (slot = 0u; slot < sizeof(zone->textures); ++slot) {
      uint8_t texture_index = zone->textures[slot];
      size_t screen_index = texture_index / (uint8_t)GLOOM_TEXTURES_PER_SCREEN;

      if (texture_index == 255u || screen_index >= GLOOM_TEXTURE_SCREENS || set->screens[screen_index].loaded) {
        continue;
      }
      if (!previous_set->screens[screen_index].loaded) {
        continue;
      }

      /*
       * amiga/gloom2.s: freetxts clears textscrns but not the global textures
       * pointer table. loadtxts then overwrites only screens named by the next
       * map, so maps can still read stale texture slots from the previous level.
       */
      if (!clone_wall_texture_screen(&previous_set->screens[screen_index], &set->screens[screen_index])) {
        fprintf(stderr, "Failed to preserve stale wall texture screen %zu for zone %zu texture %u\n", screen_index,
                zone_index, texture_index);
        ok = false;
      } else if (set->loaded_count <= screen_index) {
        set->loaded_count = screen_index + 1u;
      }
    }
  }

  return ok;
}

static void free_flat_texture(FlatTexture *texture) {
  if (texture == NULL) {
    return;
  }

  free(texture->texels);
#ifdef GLOOM_DOS_SDL3
  free(texture->shaded_texels);
#endif
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

static bool clone_flat_texture(const FlatTexture *source, FlatTexture *destination) {
  enum { FLAT_TEXEL_BYTES = GLOOM_FLAT_TEXTURE_SIZE * GLOOM_FLAT_TEXTURE_SIZE };

  if (source == NULL || destination == NULL || !source->loaded || source->texels == NULL) {
    return false;
  }

  free_flat_texture(destination);
  *destination = *source;
  destination->texels = NULL;
#ifdef GLOOM_DOS_SDL3
  destination->shaded_texels = NULL;
#endif

  destination->texels = (uint8_t *)malloc((size_t)FLAT_TEXEL_BYTES);
  if (destination->texels == NULL) {
    memset(destination, 0, sizeof(*destination));
    return false;
  }
  memcpy(destination->texels, source->texels, (size_t)FLAT_TEXEL_BYTES);

#ifdef GLOOM_DOS_SDL3
  if (source->shaded_texels != NULL) {
    size_t shaded_count = 16u * (size_t)FLAT_TEXEL_BYTES;

    destination->shaded_texels = (uint8_t *)malloc(shaded_count);
    if (destination->shaded_texels == NULL) {
      free_flat_texture(destination);
      return false;
    }
    memcpy(destination->shaded_texels, source->shaded_texels, shaded_count);
  }
#endif

  return true;
}

static bool clone_flat_texture_set(const FlatTextureSet *source, FlatTextureSet *destination) {
  if (source == NULL || destination == NULL || source->tile_tag[0] == '\0') {
    return false;
  }

  free_flat_texture_set(destination);
  if (!clone_flat_texture(&source->floor, &destination->floor) ||
      !clone_flat_texture(&source->roof, &destination->roof)) {
    free_flat_texture_set(destination);
    return false;
  }
  (void)snprintf(destination->tile_tag, sizeof(destination->tile_tag), "%s", source->tile_tag);
  return true;
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

static bool load_game_script_blob(uint8_t **out_script, size_t *out_script_size);

static bool resolve_script_tile_tag_for_map(const char *map_path, char *out_tag, size_t out_tag_size) {
  char map_name[64] = {0};
  char current_tile[32] = {0};
  uint8_t *script = NULL;
  size_t script_size = 0u;

  if (!map_leaf_name(map_path, map_name, sizeof(map_name)) || out_tag == NULL || out_tag_size == 0u) {
    return false;
  }

  if (strlen(map_name) >= 6u && memcmp(map_name, "com", 3u) == 0 && map_name[3] >= '1' && map_name[3] <= '3' &&
      map_name[4] == '_') {
    /*
     * amiga/gloom2.s: pickcombat copies comseriesnum into floortag before
     * loadtile, so combat maps use the single-character series tile tag.
     */
    if (out_tag_size < 2u) {
      return false;
    }
    out_tag[0] = map_name[3];
    out_tag[1] = '\0';
    return true;
  }

  if (!load_game_script_blob(&script, &script_size)) {
    fprintf(stderr, "Unable to load game script for tile selection\n");
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

static bool load_game_script_blob(uint8_t **out_script, size_t *out_script_size) {
  const char *script_candidates[2] = {"amiga/misc/script", "amiga/data/misc/script"};
  static uint8_t *cached_script = NULL;
  static size_t cached_script_size = 0u;
  size_t candidate_index = 0u;
  char error[256] = {0};

  if (out_script == NULL || out_script_size == NULL) {
    return false;
  }

  *out_script = NULL;
  *out_script_size = 0u;

  if (cached_script != NULL && cached_script_size > 0u) {
    uint8_t *script_copy = (uint8_t *)malloc(cached_script_size);

    if (script_copy == NULL) {
      fprintf(stderr, "Unable to copy cached game script (%zu bytes)\n", cached_script_size);
      return false;
    }
    memcpy(script_copy, cached_script, cached_script_size);
    *out_script = script_copy;
    *out_script_size = cached_script_size;
    return true;
  }

  for (candidate_index = 0u; candidate_index < sizeof(script_candidates) / sizeof(script_candidates[0]); ++candidate_index) {
    char resolved_path[1024] = {0};
    uint8_t *script = NULL;
    size_t script_size = 0u;

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

    cached_script = script;
    cached_script_size = script_size;

    script = (uint8_t *)malloc(cached_script_size);
    if (script == NULL) {
      fprintf(stderr, "Unable to copy cached game script (%zu bytes)\n", cached_script_size);
      return false;
    }
    memcpy(script, cached_script, cached_script_size);
    *out_script = script;
    *out_script_size = cached_script_size;
    return true;
  }

  fprintf(stderr, "Unable to load game script for scriptplay level progression\n");
  return false;
}

static bool script_play_ordinal_for_map(const char *map_path, uint32_t *out_ordinal) {
  char map_name[64] = {0};
  uint8_t *script = NULL;
  size_t script_size = 0u;
  size_t pos = 0u;
  uint32_t ordinal = 0u;

  if (!map_leaf_name(map_path, map_name, sizeof(map_name)) || out_ordinal == NULL) {
    return false;
  }
  *out_ordinal = 0u;

  if (!load_game_script_blob(&script, &script_size)) {
    return false;
  }

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

    if (line_len > 5u && memcmp(line, "play_", 5u) == 0) {
      const char *script_map_name = line + 5u;
      size_t script_map_len = line_len - 5u;

      ordinal += 1u;
      if (script_map_len == strlen(map_name) && memcmp(script_map_name, map_name, script_map_len) == 0) {
        *out_ordinal = ordinal;
        free(script);
        return true;
      }
    }
  }

  fprintf(stderr, "No script play_ ordinal found for map %s\n", map_name);
  free(script);
  return false;
}

static bool resolve_script_level_intro_for_map(const char *map_path, ScriptLevelIntro *out_intro) {
  char map_name[64] = {0};
  char current_picture[sizeof(((ScriptLevelIntro *)0)->picture)] = {0};
  char current_text[sizeof(((ScriptLevelIntro *)0)->text)] = {0};
  uint8_t *script = NULL;
  size_t script_size = 0u;
  size_t pos = 0u;

  if (!map_leaf_name(map_path, map_name, sizeof(map_name)) || out_intro == NULL) {
    return false;
  }
  memset(out_intro, 0, sizeof(*out_intro));

  if (!load_game_script_blob(&script, &script_size)) {
    return false;
  }

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

    if (line_len > 5u && memcmp(line, "pict_", 5u) == 0) {
      if (!copy_script_token(current_picture, sizeof(current_picture), line + 5u, line_len - 5u)) {
        free(script);
        return false;
      }
    } else if (line_len > 5u && memcmp(line, "text_", 5u) == 0) {
      if (!copy_script_token(current_text, sizeof(current_text), line + 5u, line_len - 5u)) {
        free(script);
        return false;
      }
    } else if (line_len > 5u && memcmp(line, "play_", 5u) == 0) {
      const char *script_map_name = line + 5u;
      size_t script_map_len = line_len - 5u;

      if (script_map_len == strlen(map_name) && memcmp(script_map_name, map_name, script_map_len) == 0) {
        if (current_picture[0] != '\0') {
          (void)snprintf(out_intro->picture, sizeof(out_intro->picture), "%s", current_picture);
        }
        if (current_text[0] != '\0') {
          (void)snprintf(out_intro->text, sizeof(out_intro->text), "%s", current_text);
        }
        free(script);
        return out_intro->picture[0] != '\0' && out_intro->text[0] != '\0';
      }
    }
  }

  fprintf(stderr, "No script intro picture/text found for map %s\n", map_name);
  free(script);
  return false;
}

#if GLOOM_RUNTIME_HAS_AUTOSAVE
#define GLOOM_AUTOSAVE_ONE_PLAYER_FILE "GLOOM1P.SAV"
#define GLOOM_AUTOSAVE_TWO_PLAYER_FILE "GLOOM2P.SAV"
#define GLOOM_AUTOSAVE_LEVEL_INTERVAL 3u

typedef struct {
  char map_path[1024];
  bool two_player_mode;
  bool combat_mode;
  uint8_t violence_mode;
  bool barrel_projectile_origin;
  int16_t player_hitpoints;
  int16_t player_lives;
  uint8_t player_weapon;
  uint8_t player_reload;
  int16_t player2_hitpoints;
  int16_t player2_lives;
  uint8_t player2_weapon;
  uint8_t player2_reload;
  uint32_t script_ordinal;
} GloomAutosaveData;

static const char *gloom_autosave_path_for_mode(bool two_player_mode) {
  return two_player_mode ? GLOOM_AUTOSAVE_TWO_PLAYER_FILE : GLOOM_AUTOSAVE_ONE_PLAYER_FILE;
}

static bool gloom_should_autosave_script_ordinal(uint32_t script_ordinal) {
  return script_ordinal > 1u && ((script_ordinal - 1u) % GLOOM_AUTOSAVE_LEVEL_INTERVAL) == 0u;
}

#ifdef __EMSCRIPTEN__
EM_JS(int, gloom_web_autosave_available_js, (const char *key), {
  try {
    return localStorage.getItem(UTF8ToString(key)) !== null ? 1 : 0;
  } catch (error) {
    console.error("Cannot check autosave localStorage", error);
    return 0;
  }
})

EM_JS(int, gloom_web_autosave_write_js, (const char *key, const char *value), {
  try {
    localStorage.setItem(UTF8ToString(key), UTF8ToString(value));
    return 1;
  } catch (error) {
    console.error("Cannot write autosave localStorage", error);
    return 0;
  }
})

EM_JS(char *, gloom_web_autosave_read_js, (const char *key), {
  try {
    var value = localStorage.getItem(UTF8ToString(key));
    var byteCount = 0;
    var ptr = 0;

    if (value === null) {
      return 0;
    }
    byteCount = lengthBytesUTF8(value) + 1;
    ptr = _malloc(byteCount);
    if (ptr === 0) {
      return 0;
    }
    stringToUTF8(value, ptr, byteCount);
    return ptr;
  } catch (error) {
    console.error("Cannot read autosave localStorage", error);
    return 0;
  }
})

EM_JS(void, gloom_web_autosave_delete_js, (const char *key), {
  try {
    localStorage.removeItem(UTF8ToString(key));
  } catch (error) {
    console.error("Cannot delete autosave localStorage", error);
  }
})
#endif

static bool gloom_autosave_available(bool two_player_mode) {
#ifdef __EMSCRIPTEN__
  return gloom_web_autosave_available_js(gloom_autosave_path_for_mode(two_player_mode)) != 0;
#else
  FILE *file = fopen(gloom_autosave_path_for_mode(two_player_mode), "r");

  if (file == NULL) {
    return false;
  }
  fclose(file);
  return true;
#endif
}

static void gloom_autosave_delete(bool two_player_mode) {
#ifdef __EMSCRIPTEN__
  gloom_web_autosave_delete_js(gloom_autosave_path_for_mode(two_player_mode));
#else
  (void)remove(gloom_autosave_path_for_mode(two_player_mode));
#endif
}

static void trim_autosave_line(char *line) {
  size_t len = 0u;

  if (line == NULL) {
    return;
  }
  len = strlen(line);
  while (len > 0u && (line[len - 1u] == '\n' || line[len - 1u] == '\r')) {
    line[len - 1u] = '\0';
    len -= 1u;
  }
}

static bool append_autosave_line(char *buffer, size_t buffer_size, size_t *io_offset, const char *fmt, ...) {
  va_list args;
  int written = 0;

  if (buffer == NULL || io_offset == NULL || fmt == NULL || *io_offset >= buffer_size) {
    return false;
  }

  va_start(args, fmt);
  written = vsnprintf(buffer + *io_offset, buffer_size - *io_offset, fmt, args);
  va_end(args);
  if (written < 0 || (size_t)written >= buffer_size - *io_offset) {
    return false;
  }
  *io_offset += (size_t)written;
  return true;
}

static bool write_autosave_text_record(const char *save_path, const char *record) {
#ifdef __EMSCRIPTEN__
  if (gloom_web_autosave_write_js(save_path, record) == 0) {
    fprintf(stderr, "Cannot autosave %s: browser localStorage write failed\n", save_path);
    return false;
  }
  return true;
#else
  FILE *file = fopen(save_path, "w");

  if (file == NULL) {
    fprintf(stderr, "Cannot autosave %s: %s\n", save_path, strerror(errno));
    return false;
  }
  if (fputs(record, file) == EOF) {
    fprintf(stderr, "Cannot write autosave %s: %s\n", save_path, strerror(errno));
    fclose(file);
    return false;
  }
  if (fclose(file) != 0) {
    fprintf(stderr, "Cannot finish autosave %s: %s\n", save_path, strerror(errno));
    return false;
  }
  return true;
#endif
}

static char *read_autosave_text_record(const char *save_path) {
#ifdef __EMSCRIPTEN__
  char *record = gloom_web_autosave_read_js(save_path);

  if (record == NULL) {
    fprintf(stderr, "Cannot load autosave %s: browser localStorage entry is missing\n", save_path);
  }
  return record;
#else
  FILE *file = fopen(save_path, "rb");
  long size = 0;
  char *record = NULL;

  if (file == NULL) {
    fprintf(stderr, "Cannot load autosave %s: %s\n", save_path, strerror(errno));
    return NULL;
  }
  if (fseek(file, 0, SEEK_END) != 0) {
    fprintf(stderr, "Cannot load autosave %s: failed to measure file size\n", save_path);
    fclose(file);
    return NULL;
  }
  size = ftell(file);
  if (size < 0) {
    fprintf(stderr, "Cannot load autosave %s: failed to read file size\n", save_path);
    fclose(file);
    return NULL;
  }
  if (fseek(file, 0, SEEK_SET) != 0) {
    fprintf(stderr, "Cannot load autosave %s: failed to rewind file\n", save_path);
    fclose(file);
    return NULL;
  }
  record = (char *)malloc((size_t)size + 1u);
  if (record == NULL) {
    fprintf(stderr, "Cannot load autosave %s: out of memory\n", save_path);
    fclose(file);
    return NULL;
  }
  if (size > 0 && fread(record, 1u, (size_t)size, file) != (size_t)size) {
    fprintf(stderr, "Cannot load autosave %s: failed to read file\n", save_path);
    free(record);
    fclose(file);
    return NULL;
  }
  record[(size_t)size] = '\0';
  fclose(file);
  return record;
#endif
}

static bool autosave_next_line(const char **io_cursor, char *line, size_t line_size) {
  const char *cursor = NULL;
  size_t len = 0u;

  if (io_cursor == NULL || *io_cursor == NULL || line == NULL || line_size == 0u || **io_cursor == '\0') {
    return false;
  }

  cursor = *io_cursor;
  while (cursor[len] != '\0' && cursor[len] != '\n') {
    len += 1u;
  }
  if (len >= line_size) {
    len = line_size - 1u;
  }
  memcpy(line, cursor, len);
  line[len] = '\0';
  while (*cursor != '\0' && *cursor != '\n') {
    cursor += 1;
  }
  if (*cursor == '\n') {
    cursor += 1;
  }
  *io_cursor = cursor;
  trim_autosave_line(line);
  return true;
}

static bool write_gloom_autosave_record(const GloomAutosaveData *save) {
  const char *save_path = NULL;
  char record[2048];
  size_t offset = 0u;

  if (save == NULL || save->map_path[0] == '\0') {
    fprintf(stderr, "Cannot autosave: missing save data or map path\n");
    return false;
  }
  if (save->combat_mode) {
    return false;
  }
  save_path = gloom_autosave_path_for_mode(save->two_player_mode);

  if (!append_autosave_line(record, sizeof(record), &offset, "GLOOM_AUTOSAVE 1\n") ||
      !append_autosave_line(record, sizeof(record), &offset, "map=%s\n", save->map_path) ||
      !append_autosave_line(record, sizeof(record), &offset, "script_ordinal=%lu\n",
                            (unsigned long)save->script_ordinal) ||
      !append_autosave_line(record, sizeof(record), &offset, "two_player=%d\n", save->two_player_mode ? 1 : 0) ||
      !append_autosave_line(record, sizeof(record), &offset, "combat=%d\n", save->combat_mode ? 1 : 0) ||
      !append_autosave_line(record, sizeof(record), &offset, "violence=%u\n", (unsigned)save->violence_mode) ||
      !append_autosave_line(record, sizeof(record), &offset, "barrel_origin=%d\n",
                            save->barrel_projectile_origin ? 1 : 0) ||
      !append_autosave_line(record, sizeof(record), &offset, "p1=%d,%d,%u,%u\n", (int)save->player_hitpoints,
                            (int)save->player_lives, (unsigned)save->player_weapon,
                            (unsigned)save->player_reload) ||
      !append_autosave_line(record, sizeof(record), &offset, "p2=%d,%d,%u,%u\n", (int)save->player2_hitpoints,
                            (int)save->player2_lives, (unsigned)save->player2_weapon,
                            (unsigned)save->player2_reload)) {
    fprintf(stderr, "Cannot autosave %s: save record is too large\n", save_path);
    return false;
  }

  return write_autosave_text_record(save_path, record);
}

static bool write_gloom_autosave(const AppState *state, const char *resolved_map_path, uint32_t script_ordinal) {
  GloomAutosaveData save;

  if (state == NULL || resolved_map_path == NULL || resolved_map_path[0] == '\0') {
    fprintf(stderr, "Cannot autosave: missing current game state or map path\n");
    return false;
  }
  if (state->combat_mode) {
    return false;
  }

  memset(&save, 0, sizeof(save));
  (void)snprintf(save.map_path, sizeof(save.map_path), "%s", resolved_map_path);
  save.script_ordinal = script_ordinal;
  save.two_player_mode = state->two_player_mode;
  save.combat_mode = state->combat_mode;
  save.violence_mode = state->violence_mode;
  save.barrel_projectile_origin = state->barrel_projectile_origin;
  save.player_hitpoints = state->player_hitpoints;
  save.player_lives = state->player_lives;
  save.player_weapon = state->player_weapon;
  save.player_reload = state->player_reload;
  save.player2_hitpoints = state->player2.player_hitpoints;
  save.player2_lives = state->player2.player_lives;
  save.player2_weapon = state->player2.player_weapon;
  save.player2_reload = state->player2.player_reload;
  return write_gloom_autosave_record(&save);
}

static bool write_gloom_autosave_for_scriptplay(const char *next_map_path, uint32_t script_ordinal,
                                                bool two_player_mode, uint8_t violence_mode,
                                                bool barrel_projectile_origin, int16_t player_hitpoints,
                                                int16_t player_lives, uint8_t player_weapon, uint8_t player_reload,
                                                int16_t player2_hitpoints, int16_t player2_lives,
                                                uint8_t player2_weapon, uint8_t player2_reload) {
  GloomAutosaveData save;

  if (next_map_path == NULL || next_map_path[0] == '\0') {
    fprintf(stderr, "Cannot autosave next script map: missing map path\n");
    return false;
  }

  memset(&save, 0, sizeof(save));
  (void)snprintf(save.map_path, sizeof(save.map_path), "%s", next_map_path);
  save.script_ordinal = script_ordinal;
  save.two_player_mode = two_player_mode;
  save.combat_mode = false;
  save.violence_mode = violence_mode;
  save.barrel_projectile_origin = barrel_projectile_origin;
  save.player_hitpoints = player_hitpoints;
  save.player_lives = player_lives;
  save.player_weapon = player_weapon;
  save.player_reload = player_reload;
  save.player2_hitpoints = player2_hitpoints;
  save.player2_lives = player2_lives;
  save.player2_weapon = player2_weapon;
  save.player2_reload = player2_reload;
  return write_gloom_autosave_record(&save);
}

static bool read_gloom_autosave(bool two_player_mode, GloomAutosaveData *out_save) {
  const char *save_path = gloom_autosave_path_for_mode(two_player_mode);
  char *record = NULL;
  const char *cursor = NULL;
  char line[1200];
  bool have_script_ordinal = false;
  bool have_two_player = false;
  bool have_combat = false;
  bool have_violence = false;
  bool have_barrel = false;
  bool have_p1 = false;
  bool have_p2 = false;
  bool path_too_long = false;

  if (out_save == NULL) {
    return false;
  }
  memset(out_save, 0, sizeof(*out_save));

  record = read_autosave_text_record(save_path);
  if (record == NULL) {
    return false;
  }
  cursor = record;

  if (!autosave_next_line(&cursor, line, sizeof(line))) {
    fprintf(stderr, "Cannot load autosave %s: record is empty\n", save_path);
    free(record);
    return false;
  }
  if (strcmp(line, "GLOOM_AUTOSAVE 1") != 0) {
    fprintf(stderr, "Cannot load autosave %s: unsupported header '%s'\n", save_path, line);
    free(record);
    return false;
  }

  while (autosave_next_line(&cursor, line, sizeof(line))) {
    int a = 0;
    int b = 0;
    unsigned c = 0u;
    unsigned d = 0u;
    unsigned value = 0u;
    unsigned long long_value = 0ul;

    if (memcmp(line, "map=", 4u) == 0) {
      if (strlen(line + 4) >= sizeof(out_save->map_path)) {
        path_too_long = true;
      } else {
        (void)snprintf(out_save->map_path, sizeof(out_save->map_path), "%s", line + 4);
      }
    } else if (sscanf(line, "script_ordinal=%lu", &long_value) == 1) {
      out_save->script_ordinal = (uint32_t)long_value;
      have_script_ordinal = true;
    } else if (sscanf(line, "two_player=%u", &value) == 1) {
      out_save->two_player_mode = value != 0u;
      have_two_player = true;
    } else if (sscanf(line, "combat=%u", &value) == 1) {
      out_save->combat_mode = value != 0u;
      have_combat = true;
    } else if (sscanf(line, "violence=%u", &value) == 1) {
      out_save->violence_mode = (uint8_t)value;
      have_violence = true;
    } else if (sscanf(line, "barrel_origin=%u", &value) == 1) {
      out_save->barrel_projectile_origin = value != 0u;
      have_barrel = true;
    } else if (sscanf(line, "p1=%d,%d,%u,%u", &a, &b, &c, &d) == 4) {
      out_save->player_hitpoints = (int16_t)a;
      out_save->player_lives = (int16_t)b;
      out_save->player_weapon = (uint8_t)c;
      out_save->player_reload = (uint8_t)d;
      have_p1 = true;
    } else if (sscanf(line, "p2=%d,%d,%u,%u", &a, &b, &c, &d) == 4) {
      out_save->player2_hitpoints = (int16_t)a;
      out_save->player2_lives = (int16_t)b;
      out_save->player2_weapon = (uint8_t)c;
      out_save->player2_reload = (uint8_t)d;
      have_p2 = true;
    }
  }

  free(record);
  if (path_too_long) {
    fprintf(stderr, "Cannot load autosave %s: map path is too long\n", save_path);
    return false;
  }
  if (out_save->map_path[0] == '\0') {
    fprintf(stderr, "Cannot load autosave %s: missing map path\n", save_path);
    return false;
  }
  if (!have_script_ordinal || !have_two_player || !have_combat || !have_violence || !have_barrel || !have_p1 ||
      !have_p2) {
    fprintf(stderr, "Cannot load autosave %s: save record is missing required state fields\n", save_path);
    return false;
  }
  if (out_save->combat_mode) {
    fprintf(stderr, "Cannot load autosave %s: combat autosaves are not written by this runtime\n", save_path);
    return false;
  }
  if (out_save->two_player_mode != two_player_mode) {
    fprintf(stderr, "Cannot load autosave %s: save mode does not match the requested player count\n", save_path);
    return false;
  }
  return true;
}
#endif

static ScriptPlayNextResult resolve_next_script_play_map_or_done(const char *current_map_path, char *out_map_path,
                                                                 size_t out_map_path_size,
                                                                 ScriptCompletion *out_completion) {
  char current_map_name[64] = {0};
  uint8_t *script = NULL;
  size_t script_size = 0u;
  size_t pos = 0u;
  bool found_current = false;

  if (!map_leaf_name(current_map_path, current_map_name, sizeof(current_map_name)) || out_map_path == NULL ||
      out_map_path_size == 0u) {
    return SCRIPT_PLAY_NEXT_ERROR;
  }
  if (out_completion != NULL) {
    memset(out_completion, 0, sizeof(*out_completion));
  }

  if (!load_game_script_blob(&script, &script_size)) {
    return SCRIPT_PLAY_NEXT_ERROR;
  }

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

    if (line_len > 5u && memcmp(line, "play_", 5u) == 0) {
      const char *map_name = line + 5u;
      size_t map_len = line_len - 5u;

      if (found_current) {
        char data_candidate[256] = {0};
        char raw_candidate[256] = {0};

        if (map_len + strlen("amiga/maps/") >= out_map_path_size) {
          free(script);
          return SCRIPT_PLAY_NEXT_ERROR;
        }
        (void)snprintf(data_candidate, sizeof(data_candidate), "amiga/data/maps/%.*s", (int)map_len, map_name);
        (void)snprintf(raw_candidate, sizeof(raw_candidate), "amiga/maps/%.*s", (int)map_len, map_name);
        /*
         * amiga/gloom2.s scriptplay loads "maps/<name>" from the packaged game
         * data. In this repo the packaged maps live under amiga/data/maps, while
         * amiga/maps also contains raw source maps that can be stale.
         */
        if (resolve_runtime_file_path(data_candidate, out_map_path, out_map_path_size)) {
          free(script);
          return SCRIPT_PLAY_NEXT_MAP;
        }
        (void)snprintf(out_map_path, out_map_path_size, "%s", raw_candidate);
        free(script);
        return SCRIPT_PLAY_NEXT_MAP;
      }

      found_current = map_len == strlen(current_map_name) && memcmp(map_name, current_map_name, map_len) == 0;
    } else if (found_current && line_len > 5u && memcmp(line, "pict_", 5u) == 0) {
      if (out_completion != NULL &&
          !copy_script_token(out_completion->picture, sizeof(out_completion->picture), line + 5u, line_len - 5u)) {
        free(script);
        return SCRIPT_PLAY_NEXT_ERROR;
      }
    } else if (found_current && line_len > 5u && memcmp(line, "text_", 5u) == 0) {
      if (out_completion != NULL &&
          !copy_script_token(out_completion->text, sizeof(out_completion->text), line + 5u, line_len - 5u)) {
        free(script);
        return SCRIPT_PLAY_NEXT_ERROR;
      }
    } else if (found_current && line_len >= 5u && memcmp(line, "done_", 5u) == 0) {
      free(script);
      return SCRIPT_PLAY_NEXT_DONE;
    }
  }

  fprintf(stderr, "No following play_ command found in misc/script after play_%s\n", current_map_name);
  free(script);
  return SCRIPT_PLAY_NEXT_ERROR;
}

static bool resolve_next_script_play_map(const char *current_map_path, char *out_map_path, size_t out_map_path_size) {
  return resolve_next_script_play_map_or_done(current_map_path, out_map_path, out_map_path_size, NULL) ==
         SCRIPT_PLAY_NEXT_MAP;
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

  /*
   * amiga/gloom2.s: scripttile builds "txts/floor<n>" and "txts/roof<n>".
   * The shipped runtime data extracted from Disk 2 lives under amiga/data/txts;
   * the top-level amiga/txts files are source/development copies for some tiles.
   */
  (void)snprintf(path0, sizeof(path0), "amiga/data/txts/%s%s", kind, tile_tag);
  (void)snprintf(path1, sizeof(path1), "data/txts/%s%s", kind, tile_tag);
  (void)snprintf(path2, sizeof(path2), "txts/%s%s", kind, tile_tag);
  (void)snprintf(path3, sizeof(path3), "amiga/txts/%s%s", kind, tile_tag);

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
#ifdef GLOOM_DOS_SDL3
  (void)load_packed_palette_with_used(out_texture->palette, data, data_size, (size_t)FLAT_TEXEL_BYTES,
                                      out_texture->palette_used);
#else
  load_packed_palette(out_texture->palette, data, data_size, (size_t)FLAT_TEXEL_BYTES);
#endif
  build_shaded_palette(out_texture->shaded_palette, out_texture->palette);
#ifdef GLOOM_DOS_SDL3
  out_texture->texels = texels;
  out_texture->loaded = true;
  if (!rebuild_dos_flat_texture_indices(out_texture)) {
    free(data);
    free_flat_texture(out_texture);
    return false;
  }
#else
  out_texture->texels = texels;
  out_texture->loaded = true;
#endif
  (void)snprintf(out_texture->source_name, sizeof(out_texture->source_name), "%s", resolved_path);

  free(data);
  return true;
}

static bool load_flat_texture_set_for_map_reusing(const char *map_path, FlatTextureSet *set,
                                                  const FlatTextureSet *reuse_set) {
  char tile_tag[32] = {0};

  if (map_path == NULL || set == NULL) {
    return false;
  }

  if (!resolve_script_tile_tag_for_map(map_path, tile_tag, sizeof(tile_tag))) {
    return false;
  }

  if (reuse_set != NULL && strcmp(reuse_set->tile_tag, tile_tag) == 0 && clone_flat_texture_set(reuse_set, set)) {
    printf("Reused script tile_%s floor/roof textures\n", set->tile_tag);
    return true;
  }

  free_flat_texture_set(set);

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

static bool load_flat_texture_set_for_map(const char *map_path, FlatTextureSet *set) {
  return load_flat_texture_set_for_map_reusing(map_path, set, NULL);
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
  tx = (size_t)(u * (float)GLOOM_TEXTURE_WIDTH);
  ty = (size_t)(v * (float)GLOOM_TEXTURE_HEIGHT);
  if (tx >= (size_t)GLOOM_TEXTURE_WIDTH) tx = (size_t)GLOOM_TEXTURE_WIDTH - 1u;
  if (ty >= (size_t)GLOOM_TEXTURE_HEIGHT) ty = (size_t)GLOOM_TEXTURE_HEIGHT - 1u;

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

static void zone_wall_texture_coordinates(const GloomZone *zone, float wall_u, int *out_tile_index,
                                          float *out_local_u, size_t *out_texture_x) {
  float scaled_u = 0.0f;
  float local_u = 0.0f;
  int tile_base = 0;
  int tile_index = 0;
  size_t texture_x = 0u;

  wall_u = clampf(wall_u, 0.0f, 1.0f);
  scaled_u = wall_u;

  if (zone != NULL) {
    if (zone->scale > 0) {
      scaled_u = (wall_u * (float)zone->scale) * 0.5f;
    } else {
      int shift = zone->scale < 0 ? -zone->scale : 0;
      float divisor = 1.0f;

      if (shift > 30) {
        shift = 30;
      }
      if (shift > 0) {
        divisor = (float)(1u << (uint32_t)shift);
      }
      /*
       * amiga/gloom2.s wall column renderer: d0 has been lsr #1 before wl_sc is
       * applied. Positive wl_sc values therefore scale from a half-range fraction,
       * while wl_sc <= 0 doubles that half-range before the right shift.
       */
      scaled_u = wall_u / divisor;
    }
  }

  tile_base = (int)scaled_u;
  local_u = scaled_u - (float)tile_base;
  if (local_u < 0.0f) {
    local_u += 1.0f;
    tile_base -= 1;
  }
  if (local_u < 0.0f) local_u = 0.0f;
  if (local_u > 1.0f) local_u = 1.0f;

  tile_index = tile_base & 7;
  texture_x = (size_t)(local_u * (float)GLOOM_TEXTURE_WIDTH);
  if (texture_x >= (size_t)GLOOM_TEXTURE_WIDTH) {
    texture_x = (size_t)GLOOM_TEXTURE_WIDTH - 1u;
  }

  if (out_tile_index != NULL) {
    *out_tile_index = tile_index;
  }
  if (out_local_u != NULL) {
    *out_local_u = local_u;
  }
  if (out_texture_x != NULL) {
    *out_texture_x = texture_x;
  }
}

#ifdef GLOOM_DOS_SDL3
static void zone_wall_texture_coordinates_fixed16(const GloomZone *zone, int32_t wall_u_fixed, int *out_tile_index,
                                                  size_t *out_texture_x) {
  int32_t scaled_u = 0;
  int tile_base = 0;
  int tile_index = 0;
  size_t texture_x = 0u;

  if (zone == NULL) {
    if (out_tile_index != NULL) *out_tile_index = 0;
    if (out_texture_x != NULL) *out_texture_x = 0u;
    return;
  }

  wall_u_fixed = dos_fixed16_clamp_unit(wall_u_fixed);
  if (zone->scale > 0) {
    scaled_u = (wall_u_fixed * (int32_t)zone->scale) >> 1;
  } else {
    int shift = zone->scale < 0 ? -zone->scale : 0;

    if (shift > 30) {
      shift = 30;
    }
    scaled_u = shift > 0 ? (wall_u_fixed >> shift) : wall_u_fixed;
  }

  tile_base = dos_fixed16_floor_to_int(scaled_u);
  tile_index = tile_base & 7;
  texture_x = (size_t)(((uint32_t)(scaled_u & 0xFFFF) * (uint32_t)GLOOM_TEXTURE_WIDTH) >> 16);
  if (texture_x >= (size_t)GLOOM_TEXTURE_WIDTH) {
    texture_x = (size_t)GLOOM_TEXTURE_WIDTH - 1u;
  }

  if (out_tile_index != NULL) {
    *out_tile_index = tile_index;
  }
  if (out_texture_x != NULL) {
    *out_texture_x = texture_x;
  }
}
#endif

static uint32_t sample_zone_wall_texture_argb(const WallTextureSet *set, const AppState *state,
                                              const GloomZone *zone, float wall_u, float wall_v) {
  float local_u = 0.0f;
  int tile_index = 0;
  uint8_t texture_index = 0;

  if (zone == NULL) {
    return 0xFF9A9A9Au;
  }

  wall_u = clampf(wall_u, 0.0f, 1.0f);
  wall_v = clampf(wall_v, 0.0f, 1.0f);

  zone_wall_texture_coordinates(zone, wall_u, &tile_index, &local_u, NULL);
  texture_index = remap_wall_texture_index(state, zone->textures[tile_index]);
  return sample_wall_texture_argb_ex(set, texture_index, local_u, wall_v, NULL);
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

static const char *object_chunk_asset_name(int16_t object_type) {
  static const char *chunk_assets[GLOOM_OBJECT_TYPE_COUNT] = {
      NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, "dragon2", NULL, "marine2", "baldy2",
      "terra2", NULL, "phantom2", "demon2", NULL, NULL, NULL, NULL, NULL, "lizard2", NULL, "troll2",
  };

  if (object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    return NULL;
  }

  return chunk_assets[object_type];
}

static bool object_type_has_blowchunx(int16_t object_type) {
  return object_chunk_asset_name(object_type) != NULL;
}

static const WeaponDefinition *weapon_definitions(void) {
  static const WeaponDefinition definitions[GLOOM_WEAPON_COUNT] = {
      {"bullet1.bin", "sparks1.bin", 1, 1, 32, 6}, {"bullet2.bin", "sparks2.bin", 5, 2, 36, 7},
      {"bullet3.bin", "sparks3.bin", 10, 2, 40, 8}, {"bullet4.bin", "sparks4.bin", 15, 3, 40, 10},
      {"bullet5.bin", "sparks5.bin", 20, 5, 24, 10},
  };

  return definitions;
}

static const ObjectCombatDefinition *object_combat_definitions(void) {
  static const ObjectCombatDefinition definitions[GLOOM_OBJECT_TYPE_COUNT] = {
      {25, 1, 13.0f, 1, 1, 0, 0x6000u, 5, -64, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {25, 1, 13.0f, 1, 1, 0, 0x6000u, 5, -64, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0x20000u, 0, 5, 0, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0x0000u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 5, 0, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 5, 0, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0x10000u, 0, 5, 0, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0x20000u, 0, 5, 0, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {250, 10, 12.0f, 16, 32, 0, 0x4000u, 5, -64, 0x0f00u, GLOOM_OBJECT_DIE_BLOWDRAGON, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0x30000u, 0, 0, 0, 0x0f00u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {5, 1, 6.0f, 16, 32, 0, 0x6000u, 5, -64, 0x0f00u, GLOOM_OBJECT_DIE_BLOWOBJECT, 0, 1, 20.0f, true},
      {10, 2, 4.0f, 8, 16, 0, 0x4000u, 3, -64, 0x0f00u, GLOOM_OBJECT_DIE_BLOWOBJECT, 0, 0, 0.0f, false},
      {35, 1, 2.0f, 32, 48, 0, 0x6000u, 0, -64, 0x0fffu, GLOOM_OBJECT_DIE_BLOWTERRA, 0, 0, 0.0f, false},
      {5, 0, 8.0f, 32, 48, 0, 0, 5, -64, 0x80f0u, GLOOM_OBJECT_DIE_BLOWOBJECT, 0, 0, 0.0f, false},
      {10, 3, 10.0f, 8, 16, 0, 0xa000u, 7, -64, 0x0ff0u, GLOOM_OBJECT_DIE_BLOWOBJECT, 0, 0, 0.0f, false},
      {25, 5, 7.0f, 32, 4, 0, 0x7000u, 5, -72, 0x0f00u, GLOOM_OBJECT_DIE_BLOWOBJECT, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0x0000u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0x0000u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0x0000u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0x0000u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {0, 0, 0.0f, 0, 0, 0, 0, 0, 0, 0x0000u, GLOOM_OBJECT_DIE_NONE, 0, 0, 0.0f, false},
      {10, 2, 6.0f, 8, 8, 0, 0x4000u, 2, -64, 0x0f0fu, GLOOM_OBJECT_DIE_BLOWOBJECT, 0, 0, 0.0f, false},
      {35, 3, 12.0f, -8, 16, 0x8000u, 0x6000u, 10, -96, 0x0f00u, GLOOM_OBJECT_DIE_BLOWDEATH, 0, 0, 0.0f, false},
      {18, 3, 6.0f, 8, 8, 0, 0x4000u, 2, -64, 0x0f00u, GLOOM_OBJECT_DIE_BLOWOBJECT, 0, 0, 0.0f, false},
  };

  return definitions;
}

static const ObjectCombatDefinition *object_type_combat_definition(int16_t object_type) {
  if (object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    return NULL;
  }

  return &object_combat_definitions()[object_type];
}

static int16_t object_type_initial_hitpoints(int16_t object_type) {
  if (object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    return 0;
  }

  return object_combat_definitions()[object_type].hitpoints;
}

static int16_t object_type_damage(int16_t object_type) {
  if (object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    return 0;
  }

  return object_combat_definitions()[object_type].damage;
}

static float object_type_move_speed(int16_t object_type) {
  if (object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    return 0.0f;
  }

  return object_combat_definitions()[object_type].move_speed;
}

static bool object_type_uses_monsterlogic(int16_t object_type) {
  return object_type == 10;
}

static bool object_type_uses_baldy_family_logic(int16_t object_type) {
  return object_type == GLOOM_OBJECT_TYPE_BALDY || object_type == GLOOM_OBJECT_TYPE_LIZARD ||
         object_type == GLOOM_OBJECT_TYPE_TROLL;
}

static int16_t object_type_punch_rate(int16_t object_type) {
  switch (object_type) {
    case GLOOM_OBJECT_TYPE_BALDY:
      return 4;
    case GLOOM_OBJECT_TYPE_LIZARD:
    case GLOOM_OBJECT_TYPE_TROLL:
      return 3;
    default:
      return 0;
  }
}

static bool object_type_uses_special_projectile_logic(int16_t object_type) {
  return object_type == GLOOM_OBJECT_TYPE_TERRA || object_type == GLOOM_OBJECT_TYPE_GHOUL ||
         object_type == GLOOM_OBJECT_TYPE_PHANTOM || object_type == GLOOM_OBJECT_TYPE_DEMON;
}

static bool object_type_is_pickup(int16_t object_type) {
  return (object_type >= 2 && object_type <= 7) || object_type == GLOOM_OBJECT_TYPE_BOUNCY ||
         (object_type >= GLOOM_OBJECT_TYPE_WEAPON1 && object_type <= GLOOM_OBJECT_TYPE_WEAPON5);
}

static int16_t object_type_pickup_weapon(int16_t object_type) {
  switch (object_type) {
    case 3:
      return 1;
    case GLOOM_OBJECT_TYPE_WEAPON1:
    case GLOOM_OBJECT_TYPE_WEAPON2:
    case GLOOM_OBJECT_TYPE_WEAPON3:
    case GLOOM_OBJECT_TYPE_WEAPON4:
    case GLOOM_OBJECT_TYPE_WEAPON5:
      return (int16_t)(object_type - GLOOM_OBJECT_TYPE_WEAPON1);
    default:
      return -1;
  }
}

static bool object_type_is_enemy(int16_t object_type) {
  return object_type_initial_hitpoints(object_type) > 0 && object_type != 0 && object_type != 1;
}

static bool object_type_is_player(int16_t object_type) {
  return object_type == 0 || object_type == 1;
}

static const char *violence_mode_name(uint8_t violence_mode) {
  switch (violence_mode) {
    case GLOOM_VIOLENCE_MEATY:
      return "MEATY";
    case GLOOM_VIOLENCE_MESSY:
      return "MESSY";
    case GLOOM_VIOLENCE_MEATY_MESSY:
      return "MEATY+MESSY";
    default:
      return "UNKNOWN";
  }
}

static void free_object_visual(ObjectVisual *visual) {
  size_t i = 0u;

  if (visual == NULL) {
    return;
  }

  for (i = 0u; i < visual->frame_count; ++i) {
    free(visual->frames[i].argb_pixels);
#ifdef GLOOM_DOS_SDL3
    free(visual->frames[i].indexed_pixels);
#endif
  }

  free(visual->frames);
  memset(visual, 0, sizeof(*visual));
}

static bool clone_object_visual(const ObjectVisual *source, ObjectVisual *destination) {
  size_t frame_index = 0u;

  if (source == NULL || destination == NULL || !source->loaded || source->frames == NULL ||
      source->frame_count == 0u) {
    return false;
  }

  free_object_visual(destination);
  *destination = *source;
  destination->frames = NULL;

  destination->frames = (ObjectFrame *)calloc(source->frame_count, sizeof(*destination->frames));
  if (destination->frames == NULL) {
    memset(destination, 0, sizeof(*destination));
    return false;
  }

  for (frame_index = 0u; frame_index < source->frame_count; ++frame_index) {
    const ObjectFrame *src_frame = &source->frames[frame_index];
    ObjectFrame *dst_frame = &destination->frames[frame_index];
    size_t pixel_count = 0u;

    *dst_frame = *src_frame;
    dst_frame->argb_pixels = NULL;
#ifdef GLOOM_DOS_SDL3
    dst_frame->indexed_pixels = NULL;
#endif

    if (src_frame->width <= 0 || src_frame->height <= 0 || src_frame->argb_pixels == NULL) {
      free_object_visual(destination);
      return false;
    }
    pixel_count = (size_t)src_frame->width * (size_t)src_frame->height;

    dst_frame->argb_pixels = (uint32_t *)malloc(pixel_count * sizeof(*dst_frame->argb_pixels));
    if (dst_frame->argb_pixels == NULL) {
      free_object_visual(destination);
      return false;
    }
    memcpy(dst_frame->argb_pixels, src_frame->argb_pixels, pixel_count * sizeof(*dst_frame->argb_pixels));

#ifdef GLOOM_DOS_SDL3
    if (src_frame->indexed_pixels != NULL) {
      dst_frame->indexed_pixels = (uint8_t *)malloc(pixel_count * sizeof(*dst_frame->indexed_pixels));
      if (dst_frame->indexed_pixels == NULL) {
        free_object_visual(destination);
        return false;
      }
      memcpy(dst_frame->indexed_pixels, src_frame->indexed_pixels, pixel_count * sizeof(*dst_frame->indexed_pixels));
    }
#endif
  }

  return true;
}

static void free_object_visual_set(ObjectVisualSet *set) {
  size_t i = 0u;

  if (set == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_OBJECT_TYPE_COUNT; ++i) {
    free_object_visual(&set->visuals[i]);
    free_object_visual(&set->chunks[i]);
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
#ifdef GLOOM_DOS_SDL3
  (void)load_packed_palette_with_used(palette, data, data_size, (size_t)palette_offset, out_visual->palette_used);
#else
  load_packed_palette(palette, data, data_size, (size_t)palette_offset);
#endif
#ifdef GLOOM_DOS_SDL3
  build_shaded_palette(out_visual->shaded_palette, palette);
  build_dos_shaded_palette_indices(out_visual->shaded_indices, palette, out_visual->palette_used);
#endif

  for (frame_index = 0u; frame_index < frame_count; ++frame_index) {
    uint32_t frame_offset = main_read_be32(data + 12u + frame_index * 4u);
    int16_t handle_x = 0;
    int16_t handle_y = 0;
    int16_t width = 0;
    int16_t height = 0;
    uint32_t *pixels = NULL;
#ifdef GLOOM_DOS_SDL3
    uint8_t *indexed_pixels = NULL;
#endif
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
#ifdef GLOOM_DOS_SDL3
    indexed_pixels = (uint8_t *)malloc((size_t)width * (size_t)height * sizeof(*indexed_pixels));
    if (indexed_pixels == NULL) {
      free(pixels);
      goto fail;
    }
#endif

    for (sx = 0u; sx < (size_t)width; ++sx) {
      size_t sy = 0u;

      for (sy = 0u; sy < (size_t)height; ++sy) {
        uint8_t palette_index = data[(size_t)frame_offset + 8u + sx * (size_t)height + sy];
        uint32_t argb = palette[palette_index];

        if (palette_index == 0u) {
          argb &= 0x00FFFFFFu;
        }

        pixels[sy * (size_t)width + sx] = argb;
#ifdef GLOOM_DOS_SDL3
        indexed_pixels[sy * (size_t)width + sx] = palette_index;
#endif
      }
    }

    frames[frame_index].handle_x = handle_x;
    frames[frame_index].handle_y = handle_y;
    frames[frame_index].width = width;
    frames[frame_index].height = height;
    frames[frame_index].argb_pixels = pixels;
#ifdef GLOOM_DOS_SDL3
    frames[frame_index].indexed_pixels = indexed_pixels;
#endif
  }

  out_visual->loaded = true;
  out_visual->eight_rotations = definition->eight_rotations;
  out_visual->scale = definition->scale;
  out_visual->fixed_frame = definition->fixed_frame;
  out_visual->use_fixed_frame = definition->use_fixed_frame;
  out_visual->rotation_shift = rotation_shift;
  out_visual->frame_count_per_rotation = frames_per_rotation;
  out_visual->collision_radius = max_w;
  out_visual->frame_count = frame_count;
  out_visual->frames = frames;
  (void)snprintf(out_visual->source_name, sizeof(out_visual->source_name), "%s", resolved_path);

  free(data);
  return true;

fail:
  for (frame_index = 0u; frame_index < frame_count; ++frame_index) {
    free(frames[frame_index].argb_pixels);
#ifdef GLOOM_DOS_SDL3
    free(frames[frame_index].indexed_pixels);
#endif
  }
  free(frames);
  free(data);
  return false;
}

static bool load_object_visual_set_for_map_reusing(const GloomMap *map, ObjectVisualSet *set,
                                                   const ObjectVisualSet *reuse_set) {
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
  if (needed[0] || needed[1]) {
    needed[0] = 1u;
    needed[1] = 1u;
  }

  for (i = 0u; i < GLOOM_OBJECT_TYPE_COUNT; ++i) {
    const char *asset_name = definitions[i].asset_name;

    if (needed[i] == 0u || asset_name == NULL) {
      continue;
    }

    if (reuse_set != NULL && reuse_set->visuals[i].loaded) {
      if (!clone_object_visual(&reuse_set->visuals[i], &set->visuals[i])) {
        free_object_visual_set(set);
        return false;
      }
    } else if (!load_object_visual_from_asset(asset_name, &definitions[i], &set->visuals[i])) {
      free_object_visual_set(set);
      return false;
    }
    if (object_type_is_enemy((int16_t)i) && object_chunk_asset_name((int16_t)i) != NULL) {
      ObjectVisualDefinition chunk_definition = {object_chunk_asset_name((int16_t)i), false, definitions[i].scale, 0u,
                                                 false};

      if (reuse_set != NULL && reuse_set->chunks[i].loaded) {
        if (!clone_object_visual(&reuse_set->chunks[i], &set->chunks[i])) {
          free_object_visual_set(set);
          return false;
        }
      } else if (!load_object_visual_from_asset(chunk_definition.asset_name, &chunk_definition, &set->chunks[i])) {
        free_object_visual_set(set);
        return false;
      }
    }
    loaded_count += 1u;
  }

  printf("Loaded %zu object visual bindings from real object assets\n", loaded_count);
  return true;
}

static bool load_object_visual_set_for_map(const GloomMap *map, ObjectVisualSet *set) {
  return load_object_visual_set_for_map_reusing(map, set, NULL);
}

static void free_weapon_visual_set(WeaponVisualSet *set) {
  size_t i = 0u;

  if (set == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_WEAPON_COUNT; ++i) {
    free_object_visual(&set->bullets[i]);
    free_object_visual(&set->sparks[i]);
  }
  free_object_visual(&set->gun);

  memset(set, 0, sizeof(*set));
}

static bool load_weapon_visual_set(WeaponVisualSet *set) {
  const WeaponDefinition *weapons = weapon_definitions();
  ObjectVisualDefinition bullet_definition = {NULL, false, 0x0200u, 0u, false};
  ObjectVisualDefinition spark_definition = {NULL, false, 0x0200u, 0u, false};
  ObjectVisualDefinition gun_definition = {"misc/gun.bin", false, 0x0100u, 0u, false};
  size_t i = 0u;

  if (set == NULL) {
    return false;
  }

  free_weapon_visual_set(set);

  for (i = 0u; i < GLOOM_WEAPON_COUNT; ++i) {
    if (!load_object_visual_from_asset(weapons[i].bullet_asset, &bullet_definition, &set->bullets[i]) ||
        !load_object_visual_from_asset(weapons[i].spark_asset, &spark_definition, &set->sparks[i])) {
      free_weapon_visual_set(set);
      return false;
    }
  }

  if (!load_object_visual_from_asset(gun_definition.asset_name, &gun_definition, &set->gun)) {
    free_weapon_visual_set(set);
    return false;
  }

  printf("Loaded %u weapon projectile/spark visual pairs and on-screen gun from original payloads\n",
         (unsigned)GLOOM_WEAPON_COUNT);
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

static const ObjectFrame *select_object_visual_frame_number(const ObjectVisual *visual, const GloomObjectSpawn *spawn,
                                                            const AppState *state, size_t frame_number) {
  size_t rotation_count = 1u;
  size_t rotation_index = 0u;
  size_t frame_index = 0u;

  if (visual == NULL || spawn == NULL || state == NULL || !visual->loaded || visual->frames == NULL ||
      visual->frame_count == 0u) {
    return NULL;
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

static const ObjectFrame *select_object_visual_frame(const ObjectVisual *visual, const GloomObjectSpawn *spawn,
                                                     const AppState *state) {
  size_t frame_number = 0u;

  if (visual == NULL) {
    return NULL;
  }

  if (visual->use_fixed_frame) {
    frame_number = visual->fixed_frame;
  } else if (visual->frame_count_per_rotation > 0u) {
    frame_number = (size_t)(sim_ticks_to_amiga_ticks(state->tick_count) / 8u) %
                   (size_t)visual->frame_count_per_rotation;
  }

  return select_object_visual_frame_number(visual, spawn, state, frame_number);
}

static void bind_debug_sprite_frame(DebugSprite *sprite, const ObjectVisual *visual, const ObjectFrame *frame) {
  if (sprite == NULL) {
    return;
  }

  sprite->frame = frame;
#ifdef GLOOM_DOS_SDL3
  sprite->shaded_indices = visual != NULL ? visual->shaded_indices : NULL;
#else
  (void)visual;
#endif
}

static void bind_debug_sprite_depth(DebugSprite *sprite, float depth) {
  if (sprite == NULL) {
    return;
  }

  sprite->depth = depth;
#ifdef GLOOM_DOS_SDL3
  sprite->screen_x_fixed = dos_float_to_fixed16(sprite->screen_x);
  sprite->screen_y_fixed = dos_float_to_fixed16(sprite->screen_y);
  sprite->screen_w_fixed = dos_float_to_fixed16(sprite->screen_w);
  sprite->screen_h_fixed = dos_float_to_fixed16(sprite->screen_h);
  sprite->depth_int = depth >= 0.0f ? (int32_t)(depth + 0.5f) : (int32_t)(depth - 0.5f);
  if (sprite->depth_int < 1) {
    sprite->depth_int = 1;
  }
  if (sprite->depth_int >= GLOOM_AMIGA_MAX_Z) {
    sprite->depth_int = GLOOM_AMIGA_MAX_Z - 1;
  }
#endif
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

static int32_t gamepad_abs_i32(int32_t value) {
  return value < 0 ? -value : value;
}

static int32_t gamepad_axis_with_deadzone(Sint16 raw_value, int deadzone) {
  int32_t value = (int32_t)raw_value;
  int32_t abs_value = gamepad_abs_i32(value);
  int32_t max_value = 32767;
  int32_t scaled = 0;

  if (abs_value <= deadzone) {
    return 0;
  }
  scaled = ((abs_value - deadzone) * max_value) / (max_value - deadzone);
  if (scaled > max_value) {
    scaled = max_value;
  }
  return value < 0 ? -scaled : scaled;
}

static int gamepad_axis_to_mouse_delta(int32_t axis_value) {
  int32_t abs_value = gamepad_abs_i32(axis_value);
  int sign = axis_value < 0 ? -1 : 1;
  int delta = 0;

  if (abs_value < GLOOM_GAMEPAD_LOOK_MIN_DELTA_AXIS) {
    return 0;
  }

  delta = (int)(abs_value / GLOOM_GAMEPAD_LOOK_AXIS_TO_MOUSE_DIV);
  if (delta < 1) {
    delta = 1;
  }
  return delta * sign;
}

static bool gamepad_slot_has_instance(SDL_JoystickID instance_id) {
  size_t i = 0u;

  for (i = 0u; i < (size_t)GLOOM_GAMEPAD_COUNT; ++i) {
    if (g_gamepads[i].controller != NULL && g_gamepads[i].instance_id == instance_id) {
      return true;
    }
  }
  return false;
}

static size_t gamepad_first_free_slot(void) {
  size_t i = 0u;

  for (i = 0u; i < (size_t)GLOOM_GAMEPAD_COUNT; ++i) {
    if (g_gamepads[i].controller == NULL) {
      return i;
    }
  }
  return (size_t)GLOOM_GAMEPAD_COUNT;
}

static void gamepad_close_slot(size_t gamepad_index) {
  SDL_GameController *controller = NULL;
  const char *name = NULL;

  if (gamepad_index >= (size_t)GLOOM_GAMEPAD_COUNT || g_gamepads[gamepad_index].controller == NULL) {
    return;
  }

  controller = g_gamepads[gamepad_index].controller;
  name = SDL_GameControllerName(controller);
  printf("Gamepad %zu disconnected: %s\n", gamepad_index + 1u, name != NULL ? name : "(unnamed)");
  SDL_GameControllerClose(controller);
  g_gamepads[gamepad_index].controller = NULL;
  g_gamepads[gamepad_index].instance_id = -1;
}

static void gamepad_handle_device_added(int device_index) {
  SDL_GameController *controller = NULL;
  SDL_Joystick *joystick = NULL;
  SDL_JoystickID instance_id = -1;
  size_t slot = 0u;

  if (device_index < 0 || device_index >= SDL_NumJoysticks() || !SDL_IsGameController(device_index)) {
    return;
  }
  instance_id = SDL_JoystickGetDeviceInstanceID(device_index);
  if (instance_id >= 0 && gamepad_slot_has_instance(instance_id)) {
    return;
  }
  slot = gamepad_first_free_slot();
  if (slot >= (size_t)GLOOM_GAMEPAD_COUNT) {
    return;
  }

  controller = SDL_GameControllerOpen(device_index);
  if (controller == NULL) {
    fprintf(stderr, "SDL_GameControllerOpen(%d) failed: %s\n", device_index, SDL_GetError());
    return;
  }
  joystick = SDL_GameControllerGetJoystick(controller);
  if (joystick == NULL) {
    fprintf(stderr, "SDL_GameControllerGetJoystick(%d) failed: %s\n", device_index, SDL_GetError());
    SDL_GameControllerClose(controller);
    return;
  }

  g_gamepads[slot].controller = controller;
  g_gamepads[slot].instance_id = SDL_JoystickInstanceID(joystick);
  {
    const char *name = SDL_GameControllerName(controller);

    printf("Gamepad %zu connected: %s\n", slot + 1u, name != NULL ? name : "(unnamed)");
  }
}

static void gamepad_handle_device_removed(SDL_JoystickID instance_id) {
  size_t i = 0u;

  for (i = 0u; i < (size_t)GLOOM_GAMEPAD_COUNT; ++i) {
    if (g_gamepads[i].controller != NULL && g_gamepads[i].instance_id == instance_id) {
      gamepad_close_slot(i);
      break;
    }
  }
}

static void gamepad_init(void) {
  int joystick_count = 0;
  int i = 0;

  memset(g_gamepads, 0, sizeof(g_gamepads));
  for (i = 0; i < GLOOM_GAMEPAD_COUNT; ++i) {
    g_gamepads[i].instance_id = -1;
  }

  SDL_GameControllerEventState(SDL_ENABLE);
  joystick_count = SDL_NumJoysticks();
  for (i = 0; i < joystick_count; ++i) {
    gamepad_handle_device_added(i);
  }
}

static void gamepad_shutdown(void) {
  size_t i = 0u;

  for (i = 0u; i < (size_t)GLOOM_GAMEPAD_COUNT; ++i) {
    gamepad_close_slot(i);
  }
}

static bool gamepad_button_down(size_t gamepad_index, SDL_GameControllerButton button) {
  if (gamepad_index >= (size_t)GLOOM_GAMEPAD_COUNT || g_gamepads[gamepad_index].controller == NULL) {
    return false;
  }
  return SDL_GameControllerGetButton(g_gamepads[gamepad_index].controller, button) != 0;
}

static PlayerControls read_gamepad_player_controls(size_t gamepad_index, unsigned player_index) {
  PlayerControls controls;
  SDL_GameController *controller = NULL;
  int32_t left_x = 0;
  int32_t left_y = 0;
  int32_t right_x = 0;
  int32_t right_trigger = 0;
  bool dpad_up = false;
  bool dpad_down = false;
  bool dpad_left = false;
  bool dpad_right = false;
  int16_t strafe = 0;
  int16_t turn = 0;

  memset(&controls, 0, sizeof(controls));
  if (gamepad_index >= (size_t)GLOOM_GAMEPAD_COUNT || g_gamepads[gamepad_index].controller == NULL) {
    return controls;
  }

  controller = g_gamepads[gamepad_index].controller;
  left_x = gamepad_axis_with_deadzone(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX),
                                      GLOOM_GAMEPAD_MOVE_DEADZONE);
  left_y = gamepad_axis_with_deadzone(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY),
                                      GLOOM_GAMEPAD_MOVE_DEADZONE);
  right_x = gamepad_axis_with_deadzone(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX),
                                       GLOOM_GAMEPAD_LOOK_DEADZONE);
  right_trigger = gamepad_axis_with_deadzone(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT),
                                             GLOOM_GAMEPAD_MOVE_DEADZONE);
  dpad_up = gamepad_button_down(gamepad_index, SDL_CONTROLLER_BUTTON_DPAD_UP);
  dpad_down = gamepad_button_down(gamepad_index, SDL_CONTROLLER_BUTTON_DPAD_DOWN);
  dpad_left = gamepad_button_down(gamepad_index, SDL_CONTROLLER_BUTTON_DPAD_LEFT);
  dpad_right = gamepad_button_down(gamepad_index, SDL_CONTROLLER_BUTTON_DPAD_RIGHT);

  controls.joyy = player_control_axis(left_y < -GLOOM_GAMEPAD_MOVE_THRESHOLD || dpad_up,
                                      left_y > GLOOM_GAMEPAD_MOVE_THRESHOLD || dpad_down);
  strafe = player_control_axis(left_x < -GLOOM_GAMEPAD_MOVE_THRESHOLD,
                               left_x > GLOOM_GAMEPAD_MOVE_THRESHOLD);
  turn = player_control_axis((player_index != 0u && right_x < -GLOOM_GAMEPAD_MOVE_THRESHOLD) || dpad_left,
                             (player_index != 0u && right_x > GLOOM_GAMEPAD_MOVE_THRESHOLD) || dpad_right);
  controls.fire = right_trigger > GLOOM_GAMEPAD_TRIGGER_THRESHOLD ||
                  gamepad_button_down(gamepad_index, SDL_CONTROLLER_BUTTON_A) ||
                  gamepad_button_down(gamepad_index, SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

  controls.turn = turn;
  if (strafe != 0) {
    controls.joyx = strafe;
    controls.joys = -1;
  }

  return controls;
}

static void merge_player_controls(PlayerControls *dst, const PlayerControls *src) {
  if (dst == NULL || src == NULL) {
    return;
  }
  if (src->joyy != 0) {
    dst->joyy = src->joyy;
  }
  if (src->joyx != 0 || src->joys != 0) {
    dst->joyx = src->joyx;
    dst->joys = src->joys;
  }
  if (src->turn != 0) {
    dst->turn = src->turn;
  }
  dst->fire = dst->fire || src->fire;
  dst->keyboard_fire_repeat = dst->keyboard_fire_repeat || src->keyboard_fire_repeat;
}

static int gamepad_look_delta(size_t gamepad_index) {
  SDL_GameController *controller = NULL;
  int32_t right_x = 0;

  if (gamepad_index >= (size_t)GLOOM_GAMEPAD_COUNT || g_gamepads[gamepad_index].controller == NULL) {
    return 0;
  }

  controller = g_gamepads[gamepad_index].controller;
  right_x = gamepad_axis_with_deadzone(SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX),
                                       GLOOM_GAMEPAD_LOOK_DEADZONE);
  return gamepad_axis_to_mouse_delta(right_x);
}

static int player1_gamepad_look_delta(void) {
  int gamepad_index = control_source_gamepad_index(g_control_config.player1);

  if (gamepad_index < 0) {
    return 0;
  }
  return gamepad_look_delta((size_t)gamepad_index);
}

static bool gamepad_menu_activate_event(const SDL_Event *event) {
  if (event == NULL || event->type != SDL_CONTROLLERBUTTONDOWN) {
    return false;
  }
  return event->cbutton.button == SDL_CONTROLLER_BUTTON_A || event->cbutton.button == SDL_CONTROLLER_BUTTON_START;
}

static bool gamepad_menu_back_event(const SDL_Event *event) {
  if (event == NULL || event->type != SDL_CONTROLLERBUTTONDOWN) {
    return false;
  }
  return event->cbutton.button == SDL_CONTROLLER_BUTTON_B || event->cbutton.button == SDL_CONTROLLER_BUTTON_BACK;
}

static void gamepad_handle_event(const SDL_Event *event) {
  if (event == NULL) {
    return;
  }
  if (event->type == SDL_CONTROLLERDEVICEADDED) {
    gamepad_handle_device_added(event->cdevice.which);
  } else if (event->type == SDL_CONTROLLERDEVICEREMOVED) {
    gamepad_handle_device_removed(event->cdevice.which);
  }
}

static int control_source_gamepad_index(GloomControlSource source) {
  if (source == GLOOM_CONTROL_GAMEPAD_1) {
    return 0;
  }
  if (source == GLOOM_CONTROL_GAMEPAD_2) {
    return 1;
  }
  return -1;
}

static const char *control_source_menu_name(GloomControlSource source) {
  if (source == GLOOM_CONTROL_GAMEPAD_1) {
    return "JOYSTICK 1";
  }
  if (source == GLOOM_CONTROL_GAMEPAD_2) {
    return "JOYSTICK 2";
  }
  return "KEYBOARD";
}

static GloomControlSource next_control_source(GloomControlSource source) {
  if (source == GLOOM_CONTROL_KEYBOARD) {
    return GLOOM_CONTROL_GAMEPAD_1;
  }
  if (source == GLOOM_CONTROL_GAMEPAD_1) {
    return GLOOM_CONTROL_GAMEPAD_2;
  }
  return GLOOM_CONTROL_KEYBOARD;
}

static void normalize_control_config(RuntimeControlConfig *config) {
  if (config == NULL) {
    return;
  }
  if (config->player1 < GLOOM_CONTROL_KEYBOARD || config->player1 > GLOOM_CONTROL_GAMEPAD_2) {
    config->player1 = GLOOM_CONTROL_KEYBOARD;
  }
  if (config->player2 < GLOOM_CONTROL_KEYBOARD || config->player2 > GLOOM_CONTROL_GAMEPAD_2) {
    config->player2 = GLOOM_CONTROL_GAMEPAD_1;
  }
  if (config->player1 == config->player2) {
    if (config->player1 == GLOOM_CONTROL_KEYBOARD) {
      config->player2 = GLOOM_CONTROL_GAMEPAD_1;
    } else if (config->player1 == GLOOM_CONTROL_GAMEPAD_1) {
      config->player2 = GLOOM_CONTROL_GAMEPAD_2;
    } else {
      config->player1 = GLOOM_CONTROL_GAMEPAD_1;
    }
  }
}

static void cycle_control_config(RuntimeControlConfig *config, unsigned player_index) {
  GloomControlSource *changed = NULL;
  GloomControlSource *other = NULL;
  GloomControlSource candidate = GLOOM_CONTROL_KEYBOARD;

  if (config == NULL || (player_index != 0u && player_index != 1u)) {
    return;
  }

  normalize_control_config(config);
  changed = player_index == 0u ? &config->player1 : &config->player2;
  other = player_index == 0u ? &config->player2 : &config->player1;
  candidate = next_control_source(*changed);
  if (candidate == *other) {
    if (candidate == GLOOM_CONTROL_GAMEPAD_1) {
      *other = GLOOM_CONTROL_GAMEPAD_2;
    } else if (candidate == GLOOM_CONTROL_GAMEPAD_2) {
      *other = GLOOM_CONTROL_GAMEPAD_1;
    } else {
      *other = GLOOM_CONTROL_GAMEPAD_1;
    }
  }
  *changed = candidate;
  normalize_control_config(config);
}

static bool player1_receives_keyboard_input(void) {
  normalize_control_config(&g_control_config);
  return g_control_config.player1 == GLOOM_CONTROL_KEYBOARD ||
         (g_control_config.player1 != GLOOM_CONTROL_KEYBOARD &&
          g_control_config.player2 != GLOOM_CONTROL_KEYBOARD);
}

#ifdef __EMSCRIPTEN__
EMSCRIPTEN_KEEPALIVE void SetWebRTCGuestConnected(int connected) {
  g_webrtc_guest_connected = connected != 0;
  if (!g_webrtc_guest_connected) {
    g_webrtc_guest_player2_input = 0u;
  }
}

EMSCRIPTEN_KEEPALIVE void SetWebRTCGuestPlayer2Input(unsigned int input) {
  g_webrtc_guest_player2_input = input;
}

EMSCRIPTEN_KEEPALIVE void SetWebMusicVolume(int percent) {
#if GLOOM_USE_SDL_MIXER
  if (percent < 0) {
    percent = 0;
  } else if (percent > 100) {
    percent = 100;
  }
  g_menu_music_volume_percent = percent;
  Mix_VolumeMusic((MIX_MAX_VOLUME * percent) / 100);
#else
  (void)percent;
#endif
}
#endif

static PlayerControls read_modern_player_controls(const uint8_t *keyboard, bool mouse_fire) {
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
  controls.keyboard_fire_repeat =
      keyboard[SDL_SCANCODE_SPACE] || keyboard[SDL_SCANCODE_LCTRL] || keyboard[SDL_SCANCODE_RCTRL];
  controls.fire = controls.keyboard_fire_repeat || mouse_fire;

  controls.turn = turn;
  if (strafe != 0) {
    controls.joyx = strafe;
    controls.joys = -1;
  }

  return controls;
}

static PlayerControls read_second_player_controls(const uint8_t *keyboard) {
  PlayerControls controls;
  int16_t strafe = 0;
  int16_t turn = 0;

  memset(&controls, 0, sizeof(controls));
  if (keyboard == NULL) {
    return controls;
  }

  controls.joyy = player_control_axis(keyboard[SDL_SCANCODE_I], keyboard[SDL_SCANCODE_K]);
  strafe = player_control_axis(keyboard[SDL_SCANCODE_U], keyboard[SDL_SCANCODE_O]);
  turn = player_control_axis(keyboard[SDL_SCANCODE_J], keyboard[SDL_SCANCODE_L]);
  controls.keyboard_fire_repeat = keyboard[SDL_SCANCODE_RETURN] || keyboard[SDL_SCANCODE_RCTRL];
  controls.fire = controls.keyboard_fire_repeat;

  controls.turn = turn;
  if (strafe != 0) {
    controls.joyx = strafe;
    controls.joys = -1;
  }

  return controls;
}

static PlayerControls read_webrtc_player2_controls(void) {
  PlayerControls controls;
  int16_t strafe = 0;
  int16_t turn = 0;

  memset(&controls, 0, sizeof(controls));
  if (!g_webrtc_guest_connected) {
    return controls;
  }

  controls.joyy = player_control_axis((g_webrtc_guest_player2_input & GLOOM_WEBRTC_INPUT_FORWARD) != 0u,
                                      (g_webrtc_guest_player2_input & GLOOM_WEBRTC_INPUT_BACK) != 0u);
  strafe = player_control_axis((g_webrtc_guest_player2_input & GLOOM_WEBRTC_INPUT_STRAFE_LEFT) != 0u,
                               (g_webrtc_guest_player2_input & GLOOM_WEBRTC_INPUT_STRAFE_RIGHT) != 0u);
  turn = player_control_axis((g_webrtc_guest_player2_input & GLOOM_WEBRTC_INPUT_LEFT) != 0u,
                             (g_webrtc_guest_player2_input & GLOOM_WEBRTC_INPUT_RIGHT) != 0u);
  controls.fire = (g_webrtc_guest_player2_input & GLOOM_WEBRTC_INPUT_FIRE) != 0u;
  controls.turn = turn;

  if (strafe != 0) {
    controls.joyx = strafe;
    controls.joys = -1;
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

static float fixed_step_amiga_scale(void) {
  return (float)GLOOM_AMIGA_GAME_TICK_HZ / (float)FIXED_TICK_HZ;
}

static int32_t scale_player_fixed_step_value(int32_t value) {
  float scaled = (float)value * fixed_step_amiga_scale();

  return scaled >= 0.0f ? (int32_t)(scaled + 0.5f) : (int32_t)(scaled - 0.5f);
}

static int32_t round_float_to_int32(float value) {
  return value >= 0.0f ? (int32_t)(value + 0.5f) : (int32_t)(value - 0.5f);
}

static float wrap_player_bounce_phase(float phase) {
  while (phase >= 65536.0f) {
    phase -= 65536.0f;
  }
  while (phase < 0.0f) {
    phase += 65536.0f;
  }
  return phase;
}

static void play_player_footstep_sfx(void) {
  audio_play_sfx(GLOOM_SFX_FOOTSTEP, 16, 0);
}

static void apply_mouse_look(AppState *state, int mouse_dx) {
  if (state == NULL || mouse_dx == 0 || state->player_dead) {
    return;
  }

  state->player_rot_fixed =
      wrap_player_rotation_fixed((int64_t)state->player_rot_fixed +
                                 ((int64_t)mouse_dx * (int64_t)GLOOM_MOUSE_LOOK_FIXED_PER_PIXEL));
  state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
}

static void update_player_keyboard_rotation(AppState *state, const PlayerControls *controls) {
  int16_t joyx = 0;
  int16_t joys = 0;
  int32_t rot_acc = scale_player_fixed_step_value(GLOOM_PLAYER_ROT_ACC);
  int32_t rot_rev_acc = scale_player_fixed_step_value(GLOOM_PLAYER_ROT_REV_ACC);
  int32_t rot_set_acc = scale_player_fixed_step_value(GLOOM_PLAYER_ROT_SET_ACC);
  int32_t max_rot_speed = scale_player_fixed_step_value(GLOOM_PLAYER_MAX_ROT_SPEED);

  if (state == NULL) {
    return;
  }

  if (controls != NULL) {
    joyx = controls->turn != 0 ? controls->turn : controls->joyx;
    joys = controls->turn != 0 ? 0 : controls->joys;
  }

  if (joys == 0 && joyx != 0) {
    int32_t accel = rot_acc;

    if ((state->player_rotspeed < 0 && joyx > 0) || (state->player_rotspeed > 0 && joyx < 0)) {
      accel = rot_rev_acc;
    }
    if (joyx < 0) {
      accel = -accel;
    }

    state->player_rotspeed += accel;
    if (state->player_rotspeed > max_rot_speed) {
      state->player_rotspeed = max_rot_speed;
    } else if (state->player_rotspeed < -max_rot_speed) {
      state->player_rotspeed = -max_rot_speed;
    }
  } else if (state->player_rotspeed < 0) {
    state->player_rotspeed += rot_set_acc;
    if (state->player_rotspeed > 0) {
      state->player_rotspeed = 0;
    }
  } else if (state->player_rotspeed > 0) {
    state->player_rotspeed -= rot_set_acc;
    if (state->player_rotspeed < 0) {
      state->player_rotspeed = 0;
    }
  }

  state->player_rot_fixed = wrap_player_rotation_fixed((int64_t)state->player_rot_fixed +
                                                       (int64_t)state->player_rotspeed);
  state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
}

static void advance_player_bounce(AppState *state) {
  uint16_t previous_phase = 0u;
  uint16_t bounce_phase = 0u;

  if (state == NULL) {
    return;
  }

  previous_phase = (uint16_t)state->player_bounce & 255u;
  state->player_bounce =
      wrap_player_bounce_phase(state->player_bounce + ((float)GLOOM_PLAYER_BOUNCE_STEP * fixed_step_amiga_scale()));
  bounce_phase = (uint16_t)state->player_bounce & 255u;

  if (previous_phase < 64u && bounce_phase >= 64u) {
    play_player_footstep_sfx();
  }
}

static void settle_player_bounce(AppState *state, bool play_footstep) {
  uint16_t bounce_phase = 0u;

  if (state == NULL || state->player_bounce == 0.0f) {
    return;
  }

  state->player_bounce =
      wrap_player_bounce_phase(state->player_bounce + ((float)GLOOM_PLAYER_UNBOUNCE_STEP * fixed_step_amiga_scale()));
  bounce_phase = (uint16_t)state->player_bounce;

  if ((bounce_phase & 127u) < (uint16_t)GLOOM_PLAYER_UNBOUNCE_STEP) {
    state->player_bounce = 0.0f;
    state->player_frame_fixed = 0u;
    if (play_footstep) {
      play_player_footstep_sfx();
    }
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
  state->camera_y = state->player_eye_y + bob;
}

static void capture_primary_player_state(const AppState *state, RuntimePlayerState *out_player) {
  if (state == NULL || out_player == NULL) {
    return;
  }

  out_player->camera_x = state->camera_x;
  out_player->camera_z = state->camera_z;
  out_player->camera_y = state->camera_y;
  out_player->camera_angle = state->camera_angle;
  out_player->player_rot_fixed = state->player_rot_fixed;
  out_player->player_rotspeed = state->player_rotspeed;
  out_player->player_bounce = state->player_bounce;
  out_player->player_frame_fixed = state->player_frame_fixed;
  out_player->player_eye_y = state->player_eye_y;
  out_player->player_hitpoints = state->player_hitpoints;
  out_player->player_lives = state->player_lives;
  out_player->player_dead = state->player_dead;
  out_player->player_death_phase = state->player_death_phase;
  out_player->player_death_timer = state->player_death_timer;
  out_player->player_weapon = state->player_weapon;
  out_player->player_reload = state->player_reload;
  out_player->player_bouncy_bullets = state->player_bouncy_bullets;
  out_player->player_reload_counter = state->player_reload_counter;
  out_player->player_keyboard_fire_repeat_counter = state->player_keyboard_fire_repeat_counter;
  out_player->player_weapon_flash = state->player_weapon_flash;
  out_player->player_palette_timer = state->player_palette_timer;
  out_player->player_mega_timer = state->player_mega_timer;
  out_player->player_invisible_timer = state->player_invisible_timer;
  out_player->player_thermo_timer = state->player_thermo_timer;
  out_player->player_hyper_timer = state->player_hyper_timer;
  out_player->player_last_fire = state->player_last_fire;
  out_player->player_pixsize = state->player_pixsize;
  out_player->player_pixsizeadd = state->player_pixsizeadd;
  out_player->player_pixsize_accum = state->player_pixsize_accum;
  out_player->player_respawn_x = state->player_respawn_x;
  out_player->player_respawn_z = state->player_respawn_z;
  out_player->player_respawn_rotation = state->player_respawn_rotation;
}

static void apply_primary_player_state(AppState *state, const RuntimePlayerState *player) {
  if (state == NULL || player == NULL) {
    return;
  }

  state->camera_x = player->camera_x;
  state->camera_z = player->camera_z;
  state->camera_y = player->camera_y;
  state->camera_angle = player->camera_angle;
  state->player_rot_fixed = player->player_rot_fixed;
  state->player_rotspeed = player->player_rotspeed;
  state->player_bounce = player->player_bounce;
  state->player_frame_fixed = player->player_frame_fixed;
  state->player_eye_y = player->player_eye_y;
  state->player_hitpoints = player->player_hitpoints;
  state->player_lives = player->player_lives;
  state->player_dead = player->player_dead;
  state->player_death_phase = player->player_death_phase;
  state->player_death_timer = player->player_death_timer;
  state->player_weapon = player->player_weapon;
  state->player_reload = player->player_reload;
  state->player_bouncy_bullets = player->player_bouncy_bullets;
  state->player_reload_counter = player->player_reload_counter;
  state->player_keyboard_fire_repeat_counter = player->player_keyboard_fire_repeat_counter;
  state->player_weapon_flash = player->player_weapon_flash;
  state->player_palette_timer = player->player_palette_timer;
  state->player_mega_timer = player->player_mega_timer;
  state->player_invisible_timer = player->player_invisible_timer;
  state->player_thermo_timer = player->player_thermo_timer;
  state->player_hyper_timer = player->player_hyper_timer;
  state->player_last_fire = player->player_last_fire;
  state->player_pixsize = player->player_pixsize;
  state->player_pixsizeadd = player->player_pixsizeadd;
  state->player_pixsize_accum = player->player_pixsize_accum;
  state->player_respawn_x = player->player_respawn_x;
  state->player_respawn_z = player->player_respawn_z;
  state->player_respawn_rotation = player->player_respawn_rotation;
}

static bool player_can_take_damage(const AppState *state) {
  return state != NULL && state->player_death_phase == GLOOM_PLAYER_DEATH_NONE && !state->player_dead &&
         state->player_hitpoints > 0;
}

static bool runtime_player_fields_can_take_damage(int16_t hitpoints, bool dead, uint8_t death_phase) {
  return death_phase == GLOOM_PLAYER_DEATH_NONE && !dead && hitpoints > 0;
}

static bool secondary_player_can_take_damage(const RuntimePlayerState *player) {
  return player != NULL && runtime_player_fields_can_take_damage(player->player_hitpoints, player->player_dead,
                                                                 player->player_death_phase);
}

static bool player_death_blocks_controls(const AppState *state) {
  if (state == NULL) {
    return false;
  }

  return state->player_death_phase == GLOOM_PLAYER_DEATH_FALLING ||
         state->player_death_phase == GLOOM_PLAYER_DEATH_DEAD_DELAY ||
         state->player_death_phase == GLOOM_PLAYER_DEATH_WAIT_RESTART ||
         state->player_death_phase == GLOOM_PLAYER_DEATH_OUT_OF_LIVES ||
         state->player_death_phase == GLOOM_PLAYER_DEATH_GAME_OVER;
}

static void start_player_death(AppState *state) {
  if (state == NULL || state->player_death_phase != GLOOM_PLAYER_DEATH_NONE) {
    return;
  }

  /* amiga/gloom2.s: playerdie -> redpal, clears ob_hitpoints, switches to playerdeath. */
  player_redpal(state);
  state->player_hitpoints = 0;
  state->player_dead = true;
  state->player_death_phase = GLOOM_PLAYER_DEATH_FALLING;
  state->player_death_timer = 0.0f;
  state->player_rotspeed = 0;
}

static void damage_player_squash(AppState *state) {
  if (!player_can_take_damage(state)) {
    return;
  }

  /* amiga/gloom2.s: playerlogic unresolved adjustpos path subtracts 1 HP, then playerdie at zero. */
  state->player_hitpoints -= 1;
  if (state->player_hitpoints <= 0) {
    start_player_death(state);
  } else {
    player_redpal(state);
  }
}

static void restart_player_after_death(AppState *state) {
  uint8_t saved_weapon = 0u;

  if (state == NULL) {
    return;
  }

  /* amiga/gloom2.s: waitrestart preserves ob_weapon while restoring player object defaults. */
  saved_weapon = state->player_weapon;
  state->camera_x = (float)state->player_respawn_x;
  state->camera_z = (float)state->player_respawn_z;
  state->player_rot_fixed = amiga_rotation_to_fixed(state->player_respawn_rotation);
  state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
  state->player_rotspeed = 0;
  state->player_bounce = 0.0f;
  state->player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  state->player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state->player_dead = false;
  state->player_death_phase = GLOOM_PLAYER_DEATH_INVINCIBLE;
  state->player_death_timer = (float)GLOOM_PLAYER_RESTART_INVINCIBLE_TICKS;
  state->player_weapon = saved_weapon;
  state->player_reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
  state->player_bouncy_bullets = 0u;
  state->player_reload_counter = 0.0f;
  state->player_weapon_flash = 0.0f;
  state->player_mega_timer = 0.0f;
  state->player_invisible_timer = 0.0f;
  state->player_thermo_timer = 0.0f;
  state->player_hyper_timer = 0.0f;
  state->player_pixsize = 0;
  state->player_pixsizeadd = 0;
  state->player_pixsize_accum = 0.0f;
  state->player_last_fire = true;
  update_player_camera_y(state);
}

static void advance_player_death_amiga_tick(AppState *state) {
  if (state == NULL) {
    return;
  }

  switch (state->player_death_phase) {
    case GLOOM_PLAYER_DEATH_FALLING:
      /* amiga/gloom2.s: playerdeath adds 4 to ob_rot and ob_eyey until ob_eyey reaches -32. */
      state->player_rot_fixed =
          wrap_player_rotation_fixed((int64_t)state->player_rot_fixed +
                                     ((int64_t)GLOOM_PLAYER_DEATH_ROT_STEP << 16));
      state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
      state->player_eye_y += (float)GLOOM_PLAYER_DEATH_EYE_STEP;
      if (state->player_eye_y >= (float)GLOOM_PLAYER_DEATH_EYE_CLAMP) {
        state->player_eye_y = (float)GLOOM_PLAYER_DEATH_EYE_CLAMP;
        state->player_death_phase = GLOOM_PLAYER_DEATH_DEAD_DELAY;
        state->player_death_timer = (float)GLOOM_PLAYER_DEAD_DELAY;
        if (state->combat_mode) {
          /* amiga/gloom2.s: playerdeath combat branch subtracts one life from the loser. */
          if (state->player_lives > 0) {
            state->player_lives -= 1;
          }
        } else if (state->player_lives > 0) {
          state->player_lives -= 1;
          if (state->two_player_mode && state->active_player_index == 0u && state->player_lives > 0) {
            state->player2.player_lives = state->player_lives;
          }
        }
      }
      break;

    case GLOOM_PLAYER_DEATH_DEAD_DELAY:
      state->player_death_timer -= 1.0f;
      if (state->player_death_timer <= 0.0f) {
        if (state->combat_mode) {
          /* amiga/gloom2.s: playerdead combat branch sets finished2=4 and pixels both players out. */
          state->finished2 = GLOOM_COMBAT_ROUND_FINISHED;
          state->finished = GLOOM_COMBAT_ROUND_FINISHED;
          state->player_pixsizeadd = 1;
          state->player2.player_pixsizeadd = 1;
        } else if (state->player_lives == 0) {
          if (state->two_player_mode && state->active_other_player_lives > 0) {
            state->player_death_phase = GLOOM_PLAYER_DEATH_OUT_OF_LIVES;
          } else {
            state->player_death_phase = GLOOM_PLAYER_DEATH_GAME_OVER;
            state->finished = 2;
          }
        } else {
          state->player_death_phase = GLOOM_PLAYER_DEATH_WAIT_RESTART;
        }
        state->player_death_timer = 0.0f;
      }
      break;

    case GLOOM_PLAYER_DEATH_INVINCIBLE:
      state->player_death_timer -= 1.0f;
      if (state->player_death_timer <= 0.0f) {
        state->player_death_phase = GLOOM_PLAYER_DEATH_NONE;
        state->player_death_timer = 0.0f;
      }
      break;

    default:
      break;
  }
}

static void update_player_death(AppState *state, const PlayerControls *controls) {
  uint32_t tick_count = 0u;
  uint32_t i = 0u;
  bool fire_pressed = false;

  if (state == NULL || state->player_death_phase == GLOOM_PLAYER_DEATH_NONE) {
    return;
  }

  tick_count = elapsed_amiga_ticks_for_sim_tick(state->tick_count);
  for (i = 0u; i < tick_count; ++i) {
    advance_player_death_amiga_tick(state);
  }

  if (state->player_death_phase != GLOOM_PLAYER_DEATH_WAIT_RESTART) {
    update_player_camera_y(state);
    return;
  }

  fire_pressed = controls != NULL && controls->fire;
  if (!fire_pressed) {
    state->player_last_fire = false;
    update_player_camera_y(state);
    return;
  }
  if (state->player_last_fire) {
    update_player_camera_y(state);
    return;
  }

  restart_player_after_death(state);
}

static void update_player_movement(AppState *state, const PlayerControls *controls) {
  float forward = 0.0f;
  float strafe = 0.0f;
  float move_speed = (float)GLOOM_PLAYER_MOVESPEED * fixed_step_amiga_scale();
  float sin_a = 0.0f;
  float cos_a = 0.0f;

  if (state == NULL || controls == NULL) {
    return;
  }

  if (controls->joyy == 0 && (controls->joys == 0 || controls->joyx == 0)) {
    settle_player_bounce(state, true);
    update_player_camera_y(state);
    return;
  }

  if (controls->joyy != 0) {
    forward = -(float)controls->joyy * move_speed;
  }
  if (controls->joys != 0 && controls->joyx != 0) {
    strafe = (float)controls->joyx * move_speed;
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
  state->player_frame_fixed =
      (uint32_t)((int32_t)state->player_frame_fixed +
                 (int32_t)((float)GLOOM_PLAYER_FRAME_SPEED_FIXED * fixed_step_amiga_scale())) &
      0x0003ffffu;
  update_player_camera_y(state);
}

static uint16_t runtime_random_word(AppState *state) {
  if (state == NULL) {
    return 0u;
  }

  state->rng_state = state->rng_state * 1103515245u + 12345u;
  return (uint16_t)(state->rng_state >> 16u);
}

static float runtime_spark_velocity(AppState *state) {
  return (float)(int16_t)runtime_random_word(state) / 2048.0f;
}

static float runtime_bloodspeed(AppState *state, uint8_t left_shift) {
  return (float)(int16_t)runtime_random_word(state) / (float)(1u << (16u - left_shift));
}

static void spawn_runtime_bloodymess(AppState *state, const RuntimeObjectState *object,
                                     const ObjectCombatDefinition *combat, uint16_t dbf_count, uint8_t velocity_shift) {
  float origin_x = 0.0f;
  float origin_y = 0.0f;
  float origin_z = 0.0f;
  uint16_t n = 0u;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  origin_x = object->x + runtime_bloodspeed(state, 5u);
  origin_y = (float)combat->guts_y + runtime_bloodspeed(state, 5u);
  origin_z = object->z + runtime_bloodspeed(state, 5u);

  for (n = 0u; n <= dbf_count; ++n) {
    size_t i = 0u;

    for (i = 0u; i < GLOOM_MAX_RUNTIME_BLOOD; ++i) {
      RuntimeBlood *blood = &state->blood[i];

      if (blood->active) {
        continue;
      }

      memset(blood, 0, sizeof(*blood));
      blood->active = true;
      blood->x = origin_x;
      blood->y = origin_y;
      blood->z = origin_z;
      blood->vx = runtime_bloodspeed(state, velocity_shift);
      blood->vy = runtime_bloodspeed(state, velocity_shift);
      blood->vz = runtime_bloodspeed(state, velocity_shift);
      blood->color_mask = combat->blood_color_mask;
      break;
    }

    if (i == GLOOM_MAX_RUNTIME_BLOOD) {
      return;
    }
  }
}

static void runtime_hurtobject(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat) {
  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  spawn_runtime_bloodymess(state, object, combat, 23u, 2u);
  if (combat->hurt_pause_ticks > 0) {
    object->hurt_pause = (float)combat->hurt_pause_ticks;
    object->frame_fixed = 4u << 16;
  }
}

static void runtime_play_hurt_sfx(AppState *state, int16_t object_type) {
  static const int grunt_sfx[4] = {GLOOM_SFX_GRUNT, GLOOM_SFX_GRUNT2, GLOOM_SFX_GRUNT3, GLOOM_SFX_GRUNT4};
  uint16_t index = 0u;

  if (state == NULL) {
    return;
  }

  switch (object_type) {
    case GLOOM_OBJECT_TYPE_TERRA:
      audio_play_sfx(GLOOM_SFX_SHOOT2, 64, 2);
      return;
    case GLOOM_OBJECT_TYPE_LIZARD:
      audio_play_sfx(GLOOM_SFX_LIZHIT, 64, 1);
      return;
    case GLOOM_OBJECT_TYPE_TROLL:
      audio_play_sfx(GLOOM_SFX_TROLLHIT, 64, 1);
      return;
    case 10:
    case GLOOM_OBJECT_TYPE_BALDY:
    case GLOOM_OBJECT_TYPE_PHANTOM:
    case GLOOM_OBJECT_TYPE_DEMON:
      index = runtime_random_word(state) & 3u;
      if (index == g_audio.last_grunt) {
        index = (index + 1u) & 3u;
      }
      g_audio.last_grunt = (uint8_t)index;
      audio_play_sfx(grunt_sfx[index], 64, 1);
      return;
    default:
      return;
  }
}

static void runtime_spawn_soul(AppState *state, const RuntimeObjectState *object, uint16_t object_index) {
  float dir_x = 0.0f;
  float dir_z = 0.0f;
  float origin_x = 0.0f;
  float origin_z = 0.0f;
  int n = 0;

  if (state == NULL || object == NULL) {
    return;
  }

  runtime_object_dir((int16_t)((object->rotation + 128) & 255), &dir_x, &dir_z);
  origin_x = state->camera_x + (-dir_x * 64.0f);
  origin_z = state->camera_z + (dir_z * 64.0f);

  for (n = 0; n <= 3; ++n) {
    size_t i = 0u;

    for (i = 0u; i < GLOOM_MAX_RUNTIME_BLOOD; ++i) {
      RuntimeBlood *blood = &state->blood[i];

      if (blood->active) {
        continue;
      }

      memset(blood, 0, sizeof(*blood));
      blood->active = true;
      blood->soul = true;
      blood->x = origin_x + (float)((int16_t)(runtime_random_word(state) & 63u) - 32);
      blood->y = 110.0f + (float)((int16_t)(runtime_random_word(state) & 63u) - 32);
      blood->z = origin_z + (float)((int16_t)(runtime_random_word(state) & 63u) - 32);
      blood->vx = -dir_x * 32.0f;
      blood->vz = dir_z * 32.0f;
      blood->vy = (float)object_index;
      blood->color_mask = (runtime_random_word(state) & 1u) != 0u ? 0x00ffu : 0x00f0u;
      break;
    }

    if (i == GLOOM_MAX_RUNTIME_BLOOD) {
      return;
    }
  }
}

static void runtime_start_deathsuck(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                    uint16_t object_index) {
  if (state == NULL || object == NULL || combat == NULL || state->death_suck_active || !player_can_take_damage(state)) {
    return;
  }

  state->death_suck_active = true;
  state->death_sucker_index = object_index;
  object->old_rotation = object->rotation;
  object->old_logic_state = object->logic_state;
  object->logic_state = GLOOM_OBJECT_LOGIC_DEATHSUCK;
  object->delay = 64.0f;
}

static void runtime_apply_object_hit(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                     int16_t object_type, uint16_t object_index) {
  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  switch (object_type) {
    case GLOOM_OBJECT_TYPE_DRAGON:
      return;
    case GLOOM_OBJECT_TYPE_GHOUL:
      spawn_runtime_bloodymess(state, object, combat, 31u, 2u);
      return;
    case GLOOM_OBJECT_TYPE_DEATHHEAD:
      runtime_start_deathsuck(state, object, combat, object_index);
      return;
    default:
      runtime_play_hurt_sfx(state, object_type);
      runtime_hurtobject(state, object, combat);
      return;
  }
}

static void spawn_runtime_blowchunx(AppState *state, const RuntimeObjectState *object, const ObjectVisual *chunk_visual,
                                    int16_t object_type) {
  uint16_t frame_count = 0u;
  int frame = 0;

  if (state == NULL || object == NULL || chunk_visual == NULL || !chunk_visual->loaded ||
      chunk_visual->frame_count_per_rotation == 0u) {
    return;
  }

  frame_count = chunk_visual->frame_count_per_rotation;
  for (frame = (int)frame_count - 1; frame >= 0; --frame) {
    size_t i = 0u;

    for (i = 0u; i < GLOOM_MAX_RUNTIME_CHUNKS; ++i) {
      RuntimeChunk *chunk = &state->chunks[i];

      if (chunk->active) {
        continue;
      }

      memset(chunk, 0, sizeof(*chunk));
      chunk->active = true;
      chunk->object_type = object_type;
      chunk->frame_number = (uint16_t)frame;
      chunk->scale = chunk_visual->scale;
      chunk->radius = (int16_t)chunk_visual->collision_radius;
      chunk->x = object->x;
      chunk->y = -64.0f;
      chunk->z = object->z;
      chunk->vx = runtime_bloodspeed(state, 4u);
      chunk->vy = runtime_bloodspeed(state, 4u) - 4.0f;
      chunk->vz = runtime_bloodspeed(state, 4u);
      break;
    }

    if (i == GLOOM_MAX_RUNTIME_CHUNKS) {
      fprintf(stderr, "Failed to port blowchunx fully: runtime chunk list is full while spawning object type %d\n",
              object_type);
      return;
    }
  }
}

static void runtime_blowobject(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                               const ObjectVisualSet *object_visuals, int16_t object_type) {
  const ObjectVisual *chunk_visual = NULL;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  if (object_type == GLOOM_OBJECT_TYPE_DEATHHEAD && state->death_suck_active) {
    state->death_suck_active = false;
  }

  if (combat->death_routine == GLOOM_OBJECT_DIE_BLOWTERRA) {
    audio_play_sfx(GLOOM_SFX_ROBODIE, 64, 20);
  } else {
    audio_play_sfx(GLOOM_SFX_DIE, 64, 2);
  }
  spawn_runtime_bloodymess(state, object, combat, 31u, 4u);
  if (object_type_has_blowchunx(object_type)) {
    if (object_visuals == NULL || object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
      fprintf(stderr, "Failed to port blowchunx: missing chunk visual set for object type %d\n", object_type);
      return;
    }

    chunk_visual = &object_visuals->chunks[object_type];
    if (!chunk_visual->loaded) {
      fprintf(stderr, "Failed to port blowchunx: chunk asset '%s' for object type %d was not loaded\n",
              object_chunk_asset_name(object_type), object_type);
      return;
    }

    spawn_runtime_blowchunx(state, object, chunk_visual, object_type);
  } else {
    spawn_runtime_bloodymess(state, object, combat, 15u, 4u);
  }
  object->active = false;
}

static void runtime_blowdragon(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                               const ObjectVisualSet *object_visuals, int16_t object_type) {
  const ObjectVisual *chunk_visual = NULL;
  int pass = 0;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  audio_play_sfx(GLOOM_SFX_DIE, 64, 50);
  audio_play_sfx(GLOOM_SFX_DIE, 64, 50);
  audio_play_sfx(GLOOM_SFX_ROBODIE, 64, 50);
  audio_play_sfx(GLOOM_SFX_ROBODIE, 64, 50);
  spawn_runtime_bloodymess(state, object, combat, 63u, 4u);
  if (object_visuals == NULL || object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    fprintf(stderr, "Failed to port blowdragon: missing chunk visual set for object type %d\n", object_type);
    return;
  }

  chunk_visual = &object_visuals->chunks[object_type];
  if (!chunk_visual->loaded) {
    fprintf(stderr, "Failed to port blowdragon: chunk asset '%s' for object type %d was not loaded\n",
            object_chunk_asset_name(object_type), object_type);
    return;
  }

  for (pass = 0; pass < 4; ++pass) {
    spawn_runtime_blowchunx(state, object, chunk_visual, object_type);
  }
  object->enemy = false;
  object->active = false;
  state->dragon_dead_delay = 127.0f;
  state->dragon_finished_reported = false;
}

static void runtime_damage_object(AppState *state, const ObjectVisualSet *object_visuals, RuntimeObjectState *object,
                                  const ObjectCombatDefinition *combat, int16_t object_type, uint16_t object_index,
                                  int16_t damage) {
  if (state == NULL || object == NULL || combat == NULL || damage <= 0 || !object->active) {
    return;
  }

  object->hitpoints -= damage;
  if (object->hitpoints <= 0) {
    if (combat->death_routine == GLOOM_OBJECT_DIE_BLOWDRAGON) {
      runtime_blowdragon(state, object, combat, object_visuals, object_type);
    } else if (combat->death_routine == GLOOM_OBJECT_DIE_BLOWOBJECT ||
               combat->death_routine == GLOOM_OBJECT_DIE_BLOWQUICK ||
               combat->death_routine == GLOOM_OBJECT_DIE_BLOWDEATH ||
               combat->death_routine == GLOOM_OBJECT_DIE_BLOWTERRA) {
      runtime_blowobject(state, object, combat, object_visuals, object_type);
    } else {
      fprintf(stderr, "Cannot kill object type %d: original death routine is not represented in the PC runtime\n",
              object_type);
    }
  } else {
    runtime_apply_object_hit(state, object, combat, object_type, object_index);
  }
}

static int16_t radians_to_amiga_rotation(float radians) {
  int32_t unit = 0;

  while (radians < 0.0f) {
    radians += 6.28318530718f;
  }
  while (radians >= 6.28318530718f) {
    radians -= 6.28318530718f;
  }

  unit = round_float_to_int32((radians * 256.0f) / 6.28318530718f) & 255;
  return (int16_t)unit;
}

static void spawn_runtime_sparks(AppState *state, uint8_t weapon_index, float x, float y, float z) {
  const WeaponDefinition *weapons = weapon_definitions();
  uint16_t frame_count = 0u;
  uint16_t frame = 0u;

  if (state == NULL || weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
    return;
  }

  frame_count = weapons[weapon_index].spark_frame_count;
  for (frame = 0u; frame < frame_count; ++frame) {
    size_t i = 0u;

    for (i = 0u; i < GLOOM_MAX_RUNTIME_SPARKS; ++i) {
      RuntimeSpark *spark = &state->sparks[i];

      if (spark->active) {
        continue;
      }

      memset(spark, 0, sizeof(*spark));
      spark->active = true;
      spark->weapon_index = weapon_index;
      spark->x = x;
      spark->y = y;
      spark->z = z;
      spark->vx = runtime_spark_velocity(state);
      spark->vy = runtime_spark_velocity(state);
      spark->vz = runtime_spark_velocity(state);
      spark->frame_index = frame;
      spark->lifetime = 15.0f + (float)(runtime_random_word(state) & 15u);
      break;
    }
  }
}

static void runtime_object_dir(int16_t rotation, float *out_x, float *out_z) {
  float angle = amiga_rotation_to_radians(rotation);

  if (out_x != NULL) {
    *out_x = SDL_sinf(angle);
  }
  if (out_z != NULL) {
    *out_z = SDL_cosf(angle);
  }
}

static int16_t runtime_angle_to_player(const AppState *state, const RuntimeObjectState *object) {
  float target_x = 0.0f;
  float target_z = 0.0f;

  if (state == NULL || object == NULL) {
    return 0;
  }

  target_x = state->camera_x;
  target_z = state->camera_z;
  if (state->two_player_mode && secondary_player_can_take_damage(&state->player2)) {
    bool use_player2 = !player_can_take_damage(state);

    if (!use_player2) {
      float p1_dx = state->camera_x - object->x;
      float p1_dz = state->camera_z - object->z;
      float p2_dx = state->player2.camera_x - object->x;
      float p2_dz = state->player2.camera_z - object->z;
      use_player2 = (p2_dx * p2_dx + p2_dz * p2_dz) < (p1_dx * p1_dx + p1_dz * p1_dz);
    }
    if (use_player2) {
      target_x = state->player2.camera_x;
      target_z = state->player2.camera_z;
    }
  }

  /* amiga/gloom2.s: pickplayer chooses the live nearest player before aiming. */
  return radians_to_amiga_rotation(SDL_atan2f(target_x - object->x, target_z - object->z));
}

static bool runtime_object_checkvecs(AppState *state, RuntimeObjectState *object, float move) {
  float dir_x = 0.0f;
  float dir_z = 0.0f;
  float new_x = 0.0f;
  float new_z = 0.0f;
  uint16_t penetration = 0u;
  size_t zone_index = 0u;

  if (state == NULL || object == NULL || move <= 0.0f) {
    return false;
  }

  runtime_object_dir(object->rotation, &dir_x, &dir_z);
  new_x = object->x + dir_x * move;
  new_z = object->z + dir_z * move;
  if (find_wall_collision_radius(state, new_x, new_z, object->radius, &penetration, &zone_index)) {
    float old_x = object->x;
    float old_z = object->z;

    (void)penetration;
    (void)zone_index;
    if (!find_wall_collision_radius(state, old_x, old_z, object->radius, &penetration, &zone_index)) {
      return false;
    }
    if (!resolve_wall_collision_radius(state, &old_x, &old_z, object->radius)) {
      return false;
    }
    object->x = old_x;
    object->z = old_z;
    return false;
  }

  object->x = new_x;
  object->z = new_z;
  return true;
}

static bool runtime_object_monsterfix(AppState *state, RuntimeObjectState *object, float move) {
  int16_t old_rotation = 0;
  int16_t turn = 64;

  if (state == NULL || object == NULL) {
    return false;
  }

  old_rotation = object->old_rotation;
  if ((int16_t)runtime_random_word(state) < 0) {
    turn = -64;
  }

  object->rotation = (int16_t)((object->rotation + turn) & 255);
  if (runtime_object_checkvecs(state, object, move)) {
    return true;
  }

  object->rotation = (int16_t)((object->rotation + 128) & 255);
  if (runtime_object_checkvecs(state, object, move)) {
    return true;
  }

  object->rotation = (int16_t)((old_rotation + 128) & 255);
  return runtime_object_checkvecs(state, object, move);
}

static void runtime_set_random_delay(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat) {
  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  object->delay = (float)combat->base_delay;
  if (combat->delay_range > 0) {
    object->delay += (float)(runtime_random_word(state) % (uint16_t)combat->delay_range);
  }
}

static void runtime_advance_monster_frame(RuntimeObjectState *object, int32_t frame_speed_fixed, float step_scale) {
  if (object == NULL) {
    return;
  }

  object->frame_fixed =
      (uint32_t)((int32_t)object->frame_fixed + (int32_t)((float)frame_speed_fixed * step_scale)) & 0x0003ffffu;
}

static bool runtime_object_collides_player_at_index(const AppState *state, const RuntimeObjectState *object,
                                                   int player_index) {
  float dx = 0.0f;
  float dz = 0.0f;
  float radius = 0.0f;
  float player_x = 0.0f;
  float player_z = 0.0f;

  if (state == NULL || object == NULL) {
    return false;
  }

  if (player_index == 0) {
    if (!player_can_take_damage(state)) {
      return false;
    }
    player_x = state->camera_x;
    player_z = state->camera_z;
  } else if (player_index == 1) {
    if (!state->two_player_mode || !secondary_player_can_take_damage(&state->player2)) {
      return false;
    }
    player_x = state->player2.camera_x;
    player_z = state->player2.camera_z;
  } else {
    return false;
  }

  radius = (float)(object->radius + state->player_radius);
  dx = player_x - object->x;
  dz = player_z - object->z;
  return SDL_fabsf(dx) < radius && SDL_fabsf(dz) < radius && (dx * dx + dz * dz) < (radius * radius);
}

static bool runtime_object_collides_player(const AppState *state, const RuntimeObjectState *object) {
  return runtime_object_collides_player_index(state, object) >= 0;
}

static int runtime_object_collides_player_index(const AppState *state, const RuntimeObjectState *object) {
  if (runtime_object_collides_player_at_index(state, object, 0)) {
    return 0;
  }
  if (runtime_object_collides_player_at_index(state, object, 1)) {
    return 1;
  }
  return -1;
}

static void damage_player_from_object_index(AppState *state, const RuntimeObjectState *object, int player_index) {
  if (state == NULL || object == NULL || object->damage <= 0) {
    return;
  }

  if (player_index == 0) {
    if (!player_can_take_damage(state)) {
      return;
    }
    state->player_hitpoints -= object->damage;
    if (state->player_hitpoints < 0) {
      state->player_hitpoints = 0;
    }
    player_redpal(state);
    if (state->player_hitpoints == 0) {
      start_player_death(state);
    }
  } else if (player_index == 1) {
    if (!state->two_player_mode || !secondary_player_can_take_damage(&state->player2)) {
      return;
    }
    state->player2.player_hitpoints -= object->damage;
    if (state->player2.player_hitpoints < 0) {
      state->player2.player_hitpoints = 0;
    }
    state->player2.player_palette_timer = (float)GLOOM_PLAYER_RED_PAL_TIMER;
  }
}

static void damage_player_from_object(AppState *state, const RuntimeObjectState *object) {
  int player_index = 0;

  if (state == NULL || object == NULL || object->damage <= 0) {
    return;
  }

  player_index = runtime_object_collides_player_index(state, object);
  if (player_index < 0) {
    player_index = 0;
  }
  damage_player_from_object_index(state, object, player_index);
}

static void runtime_update_object_player_contacts(AppState *state, const ObjectVisualSet *object_visuals) {
  size_t i = 0u;
  size_t count = 0u;

  if (state == NULL) {
    return;
  }

  count = state->map.object_spawn_count;
  if (count > (size_t)GLOOM_MAX_RUNTIME_OBJECTS) {
    count = (size_t)GLOOM_MAX_RUNTIME_OBJECTS;
  }

  for (i = 0u; i < count; ++i) {
    const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
    RuntimeObjectState *object = &state->objects[i];
    const ObjectCombatDefinition *combat = NULL;
    int player_index = -1;
    uint8_t contact_marker = 0u;

    if (!object->active || !object->enemy) {
      continue;
    }
    if (spawn->object_type < 0 || spawn->object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
      continue;
    }

    /*
     * amiga/gloom2.s: obj_loop/checkcoll tests enemy ob_collwith against player
     * ob_colltype, stores ob_washit for the same overlapping pair, and only re-arms
     * contact damage after separation.
     */
    player_index = runtime_object_collides_player_index(state, object);
    if (player_index < 0) {
      object->contact_was_hit_player = 0u;
      continue;
    }

    contact_marker = (uint8_t)(player_index + 1);
    if (object->contact_was_hit_player == contact_marker) {
      continue;
    }
    object->contact_was_hit_player = contact_marker;
    if (state->finished2 != 0) {
      continue;
    }

    combat = object_type_combat_definition(spawn->object_type);
    damage_player_from_object_index(state, object, player_index);
    if (object->active && combat != NULL) {
      runtime_damage_object(state, object_visuals, object, combat, spawn->object_type, (uint16_t)i,
                            GLOOM_PLAYER_CONTACT_DAMAGE);
    }
  }
}

static void runtime_baldy_family_to_normal(AppState *state, RuntimeObjectState *object,
                                           const ObjectCombatDefinition *combat, float step_scale) {
  float move = 0.0f;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  object->logic_state = GLOOM_OBJECT_LOGIC_NORMAL;
  object->move_speed = combat->move_speed;
  object->frame_speed_fixed = (int32_t)combat->frame_speed_fixed;
  runtime_set_random_delay(state, object, combat);
  move = object->move_speed * step_scale;
  (void)runtime_object_monsterfix(state, object, move);
}

static void runtime_baldy_family_enter_charge(RuntimeObjectState *object, const ObjectCombatDefinition *combat) {
  if (object == NULL || combat == NULL) {
    return;
  }

  object->rotation = (int16_t)(object->rotation & 255);
  object->move_speed = combat->move_speed * 4.0f;
  object->frame_speed_fixed = (int32_t)combat->frame_speed_fixed * 4;
  object->logic_state = GLOOM_OBJECT_LOGIC_BALDYCHARGE;
}

static void update_baldy_family_object(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                       int16_t object_type, float step_scale) {
  float move = 0.0f;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  if (object->logic_state == GLOOM_OBJECT_LOGIC_BALDYPUNCH) {
    if (!runtime_object_collides_player(state, object)) {
      object->frame_fixed = 0u;
      runtime_baldy_family_to_normal(state, object, combat, step_scale);
      return;
    }
    if (object->delay > 0.0f) {
      return;
    }

    object->delay = (float)object_type_punch_rate(object_type);
    if ((object->frame_fixed >> 16u) == 0u) {
      object->rotation = runtime_angle_to_player(state, object);
      object->frame_fixed = 5u << 16;
      /* amiga/gloom2.s: baldypunch clears ob_washit; obj_loop applies the contact damage. */
      object->contact_was_hit_player = 0u;
    } else {
      object->frame_fixed = 0u;
    }
    return;
  }

  if (object->logic_state == GLOOM_OBJECT_LOGIC_BALDYCHARGE) {
    int16_t target_rotation = 0;
    int16_t diff = 0;

    move = object->move_speed * step_scale;
    if (!runtime_object_checkvecs(state, object, move)) {
      runtime_baldy_family_to_normal(state, object, combat, step_scale);
      return;
    }

    target_rotation = runtime_angle_to_player(state, object);
    diff = (int16_t)(target_rotation - object->rotation);
    if (diff > 32 || diff < -32) {
      runtime_baldy_family_to_normal(state, object, combat, step_scale);
      return;
    }

    {
      int player_index = runtime_object_collides_player_index(state, object);

      if (player_index >= 0) {
        object->contact_was_hit_player = (uint8_t)(player_index + 1);
        object->logic_state = GLOOM_OBJECT_LOGIC_BALDYPUNCH;
        object->delay = (float)object_type_punch_rate(object_type);
        object->frame_fixed = 0u;
        return;
      }
    }

    runtime_advance_monster_frame(object, object->frame_speed_fixed, step_scale);
    return;
  }

  if (object->delay <= 0.0f) {
    object->rotation = runtime_angle_to_player(state, object);
    if (object_type == GLOOM_OBJECT_TYPE_LIZARD) {
      float dx = object->x - state->camera_x;
      float dz = object->z - state->camera_z;
      if (dx * dx + dz * dz < 256.0f * 256.0f) {
        audio_play_sfx(GLOOM_SFX_LIZARD, 32, 5);
      }
    } else if (object_type == GLOOM_OBJECT_TYPE_TROLL) {
      float dx = object->x - state->camera_x;
      float dz = object->z - state->camera_z;
      if (dx * dx + dz * dz < 320.0f * 320.0f) {
        audio_play_sfx(GLOOM_SFX_TROLLMAD, 64, 5);
      }
    }
    runtime_baldy_family_enter_charge(object, combat);
    return;
  }

  object->old_rotation = object->rotation;
  move = object->move_speed * step_scale;
  if (!runtime_object_checkvecs(state, object, move)) {
    (void)runtime_object_monsterfix(state, object, move);
  }
  runtime_advance_monster_frame(object, object->frame_speed_fixed, step_scale);
}

static int16_t runtime_abs_rotation_delta(int16_t a, int16_t b) {
  int16_t delta = (int16_t)((a & 255) - (b & 255));

  if (delta < 0) {
    delta = (int16_t)-delta;
  }
  return delta;
}

static void runtime_deathbounce(RuntimeObjectState *object, float step_scale) {
  float angle = 0.0f;

  if (object == NULL) {
    return;
  }

  object->bounce_phase += 4.0f * step_scale;
  while (object->bounce_phase >= 256.0f) {
    object->bounce_phase -= 256.0f;
  }
  angle = amiga_rotation_to_radians((int16_t)round_float_to_int32(object->bounce_phase));
  object->y = -48.0f + SDL_sinf(angle) * 64.0f;
}

static void runtime_deathanim(RuntimeObjectState *object, float step_scale) {
  int32_t frame = 0;
  int32_t speed = 0;

  if (object == NULL) {
    return;
  }

  frame = (int32_t)object->frame_fixed + (int32_t)((float)object->frame_speed_fixed * step_scale);
  speed = object->frame_speed_fixed;
  if (frame < 0x8000 || frame >= 0x28000) {
    speed = -speed;
    frame += speed;
    object->frame_speed_fixed = speed;
  }
  object->frame_fixed = (uint32_t)frame;
}

static void update_deathhead_object(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                    float step_scale) {
  float move = 0.0f;
  int16_t target_rotation = 0;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  runtime_deathbounce(object, step_scale);

  if (object->logic_state == GLOOM_OBJECT_LOGIC_DEATHSUCK) {
    runtime_deathanim(object, step_scale);
    object->delay -= step_scale;
    if (object->delay <= 0.0f) {
      object->rotation = object->old_rotation;
      object->logic_state = object->old_logic_state;
      state->death_suck_active = false;
      runtime_set_random_delay(state, object, combat);
      return;
    }

    object->rotation = runtime_angle_to_player(state, object);
    runtime_spawn_soul(state, object, state->death_sucker_index);
    return;
  }

  if (object->logic_state == GLOOM_OBJECT_LOGIC_DEATHCHARGE) {
    runtime_deathanim(object, step_scale);
    target_rotation = runtime_angle_to_player(state, object);
    if (runtime_abs_rotation_delta(target_rotation, object->rotation) >= 128) {
      object->logic_state = GLOOM_OBJECT_LOGIC_NORMAL;
      object->frame_fixed = 0x8000u;
      runtime_set_random_delay(state, object, combat);
      return;
    }

    move = object->move_speed * step_scale;
    if (!runtime_object_checkvecs(state, object, move)) {
      object->rotation = (int16_t)((object->rotation + 128) & 255);
      object->logic_state = GLOOM_OBJECT_LOGIC_NORMAL;
      object->frame_fixed = 0x8000u;
      runtime_set_random_delay(state, object, combat);
    }
    return;
  }

  move = object->move_speed * step_scale;
  if (!runtime_object_checkvecs(state, object, move)) {
    object->rotation = (int16_t)((object->rotation + 128) & 255);
    runtime_set_random_delay(state, object, combat);
  } else {
    target_rotation = runtime_angle_to_player(state, object);
    if (runtime_abs_rotation_delta(object->rotation, target_rotation) < 16) {
      object->rotation = target_rotation;
      object->logic_state = GLOOM_OBJECT_LOGIC_DEATHCHARGE;
      return;
    }
  }

  object->rotation = (int16_t)((object->rotation + (int16_t)object->delay) & 255);
}

static void runtime_monstermove(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                float step_scale) {
  float move = 0.0f;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  object->old_rotation = object->rotation;
  move = object->move_speed * step_scale;
  if (!runtime_object_checkvecs(state, object, move)) {
    (void)runtime_object_monsterfix(state, object, move);
  }
  runtime_advance_monster_frame(object, object->frame_speed_fixed, step_scale);
}

static void update_terra_object(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                float step_scale) {
  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  if (object->logic_state == GLOOM_OBJECT_LOGIC_TERRAFIRE) {
    object->delay -= step_scale;
    if (object->delay > 0.0f) {
      return;
    }

    object->delay = 12.0f;
    object->rotation = runtime_angle_to_player(state, object);
    (void)spawn_enemy_projectile_params(state, object, 3u, 1, 3, 16.0f, -60.0f);
    audio_play_sfx(GLOOM_SFX_SHOOT3, 32, 5);
    object->delay2 -= 1.0f;
    if (object->delay2 <= 0.0f) {
      runtime_set_random_delay(state, object, combat);
      object->logic_state = GLOOM_OBJECT_LOGIC_NORMAL;
    }
    return;
  }

  object->old_rotation = object->rotation;
  object->delay -= step_scale;
  if (object->delay <= 0.0f) {
    object->frame_fixed = 0u;
    object->delay = 1.0f;
    object->delay2 = 5.0f;
    object->logic_state = GLOOM_OBJECT_LOGIC_TERRAFIRE;
    return;
  }

  if ((((int)SDL_ceilf(object->delay)) & 31) == 0) {
    audio_play_sfx(GLOOM_SFX_ROBOT, 64, 10);
  }
  runtime_monstermove(state, object, combat, step_scale);
}

static void update_ghoul_object(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                float step_scale) {
  float dir_x = 0.0f;
  float dir_z = 0.0f;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  object->bounce_phase += 8.0f * step_scale;
  while (object->bounce_phase >= 256.0f) {
    object->bounce_phase -= 256.0f;
  }
  object->y = -32.0f + SDL_sinf(amiga_rotation_to_radians((int16_t)round_float_to_int32(object->bounce_phase))) * 32.0f;
  object->rotation = runtime_angle_to_player(state, object);

  object->delay -= step_scale;
  if (object->delay <= 0.0f) {
    object->frame_fixed = 1u << 16;
    object->frame_speed_fixed = 0x2000;
    (void)spawn_enemy_projectile_params(state, object, 1u, 1, 3, 20.0f, -64.0f);
    runtime_set_random_delay(state, object, combat);
  }

  if (runtime_random_word(state) < (uint16_t)((uint16_t)object->move_speed << 8u)) {
    runtime_object_dir(object->rotation, &dir_x, &dir_z);
    object->vx = dir_x * object->move_speed;
    object->vz = dir_z * object->move_speed;
    audio_play_sfx(GLOOM_SFX_GHOUL, 32, -5);
  }
  object->x += object->vx * step_scale;
  object->z += object->vz * step_scale;

  if (object->frame_speed_fixed != 0) {
    runtime_advance_monster_frame(object, object->frame_speed_fixed, step_scale);
    if ((object->frame_fixed >> 16u) >= 3u) {
      object->frame_fixed = 0u;
      object->frame_speed_fixed = 0;
    }
  }
}

static void update_phantom_object(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                  float step_scale) {
  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  object->old_rotation = object->rotation;
  object->delay -= step_scale;
  if (object->delay <= 0.0f) {
    object->rotation = runtime_angle_to_player(state, object);
    object->pause_delay = 7.0f;
    object->frame_fixed = 5u << 16;
    (void)spawn_enemy_projectile_params(state, object, 2u, 1, 3, 20.0f, -60.0f);
    return;
  }

  runtime_monstermove(state, object, combat, step_scale);
}

static int16_t demon_wtable_scaled_damage(uint8_t weapon_index) {
  const WeaponDefinition *weapons = weapon_definitions();

  if (weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
    return 0;
  }
  return (int16_t)(((int32_t)weapons[weapon_index].damage * 0xc000) >> 16);
}

static void update_demon_object(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                float step_scale) {
  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  if (object->logic_state == GLOOM_OBJECT_LOGIC_DEMONPAUSE) {
    const WeaponDefinition *weapons = weapon_definitions();
    int delay_whole = (int)SDL_ceilf(object->delay);

    object->frame_fixed = (delay_whole & 4) != 0 ? (5u << 16) : 0u;
    if ((delay_whole & 7) == 7 && delay_whole != (int)object->delay2) {
      uint8_t weapon_index = (uint8_t)((delay_whole >> 3) & 7);

      if (weapon_index < (uint8_t)GLOOM_WEAPON_COUNT) {
        (void)spawn_enemy_projectile_params(state, object, weapon_index, weapons[weapon_index].hitpoints,
                                            demon_wtable_scaled_damage(weapon_index), (float)weapons[weapon_index].speed,
                                            -90.0f);
        audio_play_sfx(weapon_sfx_id(weapon_index), 32, 0);
      }
      object->delay2 = (float)delay_whole;
    }

    object->delay -= step_scale;
    if (object->delay <= 0.0f) {
      runtime_set_random_delay(state, object, combat);
      object->logic_state = GLOOM_OBJECT_LOGIC_NORMAL;
    }
    return;
  }

  object->old_rotation = object->rotation;
  object->delay -= step_scale;
  if (object->delay <= 0.0f) {
    object->rotation = runtime_angle_to_player(state, object);
    object->delay = 39.0f;
    object->delay2 = -1.0f;
    object->logic_state = GLOOM_OBJECT_LOGIC_DEMONPAUSE;
    return;
  }

  runtime_monstermove(state, object, combat, step_scale);
}

static void update_special_projectile_object(AppState *state, RuntimeObjectState *object,
                                             const ObjectCombatDefinition *combat, int16_t object_type,
                                             float step_scale) {
  switch (object_type) {
    case GLOOM_OBJECT_TYPE_TERRA:
      update_terra_object(state, object, combat, step_scale);
      return;
    case GLOOM_OBJECT_TYPE_GHOUL:
      update_ghoul_object(state, object, combat, step_scale);
      return;
    case GLOOM_OBJECT_TYPE_PHANTOM:
      update_phantom_object(state, object, combat, step_scale);
      return;
    case GLOOM_OBJECT_TYPE_DEMON:
      update_demon_object(state, object, combat, step_scale);
      return;
    default:
      return;
  }
}

static int16_t runtime_get_object_rot_speed(AppState *state, RuntimeObjectState *object) {
  int16_t rot_speed = 0;

  if (state == NULL || object == NULL) {
    return 0;
  }

  rot_speed = object->rot_speed;
  if (rot_speed == 0) {
    rot_speed = (runtime_random_word(state) & 1u) != 0u ? 4 : -4;
    object->rot_speed = rot_speed;
  }
  return rot_speed;
}

static void update_dragon_object(AppState *state, RuntimeObjectState *object, const ObjectCombatDefinition *combat,
                                 float step_scale) {
  float move = 0.0f;
  int16_t target_rotation = 0;
  int16_t near_threshold = 0;

  if (state == NULL || object == NULL || combat == NULL) {
    return;
  }

  runtime_advance_monster_frame(object, object->frame_speed_fixed, step_scale);

  object->delay -= step_scale;
  if (object->delay < 0.0f) {
    int delay_whole = (int)SDL_floorf(object->delay);

    if (delay_whole <= -128) {
      object->delay = 47.0f;
    } else if ((delay_whole & 7) == 0 && delay_whole != (int)object->delay2) {
      (void)spawn_enemy_projectile_params_ex(state, object, 4u, 1, 3, 16.0f, -144.0f, true);
      object->delay2 = (float)delay_whole;
    }
  }

  move = object->move_speed * step_scale;
  if (!runtime_object_checkvecs(state, object, move)) {
    object->rotation = (int16_t)((object->rotation + runtime_get_object_rot_speed(state, object) * 4) & 255);
    return;
  }

  target_rotation = runtime_angle_to_player(state, object);
  near_threshold = object->rot_speed != 0 ? 6 : 24;
  if (runtime_abs_rotation_delta(object->rotation, target_rotation) >= near_threshold) {
    object->rotation = (int16_t)((object->rotation + runtime_get_object_rot_speed(state, object)) & 255);
    return;
  }

  if (object->rot_speed != 0) {
    object->rot_speed = 0;
    audio_play_sfx(GLOOM_SFX_DRAGON, 64, 20);
  }
}

static bool spawn_enemy_projectile(AppState *state, const RuntimeObjectState *object,
                                   const ObjectCombatDefinition *combat) {
  if (state == NULL || object == NULL || combat == NULL || combat->ranged_weapon >= (uint8_t)GLOOM_WEAPON_COUNT ||
      combat->projectile_speed <= 0.0f) {
    return false;
  }

  return spawn_enemy_projectile_params(state, object, combat->ranged_weapon, 1, combat->ranged_damage,
                                       combat->projectile_speed, -60.0f);
}

static bool spawn_enemy_projectile_params(AppState *state, const RuntimeObjectState *object, uint8_t weapon_index,
                                          int16_t hitpoints, int16_t damage, float speed, float y_offset) {
  return spawn_enemy_projectile_params_ex(state, object, weapon_index, hitpoints, damage, speed, y_offset, false);
}

static bool spawn_enemy_projectile_params_ex(AppState *state, const RuntimeObjectState *object, uint8_t weapon_index,
                                             int16_t hitpoints, int16_t damage, float speed, float y_offset,
                                             bool homing) {
  size_t i = 0u;
  float dir_x = 0.0f;
  float dir_z = 0.0f;

  if (state == NULL || object == NULL || weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT || hitpoints <= 0 ||
      damage < 0 || speed <= 0.0f) {
    return false;
  }

  runtime_object_dir(object->rotation, &dir_x, &dir_z);

  for (i = 0u; i < GLOOM_MAX_RUNTIME_PROJECTILES; ++i) {
    RuntimeProjectile *projectile = &state->projectiles[i];

    if (projectile->active) {
      continue;
    }

    memset(projectile, 0, sizeof(*projectile));
    projectile->active = true;
    projectile->weapon_index = weapon_index;
    projectile->enemy = true;
    projectile->homing = homing;
    projectile->x = object->x;
    projectile->y = object->y + y_offset;
    projectile->z = object->z;
    projectile->vx = dir_x * speed;
    projectile->vz = dir_z * speed;
    projectile->hitpoints = hitpoints;
    projectile->damage = damage;
    return true;
  }

  return false;
}

static void update_runtime_objects(AppState *state) {
  float step_scale = fixed_step_amiga_scale();
  size_t i = 0u;
  size_t count = 0u;

  if (state == NULL) {
    return;
  }

  if (state->dragon_dead_delay > 0.0f) {
    state->dragon_dead_delay -= step_scale;
    if (state->dragon_dead_delay <= 0.0f) {
      /* amiga/gloom2.s: dragondead waits ob_delay then sets finished=3. */
      state->finished = GLOOM_LEVEL_COMPLETE_FINISHED;
      state->dragon_finished_reported = true;
      state->dragon_dead_delay = 0.0f;
    }
  }

  if ((player_death_blocks_controls(state) || state->player_hitpoints <= 0) &&
      (!state->two_player_mode || !secondary_player_can_take_damage(&state->player2))) {
    return;
  }

  count = state->map.object_spawn_count;
  if (count > (size_t)GLOOM_MAX_RUNTIME_OBJECTS) {
    count = (size_t)GLOOM_MAX_RUNTIME_OBJECTS;
  }

  for (i = 0u; i < count; ++i) {
    const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
    RuntimeObjectState *object = &state->objects[i];
    const ObjectCombatDefinition *combat = object_type_combat_definition(spawn->object_type);
    bool uses_monsterlogic = object_type_uses_monsterlogic(spawn->object_type);
    bool uses_baldy_family_logic = object_type_uses_baldy_family_logic(spawn->object_type);
    bool uses_special_projectile_logic = object_type_uses_special_projectile_logic(spawn->object_type);

    if (!object->active || !object->enemy) {
      continue;
    }

    if (object->hurt_pause > 0.0f) {
      object->hurt_pause -= step_scale;
      if (object->hurt_pause <= 0.0f) {
        object->hurt_pause = 0.0f;
        object->frame_fixed = 0u;
      }
      continue;
    }
    if (object->pause_delay > 0.0f) {
      object->pause_delay -= step_scale;
      if (object->pause_delay > 0.0f) {
        continue;
      }
      object->pause_delay = 0.0f;
      if (combat != NULL && combat->delay_range > 0u) {
        int16_t target_rotation = runtime_angle_to_player(state, object);
        int16_t player_rotation = (int16_t)((state->player_rot_fixed >> 16) & 255);
        int16_t diff = (int16_t)(player_rotation - target_rotation);

        object->delay = (float)combat->base_delay + (float)(runtime_random_word(state) % combat->delay_range);
        if (diff < 0) {
          diff = (int16_t)-diff;
        }
        if (diff >= 64 && diff < 192) {
          object->rotation = object->old_rotation;
        }
      }
      continue;
    }

    if (spawn->object_type == GLOOM_OBJECT_TYPE_DRAGON && combat != NULL) {
      update_dragon_object(state, object, combat, step_scale);
      continue;
    }

    if (spawn->object_type == GLOOM_OBJECT_TYPE_DEATHHEAD && combat != NULL) {
      update_deathhead_object(state, object, combat, step_scale);
      continue;
    }

    if (uses_special_projectile_logic && combat != NULL) {
      update_special_projectile_object(state, object, combat, spawn->object_type, step_scale);
      continue;
    }

    if (object->delay > 0.0f) {
      object->delay -= step_scale;
      if (object->delay < 0.0f) {
        object->delay = 0.0f;
      }
    }

    if (uses_baldy_family_logic && combat != NULL) {
      update_baldy_family_object(state, object, combat, spawn->object_type, step_scale);
      continue;
    }

    if (uses_monsterlogic && combat != NULL && combat->uses_fire1 && object->hurt_pause <= 0.0f &&
        object->delay <= 0.0f) {
      int16_t aim_noise = (int16_t)((runtime_random_word(state) & 31u) - 16u);

      object->old_rotation = object->rotation;
      object->rotation = (int16_t)((runtime_angle_to_player(state, object) + aim_noise) & 255);
      (void)spawn_enemy_projectile(state, object, combat);
      object->pause_delay = 7.0f;
      object->frame_fixed = 0u;
      continue;
    }

    if (uses_monsterlogic && object->hurt_pause <= 0.0f) {
      float move = object->move_speed * step_scale;

      object->old_rotation = object->rotation;
      if (!runtime_object_checkvecs(state, object, move)) {
        (void)runtime_object_monsterfix(state, object, move);
      }
      if (combat != NULL) {
        runtime_advance_monster_frame(object, object->frame_speed_fixed, step_scale);
      }
    }
  }
}

static float distance_sq_point_to_segment(float px, float pz, float ax, float az, float bx, float bz) {
  float abx = bx - ax;
  float abz = bz - az;
  float apx = px - ax;
  float apz = pz - az;
  float len_sq = abx * abx + abz * abz;
  float t = 0.0f;
  float dx = 0.0f;
  float dz = 0.0f;

  if (len_sq > 0.0001f) {
    t = (apx * abx + apz * abz) / len_sq;
    t = clampf(t, 0.0f, 1.0f);
  }

  dx = px - (ax + abx * t);
  dz = pz - (az + abz * t);
  return dx * dx + dz * dz;
}

static bool damage_projectile_enemy(AppState *state, const ObjectVisualSet *object_visuals, RuntimeProjectile *projectile,
                                    float prev_x, float prev_z) {
  size_t i = 0u;
  size_t count = 0u;

  if (state == NULL || projectile == NULL) {
    return false;
  }

  count = state->map.object_spawn_count;
  if (count > (size_t)GLOOM_MAX_RUNTIME_OBJECTS) {
    count = (size_t)GLOOM_MAX_RUNTIME_OBJECTS;
  }

  for (i = 0u; i < count; ++i) {
    const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
    RuntimeObjectState *object = &state->objects[i];
    const ObjectVisual *visual = NULL;
    const ObjectCombatDefinition *combat = NULL;
    float radius = 48.0f;
    float hit_radius = 0.0f;
    float hit_radius_sq = 0.0f;
    float enemy_x = 0.0f;
    float enemy_z = 0.0f;
    bool hit = false;

    if (!object->active || !object->enemy) {
      continue;
    }

    if (spawn->object_type < 0 || spawn->object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
      continue;
    }
    combat = object_type_combat_definition(spawn->object_type);

    if (object_visuals != NULL) {
      visual = &object_visuals->visuals[spawn->object_type];
      if (visual->loaded && visual->collision_radius > 0u) {
        radius = ((float)visual->collision_radius * (float)visual->scale) / 512.0f;
      }
    }

    enemy_x = object->x;
    enemy_z = object->z;
    hit_radius = radius + (float)GLOOM_PROJECTILE_RADIUS;
    hit_radius_sq = hit_radius * hit_radius;

    hit = distance_sq_point_to_segment(enemy_x, enemy_z, prev_x, prev_z, projectile->x, projectile->z) <= hit_radius_sq;
    if (!hit && projectile->barrel_origin) {
      hit = distance_sq_point_to_segment(enemy_x, enemy_z, projectile->player_origin_x, projectile->player_origin_z,
                                         projectile->x, projectile->z) <= hit_radius_sq;
    }

    if (!hit) {
      continue;
    }

    if (combat == NULL) {
      return true;
    }

    runtime_damage_object(state, object_visuals, object, combat, spawn->object_type, (uint16_t)i, projectile->damage);

    return true;
  }

  return false;
}

static bool damage_projectile_player(AppState *state, RuntimeProjectile *projectile, float prev_x, float prev_z) {
  float hit_radius = 0.0f;

  if (state == NULL || projectile == NULL) {
    return false;
  }

  hit_radius = (float)state->player_radius + (float)GLOOM_PROJECTILE_RADIUS;
  if (player_can_take_damage(state) &&
      distance_sq_point_to_segment(state->camera_x, state->camera_z, prev_x, prev_z, projectile->x, projectile->z) <=
          hit_radius * hit_radius) {
    state->player_hitpoints -= projectile->damage;
    if (state->player_hitpoints < 0) {
      state->player_hitpoints = 0;
    }
    player_redpal(state);
    if (state->player_hitpoints == 0) {
      start_player_death(state);
    }
    spawn_runtime_sparks(state, projectile->weapon_index, state->camera_x, (float)GLOOM_PLAYER_FIRE_Y,
                         state->camera_z);
    return true;
  }

  if (state->two_player_mode && secondary_player_can_take_damage(&state->player2) &&
      distance_sq_point_to_segment(state->player2.camera_x, state->player2.camera_z, prev_x, prev_z, projectile->x,
                                   projectile->z) <= hit_radius * hit_radius) {
    state->player2.player_hitpoints -= projectile->damage;
    if (state->player2.player_hitpoints < 0) {
      state->player2.player_hitpoints = 0;
    }
    state->player2.player_palette_timer = (float)GLOOM_PLAYER_RED_PAL_TIMER;
    spawn_runtime_sparks(state, projectile->weapon_index, state->player2.camera_x, (float)GLOOM_PLAYER_FIRE_Y,
                         state->player2.camera_z);
    return true;
  }

  return false;
}

static bool damage_projectile_combat_player(AppState *state, RuntimeProjectile *projectile, float prev_x,
                                            float prev_z) {
  float hit_radius = 0.0f;

  if (state == NULL || projectile == NULL || projectile->enemy || !state->combat_mode || !state->two_player_mode) {
    return false;
  }

  hit_radius = (float)state->player_radius + (float)GLOOM_PROJECTILE_RADIUS;
  if (projectile->player_owner_index == 0u && secondary_player_can_take_damage(&state->player2) &&
      distance_sq_point_to_segment(state->player2.camera_x, state->player2.camera_z, prev_x, prev_z, projectile->x,
                                   projectile->z) <= hit_radius * hit_radius) {
    state->player2.player_hitpoints -= projectile->damage;
    if (state->player2.player_hitpoints < 0) {
      state->player2.player_hitpoints = 0;
    }
    state->player2.player_palette_timer = (float)GLOOM_PLAYER_RED_PAL_TIMER;
    spawn_runtime_sparks(state, projectile->weapon_index, state->player2.camera_x, (float)GLOOM_PLAYER_FIRE_Y,
                         state->player2.camera_z);
    return true;
  }

  if (projectile->player_owner_index == 1u && player_can_take_damage(state) &&
      distance_sq_point_to_segment(state->camera_x, state->camera_z, prev_x, prev_z, projectile->x, projectile->z) <=
          hit_radius * hit_radius) {
    state->player_hitpoints -= projectile->damage;
    if (state->player_hitpoints < 0) {
      state->player_hitpoints = 0;
    }
    player_redpal(state);
    spawn_runtime_sparks(state, projectile->weapon_index, state->camera_x, (float)GLOOM_PLAYER_FIRE_Y,
                         state->camera_z);
    return true;
  }

  return false;
}

static void update_homing_projectile_velocity(AppState *state, RuntimeProjectile *projectile, float step_scale) {
  float dx = 0.0f;
  float dz = 0.0f;
  float len = 0.0f;
  float target_x = 0.0f;
  float target_z = 0.0f;

  if (state == NULL || projectile == NULL || !projectile->homing) {
    return;
  }

  target_x = state->camera_x;
  target_z = state->camera_z;
  if (state->two_player_mode && secondary_player_can_take_damage(&state->player2)) {
    bool use_player2 = !player_can_take_damage(state);

    if (!use_player2) {
      float p1_dx = state->camera_x - projectile->x;
      float p1_dz = state->camera_z - projectile->z;
      float p2_dx = state->player2.camera_x - projectile->x;
      float p2_dz = state->player2.camera_z - projectile->z;
      use_player2 = (p2_dx * p2_dx + p2_dz * p2_dz) < (p1_dx * p1_dx + p1_dz * p1_dz);
    }
    if (use_player2) {
      target_x = state->player2.camera_x;
      target_z = state->player2.camera_z;
    }
  }

  dx = target_x - projectile->x;
  dz = target_z - projectile->z;
  len = SDL_sqrtf(dx * dx + dz * dz);
  if (len <= 0.0001f) {
    return;
  }

  projectile->vx += (dx / len) * step_scale;
  projectile->vz += (dz / len) * step_scale;
  if (SDL_fabsf(projectile->vx) >= 32.0f) {
    projectile->vx -= (dx / len) * step_scale;
  }
  if (SDL_fabsf(projectile->vz) >= 32.0f) {
    projectile->vz -= (dz / len) * step_scale;
  }
}

static bool spawn_player_projectile_at_angle(AppState *state, uint8_t weapon_index, float angle, bool play_sound) {
  const WeaponDefinition *weapons = weapon_definitions();
  const WeaponDefinition *weapon = NULL;
  size_t i = 0u;
  float sin_a = 0.0f;
  float cos_a = 0.0f;
  float start_x = 0.0f;
  float start_z = 0.0f;

  if (state == NULL || weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
    return false;
  }

  weapon = &weapons[weapon_index];
  sin_a = SDL_sinf(angle);
  cos_a = SDL_cosf(angle);
  start_x = state->camera_x;
  start_z = state->camera_z;

  if (state->barrel_projectile_origin) {
    start_x += sin_a * (float)GLOOM_PROJECTILE_BARREL_FORWARD;
    start_z += cos_a * (float)GLOOM_PROJECTILE_BARREL_FORWARD;
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_PROJECTILES; ++i) {
    RuntimeProjectile *projectile = &state->projectiles[i];

    if (projectile->active) {
      continue;
    }

    memset(projectile, 0, sizeof(*projectile));
    projectile->active = true;
    projectile->weapon_index = weapon_index;
    projectile->barrel_origin = state->barrel_projectile_origin;
    projectile->player_owner_index = state->active_player_index;
    projectile->x = start_x;
    projectile->y = (float)GLOOM_PLAYER_FIRE_Y;
    projectile->z = start_z;
    projectile->player_origin_x = state->camera_x;
    projectile->player_origin_z = state->camera_z;
    projectile->vx = sin_a * (float)weapon->speed;
    projectile->vz = cos_a * (float)weapon->speed;
    projectile->hitpoints = weapon->hitpoints;
    projectile->damage = weapon->damage;
    projectile->bounce_count = state->player_bouncy_bullets;
    if (play_sound) {
      audio_play_sfx(weapon_sfx_id(weapon_index), 32, 0);
    }
    return true;
  }

  return false;
}

static bool spawn_player_projectile(AppState *state, uint8_t weapon_index) {
  if (state == NULL) {
    return false;
  }

  return spawn_player_projectile_at_angle(state, weapon_index, state->camera_angle, true);
}

static bool player_projectile_visible_from_owner_view(const AppState *state, const RuntimeProjectile *projectile,
                                                      float view_depth) {
  float dx = 0.0f;
  float dz = 0.0f;

  if (state == NULL || projectile == NULL || projectile->enemy ||
      projectile->player_owner_index != state->active_player_index) {
    return true;
  }

  if (view_depth < (float)GLOOM_PLAYER_PROJECTILE_RENDER_MIN_DISTANCE) {
    return false;
  }

  dx = projectile->x - state->camera_x;
  dz = projectile->z - state->camera_z;
  return (dx * dx + dz * dz) >=
         (float)(GLOOM_PLAYER_PROJECTILE_RENDER_MIN_DISTANCE * GLOOM_PLAYER_PROJECTILE_RENDER_MIN_DISTANCE);
}

static void update_projectiles(AppState *state, const ObjectVisualSet *object_visuals) {
  float step_scale = fixed_step_amiga_scale();
  size_t i = 0u;

  if (state == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_PROJECTILES; ++i) {
    RuntimeProjectile *projectile = &state->projectiles[i];
    uint16_t penetration = 0u;
    size_t zone_index = 0u;
    float prev_x = 0.0f;
    float prev_z = 0.0f;

    if (!projectile->active) {
      continue;
    }

    prev_x = projectile->x;
    prev_z = projectile->z;
    projectile->x += projectile->vx * step_scale;
    projectile->z += projectile->vz * step_scale;
    projectile->frame_phase += step_scale;

    if (projectile->enemy && damage_projectile_player(state, projectile, prev_x, prev_z)) {
      projectile->active = false;
      continue;
    }

    if (!projectile->enemy && damage_projectile_combat_player(state, projectile, prev_x, prev_z)) {
      projectile->active = false;
      continue;
    }

    if (!projectile->enemy && !state->combat_mode &&
        damage_projectile_enemy(state, object_visuals, projectile, prev_x, prev_z)) {
      projectile->active = false;
      continue;
    }

    if (projectile->x < 0.0f || projectile->z < 0.0f || projectile->x > 32767.0f || projectile->z > 32767.0f) {
      projectile->active = false;
      continue;
    }

    if (find_wall_collision_radius(state, projectile->x, projectile->z, (int16_t)GLOOM_PROJECTILE_RADIUS,
                                   &penetration, &zone_index)) {
      (void)penetration;
      if (projectile->bounce_count > 0u && zone_index < state->map.zone_count) {
        const GloomZone *zone = &state->map.zones[zone_index];
        float nx = -(float)zone->na;
        float nz = (float)zone->nb;
        float n_len = SDL_sqrtf(nx * nx + nz * nz);

        projectile->bounce_count -= 1u;
        projectile->x = prev_x;
        projectile->z = prev_z;
        if (n_len <= 0.0001f) {
          fprintf(stderr, "Cannot port calcbounce for zone %zu: original wall normal is zero\n", zone_index);
          projectile->active = false;
          continue;
        }
        nx /= n_len;
        nz /= n_len;
        {
          float dot = projectile->vx * nx + projectile->vz * nz;
          projectile->vx -= 2.0f * dot * nx;
          projectile->vz -= 2.0f * dot * nz;
        }
        continue;
      }
      spawn_runtime_sparks(state, projectile->weapon_index, projectile->x, projectile->y, projectile->z);
      projectile->active = false;
      continue;
    }

    update_homing_projectile_velocity(state, projectile, step_scale);
  }
}

static void update_sparks(AppState *state) {
  float step_scale = fixed_step_amiga_scale();
  size_t i = 0u;

  if (state == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_SPARKS; ++i) {
    RuntimeSpark *spark = &state->sparks[i];

    if (!spark->active) {
      continue;
    }

    spark->lifetime -= step_scale;
    if (spark->lifetime <= 0.0f) {
      spark->active = false;
      continue;
    }

    spark->x += spark->vx * step_scale;
    spark->y += spark->vy * step_scale;
    spark->z += spark->vz * step_scale;
  }
}

static void update_blood(AppState *state) {
  float step_scale = fixed_step_amiga_scale();
  size_t i = 0u;

  if (state == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_BLOOD; ++i) {
    RuntimeBlood *blood = &state->blood[i];

    if (!blood->active) {
      continue;
    }

    if (blood->soul) {
      uint16_t target_index = (uint16_t)blood->vy;
      RuntimeObjectState *target = NULL;
      float dx = 0.0f;
      float dz = 0.0f;

      if (!state->death_suck_active || target_index >= (uint16_t)GLOOM_MAX_RUNTIME_OBJECTS) {
        blood->active = false;
        continue;
      }
      target = &state->objects[target_index];
      if (!target->active || target->logic_state != GLOOM_OBJECT_LOGIC_DEATHSUCK) {
        blood->active = false;
        continue;
      }

      blood->x += blood->vx * step_scale;
      blood->z += blood->vz * step_scale;
      dx = blood->x - target->x;
      dz = blood->z - target->z;
      if ((dx * dx + dz * dz) < (64.0f * 64.0f)) {
        blood->active = false;
      }
      continue;
    }

    blood->vy += 0.5f * step_scale;
    blood->y += blood->vy * step_scale;
    if (blood->y >= 0.0f) {
      blood->active = false;
      continue;
    }

    blood->x += blood->vx * step_scale;
    blood->z += blood->vz * step_scale;
  }
}

static void spawn_runtime_gore(AppState *state, const RuntimeChunk *chunk) {
  RuntimeGore *gore = NULL;
  size_t i = 0u;

  if (state == NULL || chunk == NULL || chunk->object_type < 0 ||
      chunk->object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_GORE; ++i) {
    if (!state->gore[i].active) {
      gore = &state->gore[i];
      break;
    }
  }

  if (gore == NULL) {
    gore = &state->gore[state->gore_write_index % (uint16_t)GLOOM_MAX_RUNTIME_GORE];
    state->gore_write_index = (uint16_t)((state->gore_write_index + 1u) % (uint16_t)GLOOM_MAX_RUNTIME_GORE);
  }

  memset(gore, 0, sizeof(*gore));
  gore->active = true;
  gore->object_type = chunk->object_type;
  gore->frame_number = chunk->frame_number;
  gore->x = chunk->x;
  gore->z = chunk->z;
}

static void update_chunks(AppState *state) {
  float step_scale = fixed_step_amiga_scale();
  size_t i = 0u;

  if (state == NULL) {
    return;
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_CHUNKS; ++i) {
    RuntimeChunk *chunk = &state->chunks[i];
    float new_x = 0.0f;
    float new_z = 0.0f;

    if (!chunk->active) {
      continue;
    }

    chunk->vy += 0.5f * step_scale;
    chunk->y += chunk->vy * step_scale;
    if (chunk->y >= 0.0f) {
      if (state->violence_mode != GLOOM_VIOLENCE_MEATY) {
        audio_play_sfx(GLOOM_SFX_SPLAT, 32, -1);
        spawn_runtime_gore(state, chunk);
      }
      chunk->active = false;
      continue;
    }

    new_x = chunk->x + chunk->vx * step_scale;
    new_z = chunk->z + chunk->vz * step_scale;
    if (state->violence_mode == GLOOM_VIOLENCE_MESSY && chunk->radius > 0 &&
        !resolve_wall_collision_radius(state, &new_x, &new_z, chunk->radius)) {
      chunk->vx = 0.0f;
      chunk->vz = 0.0f;
      continue;
    }

    chunk->x = new_x;
    chunk->z = new_z;
  }
}

static bool update_player_pixsize_transition(AppState *state) {
  float previous_pixsize = 0.0f;
  bool reached_midpoint = false;

  if (state == NULL || state->player_pixsizeadd == 0) {
    return false;
  }

  previous_pixsize = state->player_pixsize_accum;
  state->player_pixsize_accum += (float)state->player_pixsizeadd * fixed_step_amiga_scale();

  if (state->player_pixsizeadd < 0 && previous_pixsize > 0.0f && state->player_pixsize_accum <= 0.0f) {
    state->player_pixsize_accum = 0.0f;
    state->player_pixsize = 0;
    state->player_pixsizeadd = 0;
    state->teleport_active = false;
    return false;
  }

  if (state->player_pixsizeadd > 0 && previous_pixsize < 24.0f && state->player_pixsize_accum >= 24.0f) {
    state->player_pixsize_accum = 24.0f;
    reached_midpoint = true;
  }

  state->player_pixsize = (int16_t)round_float_to_int32(state->player_pixsize_accum);

  if (reached_midpoint) {
    state->finished = state->finished2;
    if (state->finished != 0) {
      if (state->finished == GLOOM_LEVEL_COMPLETE_FINISHED) {
        state->level_transition_reported = true;
      }
      return true;
    }

    state->camera_x = (float)state->teleport_x;
    state->camera_z = (float)state->teleport_z;
    state->player_rot_fixed = amiga_rotation_to_fixed(state->teleport_rotation);
    state->player_rotspeed = 0;
    state->camera_angle = player_rotation_fixed_to_radians(state->player_rot_fixed);
    state->player_pixsizeadd = (int16_t)-state->player_pixsizeadd;
  }

  return true;
}

static void update_player_weapon(AppState *state, const PlayerControls *controls) {
  float step_scale = fixed_step_amiga_scale();
  bool fire_pressed = false;
  bool repeat_fire = false;

  if (state == NULL) {
    return;
  }

  if (state->player_weapon_flash > 0.0f) {
    state->player_weapon_flash -= step_scale;
    if (state->player_weapon_flash < 0.0f) {
      state->player_weapon_flash = 0.0f;
    }
  }

  if (state->player_reload_counter > 0.0f) {
    state->player_reload_counter -= step_scale;
    if (state->player_reload_counter < 0.0f) {
      state->player_reload_counter = 0.0f;
    }
  }
  if (state->player_keyboard_fire_repeat_counter > 0.0f) {
    state->player_keyboard_fire_repeat_counter -= step_scale;
    if (state->player_keyboard_fire_repeat_counter < 0.0f) {
      state->player_keyboard_fire_repeat_counter = 0.0f;
    }
  }

  fire_pressed = controls != NULL && controls->fire;
  repeat_fire = controls != NULL && controls->keyboard_fire_repeat;
  if (!fire_pressed) {
    state->player_last_fire = false;
    state->player_keyboard_fire_repeat_counter = 0.0f;
    return;
  }
  if (!repeat_fire) {
    state->player_keyboard_fire_repeat_counter = 0.0f;
  }

  if ((!state->player_last_fire ||
       (repeat_fire && state->player_keyboard_fire_repeat_counter <= 0.0f)) &&
      state->player_reload_counter <= 0.0f) {
    bool fired = false;

    if (state->player_mega_timer > 0.0f) {
      int16_t base_rotation = radians_to_amiga_rotation(state->camera_angle);
      float left_angle = 0.0f;
      float right_angle = 0.0f;

      if (state->player_mega_timer >= (float)GLOOM_PLAYER_MEGA_OVERKILL) {
        left_angle = amiga_rotation_to_radians((int16_t)((base_rotation + 8) & 255));
        right_angle = amiga_rotation_to_radians((int16_t)((base_rotation - 8) & 255));
        fired = spawn_player_projectile_at_angle(state, state->player_weapon, left_angle, false);
        fired = spawn_player_projectile_at_angle(state, state->player_weapon, right_angle, false) || fired;
        fired = spawn_player_projectile_at_angle(state, state->player_weapon, state->camera_angle, true) || fired;
      } else {
        left_angle = amiga_rotation_to_radians((int16_t)((base_rotation + 4) & 255));
        right_angle = amiga_rotation_to_radians((int16_t)((base_rotation - 4) & 255));
        fired = spawn_player_projectile_at_angle(state, state->player_weapon, left_angle, false);
        fired = spawn_player_projectile_at_angle(state, state->player_weapon, right_angle, true) || fired;
      }
    } else {
      fired = spawn_player_projectile(state, state->player_weapon);
    }

    if (fired) {
      state->player_reload_counter = (float)state->player_reload;
      state->player_keyboard_fire_repeat_counter =
          repeat_fire ? (float)GLOOM_KEYBOARD_HELD_FIRE_REPEAT_TICKS : 0.0f;
      state->player_weapon_flash = (float)GLOOM_GUN_FLASH_AMIGA_TICKS;
    }
  }

  state->player_last_fire = true;
}

static void update_player_power_timers(AppState *state) {
  float step_scale = fixed_step_amiga_scale();

  if (state == NULL) {
    return;
  }

  if (state->player_mega_timer > 0.0f) {
    state->player_mega_timer -= step_scale;
    if (state->player_mega_timer < 0.0f) {
      state->player_mega_timer = 0.0f;
    }
  }
  if (state->player_palette_timer > 0.0f) {
    state->player_palette_timer -= step_scale;
    if (state->player_palette_timer < 0.0f) {
      state->player_palette_timer = 0.0f;
    }
  }
  if (state->player_thermo_timer > 0.0f) {
    state->player_thermo_timer -= step_scale;
    if (state->player_thermo_timer < 0.0f) {
      state->player_thermo_timer = 0.0f;
    }
  }
  if (state->player_invisible_timer > 0.0f) {
    state->player_invisible_timer -= step_scale;
    if (state->player_invisible_timer < 0.0f) {
      state->player_invisible_timer = 0.0f;
    }
  }
  if (state->player_hyper_timer < 0) {
    state->player_hyper_timer -= 4.0f * step_scale;
    if (-state->player_hyper_timer >= 0x280) {
      state->player_hyper_timer = (float)((750 << 2) + 0x280);
    }
  } else if (state->player_hyper_timer > 0) {
    state->player_hyper_timer -= 4.0f * step_scale;
    if (state->player_hyper_timer < 0) {
      state->player_hyper_timer = 0.0f;
    }
  }
}

static bool apply_player_weapon_pickup(AppState *state, int16_t weapon_index) {
  if (state == NULL || weapon_index < 0 || weapon_index >= GLOOM_WEAPON_COUNT) {
    return false;
  }

  if ((uint8_t)weapon_index != state->player_weapon) {
    state->player_weapon = (uint8_t)weapon_index;
    state->player_reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
    state->player_reload_counter = 0.0f;
    return true;
  }

  if (state->player_reload > 0u) {
    state->player_reload -= 1u;
  }
  if (state->player_reload == 0u) {
    state->player_reload = 1u;
    state->player_mega_timer += 250.0f;
  }
  return true;
}

static bool apply_player_pickup(AppState *state, RuntimeObjectState *object, int16_t object_type) {
  bool consumed = true;

  if (state == NULL || object == NULL) {
    return false;
  }

  audio_play_sfx(GLOOM_SFX_TOKEN, 64, 0);
  switch (object_type) {
    case 2:
      state->player_hitpoints += 5;
      if (state->player_hitpoints > GLOOM_PLAYER_INITIAL_HEALTH) {
        state->player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
      }
      break;
    case 3:
    case GLOOM_OBJECT_TYPE_WEAPON1:
    case GLOOM_OBJECT_TYPE_WEAPON2:
    case GLOOM_OBJECT_TYPE_WEAPON3:
    case GLOOM_OBJECT_TYPE_WEAPON4:
    case GLOOM_OBJECT_TYPE_WEAPON5:
      consumed = apply_player_weapon_pickup(state, object_type_pickup_weapon(object_type));
      break;
    case 4:
    case 5:
      state->player_thermo_timer += 1500.0f;
      break;
    case 6:
      state->player_invisible_timer += 1500.0f;
      break;
    case 7:
      if (state->player_hyper_timer != 0.0f) {
        consumed = false;
      } else {
        state->player_hyper_timer = -512.0f;
      }
      break;
    case GLOOM_OBJECT_TYPE_BOUNCY:
      if (state->player_bouncy_bullets >= 3u) {
        consumed = false;
      } else {
        state->player_bouncy_bullets += 1u;
      }
      break;
    default:
      consumed = false;
      break;
  }

  if (consumed) {
    object->active = false;
  }
  return consumed;
}

static void update_pickup_animations(AppState *state) {
  size_t i = 0u;
  size_t count = 0u;

  if (state == NULL ||
      ((player_death_blocks_controls(state) || state->player_hitpoints <= 0) &&
       (!state->two_player_mode || !secondary_player_can_take_damage(&state->player2)))) {
    return;
  }

  count = state->map.object_spawn_count;
  if (count > (size_t)GLOOM_MAX_RUNTIME_OBJECTS) {
    count = (size_t)GLOOM_MAX_RUNTIME_OBJECTS;
  }

  for (i = 0u; i < count; ++i) {
    const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
    RuntimeObjectState *object = &state->objects[i];

    if (!object->active || !object_type_is_pickup(spawn->object_type)) {
      continue;
    }

    if (object_type_pickup_weapon(spawn->object_type) >= 0) {
      object->bounce_phase += 8.0f * fixed_step_amiga_scale();
      while (object->bounce_phase >= 128.0f) {
        object->bounce_phase -= 128.0f;
      }
      object->render_y_offset =
          -SDL_sinf(amiga_rotation_to_radians((int16_t)round_float_to_int32(object->bounce_phase))) * 128.0f;
      runtime_advance_monster_frame(object, 0x8000, fixed_step_amiga_scale());
    } else if (spawn->object_type == GLOOM_OBJECT_TYPE_BOUNCY) {
      static const uint32_t frames[4] = {3u << 16, 4u << 16, 3u << 16, 5u << 16};
      int frame_index = ((int)object->delay >> 1) & 3;

      object->delay += fixed_step_amiga_scale();
      object->frame_fixed = frames[frame_index];
    }
  }
}

static void update_pickup_collisions(AppState *state) {
  size_t i = 0u;
  size_t count = 0u;

  if (state == NULL || player_death_blocks_controls(state) || state->player_hitpoints <= 0) {
    return;
  }

  count = state->map.object_spawn_count;
  if (count > (size_t)GLOOM_MAX_RUNTIME_OBJECTS) {
    count = (size_t)GLOOM_MAX_RUNTIME_OBJECTS;
  }

  for (i = 0u; i < count; ++i) {
    const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
    RuntimeObjectState *object = &state->objects[i];
    float dx = 0.0f;
    float dz = 0.0f;
    float radius = 0.0f;

    if (!object->active || !object_type_is_pickup(spawn->object_type)) {
      continue;
    }

    dx = object->x - state->camera_x;
    dz = object->z - state->camera_z;
    radius = (float)state->player_radius + (float)object->radius;
    if (dx * dx + dz * dz <= radius * radius) {
      (void)apply_player_pickup(state, object, spawn->object_type);
    }
  }
}

static void update_pickups(AppState *state) {
  update_pickup_animations(state);
  update_pickup_collisions(state);
}

static void mirror_two_player_lives_after_active_update(AppState *state, int16_t previous_lives,
                                                        RuntimePlayerState *inactive_player) {
  if (state == NULL || inactive_player == NULL || !state->two_player_mode || state->player_lives == previous_lives) {
    return;
  }

  /* amiga/gloom2.s: playerdeath mirrors remaining nonzero cooperative lives to the other player. */
  if (state->player_lives > 0) {
    inactive_player->player_lives = state->player_lives;
  }
}

static void update_active_player_logic(AppState *state, const PlayerControls *controls) {
  if (update_player_pixsize_transition(state)) {
    update_player_camera_y(state);
    return;
  }

  if (state->player_death_phase != GLOOM_PLAYER_DEATH_NONE) {
    if (state->player_dead) {
      settle_player_bounce(state, false);
    }
    update_player_death(state, controls);
    if (player_death_blocks_controls(state)) {
      return;
    }
  }

  {
    float corrected_x = state->camera_x;
    float corrected_z = state->camera_z;

    if (!resolve_player_wall_collision(state, &corrected_x, &corrected_z)) {
      damage_player_squash(state);
      update_player_camera_y(state);
      return;
    }
    state->camera_x = corrected_x;
    state->camera_z = corrected_z;
  }

  check_event_triggers(state);

  if (state->player_hitpoints <= 0) {
    start_player_death(state);
    settle_player_bounce(state, false);
    update_player_death(state, controls);
    return;
  }

  update_player_keyboard_rotation(state, controls);
  update_player_movement(state, controls);
  update_pickup_collisions(state);
  update_player_weapon(state, controls);
}

static void update_with_controls(AppState *state, const PlayerControls *controls, const PlayerControls *controls2,
                                 const ObjectVisualSet *object_visuals) {
  int16_t previous_lives = 0;

  state->tick_count += 1;
  audio_vblank_tick();
  update_wall_animations(state);
  update_rotpolys(state);
  update_doors(state);
  update_runtime_objects(state);
  runtime_update_object_player_contacts(state, object_visuals);
  update_pickup_animations(state);
  update_projectiles(state, object_visuals);
  update_sparks(state);
  update_blood(state);
  update_chunks(state);

  state->active_player_index = 0u;
  state->active_other_player_lives = state->two_player_mode ? state->player2.player_lives : 0;
  if (!player_death_blocks_controls(state)) {
    update_player_power_timers(state);
  }
  previous_lives = state->player_lives;
  update_active_player_logic(state, controls);
  mirror_two_player_lives_after_active_update(state, previous_lives, &state->player2);

  if (state->two_player_mode) {
    RuntimePlayerState player1;

    capture_primary_player_state(state, &player1);
    apply_primary_player_state(state, &state->player2);
    state->active_player_index = 1u;
    state->active_other_player_lives = player1.player_lives;
    if (!player_death_blocks_controls(state)) {
      update_player_power_timers(state);
    }
    previous_lives = state->player_lives;
    update_active_player_logic(state, controls2);
    mirror_two_player_lives_after_active_update(state, previous_lives, &player1);
    capture_primary_player_state(state, &state->player2);
    apply_primary_player_state(state, &player1);
    state->active_player_index = 0u;
    state->active_other_player_lives = state->player2.player_lives;
  }
  runtime_update_object_player_contacts(state, object_visuals);
}

static void update(AppState *state, const uint8_t *keyboard, bool mouse_fire, const ObjectVisualSet *object_visuals) {
  PlayerControls controls;
  PlayerControls controls2;
  int gamepad_index = -1;

  memset(&controls, 0, sizeof(controls));
  memset(&controls2, 0, sizeof(controls2));
  normalize_control_config(&g_control_config);

  if (player1_receives_keyboard_input()) {
    controls = read_modern_player_controls(keyboard, mouse_fire);
  }
  if (g_control_config.player2 == GLOOM_CONTROL_KEYBOARD) {
    controls2 = read_second_player_controls(keyboard);
  }

  gamepad_index = control_source_gamepad_index(g_control_config.player1);
  if (gamepad_index >= 0) {
    PlayerControls gamepad_controls = read_gamepad_player_controls((size_t)gamepad_index, 0u);

    merge_player_controls(&controls, &gamepad_controls);
  }
  gamepad_index = control_source_gamepad_index(g_control_config.player2);
  if (gamepad_index >= 0) {
    PlayerControls gamepad_controls = read_gamepad_player_controls((size_t)gamepad_index, 1u);

    merge_player_controls(&controls2, &gamepad_controls);
  }
  if (g_webrtc_guest_connected) {
    PlayerControls webrtc_controls = read_webrtc_player2_controls();

    /* WebRTC guest devices are remote player-two input, independent of local control assignment. */
    merge_player_controls(&controls2, &webrtc_controls);
  }
  update_with_controls(state, &controls, &controls2, object_visuals);
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

static double profile_now_ms(void) {
  static uint64_t frequency = 0u;
  uint64_t counter = 0u;

  if (!g_profiler.enabled) {
    return 0.0;
  }

  if (frequency == 0u) {
    frequency = SDL_GetPerformanceFrequency();
    if (frequency == 0u) {
      return 0.0;
    }
  }

  counter = SDL_GetPerformanceCounter();
  return ((double)counter * 1000.0) / (double)frequency;
}

static void profile_add_elapsed(double *bucket, double start_ms) {
  if (!g_profiler.enabled || bucket == NULL || start_ms <= 0.0) {
    return;
  }

  *bucket += profile_now_ms() - start_ms;
}

static void profile_logf(const char *fmt, ...) {
  char line[256];
  va_list args;

  if (fmt == NULL) {
    return;
  }

  va_start(args, fmt);
  (void)vsnprintf(line, sizeof(line), fmt, args);
  va_end(args);
  line[sizeof(line) - 1u] = '\0';

  printf("%s\n", line);
#ifdef GLOOM_DOS_SDL3
  dos_logf("%s", line);
#endif
}

static void profile_report(int render_width, int render_height) {
  double frames = (double)g_profiler.frames;

  if (!g_profiler.enabled || g_profiler.frames == 0u) {
    return;
  }

  profile_logf("PROFILE frames=%u resolution=%dx%d avg_frame_ms=%.3f approx_fps=%.2f updates/frame=%.2f",
               (unsigned)g_profiler.frames, render_width, render_height, g_profiler.frame_ms / frames,
               g_profiler.frame_ms > 0.0 ? (frames * 1000.0) / g_profiler.frame_ms : 0.0,
               (double)g_profiler.update_steps / frames);
  profile_logf("PROFILE loop_ms events=%.3f update=%.3f interpolate=%.3f render_call=%.3f",
               g_profiler.events_ms / frames, g_profiler.update_ms / frames, g_profiler.interpolate_ms / frames,
               g_profiler.render_ms / frames);
  profile_logf("PROFILE render_ms lock_clear=%.3f flats=%.3f walls=%.3f sprites=%.3f weapon=%.3f hud=%.3f red=%.3f present=%.3f",
               g_profiler.render_lock_clear_ms / frames, g_profiler.render_flat_ms / frames,
               g_profiler.render_wall_ms / frames, g_profiler.render_sprites_ms / frames,
               g_profiler.render_weapon_ms / frames, g_profiler.render_hud_ms / frames,
               g_profiler.render_red_ms / frames, g_profiler.render_present_ms / frames);
  profile_logf("PROFILE wall_ms search=%.3f draw=%.3f",
               g_profiler.render_wall_search_ms / frames, g_profiler.render_wall_draw_ms / frames);
  profile_logf("PROFILE counts wall_candidates/frame=%.2f sprites/frame=%.2f",
               (double)g_profiler.wall_candidates / frames, (double)g_profiler.sprites / frames);
}

#ifdef GLOOM_DOS_SDL3
static int32_t dos_float_to_fixed16(float value) {
  float scaled = 0.0f;
  int32_t fixed = 0;

  if (value >= 32767.999f) return INT32_MAX;
  if (value <= -32768.0f) return INT32_MIN;

  scaled = value * 65536.0f;
  fixed = (int32_t)scaled;
  if (scaled < 0.0f && (float)fixed > scaled) {
    fixed -= 1;
  }
  return fixed;
}

static inline int dos_fixed16_floor_to_int(int32_t value) {
  return (int)(value >> 16);
}

static inline int32_t dos_div_fixed16(int64_t numerator_fixed, int32_t denominator_fixed) {
  int64_t value = 0;

  if (denominator_fixed == 0) {
    return numerator_fixed < 0 ? INT32_MIN : INT32_MAX;
  }

  value = (numerator_fixed << 16) / (int64_t)denominator_fixed;
  if (value < INT32_MIN) return INT32_MIN;
  if (value > INT32_MAX) return INT32_MAX;
  return (int32_t)value;
}

static inline int32_t dos_fixed16_clamp_unit(int32_t value) {
  if (value < 0) return 0;
  if (value > 0x10000) return 0x10000;
  return value;
}
#endif

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

static int16_t player_rotation_fixed_to_amiga(int32_t rotation_fixed) {
  return (int16_t)(((rotation_fixed + 0x8000) >> 16) & 255);
}

static float amiga_rotation_to_radians(int16_t rotation) {
  return player_rotation_fixed_to_radians(amiga_rotation_to_fixed(rotation));
}

static void initialize_depth_tables(void) {
  int z = 0;

  if (g_depth_tables_initialized) {
    return;
  }

  for (z = 0; z < GLOOM_AMIGA_MAX_Z; ++z) {
    int source_z = (GLOOM_AMIGA_MAX_Z - 1) - z;
    int root = (int)SDL_sqrtf((float)(source_z << 3));
    int palette_index = (root >> 3) ^ 15;

    if (palette_index < 0) palette_index = 0;
    if (palette_index > 15) palette_index = 15;
    g_depth_dark_indices[z] = (uint8_t)palette_index;
    g_depth_subtract_values[z] = (uint8_t)(palette_index * 17);
  }

  g_depth_tables_initialized = true;
}

static uint8_t amiga_depth_dark_index(float depth) {
  int z = floor_to_int(depth);

  if (!g_depth_tables_initialized) {
    initialize_depth_tables();
  }

  if (z < 0) {
    z = 0;
  }
  if (z >= GLOOM_AMIGA_MAX_Z) {
    z = GLOOM_AMIGA_MAX_Z - 1;
  }

  return g_depth_dark_indices[z];
}

#ifdef GLOOM_DOS_SDL3
static uint8_t amiga_depth_dark_index_int(int32_t depth) {
  int32_t z = depth;

  if (!g_depth_tables_initialized) {
    initialize_depth_tables();
  }

  if (z < 0) {
    z = 0;
  }
  if (z >= GLOOM_AMIGA_MAX_Z) {
    z = GLOOM_AMIGA_MAX_Z - 1;
  }

  return g_depth_dark_indices[z];
}

#endif

static int depth_subtract_for_depth(float depth) {
  int z = floor_to_int(depth);

  if (!g_depth_tables_initialized) {
    initialize_depth_tables();
  }

  if (z < 0) {
    z = 0;
  }
  if (z >= GLOOM_AMIGA_MAX_Z) {
    z = GLOOM_AMIGA_MAX_Z - 1;
  }

  return (int)g_depth_subtract_values[z];
}

static uint32_t shade_argb_subtract(uint32_t argb, int subtract) {
  uint8_t alpha = (uint8_t)(argb >> 24);
  int r = (int)((argb >> 16) & 0xFFu) - subtract;
  int g = (int)((argb >> 8) & 0xFFu) - subtract;
  int b = (int)(argb & 0xFFu) - subtract;

  if (r < 0) r = 0;
  if (g < 0) g = 0;
  if (b < 0) b = 0;

  return ((uint32_t)alpha << 24u) | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
}

static uint32_t apply_amiga_depth_argb(uint32_t argb, float depth) {
  return shade_argb_subtract(argb, depth_subtract_for_depth(depth));
}

static uint32_t amiga_blood_argb(uint16_t color_mask, float depth) {
  static const uint16_t blcols[16] = {0x0cccu, 0x0bbbu, 0x0aaau, 0x0999u, 0x0888u, 0x0777u, 0x0666u, 0x0555u,
                                     0x0444u, 0x0333u, 0x0222u, 0x0111u, 0x0111u, 0x0111u, 0x0111u, 0x0111u};
  uint16_t color = (uint16_t)(blcols[amiga_depth_dark_index(depth)] & color_mask & 0x0fffu);
  uint8_t r = (uint8_t)(((color >> 8u) & 0x0fu) * 17u);
  uint8_t g = (uint8_t)(((color >> 4u) & 0x0fu) * 17u);
  uint8_t b = (uint8_t)((color & 0x0fu) * 17u);

  return 0xff000000u | ((uint32_t)r << 16u) | ((uint32_t)g << 8u) | (uint32_t)b;
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

#ifndef GLOOM_DOS_SDL3
static int compare_wall_candidates(const void *a, const void *b) {
  const WallCandidate *wa = (const WallCandidate *)a;
  const WallCandidate *wb = (const WallCandidate *)b;

  if (wa->sort_depth < wb->sort_depth) return -1;
  if (wa->sort_depth > wb->sort_depth) return 1;
  return 0;
}

static void sort_wall_candidates(WallCandidate *candidates, size_t candidate_count) {
  if (candidates == NULL || candidate_count <= 1u) {
    return;
  }

  qsort(candidates, candidate_count, sizeof(candidates[0]), compare_wall_candidates);
}
#endif

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
#ifndef GLOOM_DOS_SDL3
      float mid_x = 0.0f;
      float mid_z = 0.0f;
      float dx = 0.0f;
      float dz = 0.0f;
#endif

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
      candidate = &out_candidates[candidate_count++];
      candidate->zone_index = zone_index;
#ifdef GLOOM_DOS_SDL3
      candidate->sort_depth = 0.0f;
#else
      mid_x = 0.5f * ((float)zone->x1 + (float)zone->x2);
      mid_z = 0.5f * ((float)zone->z1 + (float)zone->z2);
      dx = mid_x - cam_x;
      dz = mid_z - cam_z;

      candidate->sort_depth = dx * dx + dz * dz;
#endif
    }
  }

#ifndef GLOOM_DOS_SDL3
  sort_wall_candidates(out_candidates, candidate_count);
#endif

  return candidate_count;
}

static bool append_wall_candidate_unique(const GloomMap *map, WallCandidate *candidates, size_t *candidate_count,
                                         size_t candidate_capacity, size_t zone_index, float cam_x, float cam_z) {
  const GloomZone *zone = NULL;
#ifndef GLOOM_DOS_SDL3
  float mid_x = 0.0f;
  float mid_z = 0.0f;
  float dx = 0.0f;
  float dz = 0.0f;
#endif
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

  candidates[*candidate_count].zone_index = zone_index;
#ifdef GLOOM_DOS_SDL3
  (void)cam_x;
  (void)cam_z;
  candidates[*candidate_count].sort_depth = 0.0f;
#else
  mid_x = 0.5f * ((float)zone->x1 + (float)zone->x2);
  mid_z = 0.5f * ((float)zone->z1 + (float)zone->z2);
  dx = mid_x - cam_x;
  dz = mid_z - cam_z;
  candidates[*candidate_count].sort_depth = dx * dx + dz * dz;
#endif
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

static bool find_wall_collision_radius(const AppState *state, float x, float z, int16_t radius,
                                       uint16_t *out_penetration, size_t *out_zone_index) {
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

  if (state == NULL || out_penetration == NULL || out_zone_index == NULL || radius <= 0 ||
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

  if ((int32_t)closest_distance - (int32_t)radius >= 0) {
    return false;
  }

  *out_penetration = (uint16_t)((int32_t)radius - (int32_t)closest_distance);
  *out_zone_index = closest_zone_index;
  return true;
}

static bool find_player_wall_collision(const AppState *state, float x, float z, uint16_t *out_penetration,
                                       size_t *out_zone_index) {
  if (state == NULL) {
    return false;
  }

  return find_wall_collision_radius(state, x, z, state->player_radius, out_penetration, out_zone_index);
}

static bool resolve_wall_collision_radius(AppState *state, float *x, float *z, int16_t radius) {
  int attempt = 0;

  if (state == NULL || x == NULL || z == NULL || radius <= 0) {
    return false;
  }

  for (attempt = 0; attempt < 3; ++attempt) {
    uint16_t penetration = 0u;
    size_t zone_index = 0u;
    const GloomZone *zone = NULL;
    float push_x = 0.0f;
    float push_z = 0.0f;

    if (!find_wall_collision_radius(state, *x, *z, radius, &penetration, &zone_index)) {
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

static bool resolve_player_wall_collision(AppState *state, float *x, float *z) {
  if (state == NULL) {
    return false;
  }

  return resolve_wall_collision_radius(state, x, z, state->player_radius);
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
  audio_play_sfx(GLOOM_SFX_DOOR, 64, 0);
}

static void start_pending_teleport(AppState *state, const GloomTeleportCommand *command) {
  if (state == NULL || command == NULL) {
    return;
  }

  state->teleport_x = command->x;
  state->teleport_z = command->z;
  state->teleport_rotation = command->rotation;

  if (state->finished != 0 || state->finished2 != 0) {
    return;
  }

  if (command->control_word != 0) {
    if (!state->lock_teleport_reported) {
      fprintf(stderr,
              "Cannot execute lock teleport command for event %u: amiga/gloom2.s exec_teleport locklogic is not "
              "ported yet\n",
              (unsigned)command->event_id);
      state->lock_teleport_reported = true;
    }
    return;
  }

  state->teleport_active = true;
  state->player_pixsize = 0;
  state->player_pixsize_accum = 0.0f;
  state->player_pixsizeadd = 2;
  audio_play_sfx(GLOOM_SFX_TELEPORT, 64, 10);
}

static void start_level_exit_transition(AppState *state) {
  if (state == NULL || state->player_pixsizeadd != 0 || state->finished2 != 0) {
    return;
  }

  state->finished2 = GLOOM_LEVEL_COMPLETE_FINISHED;
  state->teleport_active = true;
  state->player_pixsize = 0;
  state->player_pixsize_accum = 0.0f;
  state->player_pixsizeadd = 1;
  audio_play_sfx(GLOOM_SFX_TELEPORT, 64, 10);
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

  if (event_id == (uint8_t)GLOOM_LEVEL_EXIT_EVENT_ID) {
    start_level_exit_transition(state);
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

    rotpoly->rot += (float)rotpoly->speed * fixed_step_amiga_scale();

    if ((rotpoly->flags & 1u) != 0u) {
      float frac_float = rotpoly->rot;
      int32_t frac = 0;

      if (frac_float <= 0.0f) {
        frac_float = 0.0f;
        rotpoly->speed = (int16_t)-rotpoly->speed;
      } else if (frac_float >= 16384.0f) {
        frac_float = 16384.0f;
        if ((rotpoly->flags & 2u) != 0u) {
          rotpoly->speed = (int16_t)-rotpoly->speed;
        } else {
          rotpoly->speed = 0;
        }
      }

      rotpoly->rot = frac_float;
      frac = round_float_to_int32(frac_float);

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
      int16_t rot_word = 0;

      while (rotpoly->rot >= 65536.0f) {
        rotpoly->rot -= 65536.0f;
      }
      while (rotpoly->rot < 0.0f) {
        rotpoly->rot += 65536.0f;
      }
      rot_word = (int16_t)((uint16_t)round_float_to_int32(rotpoly->rot));

      for (zone_index = 0u; zone_index < zone_count; ++zone_index) {
        const RuntimeRotPolyVertex *vertex = &rotpoly->vertices[zone_index];
        int16_t rel_x = 0;
        int16_t rel_z = 0;

        rotate_amiga_1024(vertex->base_x, vertex->base_z, rot_word, &rel_x, &rel_z);
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
      int16_t rot_word = (int16_t)((uint16_t)round_float_to_int32(rotpoly->rot));

      for (zone_index = 0u; zone_index < zone_count; ++zone_index) {
        const RuntimeRotPolyVertex *vertex = &rotpoly->vertices[zone_index];
        GloomZone *zone = &state->map.zones[first_zone + zone_index];
        int16_t na = 0;
        int16_t nb = 0;

        rotate_amiga_1024(vertex->base_na, vertex->base_nb, rot_word, &na, &nb);
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
    int32_t frac_fixed = 0;
    int32_t width_x = 0;
    int32_t width_z = 0;
    int32_t move_x = 0;
    int32_t move_z = 0;

    if (!door->active || door->zone_index >= state->map.zone_count) {
      continue;
    }

    zone = &state->map.zones[door->zone_index];
    door->frac += (float)door->frac_add * fixed_step_amiga_scale();
    if (door->frac < 0.0f) {
      door->frac = 0.0f;
    }
    if (door->frac > 16384.0f) {
      door->frac = 16384.0f;
    }

    frac_fixed = round_float_to_int32(door->frac);
    zone->event_id = (int16_t)(uint16_t)(frac_fixed * 2);

    width_x = (int32_t)door->base_x2 - (int32_t)door->base_x1;
    width_z = (int32_t)door->base_z2 - (int32_t)door->base_z1;
    move_x = amiga_arithmetic_shift_right_16((int64_t)width_x * (int64_t)frac_fixed * 4);
    move_z = amiga_arithmetic_shift_right_16((int64_t)width_z * (int64_t)frac_fixed * 4);

    zone->x1 = (int16_t)((int32_t)door->base_x1 - move_x);
    zone->z1 = (int16_t)((int32_t)door->base_z1 - move_z);
    zone->x2 = (int16_t)((int32_t)zone->x1 + width_x);
    zone->z2 = (int16_t)((int32_t)zone->z1 + width_z);

    if (frac_fixed == 0 || frac_fixed == 0x4000) {
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

  if (state == NULL || state->player_radius <= 0 || state->player_pixsizeadd != 0 || state->finished2 != 0 ||
      !state->map.has_grid_cells || state->map.ppnt_blob == NULL || state->map.ppnt_blob_size < 2u ||
      state->map.zone_count > (size_t)GLOOM_MAX_RENDER_ZONES) {
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
      int16_t event_id = 0;

      if (zone_index >= state->map.zone_count || zone_index >= (size_t)GLOOM_MAX_RENDER_ZONES || seen[zone_index]) {
        continue;
      }

      seen[zone_index] = 1u;
      zone = &state->map.zones[zone_index];
      distance = zone_segment_distance(zone, (int16_t)cam_x, (int16_t)cam_z);
      if ((int32_t)distance - (int32_t)state->player_radius >= 0) {
        continue;
      }

      event_id = zone->event_id;
      if (event_id <= 0 || event_id > (int16_t)GLOOM_EVENT_COUNT) {
        return;
      }
      execute_map_event(state, (uint8_t)event_id);
      return;
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

static void free_render_framebuffer(RenderFramebuffer *framebuffer) {
  if (framebuffer == NULL) {
    return;
  }

  if (framebuffer->texture != NULL) {
    SDL_DestroyTexture(framebuffer->texture);
  }
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_palette != NULL) {
    SDL_DestroyPalette(framebuffer->index_palette);
  }
  free(framebuffer->index_coverage);
  free(framebuffer->last_frame_indices);
#endif
  free(framebuffer->last_frame_pixels);
  free(framebuffer->depth_buffer);
  free(framebuffer->sprite_depth_buffer);
#ifdef GLOOM_DOS_SDL3
  free(framebuffer->depth_buffer_int);
  free(framebuffer->sprite_depth_buffer_int);
#endif
  free(framebuffer->filled_stamps);
  memset(framebuffer, 0, sizeof(*framebuffer));
}

static bool ensure_render_framebuffer(SDL_Renderer *renderer, RenderFramebuffer *framebuffer, int width, int height) {
  float *depth_buffer = NULL;
  float *sprite_depth_buffer = NULL;
#ifdef GLOOM_DOS_SDL3
  int32_t *depth_buffer_int = NULL;
  int32_t *sprite_depth_buffer_int = NULL;
#endif
  uint32_t *filled_stamps = NULL;
  uint32_t *last_frame_pixels = NULL;
  SDL_Texture *texture = NULL;
#ifdef GLOOM_DOS_SDL3
  SDL_Palette *index_palette = NULL;
  uint8_t *index_coverage = NULL;
  uint8_t *last_frame_indices = NULL;
#endif

  if (renderer == NULL || framebuffer == NULL || width <= 0 || height <= 0) {
    return false;
  }

  if (framebuffer->texture != NULL && framebuffer->width == width && framebuffer->height == height) {
    return true;
  }

#ifndef GLOOM_DOS_SDL3
  depth_buffer = (float *)malloc((size_t)width * sizeof(*depth_buffer));
  sprite_depth_buffer = (float *)malloc((size_t)width * sizeof(*sprite_depth_buffer));
#else
  depth_buffer_int = (int32_t *)malloc((size_t)width * sizeof(*depth_buffer_int));
  sprite_depth_buffer_int = (int32_t *)malloc((size_t)width * sizeof(*sprite_depth_buffer_int));
#endif
  filled_stamps = (uint32_t *)calloc((size_t)height, sizeof(*filled_stamps));
#ifdef GLOOM_DOS_SDL3
  last_frame_indices = (uint8_t *)malloc((size_t)width * (size_t)height * sizeof(*last_frame_indices));
#else
  last_frame_pixels = (uint32_t *)malloc((size_t)width * (size_t)height * sizeof(*last_frame_pixels));
#endif
#ifdef GLOOM_DOS_SDL3
  if (depth_buffer_int == NULL || sprite_depth_buffer_int == NULL || filled_stamps == NULL ||
      last_frame_indices == NULL) {
#else
  if (depth_buffer == NULL || sprite_depth_buffer == NULL || filled_stamps == NULL || last_frame_pixels == NULL) {
#endif
    fprintf(stderr, "Cannot allocate PC renderer scratch buffers %dx%d\n", width, height);
    free(depth_buffer);
    free(sprite_depth_buffer);
#ifdef GLOOM_DOS_SDL3
    free(depth_buffer_int);
    free(sprite_depth_buffer_int);
    free(index_coverage);
    free(last_frame_indices);
#endif
    free(filled_stamps);
    free(last_frame_pixels);
    return false;
  }

#ifdef GLOOM_DOS_SDL3
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_INDEX8, SDL_TEXTUREACCESS_STREAMING, width, height);
#else
  texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
#endif
  if (texture == NULL) {
    fprintf(stderr, "Cannot create PC renderer framebuffer texture %dx%d: %s\n", width, height, SDL_GetError());
    free(depth_buffer);
    free(sprite_depth_buffer);
#ifdef GLOOM_DOS_SDL3
    free(depth_buffer_int);
    free(sprite_depth_buffer_int);
    free(index_coverage);
    free(last_frame_indices);
#endif
    free(filled_stamps);
    free(last_frame_pixels);
    return false;
  }
#ifdef GLOOM_DOS_SDL3
  if (!attach_dos_index_palette(texture, &index_palette)) {
    SDL_DestroyTexture(texture);
    free(depth_buffer);
    free(sprite_depth_buffer);
    free(depth_buffer_int);
    free(sprite_depth_buffer_int);
    free(index_coverage);
    free(last_frame_indices);
    free(filled_stamps);
    free(last_frame_pixels);
    return false;
  }
#endif
  (void)SDL_SetTextureScaleMode(texture, SDL_ScaleModeNearest);
  (void)SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_NONE);

  free_render_framebuffer(framebuffer);
  framebuffer->texture = texture;
#ifdef GLOOM_DOS_SDL3
  framebuffer->index_palette = index_palette;
  framebuffer->index_coverage = index_coverage;
  framebuffer->last_frame_indices = last_frame_indices;
#endif
  framebuffer->last_frame_pixels = last_frame_pixels;
  framebuffer->depth_buffer = depth_buffer;
  framebuffer->sprite_depth_buffer = sprite_depth_buffer;
#ifdef GLOOM_DOS_SDL3
  framebuffer->depth_buffer_int = depth_buffer_int;
  framebuffer->sprite_depth_buffer_int = sprite_depth_buffer_int;
#endif
  framebuffer->filled_stamps = filled_stamps;
  framebuffer->filled_stamp = 0u;
  framebuffer->width = width;
  framebuffer->height = height;
  return true;
}

static bool begin_render_framebuffer(RenderFramebuffer *framebuffer) {
  void *pixels = NULL;
  int pitch_bytes = 0;

  if (framebuffer == NULL || framebuffer->texture == NULL || framebuffer->width <= 0 || framebuffer->height <= 0) {
    return false;
  }

  if (SDL_LockTexture(framebuffer->texture, NULL, &pixels, &pitch_bytes) != 0) {
    fprintf(stderr, "Cannot lock PC renderer framebuffer texture: %s\n", SDL_GetError());
    return false;
  }
  if (pixels == NULL) {
    fprintf(stderr, "Cannot render: SDL framebuffer lock returned no pixels for %dx%d\n", framebuffer->width,
            framebuffer->height);
    SDL_UnlockTexture(framebuffer->texture);
#ifdef GLOOM_DOS_SDL3
    framebuffer->index_pixels = NULL;
    framebuffer->index_pitch_pixels = 0;
#endif
    framebuffer->pixels = NULL;
    framebuffer->pitch_pixels = 0;
    return false;
  }

#ifdef GLOOM_DOS_SDL3
  if (pitch_bytes < framebuffer->width) {
    fprintf(stderr, "Cannot render: SDL framebuffer pitch %d is invalid for %dx%d indexed pixels\n", pitch_bytes,
            framebuffer->width, framebuffer->height);
    SDL_UnlockTexture(framebuffer->texture);
    framebuffer->index_pixels = NULL;
    framebuffer->index_pitch_pixels = 0;
    framebuffer->pixels = NULL;
    framebuffer->pitch_pixels = 0;
    return false;
  }

  framebuffer->index_pixels = (uint8_t *)pixels;
  framebuffer->index_pitch_pixels = pitch_bytes;
  framebuffer->pixels = NULL;
  framebuffer->pitch_pixels = 0;
#else
  if (pitch_bytes < framebuffer->width * (int)sizeof(uint32_t) ||
      (pitch_bytes % (int)sizeof(uint32_t)) != 0) {
    fprintf(stderr, "Cannot render: SDL framebuffer pitch %d is invalid for %dx%d ARGB pixels\n", pitch_bytes,
            framebuffer->width, framebuffer->height);
    SDL_UnlockTexture(framebuffer->texture);
    framebuffer->pixels = NULL;
    framebuffer->pitch_pixels = 0;
    return false;
  }

  framebuffer->pixels = (uint32_t *)pixels;
  framebuffer->pitch_pixels = pitch_bytes / (int)sizeof(uint32_t);
#endif
  return true;
}

static void end_render_framebuffer(RenderFramebuffer *framebuffer) {
  if (framebuffer == NULL || framebuffer->texture == NULL) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL) {
    return;
  }
#else
  if (framebuffer->pixels == NULL) {
    return;
  }
#endif

  SDL_UnlockTexture(framebuffer->texture);
#ifdef GLOOM_DOS_SDL3
  framebuffer->index_pixels = NULL;
  framebuffer->index_pitch_pixels = 0;
#endif
  framebuffer->pixels = NULL;
  framebuffer->pitch_pixels = 0;
}

static void present_framebuffer_texture(SDL_Renderer *renderer, const RenderFramebuffer *framebuffer, int render_width,
                                        int render_height) {
  SDL_Rect dst = {0, 0, render_width, render_height};

  if (renderer == NULL || framebuffer == NULL || framebuffer->texture == NULL) {
    return;
  }
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
  (void)SDL_RenderCopy(renderer, framebuffer->texture, NULL, &dst);
  SDL_RenderPresent(renderer);
#ifdef GLOOM_DOS_SDL3
  audio_dos_pump(&g_audio);
#endif
}

static bool runtime_menu_accepts_mouse(void) {
#ifdef GLOOM_DOS_SDL3
  return false;
#else
  return true;
#endif
}

static void runtime_prepare_menu_input(void) {
#ifdef GLOOM_DOS_SDL3
  (void)SDL_ShowCursor(SDL_DISABLE);
  SDL_FlushEvent(SDL_MOUSEMOTION);
#endif
}

static void framebuffer_clear(RenderFramebuffer *framebuffer, uint32_t argb) {
  size_t count = 0u;
  size_t i = 0u;

  if (framebuffer == NULL || framebuffer->width <= 0 || framebuffer->height <= 0) {
    return;
  }

  count = (size_t)framebuffer->width;
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
  {
    uint8_t index = dos_argb_to_index(argb);

    for (i = 0u; i < (size_t)framebuffer->height; ++i) {
      memset(framebuffer->index_pixels + (i * (size_t)framebuffer->index_pitch_pixels), index, count);
    }
    if (framebuffer->index_coverage != NULL) {
      memset(framebuffer->index_coverage, 0, (size_t)framebuffer->width * (size_t)framebuffer->height);
    }
  }
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }
  for (i = 0u; i < (size_t)framebuffer->height; ++i) {
    uint32_t *row = framebuffer->pixels + (i * (size_t)framebuffer->pitch_pixels);
    size_t x = 0u;

    for (x = 0u; x < count; ++x) {
      row[x] = argb;
    }
  }
#endif
}

static void framebuffer_clear_row_span(RenderFramebuffer *framebuffer, int dst_y, int x, int w, uint32_t argb) {
#ifdef GLOOM_DOS_SDL3
  uint8_t *row = NULL;
  uint8_t index = 0u;
#else
  uint32_t *row = NULL;
#endif
  int col = 0;

  if (framebuffer == NULL || dst_y < 0 || dst_y >= framebuffer->height || w <= 0) {
    return;
  }
  if (x < 0) {
    w += x;
    x = 0;
  }
  if (x + w > framebuffer->width) {
    w = framebuffer->width - x;
  }
  if (w <= 0) {
    return;
  }

#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
  row = framebuffer->index_pixels + ((size_t)dst_y * (size_t)framebuffer->index_pitch_pixels) + (size_t)x;
  index = dos_argb_to_index(argb);
  memset(row, index, (size_t)w);
  if (framebuffer->index_coverage != NULL) {
    memset(framebuffer->index_coverage + ((size_t)dst_y * (size_t)framebuffer->width) + (size_t)x, 0, (size_t)w);
  }
  (void)col;
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }
  row = framebuffer->pixels + ((size_t)dst_y * (size_t)framebuffer->pitch_pixels) + (size_t)x;
  for (col = 0; col < w; ++col) {
    row[col] = argb;
  }
#endif
}

static void snapshot_render_framebuffer(RenderFramebuffer *framebuffer) {
  int y = 0;

#ifdef GLOOM_DOS_SDL3
  if (framebuffer == NULL || framebuffer->index_pixels == NULL || framebuffer->last_frame_indices == NULL ||
      framebuffer->width <= 0 || framebuffer->height <= 0 || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }

  for (y = 0; y < framebuffer->height; ++y) {
    memcpy(framebuffer->last_frame_indices + ((size_t)y * (size_t)framebuffer->width),
           framebuffer->index_pixels + ((size_t)y * (size_t)framebuffer->index_pitch_pixels),
           (size_t)framebuffer->width * sizeof(*framebuffer->last_frame_indices));
  }
#else
  if (framebuffer == NULL || framebuffer->pixels == NULL || framebuffer->last_frame_pixels == NULL ||
      framebuffer->width <= 0 || framebuffer->height <= 0 || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }

  for (y = 0; y < framebuffer->height; ++y) {
    memcpy(framebuffer->last_frame_pixels + ((size_t)y * (size_t)framebuffer->width),
           framebuffer->pixels + ((size_t)y * (size_t)framebuffer->pitch_pixels),
           (size_t)framebuffer->width * sizeof(*framebuffer->last_frame_pixels));
  }
#endif
}

static void apply_player_red_palette_rect(RenderFramebuffer *framebuffer, const AppState *state, int x, int y, int w,
                                          int h) {
  int row_index = 0;

  if (framebuffer == NULL || state == NULL || state->player_palette_timer <= 0.0f) {
    return;
  }

  if (x < 0) {
    w += x;
    x = 0;
  }
  if (y < 0) {
    h += y;
    y = 0;
  }
  if (x + w > framebuffer->width) {
    w = framebuffer->width - x;
  }
  if (y + h > framebuffer->height) {
    h = framebuffer->height - y;
  }
  if (w <= 0 || h <= 0) {
    return;
  }

#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }

  for (row_index = 0; row_index < h; ++row_index) {
    uint8_t *row = framebuffer->index_pixels + ((size_t)(y + row_index) * (size_t)framebuffer->index_pitch_pixels);
    int col = 0;

    for (col = 0; col < w; ++col) {
      row[x + col] = g_dos_red_palette_indices[row[x + col]];
    }
  }
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }

  for (row_index = 0; row_index < h; ++row_index) {
    uint32_t *row = framebuffer->pixels + ((size_t)(y + row_index) * (size_t)framebuffer->pitch_pixels);
    int col = 0;

    for (col = 0; col < w; ++col) {
      uint32_t argb = row[x + col];
      int r = (int)((argb >> 16) & 0xFFu);
      int g = (int)((argb >> 8) & 0xFFu);
      int b = (int)(argb & 0xFFu);
      int red = (r + g + b + (16 * 17)) / 3;

      if (red > 255) {
        red = 255;
      }
      row[x + col] = 0xFF000000u | ((uint32_t)red << 16);
    }
  }
#endif
}

static void apply_player_red_palette(RenderFramebuffer *framebuffer, const AppState *state) {
  if (framebuffer == NULL) {
    return;
  }
  apply_player_red_palette_rect(framebuffer, state, 0, 0, framebuffer->width, framebuffer->height);
}

static float projection_focal_for_viewport(int w, int h) {
  float scale_x = (float)w / (float)GLOOM_AMIGA_VIEW_COLUMNS;
  float scale_y = (float)h / (float)GLOOM_AMIGA_VIEW_ROWS;
  float scale = scale_x > scale_y ? scale_x : scale_y;

  if (scale < 0.01f) {
    scale = 0.01f;
  }

  return (float)GLOOM_PC_VIEW_FOCAL * scale;
}

#ifdef GLOOM_DOS_SDL3
enum {
  DOS_FLAT_FRAC_BITS = 9,
  DOS_FLAT_FRAC_SCALE = 1 << DOS_FLAT_FRAC_BITS,
  DOS_FLAT_TRIG_BITS = 9,
  DOS_FLAT_TRIG_SCALE = 1 << DOS_FLAT_TRIG_BITS
};

static inline size_t dos_flat_texel_offset(int32_t world_x_fixed, int32_t world_z_fixed) {
  return (size_t)((((uint32_t)world_x_fixed >> (DOS_FLAT_FRAC_BITS - 7)) & 0x3F80u) |
                  (((uint32_t)world_z_fixed >> DOS_FLAT_FRAC_BITS) & 0x7Fu));
}

static inline void dos_render_flat_row_shaded(uint8_t *dst, int count, const uint8_t *shaded_texels,
                                              int32_t world_x_fixed, int32_t world_z_fixed,
                                              int32_t world_x_step_fixed, int32_t world_z_step_fixed) {
  if (dst == NULL || shaded_texels == NULL || count <= 0) {
    return;
  }

  while (count >= 4) {
    dst[0] = shaded_texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];
    world_x_fixed += world_x_step_fixed;
    world_z_fixed += world_z_step_fixed;
    dst[1] = shaded_texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];
    world_x_fixed += world_x_step_fixed;
    world_z_fixed += world_z_step_fixed;
    dst[2] = shaded_texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];
    world_x_fixed += world_x_step_fixed;
    world_z_fixed += world_z_step_fixed;
    dst[3] = shaded_texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];
    world_x_fixed += world_x_step_fixed;
    world_z_fixed += world_z_step_fixed;
    dst += 4;
    count -= 4;
  }

  while (count > 0) {
    *dst++ = shaded_texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];
    world_x_fixed += world_x_step_fixed;
    world_z_fixed += world_z_step_fixed;
    count -= 1;
  }
}
#endif

static void render_flat_textures(RenderFramebuffer *framebuffer, const AppState *state, const FlatTextureSet *flats, int x,
                                 int y, int w, int h, float focal, float far_depth, float view_cos,
                                 float view_sin
#ifdef GLOOM_DOS_SDL3
                                 ,
                                 bool skip_existing_pixels
#endif
) {
#ifdef GLOOM_DOS_SDL3
  int32_t eye_height = 0;
  int32_t ceiling_delta = 0;
  int32_t cam_x_fixed = 0;
  int32_t cam_z_fixed = 0;
  int32_t cos_fixed = 0;
  int32_t sin_fixed = 0;
  int focal_int = 0;
  int far_depth_int = 0;
  int row = 0;
#else
  float eye_height = 0.0f;
  float ceiling_delta = 0.0f;
  int row = 0;
#endif

  if (framebuffer == NULL || state == NULL || flats == NULL || !flats->floor.loaded || !flats->roof.loaded) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
  if (skip_existing_pixels && framebuffer->index_coverage == NULL) {
    return;
  }
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }
#endif

#ifdef GLOOM_DOS_SDL3
  eye_height = round_float_to_int32(-state->camera_y);
  ceiling_delta = round_float_to_int32(255.0f + state->camera_y);
  cam_x_fixed = round_float_to_int32(state->camera_x * (float)DOS_FLAT_FRAC_SCALE);
  cam_z_fixed = round_float_to_int32(state->camera_z * (float)DOS_FLAT_FRAC_SCALE);
  cos_fixed = round_float_to_int32(view_cos * (float)DOS_FLAT_TRIG_SCALE);
  sin_fixed = round_float_to_int32(view_sin * (float)DOS_FLAT_TRIG_SCALE);
  focal_int = round_float_to_int32(focal);
  far_depth_int = round_float_to_int32(far_depth);

  if (eye_height < 1) {
    eye_height = 1;
  }
  if (ceiling_delta < 1) {
    ceiling_delta = 1;
  }
  if (focal_int < 1) {
    focal_int = 1;
  }
  if (far_depth_int < 2) {
    far_depth_int = 2;
  }
#else
  eye_height = -state->camera_y;
  ceiling_delta = 255.0f + state->camera_y;

  if (eye_height < 1.0f) {
    eye_height = 1.0f;
  }
  if (ceiling_delta < 1.0f) {
    ceiling_delta = 1.0f;
  }
#endif

  for (row = 0; row < h; ++row) {
#ifdef GLOOM_DOS_SDL3
    int centered_y = row - (h / 2);
    const FlatTexture *texture = NULL;
    const uint8_t *texels = NULL;
    const uint8_t *palette = NULL;
    const uint8_t *shaded_texels = NULL;
    int32_t plane_distance = 0;
    int32_t depth = 0;
    int32_t depth_fixed = 0;
    int32_t view_x_fixed = 0;
    int32_t view_x_step_fixed = 0;
    int32_t world_x_fixed = 0;
    int32_t world_z_fixed = 0;
    int32_t world_x_step_fixed = 0;
    int32_t world_z_step_fixed = 0;
    uint8_t *dst_row = NULL;
    uint8_t *coverage_row = NULL;
    int abs_centered_y = 0;
    int col = 0;

    if (centered_y > 0) {
      texture = &flats->floor;
      plane_distance = eye_height;
      abs_centered_y = centered_y;
    } else if (centered_y < 0) {
      texture = &flats->roof;
      plane_distance = ceiling_delta;
      abs_centered_y = -centered_y;
    } else {
      if (!skip_existing_pixels) {
        framebuffer_clear_row_span(framebuffer, y + row, x, w, 0xFF000000u);
      }
      continue;
    }

    depth = (int32_t)(((int64_t)plane_distance * (int64_t)focal_int) / (int64_t)abs_centered_y);
    if (depth < 1 || depth >= far_depth_int) {
      if (!skip_existing_pixels) {
        framebuffer_clear_row_span(framebuffer, y + row, x, w, 0xFF000000u);
      }
      continue;
    }

    texels = texture->texels;
    {
      int shade_index = (int)amiga_depth_dark_index_int(depth);

      palette = texture->shaded_indices[shade_index];
      if (texture->shaded_texels != NULL) {
        shaded_texels = texture->shaded_texels +
                        ((size_t)shade_index * (size_t)GLOOM_FLAT_TEXTURE_SIZE *
                         (size_t)GLOOM_FLAT_TEXTURE_SIZE);
      }
    }
    depth_fixed = depth << DOS_FLAT_FRAC_BITS;
    view_x_fixed = ((((0 - (w / 2)) * depth) << DOS_FLAT_FRAC_BITS) / focal_int);
    view_x_step_fixed = ((depth << DOS_FLAT_FRAC_BITS) / focal_int);
    world_x_fixed = cam_x_fixed + ((view_x_fixed * cos_fixed) >> DOS_FLAT_TRIG_BITS) +
                    ((depth_fixed * sin_fixed) >> DOS_FLAT_TRIG_BITS);
    world_z_fixed = cam_z_fixed - ((view_x_fixed * sin_fixed) >> DOS_FLAT_TRIG_BITS) +
                    ((depth_fixed * cos_fixed) >> DOS_FLAT_TRIG_BITS);
    world_x_step_fixed = ((view_x_step_fixed * cos_fixed) >> DOS_FLAT_TRIG_BITS);
    world_z_step_fixed = -((view_x_step_fixed * sin_fixed) >> DOS_FLAT_TRIG_BITS);
    dst_row = framebuffer->index_pixels + ((size_t)(y + row) * (size_t)framebuffer->index_pitch_pixels + (size_t)x);
    coverage_row = skip_existing_pixels && framebuffer->index_coverage != NULL
                       ? framebuffer->index_coverage +
                             ((size_t)(y + row) * (size_t)framebuffer->width + (size_t)x)
                       : NULL;

    if (skip_existing_pixels && shaded_texels != NULL) {
      for (col = 0; col < w; ++col) {
        if (coverage_row[col] == 0u) {
          dst_row[col] = shaded_texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];
        }
        world_x_fixed += world_x_step_fixed;
        world_z_fixed += world_z_step_fixed;
      }
    } else if (skip_existing_pixels) {
      for (col = 0; col < w; ++col) {
        if (coverage_row[col] == 0u) {
          uint8_t palette_index = texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];

          dst_row[col] = palette[palette_index];
        }
        world_x_fixed += world_x_step_fixed;
        world_z_fixed += world_z_step_fixed;
      }
    } else if (shaded_texels != NULL) {
      dos_render_flat_row_shaded(dst_row, w, shaded_texels, world_x_fixed, world_z_fixed, world_x_step_fixed,
                                 world_z_step_fixed);
    } else {
      for (col = 0; col < w; ++col) {
        uint8_t palette_index = texels[dos_flat_texel_offset(world_x_fixed, world_z_fixed)];

        dst_row[col] = palette[palette_index];
        world_x_fixed += world_x_step_fixed;
        world_z_fixed += world_z_step_fixed;
      }
    }
#else
    float centered_y = (float)(row - (h / 2));
    const FlatTexture *texture = NULL;
    const uint8_t *texels = NULL;
    const uint32_t *palette = NULL;
    float plane_distance = 0.0f;
    float depth = 0.0f;
    float view_x = 0.0f;
    float view_x_step = 0.0f;
    float world_x = 0.0f;
    float world_z = 0.0f;
    float world_x_step = 0.0f;
    float world_z_step = 0.0f;
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
      framebuffer_clear_row_span(framebuffer, y + row, x, w, 0xFF000000u);
      continue;
    }

    if (depth < 1.0f || depth >= far_depth) {
      framebuffer_clear_row_span(framebuffer, y + row, x, w, 0xFF000000u);
      continue;
    }

    texels = texture->texels;
    view_x = ((float)(0 - (w / 2)) * depth) / focal;
    view_x_step = depth / focal;
    world_x = state->camera_x + (view_x * view_cos) + (depth * view_sin);
    world_z = state->camera_z - (view_x * view_sin) + (depth * view_cos);
    world_x_step = view_x_step * view_cos;
    world_z_step = -view_x_step * view_sin;
    palette = texture->shaded_palette[amiga_depth_dark_index(depth)];

    for (col = 0; col < w; ++col) {
      size_t dst = (size_t)(y + row) * (size_t)framebuffer->pitch_pixels + (size_t)(x + col);
      int tx = floor_to_int(world_x) & (GLOOM_FLAT_TEXTURE_SIZE - 1);
      int tz = floor_to_int(world_z) & (GLOOM_FLAT_TEXTURE_SIZE - 1);
      uint8_t palette_index = texels[(tx * GLOOM_FLAT_TEXTURE_SIZE) + tz];
      uint32_t argb = palette[palette_index];

      framebuffer->pixels[dst] = 0xFF000000u | (argb & 0x00FFFFFFu);
      world_x += world_x_step;
      world_z += world_z_step;
    }
#endif
  }
}

#ifdef GLOOM_DOS_SDL3
static int32_t dos_mul_shift15(int32_t a, int32_t b) {
  return (a * b) >> 15;
}

static int dos_project_view_x(int32_t view_x, int32_t view_z, int focal, int half_width) {
  if (view_z == 0) {
    return view_x < 0 ? -32768 : 32767;
  }

  return half_width + (int)((view_x * focal) / view_z);
}

static void bind_dos_wall_column_texture(const WallTextureSet *wall_textures, const AppState *state,
                                         const GloomZone *zone, int32_t wall_u, int dark_index,
                                         DosRayWallHit *hit) {
  int tile_index = 0;
  uint8_t texture_index = 0u;
  size_t screen_index = 0u;
  size_t local_index = 0u;
  size_t tx = 0u;

  if (hit == NULL) {
    return;
  }

  hit->fast_wall_column = false;
  hit->transparent_wall_column = true;
  hit->wall_texels = NULL;
  hit->wall_shaded_texels = NULL;
  hit->wall_palette = NULL;
  hit->wall_texel_base = 0u;

  if (wall_textures == NULL || state == NULL || zone == NULL || dark_index < 0 || dark_index >= 16) {
    return;
  }

  zone_wall_texture_coordinates_fixed16(zone, wall_u, &tile_index, &tx);
  texture_index = remap_wall_texture_index(state, zone->textures[tile_index]);
  screen_index = texture_index / (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
  local_index = texture_index % (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
  if (screen_index < (size_t)GLOOM_TEXTURE_SCREENS && wall_textures->screens[screen_index].loaded &&
      wall_textures->screens[screen_index].texels != NULL &&
      wall_textures->screens[screen_index].column_texels != NULL &&
      local_index < wall_textures->screens[screen_index].texture_count) {
    const WallTextureScreen *screen = &wall_textures->screens[screen_index];

    hit->wall_texels = screen->column_texels;
    if (screen->column_shaded_texels != NULL) {
      hit->wall_shaded_texels =
          screen->column_shaded_texels +
          ((size_t)dark_index * screen->texture_count * (size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT);
    }
    hit->wall_palette = screen->shaded_indices[dark_index];
    hit->wall_texel_base = local_index * ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT) +
                           tx * (size_t)GLOOM_TEXTURE_HEIGHT;
    hit->transparent_wall_column =
        screen->column_flags != NULL &&
        screen->column_flags[(local_index * (size_t)GLOOM_TEXTURE_WIDTH) + tx] != 0u;
    hit->fast_wall_column = true;
  }
}

static void render_wall_columns_dos_fixed(RenderFramebuffer *framebuffer, const AppState *state,
                                          const WallTextureSet *wall_textures,
                                          const WallCandidate *wall_candidates, size_t wall_candidate_count, int x,
                                          int y, int w, int h, int horizon_y, float view_cos, float view_sin,
                                          float focal) {
  DosSceneWall scene_walls[GLOOM_MAX_WALL_CANDIDATES];
  int32_t *depth_buffer = NULL;
  uint32_t *filled_stamps = NULL;
  size_t scene_wall_count = 0u;
  size_t i = 0u;
  int focal_int = round_float_to_int32(focal);
  int half_width = w / 2;
  int32_t cam_x = 0;
  int32_t cam_z = 0;
  int32_t cam_y = 0;
  int32_t cos_fixed = round_float_to_int32(view_cos * 32767.0f);
  int32_t sin_fixed = round_float_to_int32(view_sin * 32767.0f);

  if (framebuffer == NULL || state == NULL || framebuffer->index_pixels == NULL ||
      framebuffer->index_pitch_pixels < framebuffer->width || framebuffer->depth_buffer_int == NULL ||
      framebuffer->filled_stamps == NULL) {
    return;
  }
  depth_buffer = framebuffer->depth_buffer_int;
  filled_stamps = framebuffer->filled_stamps;
  cam_x = round_float_to_int32(state->camera_x);
  cam_z = round_float_to_int32(state->camera_z);
  cam_y = round_float_to_int32(state->camera_y);

  if (focal_int < 1) {
    focal_int = 1;
  }

  for (i = 0u; i < wall_candidate_count; ++i) {
    const GloomZone *z = &state->map.zones[wall_candidates[i].zone_index];
    int32_t x1 = (int32_t)z->x1 - cam_x;
    int32_t z1 = (int32_t)z->z1 - cam_z;
    int32_t x2 = (int32_t)z->x2 - cam_x;
    int32_t z2 = (int32_t)z->z2 - cam_z;
    int32_t vx1 = dos_mul_shift15(x1, cos_fixed) - dos_mul_shift15(z1, sin_fixed);
    int32_t vz1 = dos_mul_shift15(x1, sin_fixed) + dos_mul_shift15(z1, cos_fixed);
    int32_t vx2 = dos_mul_shift15(x2, cos_fixed) - dos_mul_shift15(z2, sin_fixed);
    int32_t vz2 = dos_mul_shift15(x2, sin_fixed) + dos_mul_shift15(z2, cos_fixed);
    int32_t face_a = vz1 - vz2;
    int32_t face_b = vx2 - vx1;
    int64_t face_c = ((int64_t)vx1 * (int64_t)face_a) + ((int64_t)vz1 * (int64_t)face_b);
    DosSceneWall *scene_wall = NULL;

    if (vz1 <= 0 && vz2 <= 0) {
      continue;
    }
    if (vz1 >= GLOOM_AMIGA_MAX_Z && vz2 >= GLOOM_AMIGA_MAX_Z) {
      continue;
    }
    if (face_c < 0) {
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
    scene_wall->sz = vz2 - vz1;
    scene_wall->column_min = 0;
    scene_wall->column_max = w - 1;

    if (vz1 > 1 && vz2 > 1) {
      int sx1 = dos_project_view_x(vx1, vz1, focal_int, half_width);
      int sx2 = dos_project_view_x(vx2, vz2, focal_int, half_width);
      int min_x = sx1 < sx2 ? sx1 : sx2;
      int max_x = sx1 > sx2 ? sx1 : sx2;

      scene_wall->column_min = min_x - 2;
      scene_wall->column_max = max_x + 2;
      if (scene_wall->column_min < 0) scene_wall->column_min = 0;
      if (scene_wall->column_max > w - 1) scene_wall->column_max = w - 1;
    }

    if (scene_wall->column_min > scene_wall->column_max) {
      scene_wall_count -= 1u;
    }
  }

  {
    int32_t ray_x = ((-half_width * 65536) / focal_int);
    int32_t ray_x_step = 65536 / focal_int;

    for (i = 0u; i < (size_t)w; ++i, ray_x += ray_x_step) {
      int screen_x = x + (int)i;
      DosRayWallHit hits[GLOOM_MAX_COLUMN_WALL_HITS];
      size_t hit_count = 0u;
      size_t wall_index = 0u;
      size_t hit_index = 0u;
      uint32_t filled_stamp = 0u;
      double profile_column_start_ms = 0.0;
      bool profile_column_sampled = false;

      framebuffer->filled_stamp += 1u;
      if (framebuffer->filled_stamp == 0u) {
        memset(filled_stamps, 0, (size_t)h * sizeof(*filled_stamps));
        framebuffer->filled_stamp = 1u;
      }
      filled_stamp = framebuffer->filled_stamp;

      profile_column_sampled = g_profiler.enabled && ((i & 15u) == 0u);
      if (profile_column_sampled) {
        profile_column_start_ms = profile_now_ms();
      }
      for (wall_index = 0u; wall_index < scene_wall_count; ++wall_index) {
        const DosSceneWall *wall = &scene_walls[wall_index];
        int32_t side1 = wall->vx1 - ((wall->vz1 * ray_x) >> 16);
        int32_t side2 = wall->vx2 - ((wall->vz2 * ray_x) >> 16);
        int32_t side_delta = side2 - side1;
        int32_t depth = 0;
        int32_t wall_u = 0;
        int dark_index = 0;
        bool occluded_by_opaque = false;
        size_t existing_hit = 0u;

        if ((int)i < wall->column_min || (int)i > wall->column_max || side_delta == 0) {
          continue;
        }
        if ((side1 < 0 && side2 < 0) || (side1 > 0 && side2 > 0)) {
          continue;
        }

        wall_u = (-side1 * 65536) / side_delta;
        if (wall_u < -33 || wall_u > 0x10000 + 33) {
          continue;
        }
        wall_u = dos_fixed16_clamp_unit(wall_u);

        depth = wall->vz1 + ((wall->sz * wall_u) >> 16);
        if (depth < 1 || depth >= GLOOM_AMIGA_MAX_Z) {
          continue;
        }

        if (wall->zone->event_id > 0) {
          int32_t open_threshold = (int32_t)((uint16_t)wall->zone->event_id << 1u);

          if (wall_u < open_threshold) {
            continue;
          }
        }

        dark_index = (int)amiga_depth_dark_index_int(depth);
        {
          DosRayWallHit hit;
          size_t insert_at = hit_count;

          hit.wall = wall;
          hit.depth = depth;
          hit.wall_u = wall_u;
          bind_dos_wall_column_texture(wall_textures, state, wall->zone, wall_u, dark_index, &hit);

          for (existing_hit = 0u; existing_hit < hit_count; ++existing_hit) {
            if (!hits[existing_hit].transparent_wall_column && hits[existing_hit].depth <= hit.depth) {
              occluded_by_opaque = true;
              break;
            }
          }
          if (occluded_by_opaque) {
            continue;
          }
          if (!hit.transparent_wall_column) {
            while (hit_count > 0u && hits[hit_count - 1u].depth > hit.depth) {
              hit_count -= 1u;
            }
            insert_at = hit_count;
          }

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
      if (profile_column_sampled) {
        g_profiler.render_wall_search_ms += (profile_now_ms() - profile_column_start_ms) * 16.0;
      }

      if (profile_column_sampled) {
        profile_column_start_ms = profile_now_ms();
      }
      for (hit_index = 0u; hit_index < hit_count; ++hit_index) {
        const DosRayWallHit *hit = &hits[hit_index];
        int32_t wall_top = horizon_y + (((GLOOM_WALL_TOP_Y - cam_y) * focal_int) / hit->depth);
        int32_t wall_bottom = horizon_y + (((GLOOM_WALL_BOTTOM_Y - cam_y) * focal_int) / hit->depth);
        int32_t wall_height = wall_bottom - wall_top;
        int y0 = (int)wall_top;
        int y1 = (int)wall_bottom;
        int draw_y = 0;
        int subtract = 0;
        bool fast_wall_column = hit->fast_wall_column;
        bool transparent_wall_column = hit->transparent_wall_column;
        const uint8_t *wall_texels = hit->wall_texels;
        const uint8_t *wall_shaded_texels = hit->wall_shaded_texels;
        const uint8_t *wall_palette = hit->wall_palette;
        size_t wall_texel_base = hit->wall_texel_base;

        if (wall_height < 1) {
          int32_t center_y = (wall_top + wall_bottom) / 2;

          wall_top = center_y;
          wall_bottom = center_y + 1;
          wall_height = 1;
          y0 = (int)wall_top;
          y1 = (int)wall_bottom;
        }

        if (y0 < y) y0 = y;
        if (y1 > y + h - 1) y1 = y + h - 1;
        if (y0 > y1 || wall_height <= 0) {
          continue;
        }

        subtract = hit->fast_wall_column ? 0 : (int)amiga_depth_dark_index_int(hit->depth) * 17;

        {
          int32_t wall_v_fixed = ((((int32_t)y0 - wall_top) << 16) + 32768) / wall_height;
          int32_t wall_v_step_fixed = (int32_t)(0x10000 / wall_height);
          int32_t tex_y_fixed = wall_v_fixed * (GLOOM_TEXTURE_HEIGHT - 1);
          int32_t tex_y_step_fixed = wall_v_step_fixed * (GLOOM_TEXTURE_HEIGHT - 1);

          if (fast_wall_column) {
            uint8_t *dst = framebuffer->index_pixels + ((size_t)y0 * (size_t)framebuffer->index_pitch_pixels) +
                           (size_t)screen_x;
            uint32_t *stamp = filled_stamps + (y0 - y);
            bool column_drawn = false;

            if (!transparent_wall_column && hit_index == 0u) {
              if (wall_shaded_texels != NULL) {
                for (draw_y = y0; draw_y <= y1; ++draw_y) {
                  size_t ty = (size_t)(tex_y_fixed >> 16);

                  if (ty >= (size_t)GLOOM_TEXTURE_HEIGHT) {
                    ty = (size_t)GLOOM_TEXTURE_HEIGHT - 1u;
                  }
                  *dst = wall_shaded_texels[wall_texel_base + ty];
                  dst += framebuffer->index_pitch_pixels;
                  tex_y_fixed += tex_y_step_fixed;
                }
              } else {
                for (draw_y = y0; draw_y <= y1; ++draw_y) {
                  size_t ty = (size_t)(tex_y_fixed >> 16);
                  uint8_t palette_index = 0u;

                  if (ty >= (size_t)GLOOM_TEXTURE_HEIGHT) {
                    ty = (size_t)GLOOM_TEXTURE_HEIGHT - 1u;
                  }
                  palette_index = wall_texels[wall_texel_base + ty];
                  *dst = wall_palette[palette_index];
                  dst += framebuffer->index_pitch_pixels;
                  tex_y_fixed += tex_y_step_fixed;
                }
              }
              column_drawn = true;
            } else if (!transparent_wall_column) {
              if (wall_shaded_texels != NULL) {
                for (draw_y = y0; draw_y <= y1; ++draw_y) {
                  if (*stamp != filled_stamp) {
                    size_t ty = (size_t)(tex_y_fixed >> 16);

                    if (ty >= (size_t)GLOOM_TEXTURE_HEIGHT) {
                      ty = (size_t)GLOOM_TEXTURE_HEIGHT - 1u;
                    }
                    *dst = wall_shaded_texels[wall_texel_base + ty];
                    *stamp = filled_stamp;
                    column_drawn = true;
                  }
                  dst += framebuffer->index_pitch_pixels;
                  stamp += 1;
                  tex_y_fixed += tex_y_step_fixed;
                }
              } else {
                for (draw_y = y0; draw_y <= y1; ++draw_y) {
                  if (*stamp != filled_stamp) {
                    size_t ty = (size_t)(tex_y_fixed >> 16);
                    uint8_t palette_index = 0u;

                    if (ty >= (size_t)GLOOM_TEXTURE_HEIGHT) {
                      ty = (size_t)GLOOM_TEXTURE_HEIGHT - 1u;
                    }
                    palette_index = wall_texels[wall_texel_base + ty];
                    *dst = wall_palette[palette_index];
                    *stamp = filled_stamp;
                    column_drawn = true;
                  }
                  dst += framebuffer->index_pitch_pixels;
                  stamp += 1;
                  tex_y_fixed += tex_y_step_fixed;
                }
              }
            } else if (hit_index == 0u) {
              for (draw_y = y0; draw_y <= y1; ++draw_y) {
                size_t ty = (size_t)(tex_y_fixed >> 16);
                uint8_t palette_index = 0u;

                if (ty >= (size_t)GLOOM_TEXTURE_HEIGHT) {
                  ty = (size_t)GLOOM_TEXTURE_HEIGHT - 1u;
                }
                palette_index = wall_texels[wall_texel_base + ty];
                if (palette_index != 0u) {
                  *dst = wall_palette[palette_index];
                  *stamp = filled_stamp;
                  column_drawn = true;
                }
                dst += framebuffer->index_pitch_pixels;
                stamp += 1;
                tex_y_fixed += tex_y_step_fixed;
              }
            } else {
              for (draw_y = y0; draw_y <= y1; ++draw_y) {
                if (*stamp != filled_stamp) {
                  size_t ty = (size_t)(tex_y_fixed >> 16);
                  uint8_t palette_index = 0u;

                  if (ty >= (size_t)GLOOM_TEXTURE_HEIGHT) {
                    ty = (size_t)GLOOM_TEXTURE_HEIGHT - 1u;
                  }
                  palette_index = wall_texels[wall_texel_base + ty];
                  *dst = wall_palette[palette_index];
                  *stamp = filled_stamp;
                  column_drawn = true;
                }
                dst += framebuffer->index_pitch_pixels;
                stamp += 1;
                tex_y_fixed += tex_y_step_fixed;
              }
            }
            if (column_drawn && hit->depth < depth_buffer[i]) {
              depth_buffer[i] = hit->depth;
            }
            if (!transparent_wall_column) {
              break;
            }
          } else {
            for (draw_y = y0; draw_y <= y1; ++draw_y) {
              uint8_t alpha = 0u;
              float wall_u = (float)hit->wall_u / 65536.0f;
              float wall_v = (float)wall_v_fixed / 65536.0f;
              uint32_t argb = 0u;
              uint8_t color_index = 0u;

              wall_v_fixed += wall_v_step_fixed;
              if (filled_stamps[draw_y - y] == filled_stamp) {
                tex_y_fixed += tex_y_step_fixed;
                continue;
              }

              argb = sample_zone_wall_texture_argb(wall_textures, state, hit->wall->zone, wall_u, wall_v);
              argb = shade_argb_subtract(argb, subtract);
              tex_y_fixed += tex_y_step_fixed;
              alpha = (uint8_t)(argb >> 24);

              if (alpha == 0u) {
                continue;
              }
              color_index = dos_argb_to_opaque_index(argb);

              framebuffer->index_pixels[(size_t)draw_y * (size_t)framebuffer->index_pitch_pixels + (size_t)screen_x] =
                  color_index;
              filled_stamps[draw_y - y] = filled_stamp;

              if (hit->depth < depth_buffer[i]) {
                depth_buffer[i] = hit->depth;
              }
            }
          }
        }
      }
      if (profile_column_sampled) {
        g_profiler.render_wall_draw_ms += (profile_now_ms() - profile_column_start_ms) * 16.0;
      }
    }
  }
}
#endif

static void append_player_debug_sprite(const AppState *state, const ObjectVisualSet *object_visuals,
                                       const RuntimePlayerState *player, int16_t object_type, DebugSprite *sprites,
                                       size_t *sprite_write, size_t sprite_capacity, int x, int w, int horizon_y,
                                       float cam_x, float cam_z, float view_cos, float view_sin, float focal,
                                       float near_plane, float far_depth, float frustum_ratio) {
  GloomObjectSpawn render_spawn;
  const ObjectVisual *visual = NULL;
  const ObjectFrame *frame = NULL;
  float dx = 0.0f;
  float dz = 0.0f;
  float svx = 0.0f;
  float svz = 0.0f;
  float object_y = 0.0f;
  float scale = 1.0f;
  float top_view_y = 0.0f;
  float left_view_x = 0.0f;
  DebugSprite *sp = NULL;

  if (state == NULL || object_visuals == NULL || player == NULL || sprites == NULL || sprite_write == NULL ||
      *sprite_write >= sprite_capacity || object_type < 0 || object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
    return;
  }
  if (player->player_lives <= 0 || player->player_hitpoints <= 0 || player->player_dead ||
      player->player_death_phase != GLOOM_PLAYER_DEATH_NONE) {
    return;
  }

  visual = &object_visuals->visuals[object_type];
  if (!visual->loaded) {
    return;
  }

  memset(&render_spawn, 0, sizeof(render_spawn));
  render_spawn.object_type = object_type;
  render_spawn.x = clamp_int16(round_float_to_int32(player->camera_x));
  render_spawn.y = 0;
  render_spawn.z = clamp_int16(round_float_to_int32(player->camera_z));
  render_spawn.rotation = player_rotation_fixed_to_amiga(player->player_rot_fixed);
  frame = select_object_visual_frame_number(visual, &render_spawn, state, (size_t)(player->player_frame_fixed >> 16u));
  if (frame == NULL || frame->width <= 0 || frame->height <= 0) {
    return;
  }

  dx = player->camera_x - cam_x;
  dz = player->camera_z - cam_z;
  svx = dx * view_cos - dz * view_sin;
  svz = dx * view_sin + dz * view_cos;
  if (svz < near_plane || svz >= far_depth || SDL_fabsf(svx) > svz * frustum_ratio * 1.1f) {
    return;
  }

  object_y = (float)render_spawn.y - state->camera_y;
  scale = (float)visual->scale / 256.0f;
  top_view_y = object_y - ((float)frame->handle_y * scale);
  left_view_x = svx - ((float)frame->handle_x * scale);

  sp = &sprites[*sprite_write];
  sp->screen_x = (float)x + (float)w * 0.5f + (left_view_x / svz) * focal;
  sp->screen_y = (float)horizon_y + (top_view_y / svz) * focal;
  sp->screen_w = ((float)frame->width * scale * focal) / svz;
  sp->screen_h = ((float)frame->height * scale * focal) / svz;
  bind_debug_sprite_depth(sp, svz);
  sp->source_pad = 0;
  bind_debug_sprite_frame(sp, visual, frame);

  if (sp->screen_w >= 1.0f && sp->screen_h >= 1.0f) {
    *sprite_write += 1u;
  }
}

static int projectile_sprite_source_pad(uint8_t weapon_index) {
  static const int source_pad_by_weapon[GLOOM_WEAPON_COUNT] = {0, 2, 3, 3, 4};

  if (weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
    return 0;
  }

  /*
   * bullet1.bin already has the expected edge shape. The larger green/purple original
   * bullet*.bin frames reach their source bounds, so the PC billboard gets transparent
   * source padding without changing the original pixels or gameplay scale.
   */
  return source_pad_by_weapon[weapon_index];
}

static void render_wall_debug(RenderFramebuffer *framebuffer, const AppState *state, const GridOffsetSet *grid_offsets,
                              const WallTextureSet *wall_textures, const FlatTextureSet *flats,
                              const ObjectVisualSet *object_visuals, const WeaponVisualSet *weapon_visuals, int x,
                              int y, int w, int h) {
  float near_plane = 1.0f;
  float view_angle = state->camera_angle;
  float cam_x = state->camera_x;
  float cam_z = state->camera_z;
  float view_cos = SDL_cosf(view_angle);
  float view_sin = SDL_sinf(view_angle);
  float focal = projection_focal_for_viewport(w, h);
  float frustum_ratio = ((float)w * 0.5f) / focal;
  float far_depth = (float)GLOOM_AMIGA_MAX_Z;
#ifdef GLOOM_DOS_SDL3
  int32_t *depth_buffer_int = NULL;
  int32_t *sprite_depth_buffer_int = NULL;
#else
  float *depth_buffer = NULL;
  float *sprite_depth_buffer = NULL;
#endif
  uint32_t *filled_stamps = NULL;
  DebugSprite debug_sprites[GLOOM_MAX_DEBUG_SPRITES];
  WallCandidate wall_candidates[GLOOM_MAX_WALL_CANDIDATES];
#ifndef GLOOM_DOS_SDL3
  SceneWall scene_walls[GLOOM_MAX_WALL_CANDIDATES];
#endif
  size_t wall_candidate_count = 0;
#ifndef GLOOM_DOS_SDL3
  size_t scene_wall_count = 0;
#endif
  size_t sprite_write = 0;
  size_t i = 0;
  int horizon_y = y + (h / 2);
  double profile_start_ms = 0.0;
#ifndef GLOOM_DOS_SDL3
  double profile_wall_phase_start_ms = 0.0;
#endif

  if (w <= 0 || h <= 0) {
    return;
  }

  if (wall_textures == NULL || wall_textures->loaded_count == 0u) {
    return;
  }

#ifdef GLOOM_DOS_SDL3
  depth_buffer_int = framebuffer != NULL ? framebuffer->depth_buffer_int : NULL;
  sprite_depth_buffer_int = framebuffer != NULL ? framebuffer->sprite_depth_buffer_int : NULL;
#else
  depth_buffer = framebuffer != NULL ? framebuffer->depth_buffer : NULL;
  sprite_depth_buffer = framebuffer != NULL ? framebuffer->sprite_depth_buffer : NULL;
#endif
  filled_stamps = framebuffer != NULL ? framebuffer->filled_stamps : NULL;
#ifdef GLOOM_DOS_SDL3
  if (depth_buffer_int == NULL || sprite_depth_buffer_int == NULL || filled_stamps == NULL ||
      framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
#else
  if (depth_buffer == NULL || sprite_depth_buffer == NULL || filled_stamps == NULL) {
#endif
    return;
  }
  memset(debug_sprites, 0, sizeof(debug_sprites));

  wall_candidate_count =
      collect_wall_candidates_from_grid(&state->map, grid_offsets, cam_x, cam_z, wall_candidates,
                                        sizeof(wall_candidates) / sizeof(wall_candidates[0]));
  append_runtime_rotpoly_wall_candidates(state, wall_candidates, &wall_candidate_count,
                                         sizeof(wall_candidates) / sizeof(wall_candidates[0]), cam_x, cam_z);
  if (g_profiler.enabled) {
    g_profiler.wall_candidates += (uint64_t)wall_candidate_count;
  }
#ifndef GLOOM_DOS_SDL3
  sort_wall_candidates(wall_candidates, wall_candidate_count);
#endif

  for (i = 0; i < (size_t)w; ++i) {
#ifdef GLOOM_DOS_SDL3
    depth_buffer_int[i] = INT32_MAX;
    sprite_depth_buffer_int[i] = INT32_MAX;
#else
    depth_buffer[i] = 1.0e9f;
    sprite_depth_buffer[i] = 1.0e9f;
#endif
  }

#ifdef GLOOM_DOS_SDL3
  profile_start_ms = profile_now_ms();
  render_flat_textures(framebuffer, state, flats, x, y, w, h, focal, far_depth, view_cos, view_sin, false);
  profile_add_elapsed(&g_profiler.render_flat_ms, profile_start_ms);
  audio_dos_pump(&g_audio);

  profile_start_ms = profile_now_ms();
  render_wall_columns_dos_fixed(framebuffer, state, wall_textures, wall_candidates, wall_candidate_count, x, y, w, h,
                                horizon_y, view_cos, view_sin, focal);
  profile_add_elapsed(&g_profiler.render_wall_ms, profile_start_ms);
  audio_dos_pump(&g_audio);
#else
  profile_start_ms = profile_now_ms();
  render_flat_textures(framebuffer, state, flats, x, y, w, h, focal, far_depth, view_cos, view_sin);
  profile_add_elapsed(&g_profiler.render_flat_ms, profile_start_ms);

  profile_start_ms = profile_now_ms();
  if (g_profiler.enabled) {
    profile_wall_phase_start_ms = profile_now_ms();
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
    scene_wall->column_min = 0;
    scene_wall->column_max = w - 1;
    if (vz1 > near_plane && vz2 > near_plane) {
      float ratio1 = vx1 / vz1;
      float ratio2 = vx2 / vz2;
      float min_ratio = ratio1 < ratio2 ? ratio1 : ratio2;
      float max_ratio = ratio1 > ratio2 ? ratio1 : ratio2;

      scene_wall->column_min = (int)((float)(w / 2) + (min_ratio * focal)) - 2;
      scene_wall->column_max = (int)((float)(w / 2) + (max_ratio * focal)) + 2;
      if (scene_wall->column_min < 0) scene_wall->column_min = 0;
      if (scene_wall->column_max > w - 1) scene_wall->column_max = w - 1;
    }
    if (scene_wall->column_min > scene_wall->column_max) {
      scene_wall_count -= 1u;
    }
  }
  if (g_profiler.enabled) {
    profile_add_elapsed(&g_profiler.render_wall_search_ms, profile_wall_phase_start_ms);
  }

  for (i = 0; i < (size_t)w; ++i) {
    int screen_x = x + (int)i;
    float ray_x = ((float)((int)i - (w / 2))) / focal;
    RayWallHit hits[GLOOM_MAX_COLUMN_WALL_HITS];
    size_t hit_count = 0u;
    size_t wall_index = 0;
    size_t hit_index = 0;
    uint32_t filled_stamp = 0u;

    framebuffer->filled_stamp += 1u;
    if (framebuffer->filled_stamp == 0u) {
      memset(filled_stamps, 0, (size_t)h * sizeof(*filled_stamps));
      framebuffer->filled_stamp = 1u;
    }
    filled_stamp = framebuffer->filled_stamp;

    if (g_profiler.enabled) {
      profile_wall_phase_start_ms = profile_now_ms();
    }
    for (wall_index = 0; wall_index < scene_wall_count; ++wall_index) {
      const SceneWall *wall = &scene_walls[wall_index];
      float sx = wall->vx2 - wall->vx1;
      float sz = wall->vz2 - wall->vz1;
      float denom = ray_x * sz - sx;
      float depth = 0.0f;
      float wall_u = 0.0f;

      if ((int)i < wall->column_min || (int)i > wall->column_max) {
        continue;
      }

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
    if (g_profiler.enabled) {
      profile_add_elapsed(&g_profiler.render_wall_search_ms, profile_wall_phase_start_ms);
    }

    if (g_profiler.enabled) {
      profile_wall_phase_start_ms = profile_now_ms();
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
      int subtract = 0;
      int dark_index = 0;
      bool fast_wall_column = false;
      bool transparent_wall_column = false;
      const uint8_t *wall_texels = NULL;
      const uint32_t *wall_palette = NULL;
      size_t wall_texel_base = 0u;

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

      dark_index = (int)amiga_depth_dark_index(hit->depth);
      subtract = dark_index * 17;
      {
        const GloomZone *zone = hit->wall->zone;
        float local_u = 0.0f;
        int tile_index = 0;
        uint8_t texture_index = 0u;
        size_t screen_index = 0u;
        size_t local_index = 0u;
        size_t tx = 0u;

        if (zone != NULL) {
          zone_wall_texture_coordinates(zone, hit->wall_u, &tile_index, &local_u, &tx);
          texture_index = remap_wall_texture_index(state, zone->textures[tile_index]);
          screen_index = texture_index / (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
          local_index = texture_index % (uint8_t)GLOOM_TEXTURES_PER_SCREEN;
          if (screen_index < (size_t)GLOOM_TEXTURE_SCREENS && wall_textures->screens[screen_index].loaded &&
              wall_textures->screens[screen_index].texels != NULL &&
              local_index < wall_textures->screens[screen_index].texture_count) {
            const WallTextureScreen *screen = &wall_textures->screens[screen_index];

            wall_texels = screen->texels;
            wall_palette = screen->shaded_palette[dark_index];
            wall_texel_base = local_index * ((size_t)GLOOM_TEXTURE_WIDTH * (size_t)GLOOM_TEXTURE_HEIGHT) + tx;
            transparent_wall_column =
                screen->column_flags != NULL &&
                screen->column_flags[(local_index * (size_t)GLOOM_TEXTURE_WIDTH) + tx] != 0u;
            fast_wall_column = true;
          }
        }
      }
#ifdef GLOOM_DOS_SDL3
      {
        int32_t wall_v_fixed = dos_float_to_fixed16(((float)y0 + 0.5f - wall_top) / wall_height);
        int32_t wall_v_step_fixed = dos_float_to_fixed16(1.0f / wall_height);

        for (draw_y = y0; draw_y <= y1; ++draw_y) {
          int32_t wall_v_current_fixed = wall_v_fixed;
          uint32_t argb = 0u;
          uint8_t alpha = 0u;

          wall_v_fixed += wall_v_step_fixed;

          if (filled_stamps[draw_y - y] == filled_stamp) {
            continue;
          }

          if (fast_wall_column) {
            int32_t wall_v_clamped = dos_fixed16_clamp_unit(wall_v_current_fixed);
            size_t ty = (size_t)((wall_v_clamped * (GLOOM_TEXTURE_HEIGHT - 1)) >> 16);
            uint8_t palette_index = wall_texels[wall_texel_base + (ty * (size_t)GLOOM_TEXTURE_WIDTH)];

            if (transparent_wall_column && palette_index == 0u) {
              continue;
            }
            argb = wall_palette[palette_index];
          } else {
            float wall_v = (float)wall_v_current_fixed / 65536.0f;

            argb = sample_zone_wall_texture_argb(wall_textures, state, hit->wall->zone, hit->wall_u, wall_v);
            argb = shade_argb_subtract(argb, subtract);
          }
          alpha = (uint8_t)(argb >> 24);

          if (alpha == 0u) {
            continue;
          }

          framebuffer->pixels[(size_t)draw_y * (size_t)framebuffer->pitch_pixels + (size_t)screen_x] =
              0xFF000000u | (argb & 0x00FFFFFFu);
          filled_stamps[draw_y - y] = filled_stamp;

          if (hit->depth < depth_buffer[i]) {
            depth_buffer[i] = hit->depth;
          }
        }
      }
#else
      for (draw_y = y0; draw_y <= y1; ++draw_y) {
        float wall_v = ((float)draw_y + 0.5f - wall_top) / wall_height;
        uint32_t argb = 0u;
        uint8_t alpha = 0u;

        if (filled_stamps[draw_y - y] == filled_stamp) {
          continue;
        }

        if (fast_wall_column) {
          size_t ty = 0u;
          uint8_t palette_index = 0u;

          if (wall_v < 0.0f) wall_v = 0.0f;
          if (wall_v > 1.0f) wall_v = 1.0f;
          ty = (size_t)(wall_v * (float)(GLOOM_TEXTURE_HEIGHT - 1));
          palette_index = wall_texels[wall_texel_base + (ty * (size_t)GLOOM_TEXTURE_WIDTH)];
          if (transparent_wall_column && palette_index == 0u) {
            continue;
          }
          argb = wall_palette[palette_index];
        } else {
          argb = sample_zone_wall_texture_argb(wall_textures, state, hit->wall->zone, hit->wall_u, wall_v);
          argb = shade_argb_subtract(argb, subtract);
        }
        alpha = (uint8_t)(argb >> 24);

        if (alpha == 0u) {
          continue;
        }

        framebuffer->pixels[(size_t)draw_y * (size_t)framebuffer->pitch_pixels + (size_t)screen_x] =
            0xFF000000u | (argb & 0x00FFFFFFu);
        filled_stamps[draw_y - y] = filled_stamp;

        if (hit->depth < depth_buffer[i]) {
          depth_buffer[i] = hit->depth;
        }
      }
#endif
    }
    if (g_profiler.enabled) {
      profile_add_elapsed(&g_profiler.render_wall_draw_ms, profile_wall_phase_start_ms);
    }
  }
  profile_add_elapsed(&g_profiler.render_wall_ms, profile_start_ms);
#endif

  profile_start_ms = profile_now_ms();
  if (state->map.object_spawn_count > 0u && object_visuals != NULL) {
    for (i = 0; i < state->map.object_spawn_count && sprite_write < GLOOM_MAX_DEBUG_SPRITES; ++i) {
      const GloomObjectSpawn *spawn = &state->map.object_spawns[i];
      GloomObjectSpawn render_spawn;
      const ObjectVisual *visual = NULL;
      const ObjectFrame *frame = NULL;
      const RuntimeObjectState *object = i < (size_t)GLOOM_MAX_RUNTIME_OBJECTS ? &state->objects[i] : NULL;
      float wx = object != NULL && object->active ? object->x : (float)spawn->x;
      float wz = object != NULL && object->active ? object->z : (float)spawn->z;
      float dx = wx - cam_x;
      float dz = wz - cam_z;
      float svx = dx * view_cos - dz * view_sin;
      float svz = dx * view_sin + dz * view_cos;
      float camera_y = state->camera_y;
      float object_y = 0.0f;
      float scale = 1.0f;
      float top_view_y = 0.0f;
      float left_view_x = 0.0f;

      if (object_type_is_player(spawn->object_type)) {
        continue;
      }

      if (object != NULL && !object->active && (object->enemy || object_type_is_pickup(spawn->object_type))) {
        continue;
      }

      if (spawn->object_type < 0 || spawn->object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
        continue;
      }

      visual = &object_visuals->visuals[spawn->object_type];
      if (!visual->loaded) {
        continue;
      }

      render_spawn = *spawn;
      if (object != NULL && object->active) {
        render_spawn.x = clamp_int16(round_float_to_int32(object->x));
        render_spawn.y = clamp_int16(round_float_to_int32(object->y + object->render_y_offset));
        render_spawn.z = clamp_int16(round_float_to_int32(object->z));
        render_spawn.rotation = object->rotation;
      }

      if (object != NULL && object->active && (object->enemy || object_type_is_pickup(spawn->object_type))) {
        size_t frame_number = (size_t)(object->frame_fixed >> 16u);

        frame = select_object_visual_frame_number(visual, &render_spawn, state, frame_number);
      } else {
        frame = select_object_visual_frame(visual, &render_spawn, state);
      }
      if (frame == NULL || frame->width <= 0 || frame->height <= 0) {
        continue;
      }

      if (svz >= near_plane && svz < far_depth && SDL_fabsf(svx) <= svz * frustum_ratio * 1.1f) {
        DebugSprite *sp = &debug_sprites[sprite_write];

        object_y = (float)render_spawn.y - camera_y;
        scale = (float)visual->scale / 256.0f;
        top_view_y = object_y - ((float)frame->handle_y * scale);
        left_view_x = svx - ((float)frame->handle_x * scale);

        sp->screen_x = (float)x + (float)w * 0.5f + (left_view_x / svz) * focal;
        sp->screen_y = (float)horizon_y + (top_view_y / svz) * focal;
        sp->screen_w = ((float)frame->width * scale * focal) / svz;
        sp->screen_h = ((float)frame->height * scale * focal) / svz;
        bind_debug_sprite_depth(sp, svz);
        sp->source_pad = 0;
        bind_debug_sprite_frame(sp, visual, frame);

        if (sp->screen_w >= 1.0f && sp->screen_h >= 1.0f) {
          sprite_write += 1u;
        }
      }
    }
  }

  if (state->two_player_mode && object_visuals != NULL) {
    int16_t other_player_object_type = state->active_player_index == 0u ? 1 : 0;

    append_player_debug_sprite(state, object_visuals, &state->player2, other_player_object_type, debug_sprites,
                               &sprite_write, GLOOM_MAX_DEBUG_SPRITES, x, w, horizon_y, cam_x, cam_z, view_cos,
                               view_sin, focal, near_plane, far_depth, frustum_ratio);
  }

  if (weapon_visuals != NULL) {
    for (i = 0u; i < GLOOM_MAX_RUNTIME_PROJECTILES && sprite_write < GLOOM_MAX_DEBUG_SPRITES; ++i) {
      const RuntimeProjectile *projectile = &state->projectiles[i];
      const ObjectVisual *visual = NULL;
      const ObjectFrame *frame = NULL;
      float dx = 0.0f;
      float dz = 0.0f;
      float svx = 0.0f;
      float svz = 0.0f;
      float object_y = 0.0f;
      float scale = 1.0f;
      float top_view_y = 0.0f;
      float left_view_x = 0.0f;
      size_t frame_index = 0u;
      int source_pad = 0;
      float source_pad_view = 0.0f;
      float padded_width = 0.0f;
      float padded_height = 0.0f;

      if (!projectile->active || projectile->weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
        continue;
      }

      visual = &weapon_visuals->bullets[projectile->weapon_index];
      if (!visual->loaded || visual->frame_count == 0u) {
        continue;
      }

      frame_index = (size_t)((uint16_t)projectile->frame_phase % (uint16_t)visual->frame_count);
      frame = &visual->frames[frame_index];
      if (frame->width <= 0 || frame->height <= 0) {
        continue;
      }

      dx = projectile->x - cam_x;
      dz = projectile->z - cam_z;
      svx = dx * view_cos - dz * view_sin;
      svz = dx * view_sin + dz * view_cos;
      if (svz < near_plane || svz >= far_depth || SDL_fabsf(svx) > svz * frustum_ratio * 1.1f) {
        continue;
      }
      if (!player_projectile_visible_from_owner_view(state, projectile, svz)) {
        continue;
      }

      object_y = projectile->y - state->camera_y;
      scale = (float)visual->scale / 256.0f;
      top_view_y = object_y - ((float)frame->handle_y * scale);
      left_view_x = svx - ((float)frame->handle_x * scale);
      source_pad = projectile_sprite_source_pad(projectile->weapon_index);
      source_pad_view = (float)source_pad * scale;
      padded_width = ((float)frame->width + (float)(source_pad * 2)) * scale;
      padded_height = ((float)frame->height + (float)(source_pad * 2)) * scale;

      debug_sprites[sprite_write].screen_x =
          (float)x + (float)w * 0.5f + ((left_view_x - source_pad_view) / svz) * focal;
      debug_sprites[sprite_write].screen_y = (float)horizon_y + ((top_view_y - source_pad_view) / svz) * focal;
      debug_sprites[sprite_write].screen_w = (padded_width * focal) / svz;
      debug_sprites[sprite_write].screen_h = (padded_height * focal) / svz;
      bind_debug_sprite_depth(&debug_sprites[sprite_write], svz);
      debug_sprites[sprite_write].source_pad = source_pad;
      bind_debug_sprite_frame(&debug_sprites[sprite_write], visual, frame);

      if (debug_sprites[sprite_write].screen_w >= 1.0f && debug_sprites[sprite_write].screen_h >= 1.0f) {
        sprite_write += 1u;
      }
    }

    for (i = 0u; i < GLOOM_MAX_RUNTIME_SPARKS && sprite_write < GLOOM_MAX_DEBUG_SPRITES; ++i) {
      const RuntimeSpark *spark = &state->sparks[i];
      const ObjectVisual *visual = NULL;
      const ObjectFrame *frame = NULL;
      float dx = 0.0f;
      float dz = 0.0f;
      float svx = 0.0f;
      float svz = 0.0f;
      float object_y = 0.0f;
      float scale = 1.0f;
      float top_view_y = 0.0f;
      float left_view_x = 0.0f;
      size_t frame_index = 0u;

      if (!spark->active || spark->weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
        continue;
      }

      visual = &weapon_visuals->sparks[spark->weapon_index];
      if (!visual->loaded || visual->frame_count == 0u) {
        continue;
      }

      frame_index = (size_t)(spark->frame_index % (uint16_t)visual->frame_count);
      frame = &visual->frames[frame_index];
      if (frame->width <= 0 || frame->height <= 0) {
        continue;
      }

      dx = spark->x - cam_x;
      dz = spark->z - cam_z;
      svx = dx * view_cos - dz * view_sin;
      svz = dx * view_sin + dz * view_cos;
      if (svz < near_plane || svz >= far_depth || SDL_fabsf(svx) > svz * frustum_ratio * 1.1f) {
        continue;
      }

      object_y = spark->y - state->camera_y;
      scale = (float)visual->scale / 256.0f;
      top_view_y = object_y - ((float)frame->handle_y * scale);
      left_view_x = svx - ((float)frame->handle_x * scale);

      debug_sprites[sprite_write].screen_x = (float)x + (float)w * 0.5f + (left_view_x / svz) * focal;
      debug_sprites[sprite_write].screen_y = (float)horizon_y + (top_view_y / svz) * focal;
      debug_sprites[sprite_write].screen_w = ((float)frame->width * scale * focal) / svz;
      debug_sprites[sprite_write].screen_h = ((float)frame->height * scale * focal) / svz;
      bind_debug_sprite_depth(&debug_sprites[sprite_write], svz);
      debug_sprites[sprite_write].source_pad = 0;
      bind_debug_sprite_frame(&debug_sprites[sprite_write], visual, frame);

      if (debug_sprites[sprite_write].screen_w >= 1.0f && debug_sprites[sprite_write].screen_h >= 1.0f) {
        sprite_write += 1u;
      }
    }
  }

  if (object_visuals != NULL) {
    for (i = 0u; i < GLOOM_MAX_RUNTIME_GORE && sprite_write < GLOOM_MAX_DEBUG_SPRITES; ++i) {
      const RuntimeGore *gore = &state->gore[i];
      const ObjectVisual *visual = NULL;
      const ObjectFrame *frame = NULL;
      float dx = 0.0f;
      float dz = 0.0f;
      float svx = 0.0f;
      float svz = 0.0f;
      float object_y = 0.0f;
      float scale = 1.0f;
      float top_view_y = 0.0f;
      float left_view_x = 0.0f;
      size_t frame_index = 0u;

      if (!gore->active || gore->object_type < 0 || gore->object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
        continue;
      }

      visual = &object_visuals->chunks[gore->object_type];
      if (!visual->loaded || visual->frame_count == 0u) {
        continue;
      }

      frame_index = (size_t)(gore->frame_number % (uint16_t)visual->frame_count);
      frame = &visual->frames[frame_index];
      if (frame->width <= 0 || frame->height <= 0) {
        continue;
      }

      dx = gore->x - cam_x;
      dz = gore->z - cam_z;
      svx = dx * view_cos - dz * view_sin;
      svz = dx * view_sin + dz * view_cos;
      if (svz < near_plane || svz >= far_depth || SDL_fabsf(svx) > svz * frustum_ratio * 1.1f) {
        continue;
      }

      object_y = -state->camera_y;
      scale = 0x0200u / 256.0f;
      top_view_y = object_y - ((float)frame->handle_y * scale);
      left_view_x = svx - ((float)frame->handle_x * scale);

      debug_sprites[sprite_write].screen_x = (float)x + (float)w * 0.5f + (left_view_x / svz) * focal;
      debug_sprites[sprite_write].screen_y = (float)horizon_y + (top_view_y / svz) * focal;
      debug_sprites[sprite_write].screen_w = ((float)frame->width * scale * focal) / svz;
      debug_sprites[sprite_write].screen_h = ((float)frame->height * scale * focal) / svz;
      bind_debug_sprite_depth(&debug_sprites[sprite_write], svz);
      debug_sprites[sprite_write].source_pad = 0;
      bind_debug_sprite_frame(&debug_sprites[sprite_write], visual, frame);

      if (debug_sprites[sprite_write].screen_w >= 1.0f && debug_sprites[sprite_write].screen_h >= 1.0f) {
        sprite_write += 1u;
      }
    }

    for (i = 0u; i < GLOOM_MAX_RUNTIME_CHUNKS && sprite_write < GLOOM_MAX_DEBUG_SPRITES; ++i) {
      const RuntimeChunk *chunk = &state->chunks[i];
      const ObjectVisual *visual = NULL;
      const ObjectFrame *frame = NULL;
      float dx = 0.0f;
      float dz = 0.0f;
      float svx = 0.0f;
      float svz = 0.0f;
      float object_y = 0.0f;
      float scale = 1.0f;
      float top_view_y = 0.0f;
      float left_view_x = 0.0f;
      size_t frame_index = 0u;

      if (!chunk->active || chunk->object_type < 0 || chunk->object_type >= (int16_t)GLOOM_OBJECT_TYPE_COUNT) {
        continue;
      }

      visual = &object_visuals->chunks[chunk->object_type];
      if (!visual->loaded || visual->frame_count == 0u) {
        continue;
      }

      frame_index = (size_t)(chunk->frame_number % (uint16_t)visual->frame_count);
      frame = &visual->frames[frame_index];
      if (frame->width <= 0 || frame->height <= 0) {
        continue;
      }

      dx = chunk->x - cam_x;
      dz = chunk->z - cam_z;
      svx = dx * view_cos - dz * view_sin;
      svz = dx * view_sin + dz * view_cos;
      if (svz < near_plane || svz >= far_depth || SDL_fabsf(svx) > svz * frustum_ratio * 1.1f) {
        continue;
      }

      object_y = chunk->y - state->camera_y;
      scale = (float)chunk->scale / 256.0f;
      top_view_y = object_y - ((float)frame->handle_y * scale);
      left_view_x = svx - ((float)frame->handle_x * scale);

      debug_sprites[sprite_write].screen_x = (float)x + (float)w * 0.5f + (left_view_x / svz) * focal;
      debug_sprites[sprite_write].screen_y = (float)horizon_y + (top_view_y / svz) * focal;
      debug_sprites[sprite_write].screen_w = ((float)frame->width * scale * focal) / svz;
      debug_sprites[sprite_write].screen_h = ((float)frame->height * scale * focal) / svz;
      bind_debug_sprite_depth(&debug_sprites[sprite_write], svz);
      debug_sprites[sprite_write].source_pad = 0;
      bind_debug_sprite_frame(&debug_sprites[sprite_write], visual, frame);

      if (debug_sprites[sprite_write].screen_w >= 1.0f && debug_sprites[sprite_write].screen_h >= 1.0f) {
        sprite_write += 1u;
      }
    }
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_BLOOD; ++i) {
    const RuntimeBlood *blood = &state->blood[i];
    float dx = 0.0f;
    float dz = 0.0f;
    float svx = 0.0f;
    float svz = 0.0f;
    float by = 0.0f;
    int screen_x = 0;
    int screen_y = 0;
    uint32_t argb = 0;

    if (!blood->active || blood->color_mask == 0u) {
      continue;
    }

    dx = blood->x - cam_x;
    dz = blood->z - cam_z;
    svx = dx * view_cos - dz * view_sin;
    svz = dx * view_sin + dz * view_cos;
    if (svz <= 0.0f || svz >= far_depth) {
      continue;
    }

    by = blood->y;
    if (by > 0.0f) {
      by = -by;
    }
    by -= state->camera_y;

    screen_x = round_float_to_int32((float)x + (float)w * 0.5f + (svx / svz) * focal);
    screen_y = round_float_to_int32((float)horizon_y + (by / svz) * focal);
    if (screen_x < x || screen_x >= x + w || screen_y < y || screen_y >= y + h) {
      continue;
    }

    argb = amiga_blood_argb(blood->color_mask, svz);
#ifdef GLOOM_DOS_SDL3
    framebuffer->index_pixels[(size_t)screen_y * (size_t)framebuffer->index_pitch_pixels + (size_t)screen_x] =
        dos_argb_to_opaque_index(argb);
#else
    framebuffer->pixels[(size_t)screen_y * (size_t)framebuffer->pitch_pixels + (size_t)screen_x] =
        0xFF000000u | (argb & 0x00FFFFFFu);
#endif
  }

  for (i = 0; i < (size_t)w; ++i) {
#ifdef GLOOM_DOS_SDL3
    sprite_depth_buffer_int[i] = depth_buffer_int[i];
#else
    sprite_depth_buffer[i] = depth_buffer[i];
#endif
  }

  if (sprite_write > 1u) {
#ifdef GLOOM_DOS_SDL3
    size_t a = 0;

    for (a = 1u; a < sprite_write; ++a) {
      DebugSprite sprite = debug_sprites[a];
      size_t insert_at = a;

      while (insert_at > 0u && debug_sprites[insert_at - 1u].depth_int < sprite.depth_int) {
        debug_sprites[insert_at] = debug_sprites[insert_at - 1u];
        insert_at -= 1u;
      }
      debug_sprites[insert_at] = sprite;
    }
#else
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
#endif
  }

  for (i = 0; i < sprite_write; ++i) {
    const DebugSprite *sp = &debug_sprites[i];
    const ObjectFrame *frame = sp->frame;
    int x0 = 0;
    int x1 = 0;
    int y0 = 0;
    int y1 = 0;
    int screen_x = 0;
#ifndef GLOOM_DOS_SDL3
    int subtract = 0;
#endif

#ifdef GLOOM_DOS_SDL3
    if (frame == NULL || sp->screen_w_fixed < 0x10000 || sp->screen_h_fixed < 0x10000) {
      continue;
    }

    x0 = dos_fixed16_floor_to_int(sp->screen_x_fixed);
    x1 = dos_fixed16_floor_to_int(sp->screen_x_fixed + sp->screen_w_fixed);
    y0 = dos_fixed16_floor_to_int(sp->screen_y_fixed);
    y1 = dos_fixed16_floor_to_int(sp->screen_y_fixed + sp->screen_h_fixed);
#else
    if (frame == NULL || sp->screen_w < 1.0f || sp->screen_h < 1.0f) {
      continue;
    }

    x0 = (int)sp->screen_x;
    x1 = (int)(sp->screen_x + sp->screen_w);
    y0 = (int)sp->screen_y;
    y1 = (int)(sp->screen_y + sp->screen_h);
#endif

    if (x1 < x || x0 > x + w - 1 || y1 < y || y0 > y + h - 1) {
      continue;
    }

    if (x0 < x) x0 = x;
    if (x1 > x + w - 1) x1 = x + w - 1;
    if (y0 < y) y0 = y;
    if (y1 > y + h - 1) y1 = y + h - 1;

#ifdef GLOOM_DOS_SDL3
    {
      int64_t tu_numerator_fixed = (((int64_t)x0 << 16) + 32768) - (int64_t)sp->screen_x_fixed;
      int64_t tv_numerator_fixed = (((int64_t)y0 << 16) + 32768) - (int64_t)sp->screen_y_fixed;
      int32_t tu_fixed = dos_div_fixed16(tu_numerator_fixed, sp->screen_w_fixed);
      int32_t tu_step_fixed = dos_div_fixed16(0x10000, sp->screen_w_fixed);
      int32_t tv_start_fixed = dos_div_fixed16(tv_numerator_fixed, sp->screen_h_fixed);
      int32_t tv_step_fixed = dos_div_fixed16(0x10000, sp->screen_h_fixed);
      int padded_width = frame->width + (sp->source_pad * 2);
      int padded_height = frame->height + (sp->source_pad * 2);
      int32_t source_x_fixed = sp->source_pad > 0 ? (tu_fixed * padded_width) - ((int32_t)sp->source_pad << 16)
                                                   : tu_fixed * (frame->width - 1);
      int32_t source_x_step_fixed =
          sp->source_pad > 0 ? tu_step_fixed * padded_width : tu_step_fixed * (frame->width - 1);
      int32_t source_y_start_fixed =
          sp->source_pad > 0 ? (tv_start_fixed * padded_height) - ((int32_t)sp->source_pad << 16)
                             : tv_start_fixed * (frame->height - 1);
      int32_t source_y_step_fixed =
          sp->source_pad > 0 ? tv_step_fixed * padded_height : tv_step_fixed * (frame->height - 1);
      int dark_index = (int)amiga_depth_dark_index_int(sp->depth_int);
      const uint8_t *indexed_pixels = frame->indexed_pixels;
      const uint8_t *sprite_palette = sp->shaded_indices != NULL ? sp->shaded_indices[dark_index] : NULL;

      if (indexed_pixels == NULL || sprite_palette == NULL) {
        continue;
      }

      for (screen_x = x0; screen_x <= x1; ++screen_x, source_x_fixed += source_x_step_fixed) {
        int rel_x = screen_x - x;
        int draw_y = 0;
        int sx = 0;
        bool column_drawn = false;
        int32_t source_y_fixed = source_y_start_fixed;
        uint8_t *dst = framebuffer->index_pixels + ((size_t)y0 * (size_t)framebuffer->index_pitch_pixels) +
                       (size_t)screen_x;

        if (sp->depth_int >= sprite_depth_buffer_int[rel_x]) {
          continue;
        }

        if (sp->source_pad > 0) {
          if (source_x_fixed < 0 || source_x_fixed >= ((int32_t)frame->width << 16)) {
            continue;
          }
          sx = source_x_fixed >> 16;
        } else {
          if (source_x_fixed < 0 || source_x_fixed > ((int32_t)(frame->width - 1) << 16)) {
            continue;
          }
          sx = source_x_fixed >> 16;
        }

        for (draw_y = y0; draw_y <= y1;
             ++draw_y, source_y_fixed += source_y_step_fixed, dst += framebuffer->index_pitch_pixels) {
          uint8_t palette_index = 0u;
          int sy = 0;

          if (sp->source_pad > 0) {
            if (source_y_fixed < 0 || source_y_fixed >= ((int32_t)frame->height << 16)) {
              continue;
            }
            sy = source_y_fixed >> 16;
          } else {
            if (source_y_fixed < 0 || source_y_fixed > ((int32_t)(frame->height - 1) << 16)) {
              continue;
            }
            sy = source_y_fixed >> 16;
          }

          palette_index = indexed_pixels[(size_t)sy * (size_t)frame->width + (size_t)sx];
          if (palette_index == 0u) {
            continue;
          }

          *dst = sprite_palette[palette_index];
          column_drawn = true;
        }

        if (column_drawn) {
          sprite_depth_buffer_int[rel_x] = sp->depth_int;
        }
      }
    }
#else
    subtract = depth_subtract_for_depth(sp->depth);
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
        int sx = 0;
        int sy = 0;

        if (tv < 0.0f || tv > 1.0f || tu < 0.0f || tu > 1.0f) {
          continue;
        }

        if (sp->source_pad > 0) {
          float padded_width = (float)(frame->width + (sp->source_pad * 2));
          float padded_height = (float)(frame->height + (sp->source_pad * 2));
          float source_x = (tu * padded_width) - (float)sp->source_pad;
          float source_y = (tv * padded_height) - (float)sp->source_pad;

          if (source_x < 0.0f || source_y < 0.0f || source_x >= (float)frame->width ||
              source_y >= (float)frame->height) {
            continue;
          }
          sx = (int)source_x;
          sy = (int)source_y;
        } else {
          sx = (int)(tu * (float)(frame->width - 1));
          sy = (int)(tv * (float)(frame->height - 1));
        }
        argb = frame->argb_pixels[(size_t)sy * (size_t)frame->width + (size_t)sx];

        alpha = (uint8_t)(argb >> 24);

        if (alpha == 0u) {
          continue;
        }

        argb = shade_argb_subtract(argb, subtract);
        framebuffer->pixels[(size_t)draw_y * (size_t)framebuffer->pitch_pixels + (size_t)screen_x] =
            0xFF000000u | (argb & 0x00FFFFFFu);
        column_drawn = true;
      }

      if (column_drawn) {
        sprite_depth_buffer[rel_x] = sp->depth;
      }
    }
#endif
  }

  if (g_profiler.enabled) {
    g_profiler.sprites += (uint64_t)sprite_write;
  }
  profile_add_elapsed(&g_profiler.render_sprites_ms, profile_start_ms);
#ifdef GLOOM_DOS_SDL3
  audio_dos_pump(&g_audio);
#endif
}

static int ui_double_scale_for_viewport(int w, int h) {
  int scale = 1;

  if (w <= 0 || h <= 0) {
    return 1;
  }

  while (scale < 16 && BASE_WIDTH * (scale * 2) <= w && GLOOM_HUD_STATUS_HEIGHT * (scale * 2) <= h) {
    scale *= 2;
  }

  return scale;
}

static void render_argb_pixels_scaled(RenderFramebuffer *framebuffer, const uint32_t *pixels, int src_width,
                                      int src_height, int x, int y, int scale) {
  int sy = 0;

  if (framebuffer == NULL || pixels == NULL || src_width <= 0 || src_height <= 0 || scale <= 0) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }
#endif

  if (x + (src_width * scale) <= 0 || y + (src_height * scale) <= 0 || x >= framebuffer->width ||
      y >= framebuffer->height) {
    return;
  }

  for (sy = 0; sy < src_height; ++sy) {
    int dst_y0 = y + (sy * scale);
    int dst_y1 = dst_y0 + scale;
    int sx = 0;

    if (dst_y1 <= 0 || dst_y0 >= framebuffer->height) {
      continue;
    }
    if (dst_y0 < 0) dst_y0 = 0;
    if (dst_y1 > framebuffer->height) dst_y1 = framebuffer->height;

    for (sx = 0; sx < src_width; ++sx) {
      uint32_t argb = pixels[(size_t)sy * (size_t)src_width + (size_t)sx];
      uint8_t alpha = 0u;
#ifdef GLOOM_DOS_SDL3
      uint8_t color_index = 0u;
#else
      uint32_t opaque_argb = 0u;
#endif
      int dst_x0 = x + (sx * scale);
      int dst_x1 = dst_x0 + scale;
      int draw_y = 0;

      alpha = (uint8_t)(argb >> 24);
      if (alpha == 0u) {
        continue;
      }
      if (dst_x1 <= 0 || dst_x0 >= framebuffer->width) {
        continue;
      }
      if (dst_x0 < 0) dst_x0 = 0;
      if (dst_x1 > framebuffer->width) dst_x1 = framebuffer->width;
#ifdef GLOOM_DOS_SDL3
      color_index = dos_argb_to_opaque_index(argb);
#else
      opaque_argb = 0xFF000000u | (argb & 0x00FFFFFFu);
#endif

      for (draw_y = dst_y0; draw_y < dst_y1; ++draw_y) {
#ifdef GLOOM_DOS_SDL3
        uint8_t *row = framebuffer->index_pixels + ((size_t)draw_y * (size_t)framebuffer->index_pitch_pixels);
        memset(row + dst_x0, color_index, (size_t)(dst_x1 - dst_x0));
#else
        uint32_t *row = framebuffer->pixels + ((size_t)draw_y * (size_t)framebuffer->pitch_pixels);
        int draw_x = 0;

        for (draw_x = dst_x0; draw_x < dst_x1; ++draw_x) {
          row[draw_x] = opaque_argb;
        }
#endif
      }
    }
  }
}

#ifndef GLOOM_DOS_SDL3
static void render_object_frame_scaled(RenderFramebuffer *framebuffer, const ObjectFrame *frame, int screen_x,
                                       int screen_y, int scale) {
  if (frame == NULL) {
    return;
  }

  render_argb_pixels_scaled(framebuffer, frame->argb_pixels, frame->width, frame->height, screen_x, screen_y, scale);
}
#endif

#ifdef GLOOM_DOS_SDL3
static void render_object_frame_scaled_dos_indexed(RenderFramebuffer *framebuffer, const ObjectVisual *visual,
                                                   const ObjectFrame *frame, int screen_x, int screen_y, int scale) {
  const uint8_t *palette = visual != NULL ? visual->shaded_indices[0] : NULL;
  int sy = 0;

  if (framebuffer == NULL || framebuffer->index_pixels == NULL ||
      framebuffer->index_pitch_pixels < framebuffer->width || visual == NULL || frame == NULL ||
      frame->indexed_pixels == NULL || palette == NULL || frame->width <= 0 || frame->height <= 0 || scale <= 0) {
    return;
  }

  if (screen_x + (frame->width * scale) <= 0 || screen_y + (frame->height * scale) <= 0 ||
      screen_x >= framebuffer->width || screen_y >= framebuffer->height) {
    return;
  }

  for (sy = 0; sy < frame->height; ++sy) {
    int dst_y0 = screen_y + (sy * scale);
    int dst_y1 = dst_y0 + scale;
    int sx = 0;

    if (dst_y1 <= 0 || dst_y0 >= framebuffer->height) {
      continue;
    }
    if (dst_y0 < 0) dst_y0 = 0;
    if (dst_y1 > framebuffer->height) dst_y1 = framebuffer->height;

    for (sx = 0; sx < frame->width; ++sx) {
      uint8_t palette_index = frame->indexed_pixels[(size_t)sy * (size_t)frame->width + (size_t)sx];
      uint8_t color_index = 0u;
      int dst_x0 = screen_x + (sx * scale);
      int dst_x1 = dst_x0 + scale;
      int draw_y = 0;

      if (palette_index == 0u || dst_x1 <= 0 || dst_x0 >= framebuffer->width) {
        continue;
      }
      if (dst_x0 < 0) dst_x0 = 0;
      if (dst_x1 > framebuffer->width) dst_x1 = framebuffer->width;

      color_index = palette[palette_index];
      for (draw_y = dst_y0; draw_y < dst_y1; ++draw_y) {
        uint8_t *row = framebuffer->index_pixels + ((size_t)draw_y * (size_t)framebuffer->index_pitch_pixels);
        memset(row + dst_x0, color_index, (size_t)(dst_x1 - dst_x0));
      }
    }
  }
}
#endif

static size_t player_weapon_muzzle_frame_index(const AppState *state) {
  static const uint8_t flash_by_weapon[GLOOM_WEAPON_COUNT] = {0u, 1u, 1u, 2u, 2u};
  uint8_t weapon_index = state != NULL ? state->player_weapon : 0u;

  if (weapon_index >= (uint8_t)GLOOM_WEAPON_COUNT) {
    weapon_index = 0u;
  }

  /*
   * amiga/gloom2.s: checkfire always selects bullet/spark art from wtable using ob_weapon.
   * ob_mega adds two/three-way spread and ob_bouncecnt changes projectile bounce count, but
   * neither changes the underlying shot colour.
   */
  return (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME + (size_t)flash_by_weapon[weapon_index];
}

static int player_weapon_muzzle_frame_y_offset(size_t muzzle_frame_index) {
  static const int offset_by_muzzle_frame[GLOOM_GUN_MUZZLE_FRAME_COUNT] = {0, 6, 10};
  size_t relative_frame = 0u;

  if (muzzle_frame_index < (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME) {
    return 0;
  }

  relative_frame = muzzle_frame_index - (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME;
  if (relative_frame >= (size_t)GLOOM_GUN_MUZZLE_FRAME_COUNT) {
    return 0;
  }

  /*
   * The red flash already sits correctly. The original gun asset's green and purple flashes
   * are taller (51/55 pixels versus red's 45), so draw them higher by the height delta to keep
   * the barrel contact point aligned with the red frame.
   */
  return offset_by_muzzle_frame[relative_frame];
}

static void render_player_weapon(RenderFramebuffer *framebuffer, const AppState *state,
                                 const WeaponVisualSet *weapon_visuals, int x, int y, int w, int h) {
  const ObjectVisual *gun = NULL;
  const ObjectFrame *base_frame = NULL;
  const ObjectFrame *muzzle_frame = NULL;
  size_t base_frame_index = GLOOM_GUN_IDLE_FRAME;
  int ui_scale = 1;
  float bounce_radians = 0.0f;
  float side_phase = 0.0f;
  float side_bob = 0.0f;
  float anchor_x = 0.0f;
  float anchor_y = 0.0f;
  int body_draw_x = 0;
  int body_draw_y = 0;

  if (framebuffer == NULL || state == NULL || weapon_visuals == NULL || w <= 0 || h <= 0) {
    return;
  }

  gun = &weapon_visuals->gun;
  if (!gun->loaded || gun->frames == NULL || gun->frame_count == 0u) {
    return;
  }

  if (state->player_weapon_flash > (float)(GLOOM_GUN_FLASH_AMIGA_TICKS / 2) &&
      gun->frame_count > (size_t)GLOOM_GUN_RECOIL_FRAME) {
    base_frame_index = GLOOM_GUN_RECOIL_FRAME;
  }
  if (base_frame_index >= gun->frame_count) {
    base_frame_index = 0u;
  }

  ui_scale = ui_double_scale_for_viewport(w, h);

  base_frame = &gun->frames[base_frame_index];
  bounce_radians = ((float)((uint16_t)state->player_bounce & 511u) * 6.28318530718f) / 512.0f;
  side_phase = SDL_sinf(bounce_radians);
  side_bob = side_phase * (float)(GLOOM_GUN_SIDE_BOB * ui_scale);
  anchor_x = (float)x + ((float)w * 0.5f);
  anchor_y = (float)y + (float)h + (float)(GLOOM_GUN_LOWER_OFFSET * ui_scale);
  anchor_y -= SDL_fabsf(side_phase) * (float)(GLOOM_GUN_SIDE_LIFT * ui_scale);
  if (base_frame_index == (size_t)GLOOM_GUN_RECOIL_FRAME) {
    anchor_y += (float)(GLOOM_GUN_RECOIL_BACK_OFFSET * ui_scale);
  }
  anchor_x += side_bob;

  if (state->player_weapon_flash > 0.0f &&
      gun->frame_count >= (size_t)(GLOOM_GUN_MUZZLE_FIRST_FRAME + GLOOM_GUN_MUZZLE_FRAME_COUNT)) {
    size_t muzzle_frame_index = player_weapon_muzzle_frame_index(state);
    float muzzle_anchor_y = (float)y + (float)h + (float)(GLOOM_GUN_MUZZLE_LOWER_OFFSET * ui_scale);

    if (muzzle_frame_index < gun->frame_count) {
      int muzzle_draw_x = 0;
      int muzzle_draw_y = 0;

      muzzle_frame = &gun->frames[muzzle_frame_index];
      muzzle_draw_x = round_float_to_int32(anchor_x - (float)(muzzle_frame->handle_x * ui_scale));
      muzzle_draw_y =
          round_float_to_int32(muzzle_anchor_y -
                               (float)((muzzle_frame->handle_y + player_weapon_muzzle_frame_y_offset(muzzle_frame_index)) *
                                       ui_scale));
#ifdef GLOOM_DOS_SDL3
      render_object_frame_scaled_dos_indexed(framebuffer, gun, muzzle_frame, muzzle_draw_x, muzzle_draw_y, ui_scale);
#else
      render_object_frame_scaled(framebuffer, muzzle_frame, muzzle_draw_x, muzzle_draw_y, ui_scale);
#endif
    }
  }

  body_draw_x = round_float_to_int32(anchor_x - (float)(base_frame->handle_x * ui_scale));
  body_draw_y = round_float_to_int32(anchor_y - (float)(base_frame->handle_y * ui_scale));
#ifdef GLOOM_DOS_SDL3
  render_object_frame_scaled_dos_indexed(framebuffer, gun, base_frame, body_draw_x, body_draw_y, ui_scale);
#else
  render_object_frame_scaled(framebuffer, base_frame, body_draw_x, body_draw_y, ui_scale);
#endif
}

#ifdef GLOOM_DOS_SDL3
static void render_hud_glyph_scaled_dos_indexed(RenderFramebuffer *framebuffer, const HudGlyph *glyph, int x, int y,
                                                int scale) {
  int sy = 0;

  if (framebuffer == NULL || glyph == NULL || glyph->indexed_pixels == NULL || glyph->coverage == NULL ||
      scale <= 0 || framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width ||
      glyph->width <= 0 || glyph->height <= 0) {
    return;
  }

  if (x + (glyph->width * scale) <= 0 || y + (glyph->height * scale) <= 0 || x >= framebuffer->width ||
      y >= framebuffer->height) {
    return;
  }

  for (sy = 0; sy < glyph->height; ++sy) {
    int dst_y0 = y + (sy * scale);
    int dst_y1 = dst_y0 + scale;
    int sx = 0;

    if (dst_y1 <= 0 || dst_y0 >= framebuffer->height) {
      continue;
    }
    if (dst_y0 < 0) dst_y0 = 0;
    if (dst_y1 > framebuffer->height) dst_y1 = framebuffer->height;

    for (sx = 0; sx < glyph->width; ++sx) {
      size_t src_index = (size_t)sy * (size_t)glyph->width + (size_t)sx;
      uint8_t color_index = glyph->indexed_pixels[src_index];
      int dst_x0 = x + (sx * scale);
      int dst_x1 = dst_x0 + scale;
      int draw_y = 0;

      if (!glyph->coverage[src_index] || dst_x1 <= 0 || dst_x0 >= framebuffer->width) {
        continue;
      }
      if (dst_x0 < 0) dst_x0 = 0;
      if (dst_x1 > framebuffer->width) dst_x1 = framebuffer->width;

      for (draw_y = dst_y0; draw_y < dst_y1; ++draw_y) {
        uint8_t *row = framebuffer->index_pixels + ((size_t)draw_y * (size_t)framebuffer->index_pitch_pixels);
        memset(row + dst_x0, color_index, (size_t)(dst_x1 - dst_x0));
      }
    }
  }
}
#endif

static void render_hud_glyph_scaled(RenderFramebuffer *framebuffer, const HudGlyph *glyph, int x, int y, int scale) {
  if (glyph == NULL) {
    return;
  }

#ifdef GLOOM_DOS_SDL3
  if (g_dos_menu_palette_active && glyph->indexed_pixels != NULL && glyph->coverage != NULL) {
    render_hud_glyph_scaled_dos_indexed(framebuffer, glyph, x, y, scale);
    return;
  }
#endif
  render_argb_pixels_scaled(framebuffer, glyph->argb_pixels, glyph->width, glyph->height, x, y, scale);
}

static void render_hud_glyph_scaled_brightness(RenderFramebuffer *framebuffer, const HudGlyph *glyph, int x, int y,
                                               int scale, uint8_t brightness) {
  int sy = 0;

  if (framebuffer == NULL || glyph == NULL || glyph->argb_pixels == NULL || scale <= 0) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }
#endif

  if (brightness >= 255u) {
    render_hud_glyph_scaled(framebuffer, glyph, x, y, scale);
    return;
  }

  for (sy = 0; sy < glyph->height; ++sy) {
    int sx = 0;

    for (sx = 0; sx < glyph->width; ++sx) {
      uint32_t src = glyph->argb_pixels[(size_t)sy * (size_t)glyph->width + (size_t)sx];
      uint32_t alpha = src >> 24;
      uint32_t r = ((src >> 16) & 0xffu) * (uint32_t)brightness / 255u;
      uint32_t g = ((src >> 8) & 0xffu) * (uint32_t)brightness / 255u;
      uint32_t b = (src & 0xffu) * (uint32_t)brightness / 255u;
      int dst_x0 = x + (sx * scale);
      int dst_x1 = dst_x0 + scale;
      int dy = 0;
#ifdef GLOOM_DOS_SDL3
      uint8_t color_index = 0u;
#else
      uint32_t color_argb = 0u;
#endif

      if (alpha == 0u) {
        continue;
      }
      if (dst_x1 <= 0 || dst_x0 >= framebuffer->width) {
        continue;
      }
      if (dst_x0 < 0) dst_x0 = 0;
      if (dst_x1 > framebuffer->width) dst_x1 = framebuffer->width;
#ifdef GLOOM_DOS_SDL3
      color_index = dos_argb_to_opaque_index(0xFF000000u | (r << 16u) | (g << 8u) | b);
#else
      color_argb = 0xff000000u | (r << 16) | (g << 8) | b;
#endif
      for (dy = 0; dy < scale; ++dy) {
        int py = y + (sy * scale) + dy;

        if (py < 0 || py >= framebuffer->height) {
          continue;
        }
#ifdef GLOOM_DOS_SDL3
        {
          uint8_t *row = framebuffer->index_pixels + ((size_t)py * (size_t)framebuffer->index_pitch_pixels);
          memset(row + dst_x0, color_index, (size_t)(dst_x1 - dst_x0));
        }
#else
        {
          uint32_t *row = framebuffer->pixels + ((size_t)py * (size_t)framebuffer->pitch_pixels);
          int px = 0;

          for (px = dst_x0; px < dst_x1; ++px) {
            row[px] = color_argb;
          }
        }
#endif
      }
    }
  }
}

static int menu_glyph_index_for_char(unsigned char ch) {
  if (ch >= (unsigned char)'0' && ch <= (unsigned char)'9') {
    return (int)(ch - (unsigned char)'0');
  }
  if (ch == (unsigned char)'\'') {
    return 57;
  }
  if (ch == (unsigned char)'!') {
    return 36;
  }
  if (ch == (unsigned char)'.') {
    return 37;
  }
  if (ch == (unsigned char)':') {
    return 38;
  }
  if (ch == 127u) {
    return 39;
  }
  return (int)(ch & 31u) + 9;
}

static void render_menu_text_brightness(RenderFramebuffer *framebuffer, const HudFont *font, const char *text,
                                        int center_x, int y, int scale, uint8_t brightness);

static void render_menu_text(RenderFramebuffer *framebuffer, const HudFont *font, const char *text, int center_x,
                             int y, int scale) {
  const uint8_t full_brightness = 255u;

  render_menu_text_brightness(framebuffer, font, text, center_x, y, scale, full_brightness);
}

static void render_menu_text_brightness(RenderFramebuffer *framebuffer, const HudFont *font, const char *text,
                                        int center_x, int y, int scale, uint8_t brightness) {
  size_t len = 0u;
  int x = 0;

  if (framebuffer == NULL || font == NULL || !font->loaded || font->glyphs == NULL || text == NULL || scale <= 0) {
    return;
  }

  len = strlen(text);
  x = center_x - (int)(((int)len * GLOOM_MENU_BIG_FONT_WIDTH * scale) / 2);
  while (*text != '\0') {
    unsigned char ch = (unsigned char)*text++;

    if (ch != (unsigned char)' ') {
      int glyph_index = menu_glyph_index_for_char(ch);

      if (glyph_index >= 0 && (size_t)glyph_index < font->glyph_count) {
        render_hud_glyph_scaled_brightness(framebuffer, &font->glyphs[glyph_index], x, y, scale, brightness);
      }
    }
    x += GLOOM_MENU_BIG_FONT_WIDTH * scale;
  }
}

static size_t copy_wrapped_menu_line(char *out, size_t out_size, const char *text, size_t text_len,
                                     size_t max_chars) {
  size_t len = 0u;
  size_t consumed = 0u;

  if (out == NULL || out_size == 0u || text == NULL || text_len == 0u) {
    return 0u;
  }
  if (max_chars == 0u) {
    max_chars = 1u;
  }

  len = text_len;
  if (len > max_chars) {
    size_t break_pos = max_chars;

    while (break_pos > 0u && text[break_pos] != ' ') {
      break_pos -= 1u;
    }
    if (break_pos == 0u) {
      break_pos = max_chars;
    }
    len = break_pos;
  }
  if (len >= out_size) {
    len = out_size - 1u;
  }
  memcpy(out, text, len);
  out[len] = '\0';

  consumed = len;
  while (consumed < text_len && text[consumed] == ' ') {
    consumed += 1u;
  }
  return consumed;
}

static void render_player_weapon_status(RenderFramebuffer *framebuffer, const AppState *state, const HudFont *hud_font,
                                        int x, int y, int w, int h) {
  int slot = 0;
  int count = 0;
  int base_x = x;
  int weapon_char = 0;
  int ui_scale = 1;
  int status_y = y + h - GLOOM_HUD_STATUS_HEIGHT;

  if (framebuffer == NULL || state == NULL || hud_font == NULL || !hud_font->loaded || hud_font->glyphs == NULL ||
      state->player_weapon >= GLOOM_WEAPON_COUNT || (size_t)GLOOM_HUD_LIFE_CHAR >= hud_font->glyph_count ||
      (size_t)GLOOM_HUD_HEALTH_CHAR >= hud_font->glyph_count) {
    return;
  }

  ui_scale = ui_double_scale_for_viewport(w, h);
  status_y = y + h - (GLOOM_HUD_STATUS_HEIGHT * ui_scale);
  if (w > BASE_WIDTH * ui_scale) {
    base_x += (w - (BASE_WIDTH * ui_scale)) / 2;
  }

  count = state->player_hitpoints;
  if (count > GLOOM_PLAYER_INITIAL_HEALTH) {
    count = GLOOM_PLAYER_INITIAL_HEALTH;
  }
  for (slot = 0; slot < count; ++slot) {
    render_hud_glyph_scaled(framebuffer, &hud_font->glyphs[GLOOM_HUD_HEALTH_CHAR],
                            base_x + ((GLOOM_HUD_HEALTH_X + (slot * GLOOM_HUD_HEALTH_SPACING)) * ui_scale),
                            status_y + (GLOOM_HUD_HEALTH_Y * ui_scale), ui_scale);
  }

  count = state->player_lives;
  if (count < 0) {
    count = 0;
  }
  for (slot = 0; slot < count; ++slot) {
    int draw_x = base_x + (GLOOM_HUD_LIVES_X * ui_scale) +
                 (((GLOOM_HUD_LIVES_RIGHT_ALIGN_COUNT - count) * GLOOM_HUD_LIVES_SPACING) * ui_scale) +
                 ((slot * GLOOM_HUD_LIVES_SPACING) * ui_scale);

    render_hud_glyph_scaled(framebuffer, &hud_font->glyphs[GLOOM_HUD_LIFE_CHAR], draw_x,
                            status_y + (GLOOM_HUD_LIVES_Y * ui_scale), ui_scale);
  }

  weapon_char = GLOOM_HUD_WEAPON_FIRST_CHAR + (int)state->player_weapon;
  if (weapon_char < 0 || (size_t)weapon_char >= hud_font->glyph_count) {
    return;
  }

  count = 6 - (int)state->player_reload;
  if (count < 0) {
    count = 0;
  }
  if (count > GLOOM_WEAPON_COUNT) {
    count = GLOOM_WEAPON_COUNT;
  }

  for (slot = 0; slot < count; ++slot) {
    int draw_x = base_x + ((GLOOM_HUD_WEAPON_X + (slot * GLOOM_HUD_WEAPON_SPACING)) * ui_scale);
    int draw_y = status_y + (GLOOM_HUD_WEAPON_Y * ui_scale);

    render_hud_glyph_scaled(framebuffer, &hud_font->glyphs[weapon_char], draw_x, draw_y, ui_scale);
  }
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

static void render_scene_contents(RenderFramebuffer *framebuffer, const AppState *state, const GridOffsetSet *grid_offsets,
                                  const WallTextureSet *wall_textures, const FlatTextureSet *flat_textures,
                                  const ObjectVisualSet *object_visuals, const WeaponVisualSet *weapon_visuals,
                                  const HudFont *hud_font, int x, int y, int w, int h) {
  double profile_start_ms = 0.0;

  render_wall_debug(framebuffer, state, grid_offsets, wall_textures, flat_textures, object_visuals, weapon_visuals, x, y,
                    w, h);
  profile_start_ms = profile_now_ms();
  render_player_weapon(framebuffer, state, weapon_visuals, x, y, w, h);
  profile_add_elapsed(&g_profiler.render_weapon_ms, profile_start_ms);
  profile_start_ms = profile_now_ms();
  render_player_weapon_status(framebuffer, state, hud_font, x, y, w, h);
  profile_add_elapsed(&g_profiler.render_hud_ms, profile_start_ms);
#ifdef GLOOM_DOS_SDL3
  audio_dos_pump(&g_audio);
#endif
}

static void free_menu_image(MenuImage *image) {
  if (image == NULL) {
    return;
  }

  free(image->pixels);
#ifdef GLOOM_DOS_SDL3
  free(image->indices);
#endif
  memset(image, 0, sizeof(*image));
}

static void free_menu_assets(MenuAssets *assets) {
  if (assets == NULL) {
    return;
  }

  free_menu_image(&assets->title);
  free_menu_image(&assets->black_magic_logo);
  free_hud_font(&assets->big_font);
}

static bool load_menu_title_image(MenuImage *out_image) {
  const char *candidates[4] = {"amiga/title.iff", "amiga/pics2/title.iff", "amiga/pics/title.iff", "title.iff"};
  size_t i = 0u;

  if (out_image == NULL) {
    return false;
  }

  free_menu_image(out_image);
  for (i = 0u; i < sizeof(candidates) / sizeof(candidates[0]); ++i) {
    char resolved_path[1024] = {0};
    GloomIffImage image;
    char error[256] = {0};
    uint32_t *pixels = NULL;
#ifdef GLOOM_DOS_SDL3
    uint8_t *indices = NULL;
    size_t source_palette_count = 0u;
#endif
    size_t pixel_index = 0u;

    memset(&image, 0, sizeof(image));
    dos_logf("DOS checkpoint: menu title try %s", candidates[i]);
    if (!resolve_runtime_file_path(candidates[i], resolved_path, sizeof(resolved_path))) {
      continue;
    }
    dos_logf("DOS checkpoint: menu title resolved %s", resolved_path);
    if (!gloom_iff_load(resolved_path, &image, error, sizeof(error))) {
      dos_logf("DOS checkpoint: menu title decode failed %s", resolved_path);
      gloom_iff_free(&image);
      continue;
    }
    if (!image.has_decoded_pixels || image.width <= 0u || image.height <= 0u) {
      gloom_iff_free(&image);
      continue;
    }

    pixels = (uint32_t *)malloc((size_t)image.width * (size_t)image.height * sizeof(*pixels));
    if (pixels == NULL) {
      gloom_iff_free(&image);
      return false;
    }
#ifdef GLOOM_DOS_SDL3
    indices = (uint8_t *)malloc(image.pixel_count * sizeof(*indices));
    if (indices == NULL) {
      free(pixels);
      gloom_iff_free(&image);
      return false;
    }
#endif

    for (pixel_index = 0u; pixel_index < image.pixel_count; ++pixel_index) {
      pixels[pixel_index] = palette_to_argb(&image, image.pixels[pixel_index]);
#ifdef GLOOM_DOS_SDL3
      indices[pixel_index] = image.pixels[pixel_index];
#endif
    }

    out_image->loaded = true;
    out_image->pixels = pixels;
#ifdef GLOOM_DOS_SDL3
    source_palette_count = image.palette_count > 0u ? image.palette_count : ((size_t)1u << image.planes);
    if (source_palette_count > 256u) {
      source_palette_count = 256u;
    }
    out_image->indices = indices;
    memset(out_image->palette_used, 0, sizeof(out_image->palette_used));
    for (pixel_index = 0u; pixel_index < 256u; ++pixel_index) {
      out_image->palette[pixel_index] = palette_to_argb(&image, (uint8_t)pixel_index);
      out_image->palette_used[pixel_index] = pixel_index < source_palette_count ? 1u : 0u;
    }
#endif
    out_image->width = (int)image.width;
    out_image->height = (int)image.height;
    gloom_iff_free(&image);
    dos_logf("DOS checkpoint: menu title loaded %dx%d", out_image->width, out_image->height);
    printf("Loaded original title image %s for menu\n", resolved_path);
    return true;
  }

  fprintf(stderr, "Missing original title image for menu\n");
  return false;
}

static bool load_iff_menu_image_from_candidates(const char *const *candidates, size_t candidate_count,
                                                MenuImage *out_image) {
  size_t i = 0u;

  if (candidates == NULL || out_image == NULL) {
    return false;
  }

  free_menu_image(out_image);
  for (i = 0u; i < candidate_count; ++i) {
    char resolved_path[1024] = {0};
    GloomIffImage image;
    char error[256] = {0};
    uint32_t *pixels = NULL;
#ifdef GLOOM_DOS_SDL3
    uint8_t *indices = NULL;
    size_t source_palette_count = 0u;
#endif
    size_t pixel_index = 0u;

    memset(&image, 0, sizeof(image));
    if (candidates[i] == NULL || !resolve_runtime_file_path(candidates[i], resolved_path, sizeof(resolved_path))) {
      continue;
    }
    if (!gloom_iff_load(resolved_path, &image, error, sizeof(error))) {
      gloom_iff_free(&image);
      continue;
    }
    if (!image.has_decoded_pixels || image.width <= 0u || image.height <= 0u) {
      gloom_iff_free(&image);
      continue;
    }

    pixels = (uint32_t *)malloc((size_t)image.width * (size_t)image.height * sizeof(*pixels));
    if (pixels == NULL) {
      gloom_iff_free(&image);
      return false;
    }
#ifdef GLOOM_DOS_SDL3
    indices = (uint8_t *)malloc(image.pixel_count * sizeof(*indices));
    if (indices == NULL) {
      free(pixels);
      gloom_iff_free(&image);
      return false;
    }
#endif

    for (pixel_index = 0u; pixel_index < image.pixel_count; ++pixel_index) {
      pixels[pixel_index] = palette_to_argb(&image, image.pixels[pixel_index]);
#ifdef GLOOM_DOS_SDL3
      indices[pixel_index] = image.pixels[pixel_index];
#endif
    }

    out_image->loaded = true;
    out_image->pixels = pixels;
#ifdef GLOOM_DOS_SDL3
    source_palette_count = image.palette_count > 0u ? image.palette_count : ((size_t)1u << image.planes);
    if (source_palette_count > 256u) {
      source_palette_count = 256u;
    }
    out_image->indices = indices;
    memset(out_image->palette_used, 0, sizeof(out_image->palette_used));
    for (pixel_index = 0u; pixel_index < 256u; ++pixel_index) {
      out_image->palette[pixel_index] = palette_to_argb(&image, (uint8_t)pixel_index);
      out_image->palette_used[pixel_index] = pixel_index < source_palette_count ? 1u : 0u;
    }
#endif
    out_image->width = (int)image.width;
    out_image->height = (int)image.height;
    gloom_iff_free(&image);
    printf("Loaded original script picture %s\n", resolved_path);
    return true;
  }

  return false;
}

static bool load_script_picture_image(const char *picture, MenuImage *out_image) {
  char path0[256] = {0};
  char path1[256] = {0};
  char path2[256] = {0};
  char path3[256] = {0};
  char path4[256] = {0};
  char path5[256] = {0};
  char path6[256] = {0};
  char path7[256] = {0};
  char path8[256] = {0};
  const char *candidates[9] = {path0, path1, path2, path3, path4, path5, path6, path7, path8};
  const char *dos_alias = NULL;

  if (picture == NULL || picture[0] == '\0' || out_image == NULL) {
    return false;
  }
  if (strcmp(picture, "spacehulk") == 0) {
    dos_alias = "SPACEHUL.IFF";
  } else if (strcmp(picture, "gothic") == 0) {
    dos_alias = "GOTHIC.IFF";
  } else if (strcmp(picture, "hell") == 0) {
    dos_alias = "HELL.IFF";
  } else if (strcmp(picture, "theend") == 0) {
    dos_alias = "THEEND.IFF";
  }

  (void)snprintf(path0, sizeof(path0), "amiga/pics/%s", picture);
  (void)snprintf(path1, sizeof(path1), "amiga/data/pics/%s", picture);
  (void)snprintf(path2, sizeof(path2), "amiga/%s.iff", picture);
  (void)snprintf(path3, sizeof(path3), "amiga/%s", picture);
  (void)snprintf(path4, sizeof(path4), "amiga/pics/%s.iff", picture);
  (void)snprintf(path5, sizeof(path5), "amiga/data/pics/%s.iff", picture);
  (void)snprintf(path6, sizeof(path6), "%s.iff", picture);
  (void)snprintf(path7, sizeof(path7), "%s", picture);
  if (dos_alias != NULL) {
    (void)snprintf(path8, sizeof(path8), "amiga/%s", dos_alias);
  }
  return load_iff_menu_image_from_candidates(candidates, sizeof(candidates) / sizeof(candidates[0]), out_image);
}

static bool load_black_magic_logo_image(MenuImage *out_image) {
#ifdef GLOOM_DOS_SDL3
  const char *candidates[2] = {"amiga/BLACKMAG.IFF", "BLACKMAG.IFF"};
#else
  const char *candidates[6] = {"amiga/blackmagic.iff", "amiga/pics/blackmagic.iff", "amiga/pics2/blackmagic.iff",
                               "amiga/data/pics/blackmagic.iff", "blackmagic.iff", "BLACKMAG.IFF"};
#endif

  return load_iff_menu_image_from_candidates(candidates, sizeof(candidates) / sizeof(candidates[0]), out_image);
}

static bool load_menu_assets(MenuAssets *assets) {
  if (assets == NULL) {
    return false;
  }

  memset(assets, 0, sizeof(*assets));
  dos_logf("DOS checkpoint: load_menu_assets begin");
  if (!load_menu_title_image(&assets->title)) {
    return false;
  }
  dos_logf("DOS checkpoint: load_menu_assets title ok");
  if (!load_black_magic_logo_image(&assets->black_magic_logo)) {
    free_menu_image(&assets->title);
    fprintf(stderr, "Missing original Black Magic logo image for menu\n");
    return false;
  }
  dos_logf("DOS checkpoint: load_menu_assets black magic logo ok");
  if (!load_menu_big_font(&assets->big_font)) {
    free_menu_image(&assets->black_magic_logo);
    free_menu_image(&assets->title);
    return false;
  }
  dos_logf("DOS checkpoint: load_menu_assets big font ok");
  return true;
}

static int menu_pixel_scale_for_viewport(int render_width, int render_height) {
  int scale_x = render_width / BASE_WIDTH;
  int scale_y = render_height / BASE_HEIGHT;
  int scale = scale_x < scale_y ? scale_x : scale_y;
  int power = 1;

  if (scale < 1) {
    return 1;
  }
  while (power * 2 <= scale && power < 8) {
    power *= 2;
  }
  return power;
}

static void render_menu_image(RenderFramebuffer *framebuffer, const MenuImage *image, int render_width,
                              int render_height) {
  int dst_y = 0;

  if (framebuffer == NULL || image == NULL || !image->loaded) {
    return;
  }

  if (render_width <= 0 || render_height <= 0 || image->width <= 0 || image->height <= 0) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (image->indices == NULL && image->pixels == NULL) {
    return;
  }
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
#else
  if (image->pixels == NULL) {
    return;
  }
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }
#endif

  for (dst_y = 0; dst_y < render_height; ++dst_y) {
#ifdef GLOOM_DOS_SDL3
    uint8_t *dst_row = framebuffer->index_pixels + ((size_t)dst_y * (size_t)framebuffer->index_pitch_pixels);
#else
    uint32_t *dst_row = framebuffer->pixels + ((size_t)dst_y * (size_t)framebuffer->pitch_pixels);
#endif
    int dst_x = 0;

    for (dst_x = 0; dst_x < render_width; ++dst_x) {
      int64_t src_x = 0;
      int64_t src_y = 0;

      if ((int64_t)render_width * (int64_t)image->height > (int64_t)render_height * (int64_t)image->width) {
        int64_t scaled_height = ((int64_t)render_width * (int64_t)image->height + (int64_t)image->width - 1) /
                                (int64_t)image->width;
        int64_t crop_y = (scaled_height - (int64_t)render_height) / 2;

        src_x = ((int64_t)dst_x * (int64_t)image->width) / (int64_t)render_width;
        src_y = (((int64_t)dst_y + crop_y) * (int64_t)image->height) / scaled_height;
      } else {
        int64_t scaled_width = ((int64_t)render_height * (int64_t)image->width + (int64_t)image->height - 1) /
                               (int64_t)image->height;
        int64_t crop_x = (scaled_width - (int64_t)render_width) / 2;

        src_x = (((int64_t)dst_x + crop_x) * (int64_t)image->width) / scaled_width;
        src_y = ((int64_t)dst_y * (int64_t)image->height) / (int64_t)render_height;
      }

      if (src_x < 0) src_x = 0;
      if (src_y < 0) src_y = 0;
      if (src_x >= image->width) src_x = image->width - 1;
      if (src_y >= image->height) src_y = image->height - 1;
#ifdef GLOOM_DOS_SDL3
      if (g_dos_menu_palette_active && image->indices != NULL) {
        dst_row[dst_x] = image->indices[(size_t)src_y * (size_t)image->width + (size_t)src_x];
      } else {
        dst_row[dst_x] = dos_argb_to_index(image->pixels[(size_t)src_y * (size_t)image->width + (size_t)src_x]);
      }
#else
      dst_row[dst_x] = image->pixels[(size_t)src_y * (size_t)image->width + (size_t)src_x];
#endif
    }
  }
}

static void menu_image_content_bounds(const MenuImage *image, int *out_x, int *out_y, int *out_w, int *out_h) {
  int min_x = 0;
  int min_y = 0;
  int max_x = 0;
  int max_y = 0;
  int y = 0;
  bool found = false;

  if (out_x == NULL || out_y == NULL || out_w == NULL || out_h == NULL) {
    return;
  }
  *out_x = 0;
  *out_y = 0;
  *out_w = image != NULL ? image->width : 0;
  *out_h = image != NULL ? image->height : 0;
  if (image == NULL || image->pixels == NULL || image->width <= 0 || image->height <= 0) {
    return;
  }

  for (y = 0; y < image->height; ++y) {
    int x = 0;

    for (x = 0; x < image->width; ++x) {
      uint32_t argb = image->pixels[(size_t)y * (size_t)image->width + (size_t)x];

      if ((argb >> 24u) == 0u || (argb & 0x00FFFFFFu) == 0u) {
        continue;
      }
      if (!found) {
        min_x = max_x = x;
        min_y = max_y = y;
        found = true;
      } else {
        if (x < min_x) min_x = x;
        if (y < min_y) min_y = y;
        if (x > max_x) max_x = x;
        if (y > max_y) max_y = y;
      }
    }
  }

  if (found) {
    *out_x = min_x;
    *out_y = min_y;
    *out_w = (max_x - min_x) + 1;
    *out_h = (max_y - min_y) + 1;
  }
}

static uint32_t *prescale_menu_image_region_box(const MenuImage *image, int src_x, int src_y, int src_w, int src_h,
                                                int dst_w, int dst_h) {
  uint32_t *pixels = NULL;
  int y = 0;

  if (image == NULL || image->pixels == NULL || image->width <= 0 || image->height <= 0 || src_w <= 0 || src_h <= 0 ||
      dst_w <= 0 || dst_h <= 0) {
    return NULL;
  }

  pixels = (uint32_t *)malloc((size_t)dst_w * (size_t)dst_h * sizeof(*pixels));
  if (pixels == NULL) {
    return NULL;
  }

  for (y = 0; y < dst_h; ++y) {
    int x = 0;
    int sample_y0 = src_y + (int)(((int64_t)y * (int64_t)src_h) / (int64_t)dst_h);
    int sample_y1 = src_y + (int)(((int64_t)(y + 1) * (int64_t)src_h) / (int64_t)dst_h);

    if (sample_y1 <= sample_y0) {
      sample_y1 = sample_y0 + 1;
    }
    if (sample_y0 < src_y) sample_y0 = src_y;
    if (sample_y1 > src_y + src_h) sample_y1 = src_y + src_h;
    if (sample_y1 > image->height) sample_y1 = image->height;

    for (x = 0; x < dst_w; ++x) {
      int sample_x0 = src_x + (int)(((int64_t)x * (int64_t)src_w) / (int64_t)dst_w);
      int sample_x1 = src_x + (int)(((int64_t)(x + 1) * (int64_t)src_w) / (int64_t)dst_w);
      uint32_t r = 0u;
      uint32_t g = 0u;
      uint32_t b = 0u;
      uint32_t count = 0u;
      int sy = 0;

      if (sample_x1 <= sample_x0) {
        sample_x1 = sample_x0 + 1;
      }
      if (sample_x0 < src_x) sample_x0 = src_x;
      if (sample_x1 > src_x + src_w) sample_x1 = src_x + src_w;
      if (sample_x1 > image->width) sample_x1 = image->width;

      for (sy = sample_y0; sy < sample_y1; ++sy) {
        int sx = 0;

        for (sx = sample_x0; sx < sample_x1; ++sx) {
          uint32_t argb = image->pixels[(size_t)sy * (size_t)image->width + (size_t)sx];

          if ((argb >> 24u) == 0u || (argb & 0x00FFFFFFu) == 0u) {
            continue;
          }
          r += (argb >> 16u) & 0xFFu;
          g += (argb >> 8u) & 0xFFu;
          b += argb & 0xFFu;
          count += 1u;
        }
      }

      if (count == 0u) {
        pixels[(size_t)y * (size_t)dst_w + (size_t)x] = 0u;
      } else {
        pixels[(size_t)y * (size_t)dst_w + (size_t)x] =
            0xFF000000u | ((r / count) << 16u) | ((g / count) << 8u) | (b / count);
      }
    }
  }

  return pixels;
}

static void render_menu_argb_pixels(RenderFramebuffer *framebuffer, const uint32_t *pixels, int image_w, int image_h,
                                    int dst_x, int dst_y) {
  int y = 0;

  if (framebuffer == NULL || pixels == NULL || image_w <= 0 || image_h <= 0) {
    return;
  }
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }
#endif

  for (y = 0; y < image_h; ++y) {
    int draw_y = dst_y + y;
    int x = 0;

    if (draw_y < 0 || draw_y >= framebuffer->height) {
      continue;
    }
    for (x = 0; x < image_w; ++x) {
      int draw_x = dst_x + x;
      uint32_t argb = pixels[(size_t)y * (size_t)image_w + (size_t)x];

      if (draw_x < 0 || draw_x >= framebuffer->width || (argb >> 24u) == 0u || (argb & 0x00FFFFFFu) == 0u) {
        continue;
      }
#ifdef GLOOM_DOS_SDL3
      framebuffer->index_pixels[(size_t)draw_y * (size_t)framebuffer->index_pitch_pixels + (size_t)draw_x] =
          dos_argb_to_index(argb);
#else
      framebuffer->pixels[(size_t)draw_y * (size_t)framebuffer->pitch_pixels + (size_t)draw_x] =
          0xFF000000u | (argb & 0x00FFFFFFu);
#endif
    }
  }
}

static int start_menu_item_count(void);

static int g_start_menu_logo_src_w = 0;
static int g_start_menu_logo_src_h = 0;

typedef struct {
  const char *title_text;
  int scale;
  int title_scale;
  int title_y;
  int menu_y;
  int logo_x;
  int logo_y;
  int logo_w;
  int logo_h;
  int credit_y;
} StartMenuLayout;

static const char *start_menu_title_text(void) {
#ifdef __EMSCRIPTEN__
  return "GLOOM WITH FRIENDS";
#else
  return "GLOOM";
#endif
}

static int start_menu_title_scale_for_viewport(int render_width, const char *title_text) {
  int title_scale = 4;
  int title_width = 0;

  if (render_width <= 0 || title_text == NULL) {
    return 1;
  }

  title_width = (int)(strlen(title_text) * (size_t)GLOOM_MENU_BIG_FONT_WIDTH * (size_t)title_scale);
  while (title_scale > 1 && title_width > render_width) {
    title_scale /= 2;
    title_width = (int)(strlen(title_text) * (size_t)GLOOM_MENU_BIG_FONT_WIDTH * (size_t)title_scale);
  }
  return title_scale;
}

static void compute_start_menu_layout(int render_width, int render_height, int item_count, int logo_src_w,
                                      int logo_src_h, StartMenuLayout *out_layout) {
  StartMenuLayout layout;
  int row_height = 0;
  int title_h = 0;
  int menu_h = 0;
  int credit_h = 0;
  int top_margin = 0;
  int bottom_margin = 0;
  int title_menu_gap = 0;
  int menu_logo_gap = 0;
  int logo_credit_gap = 0;
  int logo_max_h = 0;
  int logo_max_w = 0;
  int logo_divisor = 1;
  int logo_area_top = 0;
  int top_group_h = 0;
  int upper_area_h = 0;

  if (out_layout == NULL) {
    return;
  }
  memset(&layout, 0, sizeof(layout));
  layout.title_text = start_menu_title_text();

  if (render_width <= 0 || render_height <= 0 || item_count <= 0) {
    *out_layout = layout;
    return;
  }

  layout.scale = menu_pixel_scale_for_viewport(render_width, render_height);
  layout.title_scale = start_menu_title_scale_for_viewport(render_width, layout.title_text);
  row_height = GLOOM_MENU_BIG_FONT_HEIGHT * layout.scale;
  title_h = GLOOM_MENU_BIG_FONT_HEIGHT * layout.title_scale;
  menu_h = item_count * row_height;
  credit_h = GLOOM_MENU_BIG_FONT_HEIGHT * layout.scale * 3;
  top_margin = 4 * layout.scale;
  bottom_margin = 2 * layout.scale;
  title_menu_gap = 4 * layout.scale;
  menu_logo_gap = 6 * layout.scale;
  logo_credit_gap = 3 * layout.scale;

  logo_max_w = render_width;
  layout.credit_y = render_height - credit_h - bottom_margin;
  logo_area_top = title_h + title_menu_gap + menu_h + menu_logo_gap + top_margin;
  logo_max_h = layout.credit_y - logo_credit_gap - logo_area_top;
  if (logo_max_h > 144 * layout.scale) {
    logo_max_h = 144 * layout.scale;
  }
  if (logo_src_w > 0 && logo_src_h > 0 && logo_max_w > 0 && logo_max_h > 0) {
    while ((logo_src_w / logo_divisor) > logo_max_w || (logo_src_h / logo_divisor) > logo_max_h) {
      if (logo_divisor >= 64) {
        break;
      }
      logo_divisor *= 2;
    }
    layout.logo_w = logo_src_w / logo_divisor;
    layout.logo_h = logo_src_h / logo_divisor;
    if (layout.logo_w > logo_max_w || layout.logo_h > logo_max_h) {
      layout.logo_w = 0;
      layout.logo_h = 0;
    }
    if (layout.logo_w <= 0 || layout.logo_h <= 0) {
      layout.logo_w = 0;
      layout.logo_h = 0;
    }
  }
  layout.logo_x = (render_width - layout.logo_w) / 2;
  layout.logo_y = layout.credit_y - logo_credit_gap - layout.logo_h;

  top_group_h = title_h + title_menu_gap + menu_h;
  upper_area_h = layout.logo_y - menu_logo_gap - top_margin;
  layout.title_y = top_margin + ((upper_area_h - top_group_h) / 2);
  if (layout.title_y < top_margin) {
    layout.title_y = top_margin;
  }
  layout.menu_y = layout.title_y + title_h + title_menu_gap;

  *out_layout = layout;
}

static int start_menu_y_for_viewport(int render_width, int render_height, int item_count) {
  StartMenuLayout layout;

  compute_start_menu_layout(render_width, render_height, item_count, g_start_menu_logo_src_w,
                            g_start_menu_logo_src_h, &layout);
  return layout.menu_y;
}

static void render_start_menu_static_marks(RenderFramebuffer *framebuffer, const MenuAssets *assets, int render_width,
                                           int render_height) {
  int item_count = start_menu_item_count();
  int logo_src_x = 0;
  int logo_src_y = 0;
  int logo_src_w = 0;
  int logo_src_h = 0;
  StartMenuLayout layout;
  uint32_t *scaled_logo = NULL;

  if (framebuffer == NULL || assets == NULL || render_width <= 0 || render_height <= 0) {
    return;
  }

  menu_image_content_bounds(&assets->black_magic_logo, &logo_src_x, &logo_src_y, &logo_src_w, &logo_src_h);
  compute_start_menu_layout(render_width, render_height, item_count, logo_src_w, logo_src_h, &layout);
  if (layout.logo_w <= 0 || layout.logo_h <= 0 || logo_src_w <= 0 || logo_src_h <= 0) {
    return;
  }

  scaled_logo = prescale_menu_image_region_box(&assets->black_magic_logo, logo_src_x, logo_src_y, logo_src_w,
                                               logo_src_h, layout.logo_w, layout.logo_h);
  if (scaled_logo != NULL) {
    render_menu_argb_pixels(framebuffer, scaled_logo, layout.logo_w, layout.logo_h, layout.logo_x, layout.logo_y);
    free(scaled_logo);
  } else {
    fprintf(stderr, "Failed to pre-scale Black Magic menu logo to %dx%d\n", layout.logo_w, layout.logo_h);
    return;
  }

  render_menu_text_brightness(framebuffer, &assets->big_font, "BLACKMAGIC 1995", render_width / 2, layout.credit_y,
                              layout.scale, 255u);
  render_menu_text_brightness(framebuffer, &assets->big_font, "RETROFOUNDRY", render_width / 2,
                              layout.credit_y + (GLOOM_MENU_BIG_FONT_HEIGHT * layout.scale), layout.scale, 255u);
#if GLOOM_RUNTIME_IS_BINARY
  render_menu_text_brightness(framebuffer, &assets->big_font, runtime_title(), render_width / 2,
                              layout.credit_y + (GLOOM_MENU_BIG_FONT_HEIGHT * layout.scale * 2), layout.scale, 255u);
#endif
}

static void copy_argb_background(RenderFramebuffer *framebuffer, const uint32_t *background,
                                 int background_pitch_pixels, int render_width, int render_height) {
  int y = 0;

  if (framebuffer == NULL || background == NULL || background_pitch_pixels < render_width || render_width <= 0 ||
      render_height <= 0) {
    return;
  }

#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }

  for (y = 0; y < render_height; ++y) {
    uint8_t *dst_row = framebuffer->index_pixels + ((size_t)y * (size_t)framebuffer->index_pitch_pixels);
    const uint32_t *src_row = background + ((size_t)y * (size_t)background_pitch_pixels);
    int x = 0;

    for (x = 0; x < render_width; ++x) {
      dst_row[x] = dos_argb_to_index(src_row[x]);
    }
  }
  if (framebuffer->index_coverage != NULL) {
    memset(framebuffer->index_coverage, 0, (size_t)framebuffer->width * (size_t)framebuffer->height);
  }
#else
  if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
    return;
  }

  for (y = 0; y < render_height; ++y) {
    memcpy(framebuffer->pixels + ((size_t)y * (size_t)framebuffer->pitch_pixels),
           background + ((size_t)y * (size_t)background_pitch_pixels), (size_t)render_width * sizeof(*background));
  }
#endif
}

static void copy_menu_background(RenderFramebuffer *framebuffer, const uint32_t *background, int render_width,
                                 int render_height) {
  copy_argb_background(framebuffer, background, render_width, render_width, render_height);
}

#ifdef GLOOM_DOS_SDL3
static void copy_dos_index_background(RenderFramebuffer *framebuffer, const uint8_t *background, int render_width,
                                      int render_height) {
  int y = 0;

  if (framebuffer == NULL || background == NULL || render_width <= 0 || render_height <= 0 ||
      framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }

  for (y = 0; y < render_height; ++y) {
    memcpy(framebuffer->index_pixels + ((size_t)y * (size_t)framebuffer->index_pitch_pixels),
           background + ((size_t)y * (size_t)render_width), (size_t)render_width * sizeof(*background));
  }
  if (framebuffer->index_coverage != NULL) {
    memset(framebuffer->index_coverage, 0, (size_t)framebuffer->width * (size_t)framebuffer->height);
  }
}
#endif

#ifndef GLOOM_DOS_SDL3
static bool cache_start_menu_background(RenderFramebuffer *framebuffer, const MenuAssets *assets, int render_width,
                                        int render_height, uint32_t **io_cache, size_t *io_capacity) {
  StartMenuLayout layout;
  size_t required = 0u;

  if (framebuffer == NULL || assets == NULL || io_cache == NULL || io_capacity == NULL || render_width <= 0 ||
      render_height <= 0) {
    return false;
  }

  required = (size_t)render_width * (size_t)render_height;
  if (*io_capacity < required) {
    uint32_t *new_cache = (uint32_t *)realloc(*io_cache, required * sizeof(**io_cache));

    if (new_cache == NULL) {
      return false;
    }
    *io_cache = new_cache;
    *io_capacity = required;
  }

  framebuffer_clear(framebuffer, 0xFF000000u);
  render_menu_image(framebuffer, &assets->title, render_width, render_height);

  compute_start_menu_layout(render_width, render_height, start_menu_item_count(), g_start_menu_logo_src_w,
                            g_start_menu_logo_src_h, &layout);
  render_menu_text_brightness(framebuffer, &assets->big_font, layout.title_text, render_width / 2, layout.title_y,
                              layout.title_scale, 255u);
  render_start_menu_static_marks(framebuffer, assets, render_width, render_height);
#ifdef GLOOM_DOS_SDL3
  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return false;
  }
  for (y = 0; y < render_height; ++y) {
    const uint8_t *src_row = framebuffer->index_pixels + ((size_t)y * (size_t)framebuffer->index_pitch_pixels);
    uint32_t *dst_row = *io_cache + ((size_t)y * (size_t)render_width);
    int x = 0;

    for (x = 0; x < render_width; ++x) {
      dst_row[x] = g_dos_index_palette_argb[src_row[x]];
    }
  }
#else
  copy_menu_background(&(RenderFramebuffer){.pixels = *io_cache, .pitch_pixels = render_width}, framebuffer->pixels,
                       render_width, render_height);
#endif
  return true;
}
#endif

#ifdef GLOOM_DOS_SDL3
static bool cache_start_menu_background_dos_indices(RenderFramebuffer *framebuffer, const MenuAssets *assets,
                                                   int render_width, int render_height, uint8_t **io_cache,
                                                   size_t *io_capacity) {
  StartMenuLayout layout;
  int y = 0;
  size_t required = 0u;

  if (framebuffer == NULL || assets == NULL || io_cache == NULL || io_capacity == NULL || render_width <= 0 ||
      render_height <= 0) {
    return false;
  }

  required = (size_t)render_width * (size_t)render_height;
  if (*io_capacity < required) {
    uint8_t *new_cache = (uint8_t *)realloc(*io_cache, required * sizeof(**io_cache));

    if (new_cache == NULL) {
      return false;
    }
    *io_cache = new_cache;
    *io_capacity = required;
  }

  framebuffer_clear(framebuffer, 0xFF000000u);
  render_menu_image(framebuffer, &assets->title, render_width, render_height);

  compute_start_menu_layout(render_width, render_height, start_menu_item_count(), g_start_menu_logo_src_w,
                            g_start_menu_logo_src_h, &layout);
  render_menu_text_brightness(framebuffer, &assets->big_font, layout.title_text, render_width / 2, layout.title_y,
                              layout.title_scale, 255u);
  render_start_menu_static_marks(framebuffer, assets, render_width, render_height);

  if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
    return false;
  }
  for (y = 0; y < render_height; ++y) {
    memcpy(*io_cache + ((size_t)y * (size_t)render_width),
           framebuffer->index_pixels + ((size_t)y * (size_t)framebuffer->index_pitch_pixels),
           (size_t)render_width * sizeof(**io_cache));
  }
  return true;
}
#endif

static int start_menu_item_count(void) {
#if GLOOM_RUNTIME_HAS_AUTOSAVE
#ifdef __EMSCRIPTEN__
  return 6;
#else
  return 7;
#endif
#elif defined(__EMSCRIPTEN__)
  return 4;
#else
  return 5;
#endif
}

static bool start_menu_item_enabled(int item_index) {
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  if (item_index == 1) {
    return gloom_autosave_available(false);
  }
  if (item_index == 3) {
    return gloom_autosave_available(true);
  }
#endif
  return item_index >= 0 && item_index < start_menu_item_count();
}

static int start_menu_next_enabled_index(int selected_index, int direction) {
  int item_count = start_menu_item_count();
  int candidate = selected_index;
  int attempts = 0;

  if (item_count <= 0) {
    return 0;
  }
  if (direction == 0) {
    direction = 1;
  }

  for (attempts = 0; attempts < item_count; ++attempts) {
    candidate += direction;
    while (candidate < 0) {
      candidate += item_count;
    }
    while (candidate >= item_count) {
      candidate -= item_count;
    }
    if (start_menu_item_enabled(candidate)) {
      return candidate;
    }
  }
  return selected_index >= 0 && selected_index < item_count ? selected_index : 0;
}

static int menu_row_at_point_from_y(int render_width, int render_height, int item_count, int menu_y, int x, int y) {
  int scale = menu_pixel_scale_for_viewport(render_width, render_height);
  int row_height = GLOOM_MENU_BIG_FONT_HEIGHT * scale;
  int index = 0;

  if (render_width <= 0 || render_height <= 0 || item_count <= 0 || x < 0 || x >= render_width || y < 0 ||
      y >= render_height) {
    return -1;
  }
  if (y < menu_y || y >= menu_y + (item_count * row_height)) {
    return -1;
  }
  index = (y - menu_y) / row_height;
  return index >= 0 && index < item_count ? index : -1;
}

static int menu_row_at_point(int render_width, int render_height, int item_count, int x, int y) {
  return menu_row_at_point_from_y(render_width, render_height, item_count,
                                  start_menu_y_for_viewport(render_width, render_height, item_count), x, y);
}

static int menu_row_at_event_point(SDL_Renderer *renderer, int render_width, int render_height, int item_count,
                                    float window_x, float window_y) {
#ifdef GLOOM_DOS_SDL3
  float render_x = window_x;
  float render_y = window_y;

  if (renderer != NULL &&
      SDL_RenderCoordinatesFromWindow(renderer, window_x, window_y, &render_x, &render_y)) {
    return menu_row_at_point(render_width, render_height, item_count, (int)render_x, (int)render_y);
  }
#else
  (void)renderer;
#endif
  return menu_row_at_point(render_width, render_height, item_count, (int)window_x, (int)window_y);
}

static int activate_start_menu_selection(int selected_index, GloomGameMode *out_game_mode,
                                         RuntimeControlConfig *io_control_config, bool *io_selected_visible,
                                         int *io_flash_ticks) {
  if (out_game_mode == NULL || io_control_config == NULL || io_selected_visible == NULL || io_flash_ticks == NULL) {
    return -1;
  }
  if (selected_index == 0) {
    *out_game_mode = GLOOM_GAME_MODE_ONE_PLAYER;
    return 1;
  }
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  if (selected_index == 1) {
    if (gloom_autosave_available(false)) {
      *out_game_mode = GLOOM_GAME_MODE_LOAD_ONE_PLAYER;
      return 1;
    }
    *io_selected_visible = false;
    *io_flash_ticks = GLOOM_MENU_FLASH_TICKS;
    return 0;
  }
  if (selected_index == 2) {
    *out_game_mode = GLOOM_GAME_MODE_TWO_PLAYER;
    return 1;
  }
  if (selected_index == 3) {
    if (gloom_autosave_available(true)) {
      *out_game_mode = GLOOM_GAME_MODE_LOAD_TWO_PLAYER;
      return 1;
    }
    *io_selected_visible = false;
    *io_flash_ticks = GLOOM_MENU_FLASH_TICKS;
    return 0;
  }
  if (selected_index > 3) {
    selected_index -= 2;
  }
#else
  if (selected_index == 1) {
    *out_game_mode = GLOOM_GAME_MODE_TWO_PLAYER;
    return 1;
  }
#endif
  if (selected_index == 2) {
    cycle_control_config(io_control_config, 0u);
    *io_selected_visible = false;
    *io_flash_ticks = GLOOM_MENU_FLASH_TICKS;
    return 0;
  }
  if (selected_index == 3) {
    cycle_control_config(io_control_config, 1u);
    *io_selected_visible = false;
    *io_flash_ticks = GLOOM_MENU_FLASH_TICKS;
    return 0;
  }
  if (selected_index == 4) {
    return -1;
  }
  return 0;
}

static void render_start_menu_frame(RenderFramebuffer *framebuffer, const MenuAssets *assets,
                                    const uint32_t *background, const uint8_t *dos_index_background,
                                    int render_width, int render_height,
                                    int selected_index, bool selected_visible,
                                    uint8_t violence_mode, const RuntimeControlConfig *control_config) {
  const char *items[7];
  int item_count = start_menu_item_count();
  char player1_item[32];
  char player2_item[32];
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  bool one_player_autosave_available = gloom_autosave_available(false);
  bool two_player_autosave_available = gloom_autosave_available(true);
#endif
  int scale = 1;
  int y = 0;
  int i = 0;

  if (framebuffer == NULL || assets == NULL) {
    return;
  }
  (void)violence_mode;
#ifndef GLOOM_DOS_SDL3
  (void)dos_index_background;
#endif

  (void)snprintf(player1_item, sizeof(player1_item), "PLAYER 1 %s",
                 control_source_menu_name(control_config != NULL ? control_config->player1 : GLOOM_CONTROL_KEYBOARD));
  (void)snprintf(player2_item, sizeof(player2_item), "PLAYER 2 %s",
                 control_source_menu_name(control_config != NULL ? control_config->player2 : GLOOM_CONTROL_GAMEPAD_1));
  items[0] = "ONE PLAYER GAME";
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  items[1] = "LOAD ONE PLAYER AUTOSAVE";
  items[2] = "TWO PLAYER GAME";
  items[3] = "LOAD TWO PLAYER AUTOSAVE";
  items[4] = player1_item;
  items[5] = player2_item;
#ifndef __EMSCRIPTEN__
  items[6] = "EXIT GLOOM";
#endif
#else
  items[1] = "TWO PLAYER GAME";
  items[2] = player1_item;
  items[3] = player2_item;
  items[4] = "EXIT GLOOM";
#endif

#ifdef GLOOM_DOS_SDL3
  if (dos_index_background != NULL) {
    copy_dos_index_background(framebuffer, dos_index_background, render_width, render_height);
  } else if (background != NULL) {
    copy_menu_background(framebuffer, background, render_width, render_height);
  } else {
    framebuffer_clear(framebuffer, 0xFF000000u);
    render_menu_image(framebuffer, &assets->title, render_width, render_height);
  }
#else
  if (background != NULL) {
    copy_menu_background(framebuffer, background, render_width, render_height);
  } else {
    framebuffer_clear(framebuffer, 0xFF000000u);
    render_menu_image(framebuffer, &assets->title, render_width, render_height);
  }
#endif

  scale = menu_pixel_scale_for_viewport(render_width, render_height);
  y = start_menu_y_for_viewport(render_width, render_height, item_count);
  for (i = 0; i < item_count; ++i) {
    uint8_t brightness = (i == selected_index && !selected_visible) ? 96u : 255u;

#if GLOOM_RUNTIME_HAS_AUTOSAVE
    if ((i == 1 && !one_player_autosave_available) || (i == 3 && !two_player_autosave_available)) {
      brightness = 96u;
    }
#endif
    render_menu_text_brightness(framebuffer, &assets->big_font, items[i], render_width / 2,
                                y + (i * GLOOM_MENU_BIG_FONT_HEIGHT * scale), scale, brightness);
  }
}

static int run_start_menu(SDL_Renderer *renderer, RenderFramebuffer *framebuffer, const MenuAssets *assets,
                          int render_width, int render_height, GloomGameMode *out_game_mode,
                          uint8_t *io_violence_mode, RuntimeControlConfig *io_control_config) {
#ifdef GLOOM_DOS_SDL3
  static uint8_t *menu_background_index_cache = NULL;
  static size_t menu_background_index_capacity = 0u;
#else
  static uint32_t *menu_background_cache = NULL;
  static size_t menu_background_capacity = 0u;
#endif
  int selected_index = 0;
  int flash_ticks = GLOOM_MENU_FLASH_TICKS;
  bool selected_visible = false;
  bool running = true;
  bool redraw = true;
  uint32_t last_tick = SDL_GetTicks();
#ifdef GLOOM_DOS_SDL3
  bool dos_logged_first_frame = false;
#endif
#ifdef __EMSCRIPTEN__
  bool web_loading_overlay_hidden = false;
#endif

  if (renderer == NULL || framebuffer == NULL || assets == NULL || out_game_mode == NULL ||
      io_violence_mode == NULL || io_control_config == NULL) {
    return -1;
  }
  runtime_prepare_menu_input();
  normalize_control_config(io_control_config);
  {
    int logo_src_x = 0;
    int logo_src_y = 0;

    menu_image_content_bounds(&assets->black_magic_logo, &logo_src_x, &logo_src_y, &g_start_menu_logo_src_w,
                              &g_start_menu_logo_src_h);
    (void)logo_src_x;
    (void)logo_src_y;
    if (g_start_menu_logo_src_w <= 0 || g_start_menu_logo_src_h <= 0) {
      g_start_menu_logo_src_w = 0;
      g_start_menu_logo_src_h = 0;
    }
  }
  if (*out_game_mode == GLOOM_GAME_MODE_TWO_PLAYER) {
#if GLOOM_RUNTIME_HAS_AUTOSAVE
    selected_index = 2;
#else
    selected_index = 1;
#endif
  } else if (*out_game_mode == GLOOM_GAME_MODE_COMBAT) {
#if GLOOM_RUNTIME_HAS_AUTOSAVE
    selected_index = 2;
#else
    selected_index = 1;
#endif
  }
#ifdef GLOOM_DOS_SDL3
  apply_dos_menu_display_palette(framebuffer, &assets->title, &assets->black_magic_logo, &assets->big_font);
#endif
  if (begin_render_framebuffer(framebuffer)) {
#ifdef GLOOM_DOS_SDL3
    (void)cache_start_menu_background_dos_indices(framebuffer, assets, render_width, render_height,
                                                  &menu_background_index_cache, &menu_background_index_capacity);
#else
    (void)cache_start_menu_background(framebuffer, assets, render_width, render_height, &menu_background_cache,
                                      &menu_background_capacity);
#endif
    end_render_framebuffer(framebuffer);
  }
  menu_music_start();

  while (running) {
    SDL_Event event;
    uint32_t now = SDL_GetTicks();
    const int item_count = start_menu_item_count();

    while (SDL_PollEvent(&event) != 0) {
      gamepad_handle_event(&event);
      if (event.type == SDL_QUIT) {
        menu_music_stop();
        return -1;
      }
      if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        SDL_Keycode sym = event.key.keysym.sym;

        if (sym == SDLK_UP || sym == SDLK_w) {
          selected_index = start_menu_next_enabled_index(selected_index, -1);
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (sym == SDLK_DOWN || sym == SDLK_s) {
          selected_index = start_menu_next_enabled_index(selected_index, 1);
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (sym == SDLK_RETURN || sym == SDLK_SPACE || sym == SDLK_LCTRL || sym == SDLK_RCTRL) {
          int result = 0;

          audio_play_ui_activate();
          menu_pause_after_selection();
          result = activate_start_menu_selection(selected_index, out_game_mode, io_control_config, &selected_visible,
                                                 &flash_ticks);
          if (result > 0) {
            menu_music_stop();
            return 0;
          }
          if (result < 0) {
            menu_music_stop();
            return -1;
          }
          redraw = true;
        } else if (sym == SDLK_ESCAPE) {
#ifndef __EMSCRIPTEN__
          audio_play_ui_back();
          menu_music_stop();
          return -1;
#endif
        }
      }
      if (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEMOTION) {
        int hover_index =
            menu_row_at_event_point(renderer, render_width, render_height, item_count, event.motion.x, event.motion.y);

        if (hover_index >= 0 && start_menu_item_enabled(hover_index) && hover_index != selected_index) {
          selected_index = hover_index;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        }
      }
      if (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_LEFT) {
        int click_index =
            menu_row_at_event_point(renderer, render_width, render_height, item_count, event.button.x, event.button.y);

        if (click_index >= 0 && start_menu_item_enabled(click_index)) {
          int result = 0;

          selected_index = click_index;
          selected_visible = true;
          audio_play_ui_activate();
          menu_pause_after_selection();
          result = activate_start_menu_selection(selected_index, out_game_mode, io_control_config, &selected_visible,
                                                 &flash_ticks);
          if (result > 0) {
            menu_music_stop();
            return 0;
          }
          if (result < 0) {
            menu_music_stop();
            return -1;
          }
          redraw = true;
        }
      }
      if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
          selected_index = start_menu_next_enabled_index(selected_index, -1);
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
          selected_index = start_menu_next_enabled_index(selected_index, 1);
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (gamepad_menu_activate_event(&event)) {
          int result = 0;

          audio_play_ui_activate();
          menu_pause_after_selection();
          result = activate_start_menu_selection(selected_index, out_game_mode, io_control_config, &selected_visible,
                                                 &flash_ticks);
          if (result > 0) {
            menu_music_stop();
            return 0;
          }
          if (result < 0) {
            menu_music_stop();
            return -1;
          }
          redraw = true;
        } else if (gamepad_menu_back_event(&event)) {
#ifndef __EMSCRIPTEN__
          audio_play_ui_back();
          menu_music_stop();
          return -1;
#endif
        }
      }
      menu_music_start();
    }

    if (now - last_tick >= (uint32_t)(1000 / GLOOM_AMIGA_GAME_TICK_HZ)) {
      last_tick = now;
      flash_ticks -= 1;
      if (flash_ticks <= 0) {
        selected_visible = !selected_visible;
        flash_ticks = GLOOM_MENU_FLASH_TICKS;
        redraw = true;
      }
    }

    if (redraw && begin_render_framebuffer(framebuffer)) {
#ifdef GLOOM_DOS_SDL3
      render_start_menu_frame(framebuffer, assets, NULL, menu_background_index_cache, render_width, render_height,
                              selected_index, selected_visible, *io_violence_mode, io_control_config);
#else
      render_start_menu_frame(framebuffer, assets, menu_background_cache, NULL, render_width, render_height, selected_index,
                              selected_visible, *io_violence_mode, io_control_config);
#endif
      end_render_framebuffer(framebuffer);
      present_framebuffer_texture(renderer, framebuffer, render_width, render_height);
#ifdef __EMSCRIPTEN__
      if (!web_loading_overlay_hidden) {
        runtime_emscripten_hide_loading_overlay();
        web_loading_overlay_hidden = true;
      }
#endif
#ifdef GLOOM_DOS_SDL3
      if (!dos_logged_first_frame) {
        dos_logf("DOS checkpoint: start menu first frame presented");
        fprintf(stderr, "DOS checkpoint: start menu first frame presented\n");
        fflush(stderr);
        dos_logged_first_frame = true;
      }
#endif
      redraw = false;
    }
#ifdef GLOOM_DOS_SDL3
    audio_dos_pump(&g_audio);
    SDL_Delay(redraw ? 1 : 8);
#else
    SDL_Delay(1);
#endif
  }

  menu_music_stop();
  return -1;
}

static bool load_combat_menu_image(MenuImage *out_image) {
  const char *candidates[5] = {"amiga/combat.iff", "amiga/combat", "amiga/pics/combat", "amiga/data/pics/combat",
                               "combat.iff"};

  return load_iff_menu_image_from_candidates(candidates, sizeof(candidates) / sizeof(candidates[0]), out_image);
}

static void render_combat_menu_frame(RenderFramebuffer *framebuffer, const MenuImage *image, const HudFont *font,
                                     int render_width, int render_height, int selected_index, bool selected_visible,
                                     int16_t lives) {
  const char *items[4];
  char lives_item[32];
  int scale = 1;
  int y = 0;
  int i = 0;

  if (framebuffer == NULL || font == NULL) {
    return;
  }

  items[0] = "PLAY SPACEHULK SERIES";
  items[1] = "PLAY GOTHIC TOMB SERIES";
  items[2] = "PLAY HELL SERIES";
  (void)snprintf(lives_item, sizeof(lives_item), "START WITH %d LIVES", (int)lives);
  items[3] = lives_item;

  framebuffer_clear(framebuffer, 0xFF000000u);
  render_menu_image(framebuffer, image, render_width, render_height);
  scale = menu_pixel_scale_for_viewport(render_width, render_height);
  y = (render_height / 2) - (((int)(sizeof(items) / sizeof(items[0])) * GLOOM_MENU_BIG_FONT_HEIGHT * scale) / 2);
  for (i = 0; i < (int)(sizeof(items) / sizeof(items[0])); ++i) {
    uint8_t brightness = (i == selected_index && !selected_visible) ? 96u : 255u;

    render_menu_text_brightness(framebuffer, font, items[i], render_width / 2,
                                y + (i * GLOOM_MENU_BIG_FONT_HEIGHT * scale), scale, brightness);
  }
}

static int run_combat_menu(SDL_Renderer *renderer, RenderFramebuffer *framebuffer, int render_width, int render_height,
                           uint8_t *out_series, int16_t *out_lives) {
  MenuImage image;
  HudFont font;
  int selected_index = 0;
  int flash_ticks = GLOOM_MENU_FLASH_TICKS;
  bool selected_visible = false;
  bool redraw = true;
  uint32_t last_tick = SDL_GetTicks();
  int16_t lives = 3;

  memset(&image, 0, sizeof(image));
  memset(&font, 0, sizeof(font));
  if (renderer == NULL || framebuffer == NULL || out_series == NULL || out_lives == NULL) {
    return -1;
  }
  runtime_prepare_menu_input();
  if (!load_combat_menu_image(&image) || !load_menu_big_font(&font)) {
    free_menu_image(&image);
    free_hud_font(&font);
    return -1;
  }
#ifdef GLOOM_DOS_SDL3
  apply_dos_menu_display_palette(framebuffer, &image, NULL, &font);
#endif

  while (true) {
    SDL_Event event;
    uint32_t now = SDL_GetTicks();

    while (SDL_PollEvent(&event) != 0) {
      gamepad_handle_event(&event);
      if (event.type == SDL_QUIT) {
        free_menu_image(&image);
        free_hud_font(&font);
        return -1;
      }
      if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        SDL_Keycode sym = event.key.keysym.sym;

        if (sym == SDLK_UP || sym == SDLK_w) {
          selected_index -= 1;
          if (selected_index < 0) selected_index = 3;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (sym == SDLK_DOWN || sym == SDLK_s) {
          selected_index += 1;
          if (selected_index > 3) selected_index = 0;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (sym == SDLK_RETURN || sym == SDLK_SPACE || sym == SDLK_LCTRL || sym == SDLK_RCTRL) {
          audio_play_ui_activate();
          menu_pause_after_selection();
          if (selected_index < 3) {
            *out_series = (uint8_t)(selected_index + 1);
            *out_lives = lives;
            free_menu_image(&image);
            free_hud_font(&font);
            return 0;
          }
          lives += 1;
          if (lives > 9) {
            lives = 2;
          }
          redraw = true;
        } else if (sym == SDLK_ESCAPE) {
          audio_play_ui_back();
          free_menu_image(&image);
          free_hud_font(&font);
          return -1;
        }
      }
      if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
          selected_index -= 1;
          if (selected_index < 0) selected_index = 3;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
          selected_index += 1;
          if (selected_index > 3) selected_index = 0;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (gamepad_menu_activate_event(&event)) {
          audio_play_ui_activate();
          menu_pause_after_selection();
          if (selected_index < 3) {
            *out_series = (uint8_t)(selected_index + 1);
            *out_lives = lives;
            free_menu_image(&image);
            free_hud_font(&font);
            return 0;
          }
          lives += 1;
          if (lives > 9) {
            lives = 2;
          }
          redraw = true;
        } else if (gamepad_menu_back_event(&event)) {
          audio_play_ui_back();
          free_menu_image(&image);
          free_hud_font(&font);
          return -1;
        }
      }
    }

    if (now - last_tick >= (uint32_t)(1000 / GLOOM_AMIGA_GAME_TICK_HZ)) {
      last_tick = now;
      flash_ticks -= 1;
      if (flash_ticks <= 0) {
        selected_visible = !selected_visible;
        flash_ticks = GLOOM_MENU_FLASH_TICKS;
        redraw = true;
      }
    }

    if (redraw && begin_render_framebuffer(framebuffer)) {
      render_combat_menu_frame(framebuffer, &image, &font, render_width, render_height, selected_index,
                               selected_visible, lives);
      end_render_framebuffer(framebuffer);
      present_framebuffer_texture(renderer, framebuffer, render_width, render_height);
      redraw = false;
    }
#ifdef GLOOM_DOS_SDL3
    SDL_Delay(redraw ? 1 : 8);
#else
    SDL_Delay(1);
#endif
  }
}

static bool run_combat_result_screen(SDL_Renderer *renderer, RenderFramebuffer *framebuffer, int render_width,
                                     int render_height, const char *message) {
  MenuImage image;
  HudFont font;
  bool running = true;

  memset(&image, 0, sizeof(image));
  memset(&font, 0, sizeof(font));
  if (renderer == NULL || framebuffer == NULL || message == NULL) {
    return false;
  }
  runtime_prepare_menu_input();
  if (!load_combat_menu_image(&image) || !load_menu_big_font(&font)) {
    free_menu_image(&image);
    free_hud_font(&font);
    return false;
  }
#ifdef GLOOM_DOS_SDL3
  apply_dos_menu_display_palette(framebuffer, &image, NULL, &font);
#endif

  while (running) {
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
      gamepad_handle_event(&event);
      if (event.type == SDL_QUIT) {
        free_menu_image(&image);
        free_hud_font(&font);
        return false;
      }
      if (event.type == SDL_KEYDOWN || (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEBUTTONDOWN) ||
          event.type == SDL_CONTROLLERBUTTONDOWN) {
        running = false;
      }
    }

    if (begin_render_framebuffer(framebuffer)) {
      framebuffer_clear(framebuffer, 0xFF000000u);
      render_menu_image(framebuffer, &image, render_width, render_height);
      render_menu_text(framebuffer, &font, message, render_width / 2, render_height / 2,
                       menu_pixel_scale_for_viewport(render_width, render_height));
      end_render_framebuffer(framebuffer);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      {
        SDL_Rect dst = {0, 0, render_width, render_height};
        (void)SDL_RenderCopy(renderer, framebuffer->texture, NULL, &dst);
      }
      SDL_RenderPresent(renderer);
    }
    SDL_Delay(1);
  }

  free_menu_image(&image);
  free_hud_font(&font);
  return true;
}

static int pause_menu_item_count(bool two_player_mode) {
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  return gloom_autosave_available(two_player_mode) ? 3 : 2;
#else
  (void)two_player_mode;
  return 2;
#endif
}

static PauseMenuResult pause_menu_result_for_index(int selected_index, bool two_player_mode) {
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  if (gloom_autosave_available(two_player_mode)) {
    if (selected_index == 0) return PAUSE_MENU_CONTINUE;
    if (selected_index == 1) return PAUSE_MENU_LOAD_GAME;
    if (selected_index == 2) return PAUSE_MENU_MAIN_MENU;
    return PAUSE_MENU_CONTINUE;
  }
#else
  (void)two_player_mode;
#endif
  return selected_index == 0 ? PAUSE_MENU_CONTINUE : PAUSE_MENU_MAIN_MENU;
}

static int pause_menu_y_for_viewport(int render_width, int render_height, int item_count) {
  int scale = menu_pixel_scale_for_viewport(render_width, render_height);

  return (render_height / 2) - ((item_count * GLOOM_MENU_BIG_FONT_HEIGHT * scale) / 2);
}

static int pause_menu_row_at_point(int render_width, int render_height, int item_count, int x, int y) {
  return menu_row_at_point_from_y(render_width, render_height, item_count,
                                  pause_menu_y_for_viewport(render_width, render_height, item_count), x, y);
}

static int pause_menu_row_at_event_point(SDL_Renderer *renderer, int render_width, int render_height, int item_count,
                                         float window_x, float window_y) {
#ifdef GLOOM_DOS_SDL3
  float render_x = window_x;
  float render_y = window_y;

  if (renderer != NULL &&
      SDL_RenderCoordinatesFromWindow(renderer, window_x, window_y, &render_x, &render_y)) {
    return pause_menu_row_at_point(render_width, render_height, item_count, (int)render_x, (int)render_y);
  }
#else
  (void)renderer;
#endif
  return pause_menu_row_at_point(render_width, render_height, item_count, (int)window_x, (int)window_y);
}

static void render_pause_menu_frame(RenderFramebuffer *framebuffer, const HudFont *font, const uint32_t *background,
                                    const uint8_t *dos_index_background, int background_pitch_pixels,
                                    int render_width, int render_height,
                                    int selected_index, bool selected_visible, bool two_player_mode) {
  const char *items[3];
  int item_count = pause_menu_item_count(two_player_mode);
  int scale = 1;
  int y = 0;
  int i = 0;

  if (framebuffer == NULL || font == NULL) {
    return;
  }

#ifdef GLOOM_DOS_SDL3
  if (dos_index_background != NULL) {
    copy_dos_index_background(framebuffer, dos_index_background, render_width, render_height);
  } else if (background != NULL && background_pitch_pixels >= render_width) {
    copy_argb_background(framebuffer, background, background_pitch_pixels, render_width, render_height);
  } else {
    framebuffer_clear(framebuffer, 0xFF000000u);
  }
#else
  (void)dos_index_background;
  if (background != NULL && background_pitch_pixels >= render_width) {
    copy_argb_background(framebuffer, background, background_pitch_pixels, render_width, render_height);
  } else {
    framebuffer_clear(framebuffer, 0xFF000000u);
  }
#endif
  items[0] = "CONTINUE";
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  if (gloom_autosave_available(two_player_mode)) {
    items[1] = "LOAD GAME";
    items[2] = "RETURN TO MAIN MENU";
  } else
#endif
  {
    items[1] = "RETURN TO MAIN MENU";
  }
  scale = menu_pixel_scale_for_viewport(render_width, render_height);
  y = pause_menu_y_for_viewport(render_width, render_height, item_count);
  for (i = 0; i < item_count; ++i) {
    uint8_t brightness = (i == selected_index && !selected_visible) ? 96u : 255u;

    render_menu_text_brightness(framebuffer, font, items[i], render_width / 2,
                                y + (i * GLOOM_MENU_BIG_FONT_HEIGHT * scale), scale, brightness);
  }
}

static PauseMenuResult run_pause_menu(SDL_Window *window, SDL_Renderer *renderer, RenderFramebuffer *framebuffer,
                                      int render_width, int render_height, bool *io_mouse_captured,
                                      double *io_mouse_dx_accum, bool *io_suppress_mouse_fire_until_button_up,
                                      bool two_player_mode) {
#ifdef GLOOM_DOS_SDL3
  static uint8_t *pause_background_indices = NULL;
  static size_t pause_background_index_capacity = 0u;
#else
  static uint32_t *pause_background = NULL;
  static size_t pause_background_capacity = 0u;
#endif
  HudFont font;
  int selected_index = 0;
  int flash_ticks = GLOOM_MENU_FLASH_TICKS;
  bool selected_visible = true;
  bool redraw = true;
  uint32_t last_tick = SDL_GetTicks();
  size_t required_pixels = 0u;
  bool pause_background_valid = false;

  memset(&font, 0, sizeof(font));
  if (window == NULL || renderer == NULL || framebuffer == NULL || io_mouse_captured == NULL ||
      io_mouse_dx_accum == NULL || io_suppress_mouse_fire_until_button_up == NULL) {
    return PAUSE_MENU_QUIT;
  }
  runtime_prepare_menu_input();
  if (!load_menu_big_font(&font)) {
    return PAUSE_MENU_QUIT;
  }
  required_pixels = (size_t)render_width * (size_t)render_height;
#ifdef GLOOM_DOS_SDL3
  if (required_pixels > 0u && pause_background_index_capacity < required_pixels) {
    uint8_t *new_background = (uint8_t *)realloc(pause_background_indices, required_pixels * sizeof(*pause_background_indices));

    if (new_background == NULL) {
      free_hud_font(&font);
      return PAUSE_MENU_QUIT;
    }
    pause_background_indices = new_background;
    pause_background_index_capacity = required_pixels;
  }
  if (pause_background_indices != NULL && framebuffer->last_frame_indices != NULL && framebuffer->width == render_width &&
      framebuffer->height == render_height) {
    int py = 0;

    for (py = 0; py < render_height; ++py) {
      memcpy(pause_background_indices + ((size_t)py * (size_t)render_width),
             framebuffer->last_frame_indices + ((size_t)py * (size_t)render_width),
             (size_t)render_width * sizeof(*pause_background_indices));
    }
    pause_background_valid = true;
  }
#else
  if (required_pixels > 0u && pause_background_capacity < required_pixels) {
    uint32_t *new_background = (uint32_t *)realloc(pause_background, required_pixels * sizeof(*pause_background));

    if (new_background == NULL) {
      free_hud_font(&font);
      return PAUSE_MENU_QUIT;
    }
    pause_background = new_background;
    pause_background_capacity = required_pixels;
  }
  if (pause_background != NULL && framebuffer->last_frame_pixels != NULL && framebuffer->width == render_width &&
      framebuffer->height == render_height) {
    int py = 0;

    for (py = 0; py < render_height; ++py) {
      memcpy(pause_background + ((size_t)py * (size_t)render_width),
             framebuffer->last_frame_pixels + ((size_t)py * (size_t)render_width),
             (size_t)render_width * sizeof(*pause_background));
    }
    pause_background_valid = true;
  }
#endif
  audio_pause_output(&g_audio, true, true);
  audio_pause_output(&g_audio, false, false);
  audio_play_ui_activate();

  while (true) {
    SDL_Event event;
    uint32_t now = SDL_GetTicks();
    const int item_count = pause_menu_item_count(two_player_mode);

#ifdef __EMSCRIPTEN__
    if (runtime_emscripten_consume_pointer_lock_lost()) {
      (void)set_runtime_mouse_capture(window, false);
      *io_mouse_captured = false;
      *io_mouse_dx_accum = 0.0;
      *io_suppress_mouse_fire_until_button_up = false;
      SDL_FlushEvent(SDL_MOUSEMOTION);
    }
#endif

    while (SDL_PollEvent(&event) != 0) {
      gamepad_handle_event(&event);
      if (event.type == SDL_QUIT) {
        audio_pause_output(&g_audio, false, false);
        free_hud_font(&font);
        return PAUSE_MENU_QUIT;
      }
      if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
        SDL_Keycode sym = event.key.keysym.sym;

        if (sym == SDLK_UP || sym == SDLK_w) {
          selected_index -= 1;
          if (selected_index < 0) selected_index = item_count - 1;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (sym == SDLK_DOWN || sym == SDLK_s) {
          selected_index += 1;
          if (selected_index >= item_count) selected_index = 0;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (sym == SDLK_RETURN || sym == SDLK_SPACE || sym == SDLK_LCTRL || sym == SDLK_RCTRL) {
          PauseMenuResult result = pause_menu_result_for_index(selected_index, two_player_mode);
          audio_play_ui_activate();
          menu_pause_after_selection();
          audio_pause_output(&g_audio, false, false);
          free_hud_font(&font);
          return result;
        } else if (sym == SDLK_p) {
          audio_play_ui_activate();
          menu_pause_after_selection();
          audio_pause_output(&g_audio, false, false);
          free_hud_font(&font);
          return PAUSE_MENU_CONTINUE;
        } else if (sym == SDLK_ESCAPE) {
          if (*io_mouse_captured || g_runtime_mouse_capture_active || SDL_GetRelativeMouseMode()) {
            audio_play_ui_back();
            (void)set_runtime_mouse_capture(window, false);
            *io_mouse_captured = false;
            *io_mouse_dx_accum = 0.0;
            *io_suppress_mouse_fire_until_button_up = false;
            SDL_FlushEvent(SDL_MOUSEMOTION);
          }
        }
      }
      if (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEBUTTONDOWN && event.button.clicks >= 2) {
        if (event.button.button == SDL_BUTTON_LEFT) {
          int click_index =
              pause_menu_row_at_event_point(renderer, render_width, render_height, item_count, event.button.x,
                                            event.button.y);

          if (click_index >= 0) {
            PauseMenuResult result = pause_menu_result_for_index(click_index, two_player_mode);

            selected_index = click_index;
            audio_play_ui_activate();
            menu_pause_after_selection();
            audio_pause_output(&g_audio, false, false);
            free_hud_font(&font);
            return result;
          }
        }
        if (!*io_mouse_captured && set_runtime_mouse_capture(window, true)) {
          *io_mouse_captured = true;
          *io_mouse_dx_accum = 0.0;
          *io_suppress_mouse_fire_until_button_up = event.button.button == SDL_BUTTON_LEFT;
          SDL_FlushEvent(SDL_MOUSEMOTION);
        }
      } else if (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEBUTTONDOWN &&
                 event.button.button == SDL_BUTTON_LEFT) {
        int click_index =
            pause_menu_row_at_event_point(renderer, render_width, render_height, item_count, event.button.x,
                                          event.button.y);

        if (click_index >= 0) {
          PauseMenuResult result = pause_menu_result_for_index(click_index, two_player_mode);

          selected_index = click_index;
          audio_play_ui_activate();
          menu_pause_after_selection();
          audio_pause_output(&g_audio, false, false);
          free_hud_font(&font);
          return result;
        }
      }
      if (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEMOTION) {
        int hover_index =
            pause_menu_row_at_event_point(renderer, render_width, render_height, item_count, event.motion.x,
                                          event.motion.y);

        if (hover_index >= 0 && hover_index != selected_index) {
          selected_index = hover_index;
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        }
      }
      if (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
        *io_suppress_mouse_fire_until_button_up = false;
      }
      if (event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP ||
            event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_DOWN) {
          if (event.cbutton.button == SDL_CONTROLLER_BUTTON_DPAD_UP) {
            selected_index -= 1;
            if (selected_index < 0) selected_index = item_count - 1;
          } else {
            selected_index += 1;
            if (selected_index >= item_count) selected_index = 0;
          }
          selected_visible = false;
          flash_ticks = GLOOM_MENU_FLASH_TICKS;
          redraw = true;
          audio_play_ui_move();
        } else if (gamepad_menu_activate_event(&event)) {
          PauseMenuResult result = pause_menu_result_for_index(selected_index, two_player_mode);
          audio_play_ui_activate();
          menu_pause_after_selection();
          audio_pause_output(&g_audio, false, false);
          free_hud_font(&font);
          return result;
        } else if (gamepad_menu_back_event(&event)) {
          audio_play_ui_back();
          audio_pause_output(&g_audio, false, false);
          free_hud_font(&font);
          return PAUSE_MENU_CONTINUE;
        }
      }
    }

    if (now - last_tick >= (uint32_t)(1000 / GLOOM_AMIGA_GAME_TICK_HZ)) {
      last_tick = now;
      flash_ticks -= 1;
      if (flash_ticks <= 0) {
        selected_visible = !selected_visible;
        flash_ticks = GLOOM_MENU_FLASH_TICKS;
        redraw = true;
      }
    }

    if (redraw && begin_render_framebuffer(framebuffer)) {
#ifdef GLOOM_DOS_SDL3
      render_pause_menu_frame(framebuffer, &font, NULL, pause_background_valid ? pause_background_indices : NULL,
                              render_width, render_width, render_height, selected_index, selected_visible,
                              two_player_mode);
#else
      render_pause_menu_frame(framebuffer, &font, pause_background_valid ? pause_background : NULL, NULL, render_width,
                              render_width, render_height, selected_index, selected_visible, two_player_mode);
#endif
      end_render_framebuffer(framebuffer);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      {
        SDL_Rect dst = {0, 0, render_width, render_height};
        (void)SDL_RenderCopy(renderer, framebuffer->texture, NULL, &dst);
      }
      SDL_RenderPresent(renderer);
      redraw = false;
    }
#ifdef GLOOM_DOS_SDL3
    SDL_Delay(redraw ? 1 : 8);
#else
    SDL_Delay(1);
#endif
  }
}

static void render_completion_frame(RenderFramebuffer *framebuffer, const MenuImage *image, const HudFont *font,
                                    const char *text, int render_width, int render_height) {
  int scale = 1;
  int y = 0;

  if (framebuffer == NULL) {
    return;
  }

  framebuffer_clear(framebuffer, 0xFF000000u);
  render_menu_image(framebuffer, image, render_width, render_height);
  if (font == NULL || text == NULL || text[0] == '\0') {
    return;
  }

  scale = menu_pixel_scale_for_viewport(render_width, render_height);
  y = render_height - (GLOOM_MENU_BIG_FONT_HEIGHT * scale * 3);
  if (y < GLOOM_MENU_BIG_FONT_HEIGHT * scale) {
    y = GLOOM_MENU_BIG_FONT_HEIGHT * scale;
  }
  render_menu_text(framebuffer, font, text, render_width / 2, y, scale);
}

static bool run_completion_screen(SDL_Renderer *renderer, RenderFramebuffer *framebuffer,
                                  const ScriptCompletion *completion, int render_width, int render_height) {
  MenuImage image;
  HudFont font;
  bool running = true;

  memset(&image, 0, sizeof(image));
  memset(&font, 0, sizeof(font));
  if (renderer == NULL || framebuffer == NULL || completion == NULL) {
    return false;
  }
  runtime_prepare_menu_input();
  if (!load_script_picture_image(completion->picture, &image)) {
    fprintf(stderr, "Cannot execute scriptdone completion screen: missing original pict_%s image\n",
            completion->picture);
    return false;
  }
  if (!load_menu_big_font(&font)) {
    free_menu_image(&image);
    return false;
  }
#ifdef GLOOM_DOS_SDL3
  apply_dos_menu_display_palette(framebuffer, &image, NULL, &font);
#endif

  while (running) {
    SDL_Event event;

    while (SDL_PollEvent(&event) != 0) {
      gamepad_handle_event(&event);
      if (event.type == SDL_QUIT) {
        free_hud_font(&font);
        free_menu_image(&image);
        return false;
      }
      if (event.type == SDL_KEYDOWN || (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEBUTTONDOWN) ||
          event.type == SDL_CONTROLLERBUTTONDOWN) {
        running = false;
      }
    }

    if (begin_render_framebuffer(framebuffer)) {
      render_completion_frame(framebuffer, &image, &font, completion->text, render_width, render_height);
      end_render_framebuffer(framebuffer);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      {
        SDL_Rect dst = {0, 0, render_width, render_height};
        (void)SDL_RenderCopy(renderer, framebuffer->texture, NULL, &dst);
      }
      SDL_RenderPresent(renderer);
    }
    SDL_Delay(1);
  }

  free_hud_font(&font);
  free_menu_image(&image);
  return true;
}

static void render_script_intro_frame(RenderFramebuffer *framebuffer, const MenuImage *image, const HudFont *font,
                                      const char *text, int render_width, int render_height) {
  char line[96];
  const char *cursor = text;
  int scale = 1;
  size_t max_chars = 1u;
  int line_count = 0;
  int line_index = 0;
  int start_y = 0;

  if (framebuffer == NULL) {
    return;
  }
  framebuffer_clear(framebuffer, 0xFF000000u);
  render_menu_image(framebuffer, image, render_width, render_height);
  if (font == NULL || text == NULL || text[0] == '\0') {
    return;
  }

  scale = menu_pixel_scale_for_viewport(render_width, render_height);
  while (scale > 1 && (GLOOM_MENU_BIG_FONT_HEIGHT * scale * 3) > render_height) {
    scale -= 1;
  }
  if (render_width > GLOOM_MENU_BIG_FONT_WIDTH * scale * 2) {
    max_chars = (size_t)((render_width - (GLOOM_MENU_BIG_FONT_WIDTH * scale * 2)) /
                         (GLOOM_MENU_BIG_FONT_WIDTH * scale));
  }

  while (*cursor != '\0') {
    size_t segment_len = 0u;
    size_t segment_pos = 0u;

    while (cursor[segment_len] != '\0' && cursor[segment_len] != '\\') {
      segment_len += 1u;
    }
    while (segment_pos < segment_len) {
      size_t consumed = copy_wrapped_menu_line(line, sizeof(line), cursor + segment_pos, segment_len - segment_pos,
                                               max_chars);
      if (consumed == 0u) {
        break;
      }
      line_count += 1;
      segment_pos += consumed;
    }
    cursor += segment_len;
    if (*cursor == '\\') {
      cursor += 1;
    }
  }
  if (line_count <= 0) {
    line_count = 1;
  }
  start_y = (render_height / 2) - ((line_count * GLOOM_MENU_BIG_FONT_HEIGHT * scale) / 2);
  if (start_y < GLOOM_MENU_BIG_FONT_HEIGHT * scale) {
    start_y = GLOOM_MENU_BIG_FONT_HEIGHT * scale;
  }

  cursor = text;
  while (*cursor != '\0') {
    size_t segment_len = 0u;
    size_t segment_pos = 0u;

    while (cursor[segment_len] != '\0' && cursor[segment_len] != '\\') {
      segment_len += 1u;
    }
    while (segment_pos < segment_len) {
      size_t consumed = copy_wrapped_menu_line(line, sizeof(line), cursor + segment_pos, segment_len - segment_pos,
                                               max_chars);
      if (consumed == 0u) {
        break;
      }
      render_menu_text(framebuffer, font, line, render_width / 2,
                       start_y + (line_index * GLOOM_MENU_BIG_FONT_HEIGHT * scale), scale);
      line_index += 1;
      segment_pos += consumed;
    }
    cursor += segment_len;
    if (*cursor == '\\') {
      cursor += 1;
    }
  }
}

static bool run_script_intro_screen(SDL_Renderer *renderer, RenderFramebuffer *framebuffer,
                                    const ScriptLevelIntro *intro, int render_width, int render_height) {
  static MenuImage image;
  static char cached_picture[sizeof(((ScriptLevelIntro *)0)->picture)] = {0};
  static HudFont font;
  static bool font_loaded = false;
  bool running = true;
  uint64_t intro_start = 0u;
  uint64_t performance_frequency = 0u;
#ifdef GLOOM_DOS_SDL3
  const double minimum_seconds = 0.0;
  const double auto_continue_seconds = 2.0;
#else
  const double minimum_seconds = 5.0;
  const double auto_continue_seconds = 5.0;
#endif

  if (renderer == NULL || framebuffer == NULL || intro == NULL || intro->picture[0] == '\0' ||
      intro->text[0] == '\0') {
    return true;
  }
  runtime_prepare_menu_input();
  if (!image.loaded || strcmp(cached_picture, intro->picture) != 0) {
    if (!load_script_picture_image(intro->picture, &image)) {
      fprintf(stderr, "Cannot execute script intro screen: missing original pict_%s image\n", intro->picture);
      return false;
    }
    (void)snprintf(cached_picture, sizeof(cached_picture), "%s", intro->picture);
  }
  if (!font_loaded) {
    memset(&font, 0, sizeof(font));
    if (!load_menu_big_font(&font)) {
      return false;
    }
    font_loaded = true;
  }
#ifdef GLOOM_DOS_SDL3
  apply_dos_menu_display_palette(framebuffer, &image, NULL, &font);
#endif
  intro_start = SDL_GetPerformanceCounter();
  performance_frequency = SDL_GetPerformanceFrequency();

  while (running) {
    SDL_Event event;
    const uint8_t *keyboard = NULL;
    double elapsed = 0.0;
    bool can_continue = true;

    if (performance_frequency > 0u) {
      uint64_t now = SDL_GetPerformanceCounter();

      elapsed = (double)(now - intro_start) / (double)performance_frequency;
    }
    can_continue = elapsed >= minimum_seconds;

    while (SDL_PollEvent(&event) != 0) {
      gamepad_handle_event(&event);
      if (event.type == SDL_QUIT) {
        return false;
      }
      if ((event.type == SDL_KEYDOWN && event.key.repeat == 0) ||
          (runtime_menu_accepts_mouse() && event.type == SDL_MOUSEBUTTONDOWN) ||
          event.type == SDL_CONTROLLERBUTTONDOWN) {
        if (can_continue) {
          running = false;
        }
      }
    }
    SDL_PumpEvents();
    keyboard = SDL_GetKeyboardState(NULL);
    if (can_continue && keyboard != NULL &&
        (keyboard[SDL_SCANCODE_SPACE] || keyboard[SDL_SCANCODE_RETURN] || keyboard[SDL_SCANCODE_KP_ENTER] ||
         keyboard[SDL_SCANCODE_LCTRL] || keyboard[SDL_SCANCODE_RCTRL] || keyboard[SDL_SCANCODE_ESCAPE])) {
      running = false;
    }
    if (running && elapsed >= auto_continue_seconds) {
      running = false;
    }

    if (begin_render_framebuffer(framebuffer)) {
      render_script_intro_frame(framebuffer, &image, &font, intro->text, render_width, render_height);
      end_render_framebuffer(framebuffer);
      SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
      SDL_RenderClear(renderer);
      {
        SDL_Rect dst = {0, 0, render_width, render_height};
        (void)SDL_RenderCopy(renderer, framebuffer->texture, NULL, &dst);
      }
      SDL_RenderPresent(renderer);
    }
    SDL_Delay(1);
  }

  return true;
}

#ifdef GLOOM_DOS_SDL3
static void pixelate_dos_locked_framebuffer(RenderFramebuffer *framebuffer, int pixsize) {
  int block = pixsize;
  int y = 0;

  if (framebuffer == NULL || framebuffer->index_pixels == NULL || framebuffer->width <= 0 || framebuffer->height <= 0 ||
      framebuffer->index_pitch_pixels < framebuffer->width) {
    return;
  }
  if (block <= 1) {
    return;
  }

  for (y = 0; y < framebuffer->height; y += block) {
    int h = block;
    int sample_y = 0;
    int x = 0;

    if (y + h > framebuffer->height) {
      h = framebuffer->height - y;
    }
    sample_y = y + (h / 2);

    for (x = 0; x < framebuffer->width; x += block) {
      int w = block;
      int row = 0;
      int sample_x = 0;
      uint8_t sample = 0u;

      if (x + w > framebuffer->width) {
        w = framebuffer->width - x;
      }
      sample_x = x + (w / 2);
      sample = framebuffer->index_pixels[((size_t)sample_y * (size_t)framebuffer->index_pitch_pixels) +
                                         (size_t)sample_x];

      for (row = 0; row < h; ++row) {
        memset(framebuffer->index_pixels + (((size_t)y + (size_t)row) * (size_t)framebuffer->index_pitch_pixels) +
                   (size_t)x,
               sample, (size_t)w);
      }
    }
  }
}
#else
static bool render_pixelate_texture(SDL_Renderer *renderer, SDL_Texture *texture, int render_width, int render_height,
                                    int pixsize) {
  int block = pixsize;
  int y = 0;

  if (renderer == NULL || texture == NULL || render_width <= 0 || render_height <= 0) {
    return false;
  }

  if (block < 1) {
    block = 1;
  }

  if (block <= 1) {
    SDL_Rect dst = {0, 0, render_width, render_height};
    return SDL_RenderCopy(renderer, texture, NULL, &dst) == 0;
  }

  for (y = 0; y < render_height; y += block) {
    int x = 0;
    int h = block;

    if (y + h > render_height) {
      h = render_height - y;
    }

    for (x = 0; x < render_width; x += block) {
      int w = block;
      SDL_Rect src;
      SDL_Rect dst;

      if (x + w > render_width) {
        w = render_width - x;
      }

      src.x = x + (w / 2);
      src.y = y + (h / 2);
      src.w = 1;
      src.h = 1;
      dst.x = x;
      dst.y = y;
      dst.w = w;
      dst.h = h;

      if (SDL_RenderCopy(renderer, texture, &src, &dst) != 0) {
        return false;
      }
    }
  }

  return true;
}
#endif

static void framebuffer_clear_outside_rect(RenderFramebuffer *framebuffer, int x, int y, int w, int h,
                                           uint32_t argb) {
  int row = 0;

  if (framebuffer == NULL || framebuffer->width <= 0 || framebuffer->height <= 0) {
    return;
  }
  for (row = 0; row < y; ++row) {
    framebuffer_clear_row_span(framebuffer, row, 0, framebuffer->width, argb);
  }
  for (row = y + h; row < framebuffer->height; ++row) {
    framebuffer_clear_row_span(framebuffer, row, 0, framebuffer->width, argb);
  }
  for (row = y; row < y + h; ++row) {
    framebuffer_clear_row_span(framebuffer, row, 0, x, argb);
    framebuffer_clear_row_span(framebuffer, row, x + w, framebuffer->width - (x + w), argb);
  }
}

static void framebuffer_scale_rect_in_place(RenderFramebuffer *framebuffer, int src_w, int src_h, int dst_x, int dst_y,
                                            int pixel_w, int pixel_h) {
  int src_y = 0;

  if (framebuffer == NULL || src_w <= 0 || src_h <= 0 || pixel_w <= 0 || pixel_h <= 0) {
    return;
  }
  if (dst_x < 0 || dst_y < 0 || dst_x + (src_w * pixel_w) > framebuffer->width ||
      dst_y + (src_h * pixel_h) > framebuffer->height) {
    return;
  }
  if (dst_x == 0 && dst_y == 0 && pixel_w == 1 && pixel_h == 1) {
    return;
  }

  for (src_y = src_h - 1; src_y >= 0; --src_y) {
    int dy = 0;

    for (dy = pixel_h - 1; dy >= 0; --dy) {
      int dst_row = dst_y + (src_y * pixel_h) + dy;
      int src_x = 0;

#ifdef GLOOM_DOS_SDL3
      uint8_t *dst = NULL;
      const uint8_t *src = NULL;

      if (framebuffer->index_pixels == NULL || framebuffer->index_pitch_pixels < framebuffer->width) {
        return;
      }
      dst = framebuffer->index_pixels + ((size_t)dst_row * (size_t)framebuffer->index_pitch_pixels) + (size_t)dst_x;
      src = framebuffer->index_pixels + ((size_t)src_y * (size_t)framebuffer->index_pitch_pixels);
      for (src_x = src_w - 1; src_x >= 0; --src_x) {
        int dx = 0;
        uint8_t value = src[src_x];

        for (dx = 0; dx < pixel_w; ++dx) {
          dst[(src_x * pixel_w) + dx] = value;
        }
      }
#else
      uint32_t *dst = NULL;
      const uint32_t *src = NULL;

      if (framebuffer->pixels == NULL || framebuffer->pitch_pixels < framebuffer->width) {
        return;
      }
      dst = framebuffer->pixels + ((size_t)dst_row * (size_t)framebuffer->pitch_pixels) + (size_t)dst_x;
      src = framebuffer->pixels + ((size_t)src_y * (size_t)framebuffer->pitch_pixels);
      for (src_x = src_w - 1; src_x >= 0; --src_x) {
        int dx = 0;
        uint32_t value = src[src_x];

        for (dx = 0; dx < pixel_w; ++dx) {
          dst[(src_x * pixel_w) + dx] = value;
        }
      }
#endif
    }
  }
}

static void render(SDL_Renderer *renderer, RenderFramebuffer *framebuffer, const AppState *state,
                   const GridOffsetSet *grid_offsets,
                   const WallTextureSet *wall_textures, const FlatTextureSet *flat_textures,
                   const ObjectVisualSet *object_visuals, const WeaponVisualSet *weapon_visuals,
                   const HudFont *hud_font, int render_width, int render_height, const char *notice_text) {
  int pixsize = state != NULL ? state->player_pixsize : 0;
  RuntimeGameplayViewport gameplay_viewport = runtime_gameplay_viewport_for_render_target(render_width, render_height);
  double profile_render_start_ms = profile_now_ms();
  double profile_start_ms = 0.0;

  if (framebuffer == NULL || framebuffer->texture == NULL || framebuffer->width != render_width ||
      framebuffer->height != render_height) {
    fprintf(stderr, "Cannot render: PC framebuffer was not prepared for %dx%d\n", render_width, render_height);
    return;
  }

  profile_start_ms = profile_now_ms();
  if (!begin_render_framebuffer(framebuffer)) {
    return;
  }
  if (gameplay_viewport.active || state == NULL || flat_textures == NULL || !flat_textures->floor.loaded ||
      !flat_textures->roof.loaded || wall_textures == NULL || wall_textures->loaded_count == 0u) {
    framebuffer_clear(framebuffer, 0xFF000000u);
  }
  profile_add_elapsed(&g_profiler.render_lock_clear_ms, profile_start_ms);
  if (state != NULL && state->two_player_mode) {
    AppState player2_state = *state;
    RuntimePlayerState player1_state;
    int left_w = gameplay_viewport.source_width / 2;
    int right_w = gameplay_viewport.source_width - left_w;

    capture_primary_player_state(state, &player1_state);
    render_scene_contents(framebuffer, state, grid_offsets, wall_textures, flat_textures, object_visuals, weapon_visuals,
                          hud_font, 0, 0, left_w, gameplay_viewport.source_height);
    profile_start_ms = profile_now_ms();
    apply_player_red_palette_rect(framebuffer, state, 0, 0, left_w, gameplay_viewport.source_height);
    profile_add_elapsed(&g_profiler.render_red_ms, profile_start_ms);
    apply_primary_player_state(&player2_state, &state->player2);
    player2_state.player2 = player1_state;
    player2_state.active_player_index = 1u;
    render_scene_contents(framebuffer, &player2_state, grid_offsets, wall_textures, flat_textures, object_visuals,
                          weapon_visuals, hud_font, left_w, 0, right_w, gameplay_viewport.source_height);
    profile_start_ms = profile_now_ms();
    apply_player_red_palette_rect(framebuffer, &player2_state, left_w, 0, right_w, gameplay_viewport.source_height);
    profile_add_elapsed(&g_profiler.render_red_ms, profile_start_ms);
  } else {
    render_scene_contents(framebuffer, state, grid_offsets, wall_textures, flat_textures, object_visuals, weapon_visuals,
                          hud_font, 0, 0, gameplay_viewport.source_width, gameplay_viewport.source_height);
    profile_start_ms = profile_now_ms();
    apply_player_red_palette_rect(framebuffer, state, 0, 0, gameplay_viewport.source_width,
                                  gameplay_viewport.source_height);
    profile_add_elapsed(&g_profiler.render_red_ms, profile_start_ms);
  }
  if (gameplay_viewport.active) {
    framebuffer_scale_rect_in_place(framebuffer, gameplay_viewport.source_width, gameplay_viewport.source_height,
                                    gameplay_viewport.dst_x, gameplay_viewport.dst_y, gameplay_viewport.pixel_width,
                                    gameplay_viewport.pixel_height);
    framebuffer_clear_outside_rect(framebuffer, gameplay_viewport.dst_x, gameplay_viewport.dst_y,
                                   gameplay_viewport.dst_width, gameplay_viewport.dst_height, 0xFF000000u);
  }
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  if (notice_text != NULL && notice_text[0] != '\0') {
    int scale = menu_pixel_scale_for_viewport(render_width, render_height);
    int y = (render_height / 2) - ((GLOOM_MENU_BIG_FONT_HEIGHT * scale) / 2);

    render_menu_text_brightness(framebuffer, hud_font, notice_text, render_width / 2, y, scale, 255u);
  }
#else
  (void)notice_text;
#endif
  snapshot_render_framebuffer(framebuffer);
#ifdef GLOOM_DOS_SDL3
  if (pixsize > 1) {
    pixelate_dos_locked_framebuffer(framebuffer, pixsize);
  }
#endif
  end_render_framebuffer(framebuffer);

  profile_start_ms = profile_now_ms();
#ifndef GLOOM_DOS_SDL3
  SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
  SDL_RenderClear(renderer);
#endif

#ifdef GLOOM_DOS_SDL3
  {
    SDL_Rect dst = {0, 0, render_width, render_height};
    if (SDL_RenderCopy(renderer, framebuffer->texture, NULL, &dst) != 0) {
      fprintf(stderr, "Cannot render DOS indexed framebuffer texture: %s\n", SDL_GetError());
    }
  }
#else
  if (pixsize > 1) {
    if (!render_pixelate_texture(renderer, framebuffer->texture, render_width, render_height, pixsize)) {
      fprintf(stderr, "Cannot render pixelate transition: SDL_RenderCopy failed: %s\n", SDL_GetError());
    }
  } else {
    SDL_Rect dst = {0, 0, render_width, render_height};
    if (SDL_RenderCopy(renderer, framebuffer->texture, NULL, &dst) != 0) {
      fprintf(stderr, "Cannot render PC framebuffer texture: %s\n", SDL_GetError());
    }
  }
#endif

  SDL_RenderPresent(renderer);
  profile_add_elapsed(&g_profiler.render_present_ms, profile_start_ms);
  profile_add_elapsed(&g_profiler.render_ms, profile_render_start_ms);
#ifdef GLOOM_DOS_SDL3
  audio_dos_pump(&g_audio);
#endif
}

static int16_t interpolate_int16(int16_t previous, int16_t current, float alpha) {
  return clamp_int16(round_float_to_int32((float)previous + (((float)current - (float)previous) * alpha)));
}

static bool zone_event_id_is_open_progress(int16_t event_id) {
  uint16_t raw = (uint16_t)event_id;

  return raw > (uint16_t)GLOOM_EVENT_COUNT && raw <= 32768u;
}

static void interpolate_render_zones(const AppState *previous, const AppState *current, float alpha,
                                     GloomZone *render_zones, size_t render_zone_count) {
  size_t i = 0u;

  if (previous == NULL || current == NULL || render_zones == NULL || previous->map.zones == NULL ||
      current->map.zones == NULL || render_zone_count != current->map.zone_count ||
      previous->map.zone_count != current->map.zone_count) {
    return;
  }

  memcpy(render_zones, current->map.zones, current->map.zone_count * sizeof(*render_zones));

  for (i = 0u; i < current->map.zone_count; ++i) {
    const GloomZone *prev_zone = &previous->map.zones[i];
    const GloomZone *cur_zone = &current->map.zones[i];
    GloomZone *render_zone = &render_zones[i];
    bool endpoints_changed = prev_zone->x1 != cur_zone->x1 || prev_zone->z1 != cur_zone->z1 ||
                             prev_zone->x2 != cur_zone->x2 || prev_zone->z2 != cur_zone->z2;

    if (endpoints_changed) {
      render_zone->x1 = interpolate_int16(prev_zone->x1, cur_zone->x1, alpha);
      render_zone->z1 = interpolate_int16(prev_zone->z1, cur_zone->z1, alpha);
      render_zone->x2 = interpolate_int16(prev_zone->x2, cur_zone->x2, alpha);
      render_zone->z2 = interpolate_int16(prev_zone->z2, cur_zone->z2, alpha);
      recalculate_zone_vectors(render_zone);
    }

    if (zone_event_id_is_open_progress(prev_zone->event_id) || zone_event_id_is_open_progress(cur_zone->event_id)) {
      uint16_t prev_open = (uint16_t)prev_zone->event_id;
      uint16_t cur_open = (uint16_t)cur_zone->event_id;
      int32_t open = round_float_to_int32((float)prev_open + (((float)cur_open - (float)prev_open) * alpha));

      if (open < 0) open = 0;
      if (open > 32768) open = 32768;
      render_zone->event_id = (int16_t)(uint16_t)open;
    }
  }
}

static AppState interpolate_render_state(const AppState *previous, const AppState *current, float alpha,
                                         GloomZone *render_zones, size_t render_zone_count) {
  AppState render_state;
  float dx = 0.0f;
  float dz = 0.0f;
  float dy = 0.0f;

  if (current == NULL) {
    memset(&render_state, 0, sizeof(render_state));
    return render_state;
  }

  render_state = *current;
  if (previous == NULL) {
    return render_state;
  }

  alpha = clampf(alpha, 0.0f, 1.0f);
  interpolate_render_zones(previous, current, alpha, render_zones, render_zone_count);
  if (render_zones != NULL && render_zone_count == current->map.zone_count) {
    render_state.map.zones = render_zones;
  }

  dx = current->camera_x - previous->camera_x;
  dz = current->camera_z - previous->camera_z;
  dy = current->camera_y - previous->camera_y;

  if (dx < 0.0f) dx = -dx;
  if (dz < 0.0f) dz = -dz;
  if (dy < 0.0f) dy = -dy;

  if (!previous->teleport_active && !current->teleport_active && dx < 512.0f && dz < 512.0f && dy < 64.0f) {
    render_state.camera_x = previous->camera_x + ((current->camera_x - previous->camera_x) * alpha);
    render_state.camera_z = previous->camera_z + ((current->camera_z - previous->camera_z) * alpha);
    render_state.camera_y = previous->camera_y + ((current->camera_y - previous->camera_y) * alpha);
  }

  {
    size_t i = 0u;

    for (i = 0u; i < GLOOM_MAX_RUNTIME_PROJECTILES; ++i) {
      if (previous->projectiles[i].active && current->projectiles[i].active &&
          previous->projectiles[i].weapon_index == current->projectiles[i].weapon_index) {
        render_state.projectiles[i].x =
            previous->projectiles[i].x + ((current->projectiles[i].x - previous->projectiles[i].x) * alpha);
        render_state.projectiles[i].y =
            previous->projectiles[i].y + ((current->projectiles[i].y - previous->projectiles[i].y) * alpha);
        render_state.projectiles[i].z =
            previous->projectiles[i].z + ((current->projectiles[i].z - previous->projectiles[i].z) * alpha);
      }
    }

    for (i = 0u; i < GLOOM_MAX_RUNTIME_SPARKS; ++i) {
      if (previous->sparks[i].active && current->sparks[i].active &&
          previous->sparks[i].weapon_index == current->sparks[i].weapon_index &&
          previous->sparks[i].frame_index == current->sparks[i].frame_index) {
        render_state.sparks[i].x = previous->sparks[i].x + ((current->sparks[i].x - previous->sparks[i].x) * alpha);
        render_state.sparks[i].y = previous->sparks[i].y + ((current->sparks[i].y - previous->sparks[i].y) * alpha);
        render_state.sparks[i].z = previous->sparks[i].z + ((current->sparks[i].z - previous->sparks[i].z) * alpha);
      }
    }

    for (i = 0u; i < GLOOM_MAX_RUNTIME_BLOOD; ++i) {
      if (previous->blood[i].active && current->blood[i].active &&
          previous->blood[i].color_mask == current->blood[i].color_mask) {
        render_state.blood[i].x = previous->blood[i].x + ((current->blood[i].x - previous->blood[i].x) * alpha);
        render_state.blood[i].y = previous->blood[i].y + ((current->blood[i].y - previous->blood[i].y) * alpha);
        render_state.blood[i].z = previous->blood[i].z + ((current->blood[i].z - previous->blood[i].z) * alpha);
      }
    }

    for (i = 0u; i < GLOOM_MAX_RUNTIME_CHUNKS; ++i) {
      if (previous->chunks[i].active && current->chunks[i].active &&
          previous->chunks[i].object_type == current->chunks[i].object_type &&
          previous->chunks[i].frame_number == current->chunks[i].frame_number) {
        render_state.chunks[i].x = previous->chunks[i].x + ((current->chunks[i].x - previous->chunks[i].x) * alpha);
        render_state.chunks[i].y = previous->chunks[i].y + ((current->chunks[i].y - previous->chunks[i].y) * alpha);
        render_state.chunks[i].z = previous->chunks[i].z + ((current->chunks[i].z - previous->chunks[i].z) * alpha);
      }
    }

    for (i = 0u; i < GLOOM_MAX_RUNTIME_OBJECTS; ++i) {
      if (previous->objects[i].active && current->objects[i].active) {
        render_state.objects[i].x =
            previous->objects[i].x + ((current->objects[i].x - previous->objects[i].x) * alpha);
        render_state.objects[i].y =
            previous->objects[i].y + ((current->objects[i].y - previous->objects[i].y) * alpha);
        render_state.objects[i].z =
            previous->objects[i].z + ((current->objects[i].z - previous->objects[i].z) * alpha);
      }
    }
  }

  render_state.player_rot_fixed = current->player_rot_fixed;
  render_state.player_frame_fixed = current->player_frame_fixed;
  render_state.camera_angle = current->camera_angle;
  if (previous->two_player_mode && current->two_player_mode) {
    render_state.player2.camera_x =
        previous->player2.camera_x + ((current->player2.camera_x - previous->player2.camera_x) * alpha);
    render_state.player2.camera_z =
        previous->player2.camera_z + ((current->player2.camera_z - previous->player2.camera_z) * alpha);
    render_state.player2.camera_y =
        previous->player2.camera_y + ((current->player2.camera_y - previous->player2.camera_y) * alpha);
    render_state.player2.player_bounce =
        previous->player2.player_bounce + ((current->player2.player_bounce - previous->player2.player_bounce) * alpha);
    render_state.player2.player_rot_fixed = current->player2.player_rot_fixed;
    render_state.player2.player_frame_fixed = current->player2.player_frame_fixed;
    render_state.player2.camera_angle = current->player2.camera_angle;
  }
  return render_state;
}

static bool combat_selftest_expect(bool condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "Combat selftest failed: %s\n", message);
    return false;
  }
  return true;
}

static bool input_selftest_expect(bool condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "Input selftest failed: %s\n", message);
    return false;
  }
  return true;
}

static int run_input_selftest(void) {
  AppState state;
  PlayerControls keyboard_controls;
  PlayerControls gamepad_controls;
  RuntimeControlConfig config;
  uint8_t keyboard_state[SDL_SCANCODE_RCTRL + 1];

  if (!input_selftest_expect(gamepad_axis_with_deadzone(0, GLOOM_GAMEPAD_MOVE_DEADZONE) == 0,
                             "zero axis stays neutral")) {
    return 1;
  }
  if (!input_selftest_expect(gamepad_axis_with_deadzone((Sint16)GLOOM_GAMEPAD_MOVE_DEADZONE,
                                                       GLOOM_GAMEPAD_MOVE_DEADZONE) == 0,
                             "deadzone edge stays neutral")) {
    return 1;
  }
  if (!input_selftest_expect(gamepad_axis_with_deadzone(32767, GLOOM_GAMEPAD_MOVE_DEADZONE) == 32767,
                             "positive axis scales to full range")) {
    return 1;
  }
  if (!input_selftest_expect(gamepad_axis_with_deadzone(-32768, GLOOM_GAMEPAD_MOVE_DEADZONE) == -32767,
                             "negative axis clamps to full range")) {
    return 1;
  }
  if (!input_selftest_expect(gamepad_axis_to_mouse_delta(0) == 0 &&
                                 gamepad_axis_to_mouse_delta(GLOOM_GAMEPAD_LOOK_MIN_DELTA_AXIS - 1) == 0,
                             "look minimum suppresses small axis motion")) {
    return 1;
  }
  if (!input_selftest_expect(gamepad_axis_to_mouse_delta(32767) > 0 &&
                                 gamepad_axis_to_mouse_delta(-32767) < 0,
                             "look axis produces signed mouse deltas")) {
    return 1;
  }

  memset(&keyboard_controls, 0, sizeof(keyboard_controls));
  memset(&gamepad_controls, 0, sizeof(gamepad_controls));
  memset(keyboard_state, 0, sizeof(keyboard_state));
  keyboard_state[SDL_SCANCODE_A] = 1u;
  keyboard_state[SDL_SCANCODE_RIGHT] = 1u;
  keyboard_controls = read_modern_player_controls(keyboard_state, false);
  if (!input_selftest_expect(keyboard_controls.joyx == -1 && keyboard_controls.joys == -1 &&
                                 keyboard_controls.turn == 1,
                             "keyboard controls allow strafe and turn together")) {
    return 1;
  }

  memset(&keyboard_controls, 0, sizeof(keyboard_controls));
  memset(&gamepad_controls, 0, sizeof(gamepad_controls));
  keyboard_controls.joyy = -1;
  gamepad_controls.joyx = 1;
  gamepad_controls.joys = -1;
  gamepad_controls.fire = true;
  merge_player_controls(&keyboard_controls, &gamepad_controls);
  if (!input_selftest_expect(keyboard_controls.joyy == -1 && keyboard_controls.joyx == 1 &&
                                 keyboard_controls.joys == -1 && keyboard_controls.fire,
                             "gamepad controls merge with keyboard controls")) {
    return 1;
  }

  config.player1 = GLOOM_CONTROL_KEYBOARD;
  config.player2 = GLOOM_CONTROL_GAMEPAD_1;
  cycle_control_config(&config, 0u);
  if (!input_selftest_expect(config.player1 == GLOOM_CONTROL_GAMEPAD_1 &&
                                 config.player2 == GLOOM_CONTROL_GAMEPAD_2,
                             "selecting two joysticks assigns player one joystick 1 and player two joystick 2")) {
    return 1;
  }
  cycle_control_config(&config, 1u);
  if (!input_selftest_expect(config.player1 != config.player2, "players cannot select the same controls")) {
    return 1;
  }
  config.player1 = GLOOM_CONTROL_KEYBOARD;
  config.player2 = GLOOM_CONTROL_KEYBOARD;
  normalize_control_config(&config);
  if (!input_selftest_expect(config.player1 != config.player2, "duplicate keyboard selection is normalized")) {
    return 1;
  }

  g_webrtc_guest_connected = true;
  g_webrtc_guest_player2_input = GLOOM_WEBRTC_INPUT_FORWARD | GLOOM_WEBRTC_INPUT_STRAFE_RIGHT | GLOOM_WEBRTC_INPUT_FIRE;
  gamepad_controls = read_webrtc_player2_controls();
  if (!input_selftest_expect(gamepad_controls.joyy == -1 && gamepad_controls.joyx == 1 &&
                                 gamepad_controls.joys == -1 && gamepad_controls.fire,
                             "WebRTC guest input maps to player two controls")) {
    return 1;
  }
  g_webrtc_guest_connected = false;
  g_webrtc_guest_player2_input = 0u;

  memset(&state, 0, sizeof(state));
  memset(&keyboard_controls, 0, sizeof(keyboard_controls));
  keyboard_controls.joyy = -1;
  state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.player_lives = GLOOM_PLAYER_INITIAL_LIVES;
  state.player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  update_player_movement(&state, &keyboard_controls);
  if (!input_selftest_expect(state.player_frame_fixed > 0u,
                             "moving player advances drawshape_8 player frame from moveplayer")) {
    return 1;
  }
  state.player_bounce = 126.0f;
  settle_player_bounce(&state, false);
  if (!input_selftest_expect(state.player_bounce == 0.0f && state.player_frame_fixed == 0u,
                             "unbounce clears player frame when movement settles")) {
    return 1;
  }

  return 0;
}

static size_t combat_selftest_active_projectile_count(const AppState *state) {
  size_t count = 0u;
  size_t i = 0u;

  if (state == NULL) {
    return 0u;
  }

  for (i = 0u; i < (size_t)GLOOM_MAX_RUNTIME_PROJECTILES; ++i) {
    if (state->projectiles[i].active) {
      count += 1u;
    }
  }
  return count;
}

static const RuntimeProjectile *combat_selftest_last_projectile(const AppState *state) {
  size_t i = 0u;

  if (state == NULL) {
    return NULL;
  }

  for (i = (size_t)GLOOM_MAX_RUNTIME_PROJECTILES; i > 0u; --i) {
    const RuntimeProjectile *projectile = &state->projectiles[i - 1u];

    if (projectile->active) {
      return projectile;
    }
  }
  return NULL;
}

static int run_combat_selftest(void) {
  const WeaponDefinition *weapons = weapon_definitions();
  const ObjectCombatDefinition *objects = object_combat_definitions();
  const int16_t expected_weapon_hitpoints[GLOOM_WEAPON_COUNT] = {1, 5, 10, 15, 20};
  const int16_t expected_weapon_damage[GLOOM_WEAPON_COUNT] = {1, 2, 2, 3, 5};
  const int16_t expected_weapon_speed[GLOOM_WEAPON_COUNT] = {32, 36, 40, 40, 24};
  AppState state;
  RuntimeObjectState *deathhead = NULL;
  const ObjectCombatDefinition *deathhead_combat = &objects[GLOOM_OBJECT_TYPE_DEATHHEAD];
  size_t i = 0u;
  size_t soul_count = 0u;
  RuntimeObjectState test_object;
  RuntimeProjectile test_projectile;
  const RuntimeProjectile *projectile = NULL;

  memset(&state, 0, sizeof(state));

  for (i = 0u; i < (size_t)GLOOM_WEAPON_COUNT; ++i) {
    char message[128];

    (void)snprintf(message, sizeof(message), "wtable weapon %zu hitpoints", i + 1u);
    if (!combat_selftest_expect(weapons[i].hitpoints == expected_weapon_hitpoints[i], message)) return 1;
    (void)snprintf(message, sizeof(message), "wtable weapon %zu damage", i + 1u);
    if (!combat_selftest_expect(weapons[i].damage == expected_weapon_damage[i], message)) return 1;
    (void)snprintf(message, sizeof(message), "wtable weapon %zu speed", i + 1u);
    if (!combat_selftest_expect(weapons[i].speed == expected_weapon_speed[i], message)) return 1;
  }

  if (!combat_selftest_expect(objects[GLOOM_OBJECT_TYPE_DRAGON].hitpoints == 250 &&
                                  objects[GLOOM_OBJECT_TYPE_DRAGON].damage == 10,
                              "objinfo dragon hp/damage")) {
    return 1;
  }
  if (!combat_selftest_expect(objects[GLOOM_OBJECT_TYPE_TERRA].hitpoints == 35 &&
                                  objects[GLOOM_OBJECT_TYPE_TERRA].damage == 1 &&
                                  objects[GLOOM_OBJECT_TYPE_TERRA].death_routine == GLOOM_OBJECT_DIE_BLOWTERRA,
                              "objinfo terra hp/damage/death routine")) {
    return 1;
  }
  if (!combat_selftest_expect(objects[GLOOM_OBJECT_TYPE_DEATHHEAD].hitpoints == 35 &&
                                  objects[GLOOM_OBJECT_TYPE_DEATHHEAD].damage == 3 &&
                                  objects[GLOOM_OBJECT_TYPE_DEATHHEAD].death_routine == GLOOM_OBJECT_DIE_BLOWDEATH,
                              "objinfo deathhead hp/damage/death routine")) {
    return 1;
  }
  if (!combat_selftest_expect(demon_wtable_scaled_damage(0u) == 0 && demon_wtable_scaled_damage(1u) == 1 &&
                                  demon_wtable_scaled_damage(2u) == 1 && demon_wtable_scaled_damage(3u) == 2 &&
                                  demon_wtable_scaled_damage(4u) == 3,
                              "demonpause 3/4 wtable damage scaling")) {
    return 1;
  }

  memset(&state, 0, sizeof(state));
  memset(&state, 0, sizeof(state));
  state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  memset(&test_object, 0, sizeof(test_object));
  test_object.damage = 3;
  damage_player_from_object(&state, &test_object);
  if (!combat_selftest_expect(state.player_hitpoints == 22 &&
                                  (int)state.player_palette_timer == GLOOM_PLAYER_RED_PAL_TIMER,
                              "playerhit redpal timer")) {
    return 1;
  }
  {
    uint32_t pixel = 0xFF102030u;
    RenderFramebuffer framebuffer;

    memset(&framebuffer, 0, sizeof(framebuffer));
    framebuffer.pixels = &pixel;
    framebuffer.width = 1;
    framebuffer.height = 1;
    framebuffer.pitch_pixels = 1;
    apply_player_red_palette(&framebuffer, &state);
    if (!combat_selftest_expect(pixel == 0xFF7A0000u, "palettesr red flash transform")) {
      return 1;
    }
  }

  memset(&state, 0, sizeof(state));
  memset(&test_object, 0, sizeof(test_object));
  state.two_player_mode = true;
  state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.camera_x = 512.0f;
  state.camera_z = 512.0f;
  state.player_radius = 33;
  state.player2.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.player2.camera_x = 32.0f;
  state.player2.camera_z = 0.0f;
  test_object.radius = 33;
  test_object.damage = 3;
  if (!combat_selftest_expect(runtime_angle_to_player(&state, &test_object) ==
                                  radians_to_amiga_rotation(
                                      SDL_atan2f(state.player2.camera_x - test_object.x,
                                                 state.player2.camera_z - test_object.z)),
                              "pickplayer two-player targets nearest live player")) {
    return 1;
  }
  damage_player_from_object(&state, &test_object);
  if (!combat_selftest_expect(state.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH &&
                                  state.player2.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH - 3 &&
                                  (int)state.player2.player_palette_timer == GLOOM_PLAYER_RED_PAL_TIMER,
                              "two-player enemy contact damages player two")) {
    return 1;
  }
  memset(&test_projectile, 0, sizeof(test_projectile));
  test_projectile.weapon_index = 0;
  test_projectile.enemy = true;
  test_projectile.x = state.player2.camera_x;
  test_projectile.z = state.player2.camera_z;
  test_projectile.damage = 3;
  state.player2.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.player2.player_palette_timer = 0.0f;
  if (!combat_selftest_expect(damage_projectile_player(&state, &test_projectile, 16.0f, 0.0f) &&
                                  state.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH &&
                                  state.player2.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH - 3 &&
                                  (int)state.player2.player_palette_timer == GLOOM_PLAYER_RED_PAL_TIMER,
                              "two-player enemy projectile damages player two")) {
    return 1;
  }
  {
    GloomObjectSpawn contact_spawns[1];

    memset(&state, 0, sizeof(state));
    memset(contact_spawns, 0, sizeof(contact_spawns));
    contact_spawns[0].object_type = GLOOM_OBJECT_TYPE_DRAGON;
    state.map.object_spawns = contact_spawns;
    state.map.object_spawn_count = 1u;
    state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
    state.player_radius = 33;
    state.objects[0].active = true;
    state.objects[0].enemy = true;
    state.objects[0].radius = 33;
    state.objects[0].damage = objects[GLOOM_OBJECT_TYPE_DRAGON].damage;
    state.objects[0].hitpoints = objects[GLOOM_OBJECT_TYPE_DRAGON].hitpoints;

    runtime_update_object_player_contacts(&state, NULL);
    if (!combat_selftest_expect(state.player_hitpoints == 15 && state.objects[0].hitpoints == 249 &&
                                    state.objects[0].contact_was_hit_player == 1u,
                                "obj_loop contact should damage player and object once on first overlap")) {
      return 1;
    }

    runtime_update_object_player_contacts(&state, NULL);
    if (!combat_selftest_expect(state.player_hitpoints == 15 && state.objects[0].hitpoints == 249,
                                "ob_washit should suppress repeated same-pair contact damage")) {
      return 1;
    }

    state.camera_x = 200.0f;
    runtime_update_object_player_contacts(&state, NULL);
    if (!combat_selftest_expect(state.objects[0].contact_was_hit_player == 0u,
                                "ob_washit should re-arm after the player separates")) {
      return 1;
    }

    state.camera_x = 0.0f;
    runtime_update_object_player_contacts(&state, NULL);
    if (!combat_selftest_expect(state.player_hitpoints == 5 && state.objects[0].hitpoints == 248,
                                "re-entering contact should damage again after separation")) {
      return 1;
    }

    memset(&state, 0, sizeof(state));
    memset(contact_spawns, 0, sizeof(contact_spawns));
    contact_spawns[0].object_type = GLOOM_OBJECT_TYPE_DRAGON;
    state.map.object_spawns = contact_spawns;
    state.map.object_spawn_count = 1u;
    state.two_player_mode = true;
    state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
    state.player_radius = 33;
    state.camera_x = 200.0f;
    state.player2.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
    state.objects[0].active = true;
    state.objects[0].enemy = true;
    state.objects[0].radius = 33;
    state.objects[0].damage = objects[GLOOM_OBJECT_TYPE_DRAGON].damage;
    state.objects[0].hitpoints = objects[GLOOM_OBJECT_TYPE_DRAGON].hitpoints;

    runtime_update_object_player_contacts(&state, NULL);
    if (!combat_selftest_expect(state.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH &&
                                    state.player2.player_hitpoints == 15 &&
                                    state.objects[0].hitpoints == 249 &&
                                    state.objects[0].contact_was_hit_player == 2u,
                                "obj_loop contact should damage player two when player one is not overlapping")) {
      return 1;
    }
  }
  memset(&state, 0, sizeof(state));
  memset(&test_projectile, 0, sizeof(test_projectile));
  state.two_player_mode = true;
  state.combat_mode = true;
  state.player_radius = 33;
  state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.player2.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.player2.camera_x = 32.0f;
  test_projectile.weapon_index = 0;
  test_projectile.player_owner_index = 0u;
  test_projectile.x = state.player2.camera_x;
  test_projectile.z = state.player2.camera_z;
  test_projectile.damage = 5;
  if (!combat_selftest_expect(damage_projectile_combat_player(&state, &test_projectile, 16.0f, 0.0f) &&
                                  state.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH &&
                                  state.player2.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH - 5,
                              "combat player projectile damages opposing player")) {
    return 1;
  }
  memset(&state, 0, sizeof(state));
  state.two_player_mode = true;
  state.combat_mode = true;
  state.active_player_index = 0u;
  state.player_hitpoints = 0;
  state.player_lives = 1;
  state.player_dead = true;
  state.player_death_phase = GLOOM_PLAYER_DEATH_FALLING;
  state.player_eye_y = (float)GLOOM_PLAYER_DEATH_EYE_CLAMP;
  advance_player_death_amiga_tick(&state);
  for (i = 0; i < GLOOM_PLAYER_DEAD_DELAY + 1; ++i) {
    advance_player_death_amiga_tick(&state);
  }
  if (!combat_selftest_expect(state.player_lives == 0 && state.finished == GLOOM_COMBAT_ROUND_FINISHED &&
                                  state.finished2 == GLOOM_COMBAT_ROUND_FINISHED,
                              "combat playerdead sets finished=4 after loser life reaches zero")) {
    return 1;
  }

  memset(&state, 0, sizeof(state));
  state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.camera_x = 1024.0f;
  state.camera_z = 1024.0f;
  state.rng_state = 0x47524f4fu;
  deathhead = &state.objects[7];
  deathhead->active = true;
  deathhead->enemy = true;
  deathhead->x = 1152.0f;
  deathhead->z = 1024.0f;
  deathhead->rotation = 64;
  deathhead->frame_fixed = deathhead_combat->initial_frame_fixed;
  deathhead->frame_speed_fixed = (int32_t)deathhead_combat->frame_speed_fixed;
  deathhead->move_speed = deathhead_combat->move_speed;

  runtime_apply_object_hit(&state, deathhead, deathhead_combat, GLOOM_OBJECT_TYPE_DEATHHEAD, 7u);
  if (!combat_selftest_expect(state.death_suck_active && state.death_sucker_index == 7u &&
                                  deathhead->logic_state == GLOOM_OBJECT_LOGIC_DEATHSUCK &&
                                  (int)deathhead->delay == 64,
                              "hurtdeath starts deathsuck state")) {
    return 1;
  }

  update_deathhead_object(&state, deathhead, deathhead_combat, 1.0f);
  for (i = 0u; i < (size_t)GLOOM_MAX_RUNTIME_BLOOD; ++i) {
    if (state.blood[i].active && state.blood[i].soul) {
      soul_count += 1u;
      if (!combat_selftest_expect(state.blood[i].color_mask == 0x00ffu || state.blood[i].color_mask == 0x00f0u,
                                  "addsoul color mask")) {
        return 1;
      }
    }
  }
  if (!combat_selftest_expect(soul_count == 4u, "deathsuck adds four soul particles per tick")) return 1;

  deathhead->delay = 0.0f;
  update_deathhead_object(&state, deathhead, deathhead_combat, 1.0f);
  if (!combat_selftest_expect(!state.death_suck_active && deathhead->logic_state == GLOOM_OBJECT_LOGIC_NORMAL,
                              "deathsuck restores old logic and clears global suck state")) {
    return 1;
  }

  memset(&state.projectiles, 0, sizeof(state.projectiles));
  state.camera_x = 128.0f;
  state.camera_z = 512.0f;
  memset(&test_object, 0, sizeof(test_object));
  test_object.active = true;
  test_object.enemy = true;
  test_object.x = 128.0f;
  test_object.z = 256.0f;
  test_object.rotation = 0;
  test_object.logic_state = GLOOM_OBJECT_LOGIC_TERRAFIRE;
  test_object.delay = 0.0f;
  test_object.delay2 = 5.0f;
  update_terra_object(&state, &test_object, &objects[GLOOM_OBJECT_TYPE_TERRA], 1.0f);
  projectile = combat_selftest_last_projectile(&state);
  if (!combat_selftest_expect(combat_selftest_active_projectile_count(&state) == 1u && projectile != NULL &&
                                  projectile->weapon_index == 3u && projectile->hitpoints == 1 &&
                                  projectile->damage == 3 && (int)projectile->vx == 0 &&
                                  (int)projectile->vz == 16,
                              "terralogic2 fires one bullet4 with hits=1 damage=3 speed=16")) {
    return 1;
  }

  memset(&state.projectiles, 0, sizeof(state.projectiles));
  state.camera_x = 128.0f;
  state.camera_z = 512.0f;
  memset(&test_object, 0, sizeof(test_object));
  test_object.active = true;
  test_object.enemy = true;
  test_object.x = 128.0f;
  test_object.z = 256.0f;
  test_object.rotation = 0;
  test_object.delay = 0.0f;
  update_phantom_object(&state, &test_object, &objects[GLOOM_OBJECT_TYPE_PHANTOM], 1.0f);
  projectile = combat_selftest_last_projectile(&state);
  if (!combat_selftest_expect(combat_selftest_active_projectile_count(&state) == 1u && projectile != NULL &&
                                  projectile->weapon_index == 2u && projectile->hitpoints == 1 &&
                                  projectile->damage == 3 && (int)projectile->vx == 0 &&
                                  (int)projectile->vz == 20 && (int)test_object.pause_delay == 7 &&
                                  (test_object.frame_fixed >> 16u) == 5u,
                              "phantomlogic fires bullet3 and enters 7-tick pause frame 5")) {
    return 1;
  }

  memset(&state.projectiles, 0, sizeof(state.projectiles));
  state.camera_x = 128.0f;
  state.camera_z = 512.0f;
  memset(&test_object, 0, sizeof(test_object));
  test_object.active = true;
  test_object.enemy = true;
  test_object.x = 128.0f;
  test_object.z = 256.0f;
  test_object.rotation = 0;
  test_object.logic_state = GLOOM_OBJECT_LOGIC_DEMONPAUSE;
  test_object.delay = 39.0f;
  test_object.delay2 = -1.0f;
  update_demon_object(&state, &test_object, &objects[GLOOM_OBJECT_TYPE_DEMON], 1.0f);
  projectile = combat_selftest_last_projectile(&state);
  if (!combat_selftest_expect(combat_selftest_active_projectile_count(&state) == 1u && projectile != NULL &&
                                  projectile->weapon_index == 4u && projectile->hitpoints == 20 &&
                                  projectile->damage == 3 && (int)projectile->vx == 0 &&
                                  (int)projectile->vz == 24,
                              "demonpause first shot uses wtable bullet5 with 3/4 damage")) {
    return 1;
  }

  memset(&state.projectiles, 0, sizeof(state.projectiles));
  state.camera_x = 128.0f;
  state.camera_z = 512.0f;
  memset(&test_object, 0, sizeof(test_object));
  test_object.active = true;
  test_object.enemy = true;
  test_object.x = 128.0f;
  test_object.z = 256.0f;
  test_object.rotation = 0;
  test_object.delay = -7.0f;
  test_object.frame_speed_fixed = (int32_t)objects[GLOOM_OBJECT_TYPE_DRAGON].frame_speed_fixed;
  test_object.move_speed = objects[GLOOM_OBJECT_TYPE_DRAGON].move_speed;
  update_dragon_object(&state, &test_object, &objects[GLOOM_OBJECT_TYPE_DRAGON], 1.0f);
  projectile = combat_selftest_last_projectile(&state);
  if (!combat_selftest_expect(combat_selftest_active_projectile_count(&state) == 1u && projectile != NULL &&
                                  projectile->weapon_index == 4u && projectile->hitpoints == 1 &&
                                  projectile->damage == 3 && (int)projectile->vx == 0 &&
                                  (int)projectile->vz == 16 && projectile->homing,
                              "dragonfire emits homing bullet5 with hits=1 damage=3 speed=16")) {
    return 1;
  }

  printf("Combat selftest passed\n");
  return 0;
}

static int run_player_death_selftest(void) {
  AppState state;
  RuntimeObjectState test_object;
  PlayerControls controls;
  int i = 0;

  memset(&state, 0, sizeof(state));
  memset(&test_object, 0, sizeof(test_object));
  memset(&controls, 0, sizeof(controls));

  state.player_hitpoints = 1;
  state.player_lives = GLOOM_PLAYER_INITIAL_LIVES;
  state.player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  state.player_respawn_x = 100;
  state.player_respawn_z = 200;
  state.player_respawn_rotation = 64;
  state.player_weapon = 3u;
  state.player_reload = 2u;
  state.player_rot_fixed = amiga_rotation_to_fixed(0);
  state.camera_angle = player_rotation_fixed_to_radians(state.player_rot_fixed);
  test_object.damage = 2;

  damage_player_from_object(&state, &test_object);
  if (!combat_selftest_expect(state.player_hitpoints == 0 && state.player_dead &&
                                  state.player_death_phase == GLOOM_PLAYER_DEATH_FALLING &&
                                  state.player_lives == GLOOM_PLAYER_INITIAL_LIVES &&
                                  (int)state.player_palette_timer == GLOOM_PLAYER_RED_PAL_TIMER,
                              "playerdie should start redpal/playerdeath without consuming a life immediately")) {
    return 1;
  }

  for (i = 0; i < 20; ++i) {
    advance_player_death_amiga_tick(&state);
  }
  if (!combat_selftest_expect(state.player_death_phase == GLOOM_PLAYER_DEATH_DEAD_DELAY &&
                                  state.player_eye_y == (float)GLOOM_PLAYER_DEATH_EYE_CLAMP &&
                                  state.player_lives == GLOOM_PLAYER_INITIAL_LIVES - 1 &&
                                  state.player_death_timer == (float)GLOOM_PLAYER_DEAD_DELAY,
                              "playerdeath should spin/fall to -32 before losing exactly one life")) {
    return 1;
  }

  for (i = 0; i < GLOOM_PLAYER_DEAD_DELAY; ++i) {
    advance_player_death_amiga_tick(&state);
  }
  if (!combat_selftest_expect(state.player_death_phase == GLOOM_PLAYER_DEATH_WAIT_RESTART,
                              "playerdead should wait 63 ticks before waitrestart")) {
    return 1;
  }

  state.player_last_fire = true;
  controls.fire = true;
  update_player_death(&state, &controls);
  if (!combat_selftest_expect(state.player_death_phase == GLOOM_PLAYER_DEATH_WAIT_RESTART,
                              "waitrestart should require a fresh fire press")) {
    return 1;
  }

  controls.fire = false;
  update_player_death(&state, &controls);
  controls.fire = true;
  update_player_death(&state, &controls);
  if (!combat_selftest_expect(!state.player_dead && state.player_death_phase == GLOOM_PLAYER_DEATH_INVINCIBLE &&
                                  state.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH &&
                                  state.player_lives == GLOOM_PLAYER_INITIAL_LIVES - 1 &&
                                  state.player_weapon == 3u && state.camera_x == 100.0f && state.camera_z == 200.0f &&
                                  state.player_rot_fixed == amiga_rotation_to_fixed(64),
                              "waitrestart should respawn at p1x/p1z/prot, preserve weapon, and enter playerlogic0")) {
    return 1;
  }

  damage_player_from_object(&state, &test_object);
  if (!combat_selftest_expect(state.player_hitpoints == GLOOM_PLAYER_INITIAL_HEALTH &&
                                  state.player_death_phase == GLOOM_PLAYER_DEATH_INVINCIBLE,
                              "playerlogic0 invincibility should ignore damage for 75 ticks")) {
    return 1;
  }

  for (i = 0; i < GLOOM_PLAYER_RESTART_INVINCIBLE_TICKS; ++i) {
    advance_player_death_amiga_tick(&state);
  }
  if (!combat_selftest_expect(state.player_death_phase == GLOOM_PLAYER_DEATH_NONE,
                              "playerlogic0 should restore normal collision after 75 ticks")) {
    return 1;
  }

  state.player_hitpoints = 2;
  damage_player_squash(&state);
  if (!combat_selftest_expect(state.player_hitpoints == 1 &&
                                  state.player_death_phase == GLOOM_PLAYER_DEATH_NONE &&
                                  (int)state.player_palette_timer == GLOOM_PLAYER_RED_PAL_TIMER,
                              "playerlogic unresolved adjustpos should injure without killing above zero HP")) {
    return 1;
  }
  damage_player_squash(&state);
  if (!combat_selftest_expect(state.player_hitpoints == 0 &&
                                  state.player_death_phase == GLOOM_PLAYER_DEATH_FALLING,
                              "playerlogic unresolved adjustpos should enter playerdie at zero HP")) {
    return 1;
  }

  state.player_hitpoints = 1;
  state.player_lives = 1;
  state.player_dead = false;
  state.player_death_phase = GLOOM_PLAYER_DEATH_NONE;
  state.player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  state.finished = 0;
  damage_player_from_object(&state, &test_object);
  for (i = 0; i < 20 + GLOOM_PLAYER_DEAD_DELAY; ++i) {
    advance_player_death_amiga_tick(&state);
  }
  if (!combat_selftest_expect(state.player_lives == 0 && state.player_death_phase == GLOOM_PLAYER_DEATH_GAME_OVER &&
                                  state.finished == 2,
                              "playerdead should set finished=2 when the final one-player life is gone")) {
    return 1;
  }

  memset(&state, 0, sizeof(state));
  state.two_player_mode = true;
  state.active_player_index = 0u;
  state.active_other_player_lives = 3;
  state.player_hitpoints = 1;
  state.player_lives = 3;
  state.player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  state.player2.player_lives = 3;
  state.player2.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  damage_player_from_object(&state, &test_object);
  for (i = 0; i < 20; ++i) {
    advance_player_death_amiga_tick(&state);
  }
  if (!combat_selftest_expect(state.player_lives == 2 && state.player2.player_lives == 2 &&
                                  state.player_death_phase == GLOOM_PLAYER_DEATH_DEAD_DELAY,
                              "two-player playerdeath should mirror nonzero shared lives to the other player")) {
    return 1;
  }

  memset(&state, 0, sizeof(state));
  state.two_player_mode = true;
  state.active_player_index = 0u;
  state.active_other_player_lives = 1;
  state.player_hitpoints = 1;
  state.player_lives = 1;
  state.player_eye_y = (float)GLOOM_PLAYER_EYE_Y;
  state.player2.player_lives = 1;
  damage_player_from_object(&state, &test_object);
  for (i = 0; i < 20 + GLOOM_PLAYER_DEAD_DELAY; ++i) {
    advance_player_death_amiga_tick(&state);
  }
  if (!combat_selftest_expect(state.player_lives == 0 && state.player_death_phase == GLOOM_PLAYER_DEATH_OUT_OF_LIVES &&
                                  state.finished == 0,
                              "two-player playerdead should leave one player out when the other still has lives")) {
    return 1;
  }

  memset(&state, 0, sizeof(state));
  state.map.object_spawns = (GloomObjectSpawn *)calloc(2u, sizeof(*state.map.object_spawns));
  if (state.map.object_spawns == NULL) {
    fprintf(stderr, "Combat selftest failed: out of memory preparing player spawn fixture\n");
    return 1;
  }
  state.map.object_spawn_count = 2u;
  state.map.object_spawns[0].object_type = 0;
  state.map.object_spawns[1].object_type = 1;
  initialize_runtime_objects(&state);
  if (!combat_selftest_expect(!state.objects[0].active && !state.objects[1].active,
                              "single-player runtime object list should not expose player spawns as sprites")) {
    free(state.map.object_spawns);
    return 1;
  }
  free(state.map.object_spawns);

  printf("Player death selftest passed\n");
  return 0;
}

static int run_sfx_selftest(void) {
  AudioSystem audio;
  size_t i = 0u;

  memset(&audio, 0, sizeof(audio));
  if (!audio_load_sfx_bank(&audio)) {
    return 1;
  }

  for (i = 0u; i < (size_t)GLOOM_SFX_COUNT; ++i) {
    const SfxSample *sample = &audio.samples[i];

    if (!sample->loaded || sample->period == 0u || sample->length_words == 0u ||
        sample->sample_count != (uint32_t)sample->length_words * 2u || sample->sample_rate <= 0.0) {
      fprintf(stderr, "SFX selftest failed: invalid decoded SFX index %zu\n", i);
      audio_free_sfx_bank(&audio);
      return 1;
    }
  }

  audio.obtained.freq = 48000;
  audio_start_channel_locked(&audio, &audio.channels[0], GLOOM_SFX_SHOOT, 32, 0);
  if (!audio.channels[0].active || audio.channels[0].busy_passes != 1u) {
    fprintf(stderr, "SFX selftest failed: playsfxnow channel did not start a non-looping one-shot\n");
    audio_free_sfx_bank(&audio);
    return 1;
  }

  {
    AudioSystem repeat_audio;
    int8_t fake_samples[2] = {64, 64};
    float stream[8];

    memset(&repeat_audio, 0, sizeof(repeat_audio));
    memset(stream, 0, sizeof(stream));
    repeat_audio.obtained.freq = 1;
    repeat_audio.samples[GLOOM_SFX_SHOOT].loaded = true;
    repeat_audio.samples[GLOOM_SFX_SHOOT].sample_count = 2u;
    repeat_audio.samples[GLOOM_SFX_SHOOT].sample_rate = 1.0;
    repeat_audio.samples[GLOOM_SFX_SHOOT].samples = fake_samples;
    audio_start_channel_locked(&repeat_audio, &repeat_audio.channels[0], GLOOM_SFX_SHOOT, 64, 0);
    audio_callback(&repeat_audio, (Uint8 *)stream, (int)sizeof(stream));
    if (stream[0] <= 0.0f || stream[2] <= 0.0f || stream[4] != 0.0f || stream[6] != 0.0f ||
        repeat_audio.channels[0].active) {
      fprintf(stderr, "SFX selftest failed: one-shot SFX looped or held the channel after completion\n");
      audio_free_sfx_bank(&audio);
      return 1;
    }
  }

  {
    AudioSystem busy_audio;
    int8_t fake_samples[2] = {64, 64};
    float stream[8];
    int c = 0;

    memset(&busy_audio, 0, sizeof(busy_audio));
    memset(stream, 0, sizeof(stream));
    busy_audio.obtained.freq = 1;
    busy_audio.samples[GLOOM_SFX_SHOOT].loaded = true;
    busy_audio.samples[GLOOM_SFX_SHOOT].sample_count = 2u;
    busy_audio.samples[GLOOM_SFX_SHOOT].sample_rate = 1.0;
    busy_audio.samples[GLOOM_SFX_SHOOT].samples = fake_samples;
    for (c = 0; c < GLOOM_AUDIO_CHANNEL_COUNT; ++c) {
      audio_start_channel_locked(&busy_audio, &busy_audio.channels[c], GLOOM_SFX_SHOOT, 64, 0);
    }
    audio_callback(&busy_audio, (Uint8 *)stream, (int)sizeof(stream));
    for (c = 0; c < GLOOM_AUDIO_CHANNEL_COUNT; ++c) {
      if (busy_audio.channels[c].active) {
        fprintf(stderr, "SFX selftest failed: completed one-shot channel %d stayed busy\n", c);
        audio_free_sfx_bank(&audio);
        return 1;
      }
    }
  }

  {
    AudioSystem ui_audio;
    float stream[8];

    memset(&ui_audio, 0, sizeof(ui_audio));
    memset(stream, 0, sizeof(stream));
    ui_audio.ui_beep_total = 4u;
    ui_audio.ui_beep_remaining = 4u;
    ui_audio.ui_beep_phase = 1.57079632679489661923;
    ui_audio.ui_beep_phase_step = 0.0;
    ui_audio.ui_beep_volume = 0.12f;
    audio_callback(&ui_audio, (Uint8 *)stream, (int)sizeof(stream));
    if (stream[0] <= 0.0f || stream[1] <= 0.0f || ui_audio.ui_beep_remaining != 0u) {
      fprintf(stderr, "SFX selftest failed: UI beep did not mix as a short stereo tone\n");
      audio_free_sfx_bank(&audio);
      return 1;
    }
  }

  audio_free_sfx_bank(&audio);
  printf("SFX selftest passed: loaded %u original Paula samples\n", (unsigned)GLOOM_SFX_COUNT);
  return 0;
}

static int run_pickup_selftest(void) {
  const WeaponDefinition *weapons = weapon_definitions();
  const SfxDefinition *sfx = sfx_definitions();
  AppState state;
  RuntimeObjectState pickup;
  PlayerControls controls;
  size_t i = 0u;
  const RuntimeProjectile *projectile = NULL;

  memset(&state, 0, sizeof(state));
  memset(&pickup, 0, sizeof(pickup));
  memset(&controls, 0, sizeof(controls));
  pickup.active = true;

  if (strcmp(sfx[GLOOM_SFX_TOKEN].symbol, "tokensfx") != 0 ||
      strcmp(sfx[GLOOM_SFX_TOKEN].path, "amiga/sfxs/token.bin") != 0) {
    fprintf(stderr, "Pickup selftest failed: playtsfx is not mapped to original tokensfx asset\n");
    return 1;
  }

  state.player_hitpoints = 21;
  if (!apply_player_pickup(&state, &pickup, 2) || state.player_hitpoints != 25 || pickup.active) {
    fprintf(stderr, "Pickup selftest failed: healthgot did not add 5 and clamp to 25\n");
    return 1;
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  state.player_weapon = 0u;
  state.player_reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
  if (!apply_player_pickup(&state, &pickup, GLOOM_OBJECT_TYPE_WEAPON3) || state.player_weapon != 2u ||
      state.player_reload != (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD || pickup.active) {
    fprintf(stderr, "Pickup selftest failed: weapongot did not switch to original weapon index\n");
    return 1;
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  state.player_weapon = 2u;
  state.player_reload = 1u;
  state.player_mega_timer = 0.0f;
  if (!apply_player_pickup(&state, &pickup, GLOOM_OBJECT_TYPE_WEAPON3) || state.player_reload != 1u ||
      (int)state.player_mega_timer != 250 || pickup.active) {
    fprintf(stderr, "Pickup selftest failed: repeated weapon did not enter mega weapon boost path\n");
    return 1;
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  if (!apply_player_pickup(&state, &pickup, 4) || (int)state.player_thermo_timer != 1500 || pickup.active) {
    fprintf(stderr, "Pickup selftest failed: thermogot did not add 1500 ticks\n");
    return 1;
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  if (!apply_player_pickup(&state, &pickup, 6) || (int)state.player_invisible_timer != 1500 || pickup.active) {
    fprintf(stderr, "Pickup selftest failed: invisigot did not add 1500 ticks\n");
    return 1;
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  state.player_hyper_timer = 0.0f;
  if (!apply_player_pickup(&state, &pickup, 7) || (int)state.player_hyper_timer != -0x200 || pickup.active) {
    fprintf(stderr, "Pickup selftest failed: invincgot did not start hyper timer\n");
    return 1;
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  state.player_bouncy_bullets = 2u;
  if (!apply_player_pickup(&state, &pickup, GLOOM_OBJECT_TYPE_BOUNCY) || state.player_bouncy_bullets != 3u ||
      pickup.active) {
    fprintf(stderr, "Pickup selftest failed: bouncygot did not increment to max 3\n");
    return 1;
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  if (apply_player_pickup(&state, &pickup, GLOOM_OBJECT_TYPE_BOUNCY) || state.player_bouncy_bullets != 3u ||
      !pickup.active) {
    fprintf(stderr, "Pickup selftest failed: bouncygot consumed pickup past original max 3\n");
    return 1;
  }

  {
    static const size_t expected_muzzle_by_weapon[GLOOM_WEAPON_COUNT] = {
        (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME, (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME + 1u,
        (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME + 1u, (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME + 2u,
        (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME + 2u};

    for (i = 0u; i < (size_t)GLOOM_WEAPON_COUNT; ++i) {
      memset(&state, 0, sizeof(state));
      state.player_weapon = (uint8_t)i;
      if (player_weapon_muzzle_frame_index(&state) != expected_muzzle_by_weapon[i]) {
        fprintf(stderr, "Pickup selftest failed: weapon %zu muzzle flash did not match original shot colour\n",
                i + 1u);
        return 1;
      }
    }
    state.player_weapon = 0u;
    state.player_bouncy_bullets = 3u;
    state.player_mega_timer = 250.0f;
    if (player_weapon_muzzle_frame_index(&state) != (size_t)GLOOM_GUN_MUZZLE_FIRST_FRAME) {
      fprintf(stderr, "Pickup selftest failed: powerups changed muzzle colour instead of preserving wtable shot art\n");
      return 1;
    }
  }

  memset(&pickup, 0, sizeof(pickup));
  pickup.active = true;
  state.map.object_spawn_count = 1u;
  state.map.object_spawns = (GloomObjectSpawn *)calloc(1u, sizeof(*state.map.object_spawns));
  if (state.map.object_spawns == NULL) {
    fprintf(stderr, "Pickup selftest failed: out of memory preparing weaponlogic fixture\n");
    return 1;
  }
  state.map.object_spawns[0].object_type = GLOOM_OBJECT_TYPE_WEAPON3;
  state.map.object_spawns[0].y = -32;
  state.objects[0] = pickup;
  state.objects[0].y = -32.0f;
  state.objects[0].radius = 24;
  state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.camera_x = 4096.0f;
  state.camera_z = 4096.0f;
  update_pickups(&state);
  if ((int)state.objects[0].y != -32 || state.objects[0].render_y_offset < -128.0f ||
      state.objects[0].render_y_offset > 0.0f) {
    fprintf(stderr, "Pickup selftest failed: weaponlogic bob changed map-authored pickup y or moved below floor\n");
    free(state.map.object_spawns);
    state.map.object_spawns = NULL;
    state.map.object_spawn_count = 0u;
    return 1;
  }
  free(state.map.object_spawns);
  state.map.object_spawns = NULL;
  state.map.object_spawn_count = 0u;

  memset(&state, 0, sizeof(state));
  memset(&pickup, 0, sizeof(pickup));
  state.map.object_spawn_count = 1u;
  state.map.object_spawns = (GloomObjectSpawn *)calloc(1u, sizeof(*state.map.object_spawns));
  if (state.map.object_spawns == NULL) {
    fprintf(stderr, "Pickup selftest failed: out of memory preparing two-player weaponlogic fixture\n");
    return 1;
  }
  state.map.object_spawns[0].object_type = GLOOM_OBJECT_TYPE_WEAPON3;
  state.objects[0].active = true;
  state.objects[0].radius = 24;
  state.objects[0].x = 0.0f;
  state.objects[0].z = 0.0f;
  state.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.player_lives = GLOOM_PLAYER_INITIAL_LIVES;
  state.player_radius = 16;
  state.camera_x = 4096.0f;
  state.camera_z = 4096.0f;
  state.two_player_mode = true;
  state.player2.player_hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
  state.player2.player_lives = GLOOM_PLAYER_INITIAL_LIVES;
  state.player2.camera_x = 8192.0f;
  state.player2.camera_z = 8192.0f;
  update_with_controls(&state, &controls, &controls, NULL);
  if (SDL_fabsf(state.objects[0].bounce_phase - (8.0f * fixed_step_amiga_scale())) > 0.001f) {
    fprintf(stderr, "Pickup selftest failed: weaponlogic bob advanced more than once in two-player mode\n");
    free(state.map.object_spawns);
    state.map.object_spawns = NULL;
    state.map.object_spawn_count = 0u;
    return 1;
  }
  free(state.map.object_spawns);
  state.map.object_spawns = NULL;
  state.map.object_spawn_count = 0u;

  for (i = 0u; i < (size_t)GLOOM_WEAPON_COUNT; ++i) {
    memset(state.projectiles, 0, sizeof(state.projectiles));
    state.camera_angle = 0.0f;
    state.player_bouncy_bullets = 2u;
    if (!spawn_player_projectile(&state, (uint8_t)i)) {
      fprintf(stderr, "Pickup selftest failed: weapon %zu did not spawn a projectile\n", i + 1u);
      return 1;
    }
    projectile = combat_selftest_last_projectile(&state);
    if (combat_selftest_active_projectile_count(&state) != 1u || projectile == NULL ||
        projectile->weapon_index != (uint8_t)i || projectile->hitpoints != weapons[i].hitpoints ||
        projectile->damage != weapons[i].damage || (int)projectile->vx != 0 ||
        (int)projectile->vz != weapons[i].speed || projectile->bounce_count != 2u) {
      fprintf(stderr, "Pickup selftest failed: weapon %zu projectile type/hits/damage/speed/bounce mismatch\n",
              i + 1u);
      return 1;
    }
  }

  memset(state.projectiles, 0, sizeof(state.projectiles));
  state.camera_angle = 0.0f;
  state.player_weapon = 3u;
  state.player_reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
  state.player_reload_counter = 0.0f;
  state.player_last_fire = false;
  state.player_mega_timer = 250.0f;
  state.player_bouncy_bullets = 0u;
  controls.fire = true;
  update_player_weapon(&state, &controls);
  if (combat_selftest_active_projectile_count(&state) != 2u) {
    fprintf(stderr, "Pickup selftest failed: mega weapon boost did not fire original two-shot spread\n");
    return 1;
  }

  memset(state.projectiles, 0, sizeof(state.projectiles));
  state.player_reload_counter = 0.0f;
  state.player_last_fire = false;
  state.player_mega_timer = (float)GLOOM_PLAYER_MEGA_OVERKILL;
  update_player_weapon(&state, &controls);
  if (combat_selftest_active_projectile_count(&state) != 3u) {
    fprintf(stderr, "Pickup selftest failed: ultra mega overkill did not fire original three-shot spread\n");
    return 1;
  }

  memset(state.projectiles, 0, sizeof(state.projectiles));
  state.player_reload_counter = 0.0f;
  state.player_keyboard_fire_repeat_counter = 0.0f;
  state.player_last_fire = true;
  state.player_mega_timer = 0.0f;
  controls.keyboard_fire_repeat = false;
  update_player_weapon(&state, &controls);
  if (combat_selftest_active_projectile_count(&state) != 0u) {
    fprintf(stderr, "Pickup selftest failed: held non-keyboard fire should keep original edge-trigger behavior\n");
    return 1;
  }
  controls.keyboard_fire_repeat = true;
  state.player_keyboard_fire_repeat_counter = (float)GLOOM_KEYBOARD_HELD_FIRE_REPEAT_TICKS;
  update_player_weapon(&state, &controls);
  if (combat_selftest_active_projectile_count(&state) != 0u) {
    fprintf(stderr, "Pickup selftest failed: held keyboard fire repeated before its repeat gate elapsed\n");
    return 1;
  }
  state.player_keyboard_fire_repeat_counter = 0.0f;
  update_player_weapon(&state, &controls);
  if (combat_selftest_active_projectile_count(&state) != 1u) {
    fprintf(stderr, "Pickup selftest failed: held keyboard fire should repeat after its repeat gate elapses\n");
    return 1;
  }
  controls.keyboard_fire_repeat = false;

  state.player_hyper_timer = -512.0f;
  update_player_power_timers(&state);
  if ((int)state.player_hyper_timer != -513) {
    fprintf(stderr, "Pickup selftest failed: hyper timer is not scaled by Amiga tick rate\n");
    return 1;
  }

  printf("Pickup selftest passed\n");
  return 0;
}

static int run_hud_selftest(void) {
  RenderFramebuffer framebuffer;
  HudGlyph glyph;
  uint32_t glyph_pixels[4] = {0xFFFF0000u, 0x00000000u, 0xFF0000FFu, 0xFF00FF00u};
  uint32_t framebuffer_pixels[100];
  int x = 0;
  int y = 0;

  if (ui_double_scale_for_viewport(320, 224) != 1 || ui_double_scale_for_viewport(640, 480) != 2 ||
      ui_double_scale_for_viewport(1280, 720) != 4 || ui_double_scale_for_viewport(2560, 1440) != 8) {
    fprintf(stderr, "HUD selftest failed: UI scale is not restricted to 1x/2x/4x/8x width doubles\n");
    return 1;
  }

  memset(&framebuffer, 0, sizeof(framebuffer));
  memset(&glyph, 0, sizeof(glyph));
  memset(framebuffer_pixels, 0, sizeof(framebuffer_pixels));
  framebuffer.pixels = framebuffer_pixels;
  framebuffer.width = 10;
  framebuffer.height = 10;
  framebuffer.pitch_pixels = 10;
  glyph.argb_pixels = glyph_pixels;
  glyph.width = 2;
  glyph.height = 2;

  render_hud_glyph_scaled(&framebuffer, &glyph, 1, 1, 4);
  for (y = 1; y < 5; ++y) {
    for (x = 1; x < 5; ++x) {
      if (framebuffer_pixels[(size_t)y * 10u + (size_t)x] != 0xFFFF0000u) {
        fprintf(stderr, "HUD selftest failed: scaled glyph did not duplicate source pixels exactly\n");
        return 1;
      }
    }
    for (x = 5; x < 9; ++x) {
      if (framebuffer_pixels[(size_t)y * 10u + (size_t)x] != 0u) {
        fprintf(stderr, "HUD selftest failed: transparent scaled glyph pixel wrote to framebuffer\n");
        return 1;
      }
    }
  }

  {
    AppState state;
    HudFont hud_font;
    HudGlyph glyphs[GLOOM_HUD_PANEL_CHAR + 1];
    uint32_t life_pixel = 0xFFFF00FFu;
    uint32_t health_pixel = 0xFF00FF00u;
    uint32_t *status_pixels = NULL;
    int slot = 0;

    memset(&state, 0, sizeof(state));
    memset(&hud_font, 0, sizeof(hud_font));
    memset(glyphs, 0, sizeof(glyphs));
    status_pixels = (uint32_t *)calloc((size_t)BASE_WIDTH * (size_t)BASE_HEIGHT, sizeof(*status_pixels));
    if (status_pixels == NULL) {
      fprintf(stderr, "HUD selftest failed: out of memory preparing status framebuffer fixture\n");
      return 1;
    }

    framebuffer.pixels = status_pixels;
    framebuffer.width = BASE_WIDTH;
    framebuffer.height = BASE_HEIGHT;
    framebuffer.pitch_pixels = BASE_WIDTH;
    hud_font.loaded = true;
    hud_font.glyphs = glyphs;
    hud_font.glyph_count = (size_t)GLOOM_HUD_PANEL_CHAR + 1u;
    glyphs[GLOOM_HUD_LIFE_CHAR].argb_pixels = &life_pixel;
    glyphs[GLOOM_HUD_LIFE_CHAR].width = 1;
    glyphs[GLOOM_HUD_LIFE_CHAR].height = 1;
    glyphs[GLOOM_HUD_HEALTH_CHAR].argb_pixels = &health_pixel;
    glyphs[GLOOM_HUD_HEALTH_CHAR].width = 1;
    glyphs[GLOOM_HUD_HEALTH_CHAR].height = 1;
    state.player_lives = 9;
    state.player_hitpoints = 0;
    state.player_reload = 6u;

    render_player_weapon_status(&framebuffer, &state, &hud_font, 0, 0, BASE_WIDTH, BASE_HEIGHT);
    for (slot = 0; slot < 9; ++slot) {
      int expected_x = GLOOM_HUD_LIVES_X + ((GLOOM_HUD_LIVES_RIGHT_ALIGN_COUNT - 9) * GLOOM_HUD_LIVES_SPACING) +
                       (slot * GLOOM_HUD_LIVES_SPACING);
      int expected_y = BASE_HEIGHT - GLOOM_HUD_STATUS_HEIGHT + GLOOM_HUD_LIVES_Y;

      if (status_pixels[(size_t)expected_y * (size_t)BASE_WIDTH + (size_t)expected_x] != life_pixel) {
        fprintf(stderr, "HUD selftest failed: 9 lives were not right-aligned from the Amiga showstats formula\n");
        free(status_pixels);
        return 1;
      }
    }
    free(status_pixels);
  }

  printf("HUD selftest passed\n");
  return 0;
}

static int run_menu_selftest(void) {
  MenuAssets assets;

  memset(&assets, 0, sizeof(assets));
  if (!load_menu_assets(&assets)) {
    fprintf(stderr, "Menu selftest failed: could not load original title/bigfont assets\n");
    return 1;
  }
  if (!assets.title.loaded || assets.title.width <= 0 || assets.title.height <= 0) {
    fprintf(stderr, "Menu selftest failed: title image did not decode\n");
    free_menu_assets(&assets);
    return 1;
  }
  if (!assets.big_font.loaded || assets.big_font.glyph_count <= 39u) {
    fprintf(stderr, "Menu selftest failed: bigfont lacks printmess2 glyph mappings\n");
    free_menu_assets(&assets);
    return 1;
  }
  {
    const int render_width = 320;
    const int render_height = 200;
    const int item_count = 2;
    const int scale = menu_pixel_scale_for_viewport(render_width, render_height);
    const int row_height = GLOOM_MENU_BIG_FONT_HEIGHT * scale;
    const int pause_y = pause_menu_y_for_viewport(render_width, render_height, item_count);

    if (pause_menu_row_at_point(render_width, render_height, item_count, render_width / 2, pause_y) != 0 ||
        pause_menu_row_at_point(render_width, render_height, item_count, render_width / 2,
                                pause_y + row_height) != 1) {
      fprintf(stderr, "Menu selftest failed: pause-menu mouse rows did not match pause-menu text layout\n");
      free_menu_assets(&assets);
      return 1;
    }
  }

  free_menu_assets(&assets);
  printf("Menu selftest passed\n");
  return 0;
}

static int verify_texture_source_flat_selftest(const char *kind, const char *tag, size_t expected_raw_size,
                                               size_t expected_decrunched_size, uint16_t expected_palette_count) {
  enum { FLAT_TEXEL_BYTES = GLOOM_FLAT_TEXTURE_SIZE * GLOOM_FLAT_TEXTURE_SIZE };
  char resolved_path[1024] = {0};
  char expected_path[64] = {0};
  uint8_t *data = NULL;
  size_t raw_size = 0u;
  size_t data_size = 0u;
  char error[256] = {0};
  uint16_t palette_count = 0u;

  if (kind == NULL || tag == NULL) {
    return 1;
  }

  (void)snprintf(expected_path, sizeof(expected_path), "amiga/data/txts/%s%s", kind, tag);
  if (!resolve_flat_texture_path(kind, tag, resolved_path, sizeof(resolved_path)) ||
      strstr(resolved_path, expected_path) == NULL) {
    fprintf(stderr, "Texture source selftest failed: %s tile_%s did not prefer shipped amiga/data/txts\n", kind,
            tag);
    return 1;
  }

  if (!read_binary_blob(resolved_path, &data, &data_size)) {
    fprintf(stderr, "Texture source selftest failed: could not read %s tile_%s\n", kind, tag);
    free(data);
    return 1;
  }
  raw_size = data_size;
  if (!gloom_decrunch_crm_buffer(&data, &data_size, error, sizeof(error))) {
    fprintf(stderr, "Texture source selftest failed: could not decrunch %s tile_%s: %s\n", kind, tag,
            error[0] ? error : "unknown error");
    free(data);
    return 1;
  }
  if (data_size < (size_t)FLAT_TEXEL_BYTES + 2u) {
    fprintf(stderr, "Texture source selftest failed: %s tile_%s is too small after decrunch\n", kind, tag);
    free(data);
    return 1;
  }

  palette_count = main_read_be16(data + (size_t)FLAT_TEXEL_BYTES);
  if (raw_size != expected_raw_size || data_size != expected_decrunched_size ||
      palette_count != expected_palette_count) {
    fprintf(stderr,
            "Texture source selftest failed: %s tile_%s is not the shipped Disk 2 flat "
            "(raw=%zu decrunched=%zu palette=%u)\n",
            kind, tag, raw_size, data_size, (unsigned)palette_count);
    free(data);
    return 1;
  }

  free(data);
  return 0;
}

static int verify_script_tile_selftest(const char *map_path, const char *expected_tag) {
  char tile_tag[32] = {0};

  if (!resolve_script_tile_tag_for_map(map_path, tile_tag, sizeof(tile_tag)) ||
      strcmp(tile_tag, expected_tag) != 0) {
    fprintf(stderr, "Texture source selftest failed: %s resolved tile_%s instead of tile_%s\n", map_path,
            tile_tag[0] ? tile_tag : "?", expected_tag);
    return 1;
  }

  return 0;
}

static int run_texture_source_selftest(void) {
  typedef struct {
    const char *tag;
    size_t floor_raw_size;
    size_t roof_raw_size;
    size_t decrunched_size;
    uint16_t palette_count;
  } ExpectedFlatTile;
  static const ExpectedFlatTile expected_tiles[] = {
      {"1", 3076u, 1754u, 16512u, 63u},
      {"2", 1610u, 1674u, 16448u, 31u},
      {"3", 1806u, 1642u, 16448u, 31u},
  };
  static const struct {
    const char *map_path;
    const char *tag;
  } expected_script_tiles[] = {
      {"amiga/data/maps/map1_1", "1"}, {"amiga/data/maps/map1_2", "1"}, {"amiga/data/maps/map1_3", "1"},
      {"amiga/data/maps/map1_4", "1"}, {"amiga/data/maps/map1_5", "1"}, {"amiga/data/maps/map1_6", "1"},
      {"amiga/data/maps/map1_7", "1"}, {"amiga/data/maps/map3_1", "2"}, {"amiga/data/maps/map3_2", "2"},
      {"amiga/data/maps/map3_3", "2"}, {"amiga/data/maps/map3_4", "2"}, {"amiga/data/maps/map3_5", "2"},
      {"amiga/data/maps/map3_6", "2"}, {"amiga/data/maps/map3_7", "2"}, {"amiga/data/maps/map4_1", "3"},
      {"amiga/data/maps/map4_2", "3"}, {"amiga/data/maps/map4_3", "3"}, {"amiga/data/maps/map4_4", "3"},
      {"amiga/data/maps/map4_5", "3"}, {"amiga/data/maps/map4_6", "3"}, {"amiga/data/maps/map4_7", "3"},
  };
  size_t i = 0u;

  for (i = 0u; i < sizeof(expected_tiles) / sizeof(expected_tiles[0]); ++i) {
    const ExpectedFlatTile *tile = &expected_tiles[i];

    if (verify_texture_source_flat_selftest("floor", tile->tag, tile->floor_raw_size, tile->decrunched_size,
                                            tile->palette_count) != 0 ||
        verify_texture_source_flat_selftest("roof", tile->tag, tile->roof_raw_size, tile->decrunched_size,
                                            tile->palette_count) != 0) {
      return 1;
    }
  }

  /*
   * amiga/gloom2.s: scripttile/fetchrest chooses the active tile for scripted
   * maps; pickcombat copies comseriesnum into floortag before loadtile.
   */
  for (i = 0u; i < sizeof(expected_script_tiles) / sizeof(expected_script_tiles[0]); ++i) {
    if (verify_script_tile_selftest(expected_script_tiles[i].map_path, expected_script_tiles[i].tag) != 0) {
      return 1;
    }
  }
  {
    int series = 0;
    int map_index = 0;

    for (series = 1; series <= 3; ++series) {
      char expected_tag[2] = {(char)('0' + series), '\0'};

      for (map_index = 1; map_index <= 7; ++map_index) {
        char map_path[64] = {0};

        (void)snprintf(map_path, sizeof(map_path), "amiga/data/maps/com%d_%d", series, map_index);
        if (verify_script_tile_selftest(map_path, expected_tag) != 0) {
          return 1;
        }
      }
    }
  }
  for (i = 0u; i < sizeof(expected_script_tiles) / sizeof(expected_script_tiles[0]); ++i) {
    FlatTextureSet flat_textures;

    memset(&flat_textures, 0, sizeof(flat_textures));
    if (!load_flat_texture_set_for_map(expected_script_tiles[i].map_path, &flat_textures)) {
      fprintf(stderr, "Texture source selftest failed: could not load flats for %s\n",
              expected_script_tiles[i].map_path);
      free_flat_texture_set(&flat_textures);
      return 1;
    }
    if (strcmp(flat_textures.tile_tag, expected_script_tiles[i].tag) != 0 || !flat_textures.floor.loaded ||
        !flat_textures.roof.loaded) {
      fprintf(stderr, "Texture source selftest failed: loaded wrong flat set for %s\n",
              expected_script_tiles[i].map_path);
      free_flat_texture_set(&flat_textures);
      return 1;
    }
    free_flat_texture_set(&flat_textures);
  }
  {
    int series = 0;

    for (series = 1; series <= 3; ++series) {
      char map_path[64] = {0};
      char expected_tag[2] = {(char)('0' + series), '\0'};
      FlatTextureSet flat_textures;

      (void)snprintf(map_path, sizeof(map_path), "amiga/data/maps/com%d_1", series);
      memset(&flat_textures, 0, sizeof(flat_textures));
      if (!load_flat_texture_set_for_map(map_path, &flat_textures)) {
        fprintf(stderr, "Texture source selftest failed: could not load combat flats for %s\n", map_path);
        free_flat_texture_set(&flat_textures);
        return 1;
      }
      if (strcmp(flat_textures.tile_tag, expected_tag) != 0 || !flat_textures.floor.loaded ||
          !flat_textures.roof.loaded) {
        fprintf(stderr, "Texture source selftest failed: loaded wrong combat flat set for %s\n", map_path);
        free_flat_texture_set(&flat_textures);
        return 1;
      }
      free_flat_texture_set(&flat_textures);
    }
  }

  printf("Texture source selftest passed\n");
  return 0;
}

static int run_wall_selftest(void) {
  static AppState state;
  GloomAnim anim;
  GloomZone zones[8];
  GloomTextureChangeCommand texture_change;
  GloomRotPolyCommand rotpoly_command;
  WallCandidate candidates[8];
  size_t candidate_count = 0u;
  int i = 0;

  memset(&state, 0, sizeof(state));
  memset(&anim, 0, sizeof(anim));
  {
    int expected_amiga_focal = 1 << 7;
    int expected_pc_view_focal = 1 << 6;

    if (GLOOM_AMIGA_FOCAL != expected_amiga_focal) {
      fprintf(stderr, "Wall selftest failed: projection focal does not match amiga/gloom2.s focshft=7\n");
      return 1;
    }
    if (GLOOM_PC_VIEW_FOCAL != expected_pc_view_focal) {
      fprintf(stderr, "Wall selftest failed: PC viewport focal no longer matches the 106x80 FOV reference\n");
      return 1;
    }
  }
  initialize_wall_texture_remap(&state);
  anim.frame_count = 3u;
  anim.first_frame = 10u;
  anim.delay = 2u;
  anim.current = 1u;
  state.map.animations = &anim;
  state.map.animation_count = 1u;

  advance_wall_animations_amiga_tick(&state);
  if (anim.current != 2u || state.wall_texture_remap[10] != 11u || state.wall_texture_remap[11] != 12u ||
      state.wall_texture_remap[12] != 10u) {
    fprintf(stderr, "Wall selftest failed: doanims texture pointer rotation did not match the Amiga table shift\n");
    return 1;
  }

  advance_wall_animations_amiga_tick(&state);
  if (anim.current != 1u || state.wall_texture_remap[10] != 11u) {
    fprintf(stderr, "Wall selftest failed: doanims countdown rotated before reaching zero\n");
    return 1;
  }

  advance_wall_animations_amiga_tick(&state);
  if (anim.current != 2u || state.wall_texture_remap[10] != 12u || state.wall_texture_remap[11] != 10u ||
      state.wall_texture_remap[12] != 11u) {
    fprintf(stderr, "Wall selftest failed: repeated doanims countdown did not keep rotating the global table\n");
    return 1;
  }

  memset(&texture_change, 0, sizeof(texture_change));
  memset(zones, 0, sizeof(zones));
  state.map.zones = zones;
  state.map.zone_count = 1u;
  state.map.texture_change_commands = &texture_change;
  state.map.texture_change_command_count = 1u;
  for (i = 0; i < 8; ++i) {
    zones[0].textures[i] = 4u;
  }
  texture_change.event_id = 5u;
  texture_change.zone_index = 0u;
  texture_change.texture_index = 10u;
  execute_map_event(&state, 5u);
  if (zones[0].textures[0] != 10u || remap_wall_texture_index(&state, zones[0].textures[0]) != 12u) {
    fprintf(stderr, "Wall selftest failed: exec_changetxt did not store the raw zone texture byte before remap\n");
    return 1;
  }

  memset(&zones[0], 0, sizeof(zones[0]));
  zones[0].scale = 2;
  {
    int tile_index = -1;
    size_t texture_x = 999u;
    float local_u = -1.0f;

    zone_wall_texture_coordinates(&zones[0], 0.25f, &tile_index, &local_u, &texture_x);
    if (tile_index != 0 || texture_x != 16u || local_u < 0.249f || local_u > 0.251f) {
      fprintf(stderr, "Wall selftest failed: wl_sc=2 did not use one full Amiga texture across the wall\n");
      return 1;
    }
    zone_wall_texture_coordinates(&zones[0], 0.5f, &tile_index, &local_u, &texture_x);
    if (tile_index != 0 || texture_x != 32u || local_u < 0.499f || local_u > 0.501f) {
      fprintf(stderr, "Wall selftest failed: wl_sc=2 advanced texture slots too early\n");
      return 1;
    }

    zones[0].scale = 4;
    zone_wall_texture_coordinates(&zones[0], 0.5f, &tile_index, &local_u, &texture_x);
    if (tile_index != 1 || texture_x != 0u || local_u > 0.001f) {
      fprintf(stderr, "Wall selftest failed: wl_sc=4 did not advance to the second texture slot halfway along wall\n");
      return 1;
    }

    zones[0].scale = 0;
    zone_wall_texture_coordinates(&zones[0], 0.5f, &tile_index, &local_u, &texture_x);
    if (tile_index != 0 || texture_x != 32u || local_u < 0.499f || local_u > 0.501f) {
      fprintf(stderr, "Wall selftest failed: wl_sc=0 did not match the Amiga doubled half-range coordinate\n");
      return 1;
    }

    zones[0].scale = -1;
    zone_wall_texture_coordinates(&zones[0], 0.75f, &tile_index, &local_u, &texture_x);
    if (tile_index != 0 || texture_x != 24u || local_u < 0.374f || local_u > 0.376f) {
      fprintf(stderr, "Wall selftest failed: wl_sc=-1 no longer matches the Amiga shifted texture coordinate\n");
      return 1;
    }
  }

  memset(&state, 0, sizeof(state));
  memset(zones, 0, sizeof(zones));
  state.map.zones = zones;
  state.map.zone_count = 1u;
  zones[0].x1 = 100;
  zones[0].z1 = 200;
  zones[0].x2 = 164;
  zones[0].z2 = 200;
  start_runtime_door(&state, 0u);
  if (!state.doors[0].active || state.doors[0].frac_add != 0x0100) {
    fprintf(stderr, "Wall selftest failed: exec_opendoor equivalent did not start a door item\n");
    return 1;
  }
  for (i = 0; i < 60; ++i) {
    update_doors(&state);
  }
  {
    int32_t frac_fixed = round_float_to_int32(state.doors[0].frac);
    int32_t expected_move = amiga_arithmetic_shift_right_16((int64_t)64 * (int64_t)frac_fixed * 4);

    if (zones[0].event_id != (int16_t)(uint16_t)(frac_fixed * 2) || zones[0].x1 != (int16_t)(100 - expected_move) ||
        zones[0].x2 != (int16_t)(164 - expected_move) || zones[0].z1 != 200 || zones[0].z2 != 200) {
      fprintf(stderr, "Wall selftest failed: dodoors geometry/open fraction update is wrong\n");
      return 1;
    }
  }

  memset(&state, 0, sizeof(state));
  memset(zones, 0, sizeof(zones));
  memset(&rotpoly_command, 0, sizeof(rotpoly_command));
  state.map.zones = zones;
  state.map.zone_count = 8u;
  zones[0].x1 = 0;
  zones[0].z1 = 0;
  zones[0].x2 = 64;
  zones[0].z2 = 0;
  zones[1].x1 = 64;
  zones[1].z1 = 0;
  zones[1].x2 = 64;
  zones[1].z2 = 64;
  zones[2].x1 = 64;
  zones[2].z1 = 64;
  zones[2].x2 = 0;
  zones[2].z2 = 64;
  zones[3].x1 = 0;
  zones[3].z1 = 64;
  zones[3].x2 = 0;
  zones[3].z2 = 0;
  zones[4].x1 = 64;
  zones[4].z1 = 0;
  zones[5].x1 = 128;
  zones[5].z1 = 0;
  zones[6].x1 = 128;
  zones[6].z1 = 64;
  zones[7].x1 = 64;
  zones[7].z1 = 64;
  rotpoly_command.first_zone_index = 0u;
  rotpoly_command.zone_count = 4u;
  rotpoly_command.speed = 4096;
  rotpoly_command.flags = 1u;
  start_runtime_rotpoly(&state, &rotpoly_command);
  update_rotpolys(&state);
  if (zones[0].x1 <= 0 || zones[0].x2 <= 64 || zones[3].x2 != zones[0].x1 || zones[3].z2 != zones[0].z1) {
    fprintf(stderr, "Wall selftest failed: dorots morph did not move live zone endpoints as a closed polygon\n");
    return 1;
  }

  memset(&state, 0, sizeof(state));
  memset(zones, 0, sizeof(zones));
  memset(&rotpoly_command, 0, sizeof(rotpoly_command));
  memset(candidates, 0, sizeof(candidates));
  candidate_count = 0u;
  state.map.zones = zones;
  state.map.zone_count = 4u;
  zones[0].x1 = 0;
  zones[0].z1 = 0;
  zones[0].x2 = 64;
  zones[0].z2 = 0;
  zones[1].x1 = 64;
  zones[1].z1 = 0;
  zones[1].x2 = 64;
  zones[1].z2 = 64;
  zones[2].x1 = 64;
  zones[2].z1 = 64;
  zones[2].x2 = 0;
  zones[2].z2 = 64;
  zones[3].x1 = 0;
  zones[3].z1 = 64;
  zones[3].x2 = 0;
  zones[3].z2 = 0;
  rotpoly_command.first_zone_index = 0u;
  rotpoly_command.zone_count = 4u;
  rotpoly_command.speed = 256;
  rotpoly_command.flags = 0u;
  start_runtime_rotpoly(&state, &rotpoly_command);
  append_runtime_rotpoly_wall_candidates(&state, candidates, &candidate_count, 8u, 32.0f, 32.0f);
  update_rotpolys(&state);
  if (candidate_count != 4u || (zones[0].x1 == 0 && zones[0].z1 == 0) || zones[3].x2 != zones[0].x1 ||
      zones[3].z2 != zones[0].z1) {
    fprintf(stderr, "Wall selftest failed: dorots rotation was not exposed to render/collision candidates\n");
    return 1;
  }

  {
    GloomMap map;
    WallTextureSet wall_textures;
    char error[256] = {0};

    memset(&map, 0, sizeof(map));
    memset(&wall_textures, 0, sizeof(wall_textures));
    if (!gloom_map_load("amiga/maps/map1_1", &map, error, sizeof(error))) {
      fprintf(stderr, "Wall selftest failed: could not parse animated source map1_1: %s\n",
              error[0] ? error : "unknown error");
      return 1;
    }
    if (map.animation_count == 0u) {
      fprintf(stderr, "Wall selftest failed: map1_1 no longer exposes an animation table fixture\n");
      gloom_map_free(&map);
      return 1;
    }
    if (!load_map_wall_textures(&map, &wall_textures) || !validate_map_wall_texture_references(&map, &wall_textures)) {
      fprintf(stderr, "Wall selftest failed: map1_1 animated wall texture references are not renderable\n");
      free_wall_texture_set(&wall_textures);
      gloom_map_free(&map);
      return 1;
    }
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&map);
  }

  printf("Wall selftest passed\n");
  return 0;
}

static void configure_horizontal_test_zone(GloomZone *zone, int16_t x1, int16_t z1, int16_t x2, int16_t z2,
                                           int16_t event_id) {
  if (zone == NULL) {
    return;
  }

  memset(zone, 0, sizeof(*zone));
  zone->x1 = x1;
  zone->z1 = z1;
  zone->x2 = x2;
  zone->z2 = z2;
  zone->na = 32766;
  zone->nb = 0;
  zone->a = 0;
  zone->b = 32766;
  zone->ln = (int16_t)(x2 - x1);
  zone->event_id = event_id;
}

static int run_switch_selftest(void) {
  AppState state;
  GloomZone zones[4];
  GloomTextureChangeCommand texture_changes[2];
  uint8_t ppnt_blob[4] = {0, 0, 0, 1};
  PlayerControls controls;
  size_t i = 0u;

  memset(&state, 0, sizeof(state));
  memset(zones, 0, sizeof(zones));
  memset(texture_changes, 0, sizeof(texture_changes));
  memset(&controls, 0, sizeof(controls));

  state.map.zones = zones;
  state.map.zone_count = 4u;
  state.map.has_grid_cells = true;
  state.map.ppnt_blob = ppnt_blob;
  state.map.ppnt_blob_size = sizeof(ppnt_blob);
  state.map.trigger_grid[0].count_minus_one = 1;
  state.map.trigger_grid[0].ppnt_word_offset = 0u;
  state.map.texture_change_commands = texture_changes;
  state.map.texture_change_command_count = 2u;
  state.player_radius = 33;
  state.camera_x = 128.0f;
  state.camera_z = 120.0f;

  configure_horizontal_test_zone(&zones[0], 96, 128, 160, 128, 2);
  configure_horizontal_test_zone(&zones[1], 96, 128, 160, 128, 3);
  for (i = 0u; i < sizeof(zones[2].textures); ++i) {
    zones[2].textures[i] = 4u;
    zones[3].textures[i] = 4u;
  }
  texture_changes[0].event_id = 2u;
  texture_changes[0].zone_index = 2u;
  texture_changes[0].texture_index = 10u;
  texture_changes[1].event_id = 3u;
  texture_changes[1].zone_index = 3u;
  texture_changes[1].texture_index = 11u;

  check_event_triggers(&state);
  if (zones[0].event_id != -2 || zones[2].textures[0] != 10u || zones[3].textures[0] != 4u) {
    fprintf(stderr,
            "Switch selftest failed: checkevent should run only the first colliding trigger without fire/use input\n");
    return 1;
  }

  controls.fire = true;
  update_player_weapon(&state, &controls);
  check_event_triggers(&state);
  if (zones[3].textures[0] != 4u) {
    fprintf(stderr, "Switch selftest failed: a cleared first trigger should block later overlapping triggers\n");
    return 1;
  }

  state.player_pixsizeadd = 1;
  zones[0].event_id = 4;
  check_event_triggers(&state);
  if (zones[0].event_id != 4) {
    fprintf(stderr, "Switch selftest failed: checkevent ran while ob_pixsizeadd was active\n");
    return 1;
  }

  printf("Switch selftest passed\n");
  return 0;
}

static int run_teleport_selftest(void) {
  AppState state;
  GloomTeleportCommand command;
  char next_map_path[1024] = {0};
  int i = 0;

  memset(&state, 0, sizeof(state));
  memset(&command, 0, sizeof(command));

  state.camera_x = 10.0f;
  state.camera_z = 20.0f;
  state.player_rot_fixed = amiga_rotation_to_fixed(1);
  command.event_id = 7u;
  command.x = 1234;
  command.z = 2345;
  command.rotation = 64;

  start_pending_teleport(&state, &command);
  if (!state.teleport_active || state.player_pixsizeadd != 2 || state.player_pixsize != 0 || state.finished != 0) {
    fprintf(stderr, "Teleport selftest failed: exec_teleport did not start ob_pixsizeadd=2 teleport pixel-out\n");
    return 1;
  }

  for (i = 0; i < 28; ++i) {
    (void)update_player_pixsize_transition(&state);
  }
  if (state.camera_x != 10.0f || state.camera_z != 20.0f || state.player_pixsizeadd != 2) {
    fprintf(stderr, "Teleport selftest failed: player moved before ob_pixsize reached 24\n");
    return 1;
  }

  (void)update_player_pixsize_transition(&state);
  if (state.camera_x != 1234.0f || state.camera_z != 2345.0f ||
      state.player_rot_fixed != amiga_rotation_to_fixed(64) || state.player_pixsizeadd != -2) {
    fprintf(stderr, "Teleport selftest failed: playertimers did not move/rotate player and reverse pixel size at 24\n");
    return 1;
  }

  for (i = 0; i < 29; ++i) {
    (void)update_player_pixsize_transition(&state);
  }
  if (state.teleport_active || state.player_pixsizeadd != 0 || state.player_pixsize != 0) {
    fprintf(stderr, "Teleport selftest failed: teleport pixel-in did not clear at ob_pixsize zero\n");
    return 1;
  }

  memset(&state, 0, sizeof(state));
  command.control_word = 1;
  start_pending_teleport(&state, &command);
  if (state.teleport_active || state.player_pixsizeadd != 0) {
    fprintf(stderr, "Teleport selftest failed: lock teleport started without ported locklogic\n");
    return 1;
  }

  memset(&state, 0, sizeof(state));
  start_level_exit_transition(&state);
  if (!state.teleport_active || state.finished2 != GLOOM_LEVEL_COMPLETE_FINISHED || state.player_pixsizeadd != 1) {
    fprintf(stderr, "Teleport selftest failed: event 24 did not start finished2=3 pixel-out\n");
    return 1;
  }
  for (i = 0; i < 58; ++i) {
    (void)update_player_pixsize_transition(&state);
  }
  if (state.finished != GLOOM_LEVEL_COMPLETE_FINISHED || !state.level_transition_reported) {
    fprintf(stderr, "Teleport selftest failed: playertimers did not promote finished2=3 to finished=3\n");
    return 1;
  }

  if (!resolve_next_script_play_map("amiga/maps/map1_1", next_map_path, sizeof(next_map_path)) ||
      strcmp(next_map_path, "amiga/data/maps/map1_2") != 0) {
    fprintf(stderr, "Teleport selftest failed: levelover/scriptplay did not resolve map1_1 to script play_map1_2\n");
    return 1;
  }

  printf("Teleport selftest passed\n");
  return 0;
}

typedef struct {
  uint32_t ticks;
  PlayerControls controls;
  int mouse_dx;
} ReplayInputStep;

typedef struct {
  uint64_t hash;
  uint64_t ticks;
  int32_t camera_x;
  int32_t camera_z;
  int32_t camera_y;
  int32_t player_rot_fixed;
  int16_t player_hitpoints;
  int16_t player_lives;
  uint16_t active_projectiles;
  uint16_t active_enemies;
} ReplayResult;

static void replay_hash_bytes(uint64_t *hash, const void *data, size_t size) {
  const uint8_t *bytes = (const uint8_t *)data;
  size_t i = 0u;

  if (hash == NULL || data == NULL) {
    return;
  }

  for (i = 0u; i < size; ++i) {
    *hash ^= (uint64_t)bytes[i];
    *hash *= 1099511628211ull;
  }
}

static void replay_hash_u64(uint64_t *hash, uint64_t value) {
  replay_hash_bytes(hash, &value, sizeof(value));
}

static void replay_hash_i32(uint64_t *hash, int32_t value) {
  replay_hash_bytes(hash, &value, sizeof(value));
}

static int32_t replay_quantize_float(float value) {
  return round_float_to_int32(value * 256.0f);
}

static uint64_t replay_fingerprint_state(const AppState *state, ReplayResult *out_result) {
  uint64_t hash = 1469598103934665603ull;
  uint16_t active_projectiles = 0u;
  uint16_t active_enemies = 0u;
  size_t i = 0u;

  if (state == NULL) {
    return 0u;
  }

  replay_hash_u64(&hash, state->tick_count);
  replay_hash_i32(&hash, replay_quantize_float(state->camera_x));
  replay_hash_i32(&hash, replay_quantize_float(state->camera_z));
  replay_hash_i32(&hash, replay_quantize_float(state->camera_y));
  replay_hash_i32(&hash, state->player_rot_fixed);
  replay_hash_i32(&hash, state->player_rotspeed);
  replay_hash_i32(&hash, replay_quantize_float(state->player_bounce));
  replay_hash_i32(&hash, (int32_t)state->player_frame_fixed);
  replay_hash_i32(&hash, state->player_hitpoints);
  replay_hash_i32(&hash, state->player_lives);
  replay_hash_i32(&hash, state->player_dead ? 1 : 0);
  replay_hash_i32(&hash, (int32_t)state->player_death_phase);
  replay_hash_i32(&hash, replay_quantize_float(state->player_death_timer));
  replay_hash_i32(&hash, replay_quantize_float(state->player_eye_y));
  replay_hash_i32(&hash, (int32_t)state->player_weapon);
  replay_hash_i32(&hash, (int32_t)state->player_reload);
  replay_hash_i32(&hash, replay_quantize_float(state->player_reload_counter));
  replay_hash_i32(&hash, replay_quantize_float(state->player_weapon_flash));
  replay_hash_i32(&hash, replay_quantize_float(state->player_mega_timer));
  replay_hash_i32(&hash, state->finished);
  replay_hash_i32(&hash, state->finished2);
  replay_hash_i32(&hash, (int32_t)state->rng_state);
  replay_hash_i32(&hash, state->two_player_mode ? 1 : 0);
  if (state->two_player_mode) {
    replay_hash_i32(&hash, replay_quantize_float(state->player2.camera_x));
    replay_hash_i32(&hash, replay_quantize_float(state->player2.camera_z));
    replay_hash_i32(&hash, replay_quantize_float(state->player2.camera_y));
    replay_hash_i32(&hash, state->player2.player_rot_fixed);
    replay_hash_i32(&hash, (int32_t)state->player2.player_frame_fixed);
    replay_hash_i32(&hash, state->player2.player_hitpoints);
    replay_hash_i32(&hash, state->player2.player_lives);
    replay_hash_i32(&hash, state->player2.player_dead ? 1 : 0);
    replay_hash_i32(&hash, (int32_t)state->player2.player_death_phase);
    replay_hash_i32(&hash, (int32_t)state->player2.player_weapon);
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_PROJECTILES; ++i) {
    const RuntimeProjectile *projectile = &state->projectiles[i];

    if (!projectile->active) {
      continue;
    }
    active_projectiles += 1u;
    replay_hash_i32(&hash, (int32_t)i);
    replay_hash_i32(&hash, projectile->enemy ? 1 : 0);
    replay_hash_i32(&hash, (int32_t)projectile->weapon_index);
    replay_hash_i32(&hash, replay_quantize_float(projectile->x));
    replay_hash_i32(&hash, replay_quantize_float(projectile->z));
    replay_hash_i32(&hash, replay_quantize_float(projectile->vx));
    replay_hash_i32(&hash, replay_quantize_float(projectile->vz));
    replay_hash_i32(&hash, projectile->hitpoints);
  }

  for (i = 0u; i < GLOOM_MAX_RUNTIME_OBJECTS; ++i) {
    const RuntimeObjectState *object = &state->objects[i];

    if (!object->active || !object->enemy) {
      continue;
    }
    active_enemies += 1u;
    replay_hash_i32(&hash, (int32_t)i);
    replay_hash_i32(&hash, replay_quantize_float(object->x));
    replay_hash_i32(&hash, replay_quantize_float(object->z));
    replay_hash_i32(&hash, object->rotation);
    replay_hash_i32(&hash, object->hitpoints);
    replay_hash_i32(&hash, (int32_t)object->logic_state);
    replay_hash_i32(&hash, (int32_t)object->contact_was_hit_player);
    replay_hash_i32(&hash, replay_quantize_float(object->delay));
    replay_hash_i32(&hash, replay_quantize_float(object->pause_delay));
    replay_hash_i32(&hash, (int32_t)object->frame_fixed);
  }

  if (out_result != NULL) {
    out_result->hash = hash;
    out_result->ticks = state->tick_count;
    out_result->camera_x = replay_quantize_float(state->camera_x);
    out_result->camera_z = replay_quantize_float(state->camera_z);
    out_result->camera_y = replay_quantize_float(state->camera_y);
    out_result->player_rot_fixed = state->player_rot_fixed;
    out_result->player_hitpoints = state->player_hitpoints;
    out_result->player_lives = state->player_lives;
    out_result->active_projectiles = active_projectiles;
    out_result->active_enemies = active_enemies;
  }

  return hash;
}

static bool replay_load_steps(const char *path, ReplayInputStep *steps, size_t max_steps, size_t *out_count) {
  FILE *file = NULL;
  char line[256] = {0};
  size_t count = 0u;
  uint32_t line_number = 0u;

  if (path == NULL || steps == NULL || max_steps == 0u || out_count == NULL) {
    return false;
  }

  file = fopen(path, "r");
  if (file == NULL) {
    fprintf(stderr, "Replay selftest failed: cannot open input replay %s\n", path);
    return false;
  }

  while (fgets(line, sizeof(line), file) != NULL) {
    uint32_t ticks = 0u;
    int joyx = 0;
    int joyy = 0;
    int joys = 0;
    int fire = 0;
    int mouse_dx = 0;
    int matched = 0;
    char *cursor = line;

    line_number += 1u;
    while (*cursor == ' ' || *cursor == '\t') {
      cursor += 1;
    }
    if (*cursor == '\0' || *cursor == '\n' || *cursor == '\r' || *cursor == '#') {
      continue;
    }

    matched = sscanf(cursor, "%u %d %d %d %d %d", &ticks, &joyx, &joyy, &joys, &fire, &mouse_dx);
    if (matched < 5 || ticks == 0u) {
      fprintf(stderr,
              "Replay selftest failed: %s:%u must be: ticks joyx joyy joys fire [mouse_dx], with ticks > 0\n",
              path, line_number);
      fclose(file);
      return false;
    }
    if (count >= max_steps) {
      fprintf(stderr, "Replay selftest failed: %s has more than %zu input rows\n", path, max_steps);
      fclose(file);
      return false;
    }

    memset(&steps[count], 0, sizeof(steps[count]));
    steps[count].ticks = ticks;
    steps[count].controls.joyx = (int16_t)joyx;
    steps[count].controls.joyy = (int16_t)joyy;
    steps[count].controls.joys = (int16_t)joys;
    steps[count].controls.fire = fire != 0;
    steps[count].mouse_dx = matched >= 6 ? mouse_dx : 0;
    count += 1u;
  }

  fclose(file);
  *out_count = count;
  return true;
}

static bool replay_run_once(const char *map_path, const ReplayInputStep *steps, size_t step_count,
                            ReplayResult *out_result) {
  AppState state;
  ObjectVisualSet object_visuals;
  char resolved_map_path[1024] = {0};
  const char *resolved = map_path;
  char error[256] = {0};
  size_t i = 0u;

  if (map_path == NULL || steps == NULL || step_count == 0u || out_result == NULL) {
    return false;
  }

  memset(&state, 0, sizeof(state));
  memset(&object_visuals, 0, sizeof(object_visuals));
  memset(&g_audio, 0, sizeof(g_audio));

  if (resolve_runtime_file_path(map_path, resolved_map_path, sizeof(resolved_map_path))) {
    resolved = resolved_map_path;
  }

  state.barrel_projectile_origin = true;
  if (!gloom_map_load(resolved, &state.map, error, sizeof(error))) {
    fprintf(stderr, "Replay selftest failed: map parse failed for %s: %s\n", resolved,
            error[0] ? error : "unknown error");
    return false;
  }
  if (!load_player_collision_radius(&state.player_radius)) {
    fprintf(stderr, "Replay selftest failed: cannot load player radius from original objs/player asset\n");
    gloom_map_free(&state.map);
    return false;
  }
  if (!load_object_visual_set_for_map(&state.map, &object_visuals)) {
    gloom_map_free(&state.map);
    return false;
  }

  compute_map_bounds(&state);
  if (!initialize_camera_from_map_spawn(&state)) {
    fprintf(stderr, "Replay selftest failed: no player spawn found in %s\n", resolved);
    free_object_visual_set(&object_visuals);
    gloom_map_free(&state.map);
    return false;
  }
  state.violence_mode = GLOOM_VIOLENCE_MEATY_MESSY;
  initialize_runtime_objects(&state);

  for (i = 0u; i < step_count; ++i) {
    uint32_t tick = 0u;

    for (tick = 0u; tick < steps[i].ticks; ++tick) {
      apply_mouse_look(&state, steps[i].mouse_dx);
      update_with_controls(&state, &steps[i].controls, NULL, &object_visuals);
      if (state.finished == GLOOM_LEVEL_COMPLETE_FINISHED) {
        break;
      }
    }
    if (state.finished == GLOOM_LEVEL_COMPLETE_FINISHED) {
      break;
    }
  }

  (void)replay_fingerprint_state(&state, out_result);
  free_object_visual_set(&object_visuals);
  gloom_map_free(&state.map);
  return true;
}

static int run_replay_selftest(int argc, char **argv) {
  static const ReplayInputStep default_steps[] = {
      {30u, {0, -1, 0, 0, false, false}, 0}, {18u, {1, -1, -1, 0, false, false}, 0},
      {12u, {0, 0, 0, 0, true, false}, 2},    {20u, {-1, 0, 0, 0, false, false}, -1},
      {25u, {0, 1, 0, 0, false, false}, 0},   {10u, {0, 0, 0, 0, true, false}, 0}};
  ReplayInputStep file_steps[256];
  const ReplayInputStep *steps = default_steps;
  size_t step_count = sizeof(default_steps) / sizeof(default_steps[0]);
  const char *map_path = "amiga/maps/map1_1";
  const char *replay_path = NULL;
  ReplayResult first;
  ReplayResult second;

  memset(file_steps, 0, sizeof(file_steps));
  memset(&first, 0, sizeof(first));
  memset(&second, 0, sizeof(second));

  if (argc > 2) {
    map_path = argv[2];
  }
  if (argc > 3) {
    replay_path = argv[3];
  }
  if (replay_path != NULL && !replay_load_steps(replay_path, file_steps, sizeof(file_steps) / sizeof(file_steps[0]),
                                                &step_count)) {
    return 1;
  }
  if (replay_path != NULL) {
    steps = file_steps;
  }
  if (step_count == 0u) {
    fprintf(stderr, "Replay selftest failed: no replay input rows were provided\n");
    return 1;
  }

  if (!replay_run_once(map_path, steps, step_count, &first) || !replay_run_once(map_path, steps, step_count, &second)) {
    return 1;
  }
  if (first.hash != second.hash) {
    fprintf(stderr, "Replay selftest failed: deterministic fingerprints differ (%016llx != %016llx)\n",
            (unsigned long long)first.hash, (unsigned long long)second.hash);
    return 1;
  }

  printf("Replay selftest passed: map=%s steps=%zu ticks=%llu hash=%016llx x=%.2f z=%.2f rot=%d hp=%d "
         "projectiles=%u enemies=%u\n",
         map_path, step_count, (unsigned long long)first.ticks, (unsigned long long)first.hash,
         (double)first.camera_x / 256.0, (double)first.camera_z / 256.0, first.player_rot_fixed,
         first.player_hitpoints, (unsigned)first.active_projectiles, (unsigned)first.active_enemies);
  return 0;
}

static void share_levelover_two_player_lives(int16_t *p1_lives, int16_t *p2_lives) {
  int16_t shared_lives = 0;

  if (p1_lives == NULL || p2_lives == NULL) {
    return;
  }

  /* amiga/gloom2.s: levelover stores the max of p1lives/p2lives back to both players. */
  shared_lives = *p1_lives >= *p2_lives ? *p1_lives : *p2_lives;
  *p1_lives = shared_lives;
  *p2_lives = shared_lives;
}

static void initialize_combat_rotation_state(AppState *state, uint8_t series, int16_t start_lives) {
  static const uint8_t initial_maps[8] = {1, 2, 3, 4, 5, 6, 7, 8};

  if (state == NULL) {
    return;
  }

  memcpy(state->combat_maps, initial_maps, sizeof(state->combat_maps));
  state->combat_maps_left = 7;
  state->combat_series = series;
  if (state->combat_series < 1u || state->combat_series > 3u) {
    state->combat_series = 1u;
  }
  state->combat_start_lives = start_lives > 0 ? start_lives : 3;
}

static bool resolve_next_combat_map(AppState *state, char *out_map_path, size_t out_map_path_size) {
  uint16_t random_word = 0u;
  int16_t index = 0;
  uint8_t map_number = 0u;

  if (state == NULL || out_map_path == NULL || out_map_path_size == 0u || !state->combat_mode) {
    return false;
  }

  if (state->combat_maps_left <= 0) {
    state->combat_maps_left = 7;
  }

  random_word = runtime_random_word(state);
  index = (int16_t)(((uint32_t)random_word * (uint32_t)state->combat_maps_left) >> 16);
  if (index < 0) {
    index = 0;
  }
  if (index >= state->combat_maps_left) {
    index = state->combat_maps_left - 1;
  }

  state->combat_maps_left -= 1;
  map_number = state->combat_maps[index];
  state->combat_maps[index] = state->combat_maps[state->combat_maps_left];
  state->combat_maps[state->combat_maps_left] = map_number;
  if (map_number == 0u) {
    map_number = 1u;
  }

  /* amiga/gloom2.s: pickcombat builds "com<series>_<map>" from commaps/comseriesnum/comseriesmap. */
  (void)snprintf(out_map_path, out_map_path_size, "amiga/maps/com%u_%u", (unsigned)state->combat_series,
                 (unsigned)map_number);
  return true;
}

static void restore_scriptplay_primary_player(AppState *state, int16_t hitpoints, int16_t lives, uint8_t weapon,
                                              uint8_t reload) {
  if (state == NULL) {
    return;
  }

  /* amiga/gloom2.s: scriptplay copies lives exactly; zero health resets health and reload only. */
  state->player_lives = lives;
  if (hitpoints == 0) {
    hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
    reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
  }
  state->player_hitpoints = hitpoints;
  state->player_weapon = weapon;
  state->player_reload = reload;
}

static void restore_scriptplay_secondary_player(RuntimePlayerState *player, int16_t hitpoints, int16_t lives,
                                                uint8_t weapon, uint8_t reload) {
  if (player == NULL) {
    return;
  }

  player->player_lives = lives;
  if (hitpoints == 0) {
    hitpoints = GLOOM_PLAYER_INITIAL_HEALTH;
    reload = (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD;
  }
  player->player_hitpoints = hitpoints;
  player->player_weapon = weapon;
  player->player_reload = reload;
}

static bool load_runtime_level(const char *map_path, AppState *state, WallTextureSet *wall_textures,
                               FlatTextureSet *flat_textures, ObjectVisualSet *object_visuals,
                               GloomZone *previous_zones, GloomZone *render_zones, bool preserve_player,
                               int16_t preserved_hitpoints, int16_t preserved_lives, uint8_t preserved_weapon,
                               uint8_t preserved_reload, int16_t preserved_p2_hitpoints, int16_t preserved_p2_lives,
                               uint8_t preserved_p2_weapon, uint8_t preserved_p2_reload, bool barrel_projectile_origin,
                               bool two_player_mode, uint8_t violence_mode, char *resolved_map_path,
                               size_t resolved_map_path_size) {
  static AppState next_state_storage;
  static WallTextureSet next_wall_textures;
  static FlatTextureSet next_flat_textures;
  static ObjectVisualSet next_object_visuals;
  AppState *next_state = &next_state_storage;
  bool next_combat_mode = state != NULL && state->combat_mode;
  uint8_t next_combat_series = state != NULL ? state->combat_series : 1u;
  int16_t next_combat_start_lives = state != NULL ? state->combat_start_lives : 3;
  uint8_t next_combat_maps[8] = {0};
  int16_t next_combat_maps_left = state != NULL ? state->combat_maps_left : 7;
  char map_path_buffer[1024] = {0};
  const char *resolved = map_path;
  char error[256] = {0};

  if (map_path == NULL || state == NULL || wall_textures == NULL || flat_textures == NULL || object_visuals == NULL ||
      previous_zones == NULL || render_zones == NULL) {
    return false;
  }

  memset(next_state, 0, sizeof(*next_state));
  if (state != NULL) {
    memcpy(next_combat_maps, state->combat_maps, sizeof(next_combat_maps));
  }
  memset(&next_wall_textures, 0, sizeof(next_wall_textures));
  memset(&next_flat_textures, 0, sizeof(next_flat_textures));
  memset(&next_object_visuals, 0, sizeof(next_object_visuals));

  if (resolve_runtime_file_path(map_path, map_path_buffer, sizeof(map_path_buffer))) {
    resolved = map_path_buffer;
  }

  next_state->barrel_projectile_origin = barrel_projectile_origin;
  next_state->two_player_mode = two_player_mode;
  next_state->combat_mode = next_combat_mode;
  next_state->combat_series = next_combat_series;
  next_state->combat_start_lives = next_combat_start_lives;
  memcpy(next_state->combat_maps, next_combat_maps, sizeof(next_state->combat_maps));
  next_state->combat_maps_left = next_combat_maps_left;
  if (!gloom_map_load(resolved, &next_state->map, error, sizeof(error))) {
    fprintf(stderr, "Map parse failed for %s: %s\n", resolved, error[0] ? error : "unknown error");
    return false;
  }

  if (!load_map_wall_textures(&next_state->map, &next_wall_textures)) {
    fprintf(stderr, "No map texture screens decoded from map texture names for %s\n", resolved);
    gloom_map_free(&next_state->map);
    return false;
  }

  if (!preserve_stale_wall_texture_references(&next_state->map, &next_wall_textures, wall_textures) ||
      !validate_map_wall_texture_references(&next_state->map, &next_wall_textures)) {
    free_wall_texture_set(&next_wall_textures);
    gloom_map_free(&next_state->map);
    return false;
  }

  if (!load_flat_texture_set_for_map_reusing(resolved, &next_flat_textures, flat_textures)) {
    free_wall_texture_set(&next_wall_textures);
    gloom_map_free(&next_state->map);
    return false;
  }

  if (!load_player_collision_radius(&next_state->player_radius)) {
    free_flat_texture_set(&next_flat_textures);
    free_wall_texture_set(&next_wall_textures);
    gloom_map_free(&next_state->map);
    return false;
  }

  if (!load_object_visual_set_for_map_reusing(&next_state->map, &next_object_visuals, object_visuals)) {
    free_flat_texture_set(&next_flat_textures);
    free_wall_texture_set(&next_wall_textures);
    gloom_map_free(&next_state->map);
    return false;
  }

  compute_map_bounds(next_state);
  if (!initialize_camera_from_map_spawn(next_state)) {
    fprintf(stderr, "No player spawn found in map event data for %s\n", resolved);
    free_object_visual_set(&next_object_visuals);
    free_flat_texture_set(&next_flat_textures);
    free_wall_texture_set(&next_wall_textures);
    gloom_map_free(&next_state->map);
    return false;
  }

  if (preserve_player) {
    restore_scriptplay_primary_player(next_state, preserved_hitpoints, preserved_lives, preserved_weapon,
                                      preserved_reload);
    if (two_player_mode) {
      restore_scriptplay_secondary_player(&next_state->player2, preserved_p2_hitpoints, preserved_p2_lives,
                                          preserved_p2_weapon, preserved_p2_reload);
    }
  }
  next_state->violence_mode = violence_mode;
  initialize_runtime_objects(next_state);

  if (next_state->map.zone_count > (size_t)GLOOM_MAX_RENDER_ZONES) {
    fprintf(stderr, "Cannot load %s: zone count %zu exceeds fixed render buffer capacity %u\n", resolved,
            next_state->map.zone_count, (unsigned)GLOOM_MAX_RENDER_ZONES);
    free_object_visual_set(&next_object_visuals);
    free_flat_texture_set(&next_flat_textures);
    free_wall_texture_set(&next_wall_textures);
    gloom_map_free(&next_state->map);
    return false;
  }

  if (next_state->map.zone_count > 0u) {
    memcpy(previous_zones, next_state->map.zones, next_state->map.zone_count * sizeof(*previous_zones));
    memcpy(render_zones, next_state->map.zones, next_state->map.zone_count * sizeof(*render_zones));
  }

  free_object_visual_set(object_visuals);
  free_flat_texture_set(flat_textures);
  free_wall_texture_set(wall_textures);
  gloom_map_free(&state->map);

  *state = *next_state;
  *wall_textures = next_wall_textures;
  *flat_textures = next_flat_textures;
  *object_visuals = next_object_visuals;
#ifdef GLOOM_DOS_SDL3
  dos_restore_game_index_palette_tables();
  rebuild_dos_level_index_mappings(wall_textures, flat_textures, object_visuals);
#endif

  if (resolved_map_path != NULL && resolved_map_path_size > 0u) {
    (void)snprintf(resolved_map_path, resolved_map_path_size, "%s", resolved);
  }

  printf("Loaded %s\n", resolved);
  printf("zones=%zu animations=%zu grid_bytes=%zu ppnt_bytes=%zu event_commands=%u object_spawns=%zu\n",
         state->map.zone_count, state->map.animation_count, state->map.grid_blob_size, state->map.ppnt_blob_size,
         total_event_commands(&state->map), state->map.object_spawn_count);
  printf("Camera spawn: x=%.0f z=%.0f angle=%.3f\n", state->camera_x, state->camera_z, state->camera_angle);
  return true;
}

#if GLOOM_RUNTIME_HAS_AUTOSAVE
static bool load_autosaved_runtime_game(bool two_player_save, SDL_Renderer *renderer, RenderFramebuffer *framebuffer,
                                        int render_width, int render_height, AppState *state,
                                        WallTextureSet *wall_textures, FlatTextureSet *flat_textures,
                                        ObjectVisualSet *object_visuals, GloomZone *previous_zones,
                                        GloomZone *render_zones, bool *io_two_player_mode, bool *io_combat_mode,
                                        uint8_t *io_violence_mode, char *resolved_map_buffer,
                                        size_t resolved_map_buffer_size, const char **io_resolved_map_path) {
  GloomAutosaveData save;
  ScriptLevelIntro intro;

  if (renderer == NULL || framebuffer == NULL || state == NULL || wall_textures == NULL || flat_textures == NULL ||
      object_visuals == NULL || previous_zones == NULL || render_zones == NULL || io_two_player_mode == NULL ||
      io_combat_mode == NULL || io_violence_mode == NULL || resolved_map_buffer == NULL || resolved_map_buffer_size == 0u ||
      io_resolved_map_path == NULL) {
    return false;
  }
  memset(&intro, 0, sizeof(intro));
  if (!read_gloom_autosave(two_player_save, &save)) {
    return false;
  }
  if (!resolve_script_level_intro_for_map(save.map_path, &intro) ||
      !run_script_intro_screen(renderer, framebuffer, &intro, render_width, render_height)) {
    fprintf(stderr, "Cannot load autosave %s: failed to show original script intro for saved map %s\n",
            gloom_autosave_path_for_mode(two_player_save), save.map_path);
    return false;
  }

  *io_two_player_mode = save.two_player_mode;
  *io_combat_mode = save.combat_mode;
  *io_violence_mode = save.violence_mode;
  state->combat_mode = save.combat_mode;

  if (!load_runtime_level(save.map_path, state, wall_textures, flat_textures, object_visuals, previous_zones,
                          render_zones, true, save.player_hitpoints, save.player_lives, save.player_weapon,
                          save.player_reload, save.player2_hitpoints, save.player2_lives, save.player2_weapon,
                          save.player2_reload, save.barrel_projectile_origin, save.two_player_mode,
                          save.violence_mode, resolved_map_buffer, resolved_map_buffer_size)) {
    fprintf(stderr, "Cannot load autosave %s: failed to load saved map %s\n",
            gloom_autosave_path_for_mode(two_player_save), save.map_path);
    return false;
  }

  *io_resolved_map_path = resolved_map_buffer;
  return true;
}

static bool autosave_selftest_expect(bool condition, const char *message) {
  if (!condition) {
    fprintf(stderr, "Autosave selftest failed: %s\n", message);
    return false;
  }
  return true;
}

static void fill_autosave_selftest_state(AppState *state, bool two_player_mode) {
  if (state == NULL) {
    return;
  }

  memset(state, 0, sizeof(*state));
  state->two_player_mode = two_player_mode;
  state->combat_mode = false;
  state->violence_mode = two_player_mode ? GLOOM_VIOLENCE_MESSY : GLOOM_VIOLENCE_MEATY_MESSY;
  state->barrel_projectile_origin = two_player_mode;
  state->player_hitpoints = two_player_mode ? 17 : 23;
  state->player_lives = two_player_mode ? 4 : 2;
  state->player_weapon = two_player_mode ? 3u : 1u;
  state->player_reload = two_player_mode ? 9u : 5u;
  state->player2.player_hitpoints = two_player_mode ? 11 : 0;
  state->player2.player_lives = two_player_mode ? 3 : 0;
  state->player2.player_weapon = two_player_mode ? 2u : 0u;
  state->player2.player_reload = two_player_mode ? 7u : 0u;
}

static bool autosave_selftest_matches_state(const GloomAutosaveData *save, const AppState *state,
                                            const char *map_path, uint32_t script_ordinal) {
  return save != NULL && state != NULL && strcmp(save->map_path, map_path) == 0 &&
         save->two_player_mode == state->two_player_mode && save->combat_mode == state->combat_mode &&
         save->violence_mode == state->violence_mode &&
         save->barrel_projectile_origin == state->barrel_projectile_origin &&
         save->player_hitpoints == state->player_hitpoints && save->player_lives == state->player_lives &&
         save->player_weapon == state->player_weapon && save->player_reload == state->player_reload &&
         save->player2_hitpoints == state->player2.player_hitpoints &&
         save->player2_lives == state->player2.player_lives &&
         save->player2_weapon == state->player2.player_weapon &&
         save->player2_reload == state->player2.player_reload && save->script_ordinal == script_ordinal;
}

static int run_autosave_selftest(void) {
  const char *one_player_map = "amiga/data/maps/map1_5";
  const char *two_player_map = "amiga/data/maps/map3_3";
  static AppState one_player_state;
  static AppState two_player_state;
  GloomAutosaveData one_player_save;
  GloomAutosaveData two_player_save;

  if (gloom_autosave_available(false) || gloom_autosave_available(true)) {
    fprintf(stderr, "Autosave selftest refused to overwrite existing %s or %s\n",
            GLOOM_AUTOSAVE_ONE_PLAYER_FILE, GLOOM_AUTOSAVE_TWO_PLAYER_FILE);
    return 1;
  }

  if (!autosave_selftest_expect(!gloom_should_autosave_script_ordinal(1u), "first level should not autosave")) return 1;
  if (!autosave_selftest_expect(!gloom_should_autosave_script_ordinal(2u), "second level should not autosave")) return 1;
  if (!autosave_selftest_expect(!gloom_should_autosave_script_ordinal(3u), "third level load should not autosave")) {
    return 1;
  }
  if (!autosave_selftest_expect(gloom_should_autosave_script_ordinal(4u),
                                "next map after three completed levels should autosave")) {
    return 1;
  }
  if (!autosave_selftest_expect(gloom_should_autosave_script_ordinal(7u),
                                "next map after six completed levels should autosave")) {
    return 1;
  }

  fill_autosave_selftest_state(&one_player_state, false);
  fill_autosave_selftest_state(&two_player_state, true);

  if (!write_gloom_autosave(&one_player_state, one_player_map, 5u)) return 1;
  if (!autosave_selftest_expect(gloom_autosave_available(false), "one-player autosave record was not created")) return 1;
  if (!autosave_selftest_expect(!gloom_autosave_available(true), "one-player autosave created two-player file")) return 1;
  if (!read_gloom_autosave(false, &one_player_save)) return 1;
  if (!autosave_selftest_expect(autosave_selftest_matches_state(&one_player_save, &one_player_state,
                                                                one_player_map, 5u),
                                "one-player autosave did not round-trip")) {
    return 1;
  }

  if (!write_gloom_autosave(&two_player_state, two_player_map, 10u)) return 1;
  if (!autosave_selftest_expect(gloom_autosave_available(false), "two-player write removed one-player file")) return 1;
  if (!autosave_selftest_expect(gloom_autosave_available(true), "two-player autosave record was not created")) return 1;
  if (!read_gloom_autosave(false, &one_player_save)) return 1;
  if (!read_gloom_autosave(true, &two_player_save)) return 1;
  if (!autosave_selftest_expect(autosave_selftest_matches_state(&one_player_save, &one_player_state,
                                                                one_player_map, 5u),
                                "two-player write changed one-player autosave")) {
    return 1;
  }
  if (!autosave_selftest_expect(autosave_selftest_matches_state(&two_player_save, &two_player_state,
                                                                two_player_map, 10u),
                                "two-player autosave did not round-trip")) {
    return 1;
  }

  gloom_autosave_delete(false);
  gloom_autosave_delete(true);
  printf("Autosave selftest passed: separate one-player and two-player saves round-trip independently\n");
  return 0;
}
#endif

static int run_level_transition_selftest(void) {
  static const char *expected_play_chain[] = {"amiga/maps/map1_1",      "amiga/data/maps/map1_2",
                                             "amiga/data/maps/map1_3", "amiga/data/maps/map1_4",
                                             "amiga/data/maps/map1_5", "amiga/data/maps/map1_6",
                                             "amiga/data/maps/map1_7", "amiga/data/maps/map3_1",
                                             "amiga/data/maps/map3_2", "amiga/data/maps/map3_3",
                                             "amiga/data/maps/map3_4", "amiga/data/maps/map3_5",
                                             "amiga/data/maps/map3_6", "amiga/data/maps/map3_7",
                                             "amiga/data/maps/map4_1", "amiga/data/maps/map4_2",
                                             "amiga/data/maps/map4_3", "amiga/data/maps/map4_4",
                                             "amiga/data/maps/map4_5", "amiga/data/maps/map4_6",
                                             "amiga/data/maps/map4_7"};
  static const char *combat_map_chain[] = {"amiga/data/maps/com1_1", "amiga/data/maps/com1_2",
                                           "amiga/data/maps/com1_3", "amiga/data/maps/com1_4",
                                           "amiga/data/maps/com1_5", "amiga/data/maps/com1_6",
                                           "amiga/data/maps/com1_7", "amiga/data/maps/com2_1",
                                           "amiga/data/maps/com2_2", "amiga/data/maps/com2_3",
                                           "amiga/data/maps/com2_4", "amiga/data/maps/com2_5",
                                           "amiga/data/maps/com2_6", "amiga/data/maps/com2_7",
                                           "amiga/data/maps/com3_1", "amiga/data/maps/com3_2",
                                           "amiga/data/maps/com3_3", "amiga/data/maps/com3_4",
                                           "amiga/data/maps/com3_5", "amiga/data/maps/com3_6",
                                           "amiga/data/maps/com3_7"};
  AppState *state = NULL;
  WallTextureSet *wall_textures = NULL;
  FlatTextureSet *flat_textures = NULL;
  ObjectVisualSet *object_visuals = NULL;
  GloomZone *previous_zones = NULL;
  GloomZone *render_zones = NULL;
  char next_map_path[1024] = {0};
  char resolved_map_path[1024] = {0};
  ScriptCompletion completion;
  ScriptLevelIntro intro;
  MenuImage completion_image;
  MenuImage intro_image;
  int16_t preserved_lives = 1;
  int16_t preserved_p2_lives = 3;
  size_t chain_index = 0u;
  size_t combat_index = 0u;
  int result = 1;

  memset(&completion, 0, sizeof(completion));
  memset(&intro, 0, sizeof(intro));
  memset(&completion_image, 0, sizeof(completion_image));
  memset(&intro_image, 0, sizeof(intro_image));
  state = (AppState *)calloc(1u, sizeof(*state));
  wall_textures = (WallTextureSet *)calloc(1u, sizeof(*wall_textures));
  flat_textures = (FlatTextureSet *)calloc(1u, sizeof(*flat_textures));
  object_visuals = (ObjectVisualSet *)calloc(1u, sizeof(*object_visuals));
  previous_zones = (GloomZone *)malloc((size_t)GLOOM_MAX_RENDER_ZONES * sizeof(*previous_zones));
  render_zones = (GloomZone *)malloc((size_t)GLOOM_MAX_RENDER_ZONES * sizeof(*render_zones));
  if (state == NULL || wall_textures == NULL || flat_textures == NULL || object_visuals == NULL ||
      previous_zones == NULL || render_zones == NULL) {
    fprintf(stderr, "Level transition selftest failed: out of memory preparing fixed zone buffers\n");
    goto cleanup;
  }

  if (!resolve_next_script_play_map("amiga/maps/map1_1", next_map_path, sizeof(next_map_path))) {
    fprintf(stderr, "Level transition selftest failed: script did not resolve the map after map1_1\n");
    goto cleanup;
  }
  if (strcmp(next_map_path, "amiga/data/maps/map1_2") != 0) {
    fprintf(stderr, "Level transition selftest failed: map1_1 advanced to %s instead of amiga/data/maps/map1_2\n",
            next_map_path);
    goto cleanup;
  }
  if (!resolve_script_level_intro_for_map("amiga/maps/map1_1", &intro) ||
      strcmp(intro.picture, "spacehulk") != 0 ||
      strcmp(intro.text, "simple stuff!\\watch out for ambushes though...") != 0) {
    fprintf(stderr, "Level transition selftest failed: original map1_1 intro picture/text was not resolved\n");
    goto cleanup;
  }
  if (!resolve_script_level_intro_for_map(next_map_path, &intro) ||
      strcmp(intro.picture, "spacehulk") != 0 ||
      strcmp(intro.text, "stay alert! watch out for aggro skinheads!") != 0) {
    fprintf(stderr, "Level transition selftest failed: original map1_2 intro picture/text was not resolved\n");
    goto cleanup;
  }
  if (!resolve_script_level_intro_for_map("amiga/data/maps/map3_1", &intro) ||
      strcmp(intro.picture, "gothic") != 0 ||
      strcmp(intro.text, "raptor madness in the gothic tomb!") != 0) {
    fprintf(stderr, "Level transition selftest failed: original map3_1 intro picture/text was not resolved\n");
    goto cleanup;
  }
  if (!resolve_script_level_intro_for_map("amiga/data/maps/map4_1", &intro) ||
      strcmp(intro.picture, "hell") != 0 ||
      strcmp(intro.text, "welcome to hell!") != 0) {
    fprintf(stderr, "Level transition selftest failed: original map4_1 intro picture/text was not resolved\n");
    goto cleanup;
  }
  if (resolve_next_script_play_map_or_done("amiga/maps/map4_7", next_map_path, sizeof(next_map_path), &completion) !=
          SCRIPT_PLAY_NEXT_DONE ||
      strcmp(completion.picture, "theend") != 0 ||
      strcmp(completion.text, "congratulations! you have completed gloom!") != 0) {
    fprintf(stderr, "Level transition selftest failed: scriptdone completion picture/text were not resolved\n");
    goto cleanup;
  }
  if (!load_script_picture_image(completion.picture, &completion_image)) {
    fprintf(stderr, "Level transition selftest failed: original completion picture could not be loaded\n");
    goto cleanup;
  }
  {
    static const char *expected_intro_pictures[] = {"spacehulk", "gothic", "hell"};
    size_t picture_index = 0u;

    for (picture_index = 0u; picture_index < sizeof(expected_intro_pictures) / sizeof(expected_intro_pictures[0]);
         ++picture_index) {
      if (!load_script_picture_image(expected_intro_pictures[picture_index], &intro_image)) {
        fprintf(stderr, "Level transition selftest failed: original pict_%s image could not be loaded\n",
                expected_intro_pictures[picture_index]);
        goto cleanup;
      }
    }
  }

  for (chain_index = 0u; chain_index + 1u < sizeof(expected_play_chain) / sizeof(expected_play_chain[0]);
       ++chain_index) {
    if (!resolve_next_script_play_map(expected_play_chain[chain_index], next_map_path, sizeof(next_map_path))) {
      fprintf(stderr, "Level transition selftest failed: script did not resolve after %s\n",
              expected_play_chain[chain_index]);
      goto cleanup;
    }
    if (strcmp(next_map_path, expected_play_chain[chain_index + 1u]) != 0) {
      fprintf(stderr, "Level transition selftest failed: %s advanced to %s instead of %s\n",
              expected_play_chain[chain_index], next_map_path, expected_play_chain[chain_index + 1u]);
      goto cleanup;
    }
  }

  share_levelover_two_player_lives(&preserved_lives, &preserved_p2_lives);
  if (!load_runtime_level("amiga/data/maps/map1_2", state, wall_textures, flat_textures, object_visuals, previous_zones,
                          render_zones, true, 11, preserved_lives, 2u, 0u, 0, preserved_p2_lives, 4u, 2u, true,
                          true, GLOOM_VIOLENCE_MEATY_MESSY, resolved_map_path, sizeof(resolved_map_path))) {
    fprintf(stderr, "Level transition selftest failed: could not load scriptplay next map amiga/data/maps/map1_2\n");
    goto cleanup;
  }

  if (!state->two_player_mode || state->player_lives != 3 || state->player2.player_lives != 3) {
    fprintf(stderr, "Level transition selftest failed: levelover shared lives were not restored to both players\n");
    goto cleanup;
  }
  if (state->player_hitpoints != 11 || state->player_weapon != 2u || state->player_reload != 0u) {
    fprintf(stderr, "Level transition selftest failed: player one scriptplay state was not preserved exactly\n");
    goto cleanup;
  }
  if (state->player2.player_hitpoints != GLOOM_PLAYER_INITIAL_HEALTH || state->player2.player_weapon != 4u ||
      state->player2.player_reload != (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD) {
    fprintf(stderr, "Level transition selftest failed: player two scriptplay zero-health reset was not applied\n");
    goto cleanup;
  }
  if (state->finished != 0 || state->finished2 != 0 || state->player_pixsizeadd != 0 ||
      state->player2.player_pixsizeadd != 0) {
    fprintf(stderr, "Level transition selftest failed: resetplayer/scriptplay transition flags were not cleared\n");
    goto cleanup;
  }

  for (chain_index = 2u; chain_index < sizeof(expected_play_chain) / sizeof(expected_play_chain[0]); ++chain_index) {
    if (!load_runtime_level(expected_play_chain[chain_index], state, wall_textures, flat_textures, object_visuals,
                            previous_zones, render_zones, true, state->player_hitpoints, state->player_lives,
                            state->player_weapon, state->player_reload, state->player2.player_hitpoints,
                            state->player2.player_lives, state->player2.player_weapon, state->player2.player_reload,
                            true, true, GLOOM_VIOLENCE_MEATY_MESSY, resolved_map_path, sizeof(resolved_map_path))) {
      fprintf(stderr, "Level transition selftest failed: could not load scriptplay chain map %s\n",
              expected_play_chain[chain_index]);
      goto cleanup;
    }
  }

  state->combat_mode = true;
  initialize_combat_rotation_state(state, 1u, 3);
  for (combat_index = 0u; combat_index < sizeof(combat_map_chain) / sizeof(combat_map_chain[0]); ++combat_index) {
    if (!load_runtime_level(combat_map_chain[combat_index], state, wall_textures, flat_textures, object_visuals,
                            previous_zones, render_zones, true, GLOOM_PLAYER_INITIAL_HEALTH, 3, 0u,
                            (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD, GLOOM_PLAYER_INITIAL_HEALTH, 3, 0u,
                            (uint8_t)GLOOM_PLAYER_INITIAL_RELOAD, true, true, GLOOM_VIOLENCE_MEATY_MESSY,
                            resolved_map_path, sizeof(resolved_map_path))) {
      fprintf(stderr, "Level transition selftest failed: could not load combat map %s\n", combat_map_chain[combat_index]);
      goto cleanup;
    }
  }

  printf("Level transition selftest passed: loaded %u campaign maps and %u combat maps\n",
         (unsigned)(sizeof(expected_play_chain) / sizeof(expected_play_chain[0])),
         (unsigned)(sizeof(combat_map_chain) / sizeof(combat_map_chain[0])));
  result = 0;

cleanup:
  free_menu_image(&completion_image);
  free_menu_image(&intro_image);
  free_object_visual_set(object_visuals);
  free_flat_texture_set(flat_textures);
  free_wall_texture_set(wall_textures);
  if (state != NULL) {
    gloom_map_free(&state->map);
  }
  free(object_visuals);
  free(flat_textures);
  free(wall_textures);
  free(state);
  free(previous_zones);
  free(render_zones);
  return result;
}

static int run_completion_selftest(void) {
  char next_map_path[1024] = {0};
  ScriptCompletion completion;
  MenuImage image;
  HudFont font;
  int result = 1;

  memset(&completion, 0, sizeof(completion));
  memset(&image, 0, sizeof(image));
  memset(&font, 0, sizeof(font));

  if (resolve_next_script_play_map_or_done("amiga/data/maps/map4_7", next_map_path, sizeof(next_map_path),
                                           &completion) != SCRIPT_PLAY_NEXT_DONE) {
    fprintf(stderr, "Completion selftest failed: map4_7 did not advance to original done_ script command\n");
    return 1;
  }
  if (strcmp(completion.picture, "theend") != 0 ||
      strcmp(completion.text, "congratulations! you have completed gloom!") != 0) {
    fprintf(stderr, "Completion selftest failed: original pict_theend/text_ completion commands were not resolved\n");
    return 1;
  }
  if (!load_script_picture_image(completion.picture, &image)) {
    fprintf(stderr, "Completion selftest failed: original pict_theend image could not be loaded\n");
    return 1;
  }
  if (!load_menu_big_font(&font)) {
    fprintf(stderr, "Completion selftest failed: original bigfont asset could not be loaded\n");
    free_menu_image(&image);
    return 1;
  }

#ifndef GLOOM_DOS_SDL3
  {
    enum { TEST_W = 320, TEST_H = 200 };
    RenderFramebuffer text_framebuffer;
    RenderFramebuffer image_framebuffer;
    uint32_t *text_pixels = NULL;
    uint32_t *image_pixels = NULL;
    size_t pixel_count = (size_t)TEST_W * (size_t)TEST_H;
    size_t i = 0u;
    size_t changed_pixels = 0u;

    memset(&text_framebuffer, 0, sizeof(text_framebuffer));
    memset(&image_framebuffer, 0, sizeof(image_framebuffer));
    text_pixels = (uint32_t *)calloc(pixel_count, sizeof(*text_pixels));
    image_pixels = (uint32_t *)calloc(pixel_count, sizeof(*image_pixels));
    if (text_pixels == NULL || image_pixels == NULL) {
      fprintf(stderr, "Completion selftest failed: out of memory preparing render buffers\n");
      free(text_pixels);
      free(image_pixels);
      goto cleanup;
    }

    text_framebuffer.pixels = text_pixels;
    text_framebuffer.pitch_pixels = TEST_W;
    text_framebuffer.width = TEST_W;
    text_framebuffer.height = TEST_H;
    image_framebuffer.pixels = image_pixels;
    image_framebuffer.pitch_pixels = TEST_W;
    image_framebuffer.width = TEST_W;
    image_framebuffer.height = TEST_H;

    framebuffer_clear(&image_framebuffer, 0xFF000000u);
    render_menu_image(&image_framebuffer, &image, TEST_W, TEST_H);
    render_completion_frame(&text_framebuffer, &image, &font, completion.text, TEST_W, TEST_H);

    for (i = 0u; i < pixel_count; ++i) {
      if (text_pixels[i] != image_pixels[i]) {
        changed_pixels += 1u;
      }
    }
    free(text_pixels);
    free(image_pixels);
    if (changed_pixels == 0u) {
      fprintf(stderr, "Completion selftest failed: completion text did not render over pict_theend\n");
      goto cleanup;
    }
  }
#endif

  printf("Completion selftest passed: map4_7 resolves pict_theend/text_/done_ and renders completion screen\n");
  result = 0;

#ifndef GLOOM_DOS_SDL3
cleanup:
#endif
  free_hud_font(&font);
  free_menu_image(&image);
  return result;
}

static bool run_menu_and_restart_game(SDL_Renderer *renderer, RenderFramebuffer *framebuffer, int render_width,
                                      int render_height, AppState *state, WallTextureSet *wall_textures,
                                      FlatTextureSet *flat_textures, ObjectVisualSet *object_visuals,
                                      GloomZone *previous_zones, GloomZone *render_zones, bool barrel_projectile_origin,
                                      bool *io_two_player_mode, bool *io_combat_mode, uint8_t *io_violence_mode,
                                      char *resolved_map_buffer, size_t resolved_map_buffer_size,
                                      const char **io_resolved_map_path) {
  MenuAssets menu_assets;
  GloomGameMode menu_game_mode = GLOOM_GAME_MODE_ONE_PLAYER;
  uint8_t menu_violence_mode = GLOOM_VIOLENCE_MEATY_MESSY;
  char first_map_path[1024] = "amiga/maps/map1_1";

  memset(&menu_assets, 0, sizeof(menu_assets));
  if (renderer == NULL || framebuffer == NULL || state == NULL || wall_textures == NULL || flat_textures == NULL ||
      object_visuals == NULL || previous_zones == NULL || render_zones == NULL || io_two_player_mode == NULL ||
      io_combat_mode == NULL || io_violence_mode == NULL || resolved_map_buffer == NULL ||
      resolved_map_buffer_size == 0u || io_resolved_map_path == NULL) {
    return false;
  }

  menu_game_mode = *io_combat_mode ? GLOOM_GAME_MODE_COMBAT
                                   : (*io_two_player_mode ? GLOOM_GAME_MODE_TWO_PLAYER : GLOOM_GAME_MODE_ONE_PLAYER);
  menu_violence_mode = *io_violence_mode;
  if (!load_menu_assets(&menu_assets)) {
    fprintf(stderr, "Failed to load original menu assets\n");
    return false;
  }
  if (run_start_menu(renderer, framebuffer, &menu_assets, render_width, render_height, &menu_game_mode,
                     &menu_violence_mode, &g_control_config) != 0) {
    free_menu_assets(&menu_assets);
    return false;
  }
#ifdef GLOOM_DOS_SDL3
  refresh_dos_index_palette(framebuffer, gloom_dos_window);
#endif
  free_menu_assets(&menu_assets);

  *io_two_player_mode = menu_game_mode != GLOOM_GAME_MODE_ONE_PLAYER;
  *io_combat_mode = menu_game_mode == GLOOM_GAME_MODE_COMBAT;
  *io_violence_mode = menu_violence_mode;
  state->combat_mode = *io_combat_mode;
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  if (menu_game_mode == GLOOM_GAME_MODE_LOAD_ONE_PLAYER || menu_game_mode == GLOOM_GAME_MODE_LOAD_TWO_PLAYER) {
    bool load_two_player = menu_game_mode == GLOOM_GAME_MODE_LOAD_TWO_PLAYER;

    if (!load_autosaved_runtime_game(load_two_player, renderer, framebuffer, render_width, render_height, state,
                                     wall_textures, flat_textures, object_visuals, previous_zones, render_zones,
                                     io_two_player_mode, io_combat_mode, io_violence_mode, resolved_map_buffer,
                                     resolved_map_buffer_size, io_resolved_map_path)) {
      return false;
    }
    return true;
  }
#endif
  if (*io_combat_mode) {
    uint8_t series = 1u;
    int16_t lives = 3;

    if (run_combat_menu(renderer, framebuffer, render_width, render_height, &series, &lives) != 0) {
      return false;
    }
#ifdef GLOOM_DOS_SDL3
    refresh_dos_index_palette(framebuffer, gloom_dos_window);
#endif
    initialize_combat_rotation_state(state, series, lives);
    if (!resolve_next_combat_map(state, first_map_path, sizeof(first_map_path))) {
      fprintf(stderr, "Failed to pick original combat map\n");
      return false;
    }
  }
  if (!*io_combat_mode) {
    ScriptLevelIntro intro;

    if (!resolve_script_level_intro_for_map(first_map_path, &intro) ||
        !run_script_intro_screen(renderer, framebuffer, &intro, render_width, render_height)) {
      return false;
    }
#ifdef GLOOM_DOS_SDL3
    refresh_dos_index_palette(framebuffer, gloom_dos_window);
#endif
  }
  if (!load_runtime_level(first_map_path, state, wall_textures, flat_textures, object_visuals, previous_zones,
                          render_zones, false, 0, 0, 0u, 0u, 0, 0, 0u, 0u, barrel_projectile_origin,
                          *io_two_player_mode, menu_violence_mode, resolved_map_buffer, resolved_map_buffer_size)) {
    fprintf(stderr, "Failed to restart game from original selected map after menu selection\n");
    return false;
  }

  *io_resolved_map_path = resolved_map_buffer;
  return true;
}

#ifdef __EMSCRIPTEN__
EM_JS(void, runtime_emscripten_canvas_cursor_default, (void), {
  var c = Module['canvas'];
  if (c) c.style.cursor = 'default';
});

EM_JS(void, runtime_emscripten_install_pointer_lock_listener, (void), {
  Module['gloomPointerLockLost'] = 0;
  document.addEventListener('pointerlockchange', function() {
    if (!document.pointerLockElement && Module['canvas']) {
      Module['gloomPointerLockLost'] = 1;
    }
  });
});

EM_JS(void, runtime_emscripten_install_fullscreen_listeners, (void), {
  function gloomToggleCanvasFullscreen() {
    var c = Module['canvas'];
    if (!c) return;
    var el = document.fullscreenElement || document.webkitFullscreenElement ||
             document.mozFullScreenElement || document.msFullscreenElement;
    if (el === c) {
      var exit = document.exitFullscreen || document.webkitExitFullscreen ||
                 document.mozCancelFullScreen || document.msExitFullscreen;
      if (exit) exit.call(document);
    } else {
      var request = c.requestFullscreen || c.webkitRequestFullscreen ||
                    c.mozRequestFullScreen || c.msRequestFullscreen;
      if (request) {
        var result = request.call(c);
        if (result && result.catch) result.catch(function() {});
      }
    }
  }
  Module['gloomToggleCanvasFullscreen'] = gloomToggleCanvasFullscreen;
  Module['gloomFullscreenResizePending'] = 0;
  var mark = function() {
    Module['gloomFullscreenResizePending'] = 1;
    if (typeof window.triggerResize === 'function') window.triggerResize();
  };
  document.addEventListener('fullscreenchange', mark, false);
  document.addEventListener('webkitfullscreenchange', mark, false);
  window.addEventListener('keydown', function(e) {
    if (e.code !== 'F11' && e.key !== 'F11' && e.keyCode !== 122) return;
    e.preventDefault();
    e.stopPropagation();
    gloomToggleCanvasFullscreen();
  }, true);
});

EM_JS(void, runtime_emscripten_canvas_fullscreen_toggle, (void), {
  if (Module['gloomToggleCanvasFullscreen']) Module['gloomToggleCanvasFullscreen']();
});

EM_JS(void, runtime_emscripten_hide_loading_overlay, (void), {
  if (typeof window.gloomHideLoadingOverlay === 'function') window.gloomHideLoadingOverlay();
});

EM_JS(int, runtime_emscripten_take_pointer_lock_lost, (void), {
  var v = Module['gloomPointerLockLost'] | 0;
  Module['gloomPointerLockLost'] = 0;
  return v;
});

EM_JS(int, runtime_emscripten_take_fullscreen_resize, (void), {
  var v = Module['gloomFullscreenResizePending'] | 0;
  Module['gloomFullscreenResizePending'] = 0;
  return v;
});

EM_JS(int, runtime_emscripten_screen_width, (void), {
  var w = (typeof screen !== 'undefined' && screen.width) ? screen.width : 0;
  if (w < 1 && typeof window !== 'undefined') w = window.innerWidth || 0;
  return w | 0;
});

EM_JS(int, runtime_emscripten_screen_height, (void), {
  var h = (typeof screen !== 'undefined' && screen.height) ? screen.height : 0;
  if (h < 1 && typeof window !== 'undefined') h = window.innerHeight || 0;
  return h | 0;
});

static bool runtime_emscripten_consume_pointer_lock_lost(void) {
  return runtime_emscripten_take_pointer_lock_lost() != 0;
}

static void runtime_emscripten_discard_pointer_lock_lost(void) {
  (void)runtime_emscripten_take_pointer_lock_lost();
}

static bool runtime_emscripten_consume_fullscreen_resize(void) {
  return runtime_emscripten_take_fullscreen_resize() != 0;
}
#else
void runtime_emscripten_canvas_cursor_default(void) {
}

void runtime_emscripten_install_pointer_lock_listener(void) {
}

void runtime_emscripten_install_fullscreen_listeners(void) {
}

void runtime_emscripten_canvas_fullscreen_toggle(void) {
}

void runtime_emscripten_hide_loading_overlay(void) {
}

static bool runtime_emscripten_consume_pointer_lock_lost(void) {
  return false;
}

static void runtime_emscripten_discard_pointer_lock_lost(void) {
}

static bool runtime_emscripten_consume_fullscreen_resize(void) {
  return false;
}
#endif

#ifndef GLOOM_DOS_SDL3
static void select_aspect_720p_resolution(int *io_width, int *io_height) {
  static const int common_aspects[][2] = {{16, 9}, {16, 10}, {3, 2}, {4, 3}, {21, 9}, {32, 9}};
  static const int aspect_720p[][2] = {{1280, 720}, {1152, 720}, {1080, 720}, {960, 720}, {1680, 720}, {2560, 720}};
  int display_width = 0;
  int display_height = 0;
  int best_index = 0;
  int best_diff_num = 0x7fffffff;
  size_t i = 0u;

  if (io_width == NULL || io_height == NULL) {
    return;
  }

#ifdef __EMSCRIPTEN__
  display_width = runtime_emscripten_screen_width();
  display_height = runtime_emscripten_screen_height();
#else
  {
    SDL_DisplayMode mode;

    memset(&mode, 0, sizeof(mode));
    if (SDL_GetCurrentDisplayMode(0, &mode) == 0 && mode.w > 0 && mode.h > 0) {
      display_width = mode.w;
      display_height = mode.h;
    }
  }
#endif

  if (display_width <= 0 || display_height <= 0) {
    display_width = DEFAULT_WINDOW_WIDTH;
    display_height = DEFAULT_WINDOW_HEIGHT;
  }

  for (i = 0u; i < sizeof(common_aspects) / sizeof(common_aspects[0]); ++i) {
    int aspect_w = common_aspects[i][0];
    int aspect_h = common_aspects[i][1];
    int64_t diff = ((int64_t)display_width * (int64_t)aspect_h) - ((int64_t)display_height * (int64_t)aspect_w);
    int64_t denom = (int64_t)display_height * (int64_t)aspect_w;
    int normalized_diff = 0;

    if (diff < 0) {
      diff = -diff;
    }
    if (denom <= 0) {
      continue;
    }
    normalized_diff = (int)((diff * 1000000) / denom);
    if (normalized_diff < best_diff_num) {
      best_diff_num = normalized_diff;
      best_index = (int)i;
    }
  }

  *io_width = aspect_720p[best_index][0];
  *io_height = aspect_720p[best_index][1];
}
#endif

static void runtime_force_cursor_visible(void) {
  int n = 0;

  while (SDL_ShowCursor(SDL_QUERY) == SDL_DISABLE && n++ < 16) {
    (void)SDL_ShowCursor(SDL_ENABLE);
  }
}

#ifdef GLOOM_DOS_SDL3
static void dos_runtime_reset_absolute_mouse_capture(void) {
  g_dos_mouse_absolute_capture = false;
  g_dos_mouse_absolute_valid = false;
  g_dos_mouse_absolute_x = 0.0f;
}

static void dos_runtime_set_mouse_pump_enabled(bool enabled) {
  SDL_SetHint("SDL_DOS_SKIP_MOUSE_PUMP", enabled ? "0" : "1");
  SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, enabled);
  SDL_SetEventEnabled(SDL_EVENT_MOUSE_BUTTON_DOWN, enabled);
  SDL_SetEventEnabled(SDL_EVENT_MOUSE_BUTTON_UP, enabled);
}

static void dos_runtime_center_mouse(SDL_Window *window) {
  int window_width = 0;
  int window_height = 0;

  if (window == NULL) {
    return;
  }
  if (!SDL_GetWindowSize(window, &window_width, &window_height) || window_width <= 0 || window_height <= 0) {
    return;
  }
  SDL_WarpMouseInWindow(window, (float)(window_width / 2), (float)(window_height / 2));
  g_dos_mouse_absolute_valid = false;
}

static double dos_runtime_mouse_motion_delta(SDL_Window *window, const SDL_MouseMotionEvent *motion) {
  double dx = 0.0;

  if (motion == NULL) {
    return 0.0;
  }
  if (motion->xrel != 0.0f) {
    return (double)motion->xrel;
  }
  if (!g_dos_mouse_absolute_capture) {
    return 0.0;
  }
  if (g_dos_mouse_absolute_valid) {
    dx = (double)(motion->x - g_dos_mouse_absolute_x);
  }
  g_dos_mouse_absolute_x = motion->x;
  g_dos_mouse_absolute_valid = true;
  dos_runtime_center_mouse(window);
  return dx;
}
#endif

static bool set_runtime_mouse_capture(SDL_Window *window, bool captured) {
  if (window == NULL) {
    return false;
  }

  if (captured) {
#ifdef GLOOM_DOS_SDL3
    dos_runtime_set_mouse_pump_enabled(true);
    dos_logf("DOS mouse: using absolute motion capture");
    g_dos_mouse_absolute_capture = true;
    g_dos_mouse_absolute_valid = false;
    dos_runtime_center_mouse(window);
#else
    if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
      fprintf(stderr, "SDL_SetRelativeMouseMode(SDL_TRUE) failed: %s\n", SDL_GetError());
      return false;
    }
#endif
    SDL_SetWindowGrab(window, SDL_TRUE);
    g_runtime_mouse_capture_active = true;
    (void)SDL_ShowCursor(SDL_DISABLE);
  } else {
#ifndef GLOOM_DOS_SDL3
    if (SDL_SetRelativeMouseMode(SDL_FALSE) != 0) {
      fprintf(stderr, "SDL_SetRelativeMouseMode(SDL_FALSE) failed: %s\n", SDL_GetError());
    }
#endif
#ifdef GLOOM_DOS_SDL3
    dos_runtime_reset_absolute_mouse_capture();
    dos_runtime_set_mouse_pump_enabled(false);
#endif
    (void)SDL_CaptureMouse(SDL_FALSE);
    SDL_SetWindowGrab(window, SDL_FALSE);
    g_runtime_mouse_capture_active = false;
    (void)SDL_ShowCursor(SDL_ENABLE);
    runtime_force_cursor_visible();
#ifdef __EMSCRIPTEN__
    emscripten_run_script("if (document.pointerLockElement) document.exitPointerLock();");
#endif
    runtime_emscripten_canvas_cursor_default();
  }

  return true;
}

static bool set_gameplay_start_mouse_capture(SDL_Window *window) {
  bool captured = false;

  runtime_emscripten_discard_pointer_lock_lost();
  captured = set_runtime_mouse_capture(window, true);
  runtime_emscripten_discard_pointer_lock_lost();
  return captured;
}

static RuntimeDisplayIniConfig g_runtime_display_ini;

static char *trim_ini_token(char *text) {
  char *end = NULL;

  if (text == NULL) {
    return NULL;
  }
  while (*text != '\0' && isspace((unsigned char)*text)) {
    ++text;
  }
  end = text + strlen(text);
  while (end > text && isspace((unsigned char)end[-1])) {
    --end;
  }
  *end = '\0';
  return text;
}

static bool parse_ini_resolution_value(const char *value, int min_width, int max_width, int min_height, int max_height,
                                       int *out_width, int *out_height) {
  char *end = NULL;
  long parsed_width = 0;
  long parsed_height = 0;

  if (value == NULL || out_width == NULL || out_height == NULL) {
    return false;
  }
  parsed_width = strtol(value, &end, 10);
  if ((*end != 'x' && *end != 'X') || parsed_width < min_width || parsed_width > max_width) {
    return false;
  }
  parsed_height = strtol(end + 1, &end, 10);
  if (*end != '\0' || parsed_height < min_height || parsed_height > max_height) {
    return false;
  }
  *out_width = (int)parsed_width;
  *out_height = (int)parsed_height;
  return true;
}

static bool parse_ini_pixel_size_value(const char *value, int *out_width, int *out_height) {
  char *end = NULL;
  long parsed_width = 0;
  long parsed_height = 0;

  if (value == NULL || out_width == NULL || out_height == NULL) {
    return false;
  }
  parsed_width = strtol(value, &end, 10);
  if (*end == '\0') {
    parsed_height = parsed_width;
  } else if (*end == 'x' || *end == 'X') {
    parsed_height = strtol(end + 1, &end, 10);
    if (*end != '\0') {
      return false;
    }
  } else {
    return false;
  }
  if (parsed_width < 1 || parsed_width > 2 || parsed_height < 1 || parsed_height > 2) {
    return false;
  }
  *out_width = (int)parsed_width;
  *out_height = (int)parsed_height;
  return true;
}

static void load_runtime_display_ini(RuntimeDisplayIniConfig *config) {
  static const char *candidates[] = {"GLOOM.INI", "gloom.ini"};
  FILE *file = NULL;
  size_t candidate_index = 0u;
  char line[256];

  if (config == NULL) {
    return;
  }
  memset(config, 0, sizeof(*config));
  config->gameplay_pixel_width = 1;
  config->gameplay_pixel_height = 1;

#ifdef GLOOM_DOS_SDL3
  for (candidate_index = 0u; candidate_index < sizeof(candidates) / sizeof(candidates[0]); ++candidate_index) {
    file = fopen(candidates[candidate_index], "r");
    if (file != NULL) {
      break;
    }
  }
#else
  (void)candidates;
  (void)candidate_index;
#endif

  if (file == NULL) {
    return;
  }

  while (fgets(line, sizeof(line), file) != NULL) {
    char *key = trim_ini_token(line);
    char *value = NULL;
    char *comment = NULL;
    int parsed_width = 0;
    int parsed_height = 0;

    if (key == NULL || key[0] == '\0' || key[0] == '#' || key[0] == ';' || key[0] == '[') {
      continue;
    }
    comment = strchr(key, ';');
    if (comment != NULL) {
      *comment = '\0';
    }
    comment = strchr(key, '#');
    if (comment != NULL) {
      *comment = '\0';
    }
    value = strchr(key, '=');
    if (value == NULL) {
      continue;
    }
    *value++ = '\0';
    key = trim_ini_token(key);
    value = trim_ini_token(value);
    if (key == NULL || value == NULL || key[0] == '\0' || value[0] == '\0') {
      continue;
    }

    if (strcmp(key, "resolution") == 0 || strcmp(key, "screen_resolution") == 0 ||
        strcmp(key, "window_size") == 0 || strcmp(key, "boot_resolution") == 0) {
      if (parse_ini_resolution_value(value, 320, 7680, 200, 4320, &parsed_width, &parsed_height)) {
        config->has_resolution = true;
        config->resolution_width = parsed_width;
        config->resolution_height = parsed_height;
      } else {
        fprintf(stderr, "Ignoring GLOOM.INI %s=%s; expected WIDTHxHEIGHT within CLI resolution limits\n", key, value);
      }
    } else if (strcmp(key, "gameplay_viewport") == 0 || strcmp(key, "viewport_size") == 0 ||
               strcmp(key, "reduced_window_size") == 0 || strcmp(key, "amiga_window_size") == 0) {
      if (parse_ini_resolution_value(value, 64, BASE_WIDTH, 64, BASE_HEIGHT, &parsed_width, &parsed_height)) {
        config->has_gameplay_viewport = true;
        config->gameplay_viewport_width = parsed_width;
        config->gameplay_viewport_height = parsed_height;
      } else {
        fprintf(stderr, "Ignoring GLOOM.INI %s=%s; expected WIDTHxHEIGHT from 64x64 through 320x224\n", key,
                value);
      }
    } else if (strcmp(key, "gameplay_pixel_size") == 0 || strcmp(key, "viewport_pixel_size") == 0 ||
               strcmp(key, "chixel_size") == 0 || strcmp(key, "pixel_size") == 0) {
      if (parse_ini_pixel_size_value(value, &parsed_width, &parsed_height)) {
        config->gameplay_pixel_width = parsed_width;
        config->gameplay_pixel_height = parsed_height;
      } else {
        fprintf(stderr, "Ignoring GLOOM.INI %s=%s; expected 1, 2, 1x1, or 2x2\n", key, value);
      }
    }
  }

  fclose(file);
}

static void configure_runtime_gameplay_viewport(int render_width, int render_height) {
  if (!g_runtime_display_ini.has_gameplay_viewport) {
    return;
  }
  if (g_runtime_display_ini.gameplay_viewport_width * g_runtime_display_ini.gameplay_pixel_width > render_width ||
      g_runtime_display_ini.gameplay_viewport_height * g_runtime_display_ini.gameplay_pixel_height > render_height) {
    fprintf(stderr,
            "Ignoring GLOOM.INI gameplay viewport %dx%d at %dx%d; it does not fit the %dx%d render target\n",
            g_runtime_display_ini.gameplay_viewport_width, g_runtime_display_ini.gameplay_viewport_height,
            g_runtime_display_ini.gameplay_pixel_width, g_runtime_display_ini.gameplay_pixel_height, render_width,
            render_height);
    g_runtime_display_ini.has_gameplay_viewport = false;
  }
}

static RuntimeGameplayViewport runtime_gameplay_viewport_for_render_target(int render_width, int render_height) {
  RuntimeGameplayViewport viewport;

  memset(&viewport, 0, sizeof(viewport));
  viewport.source_width = render_width;
  viewport.source_height = render_height;
  viewport.pixel_width = 1;
  viewport.pixel_height = 1;
  viewport.dst_width = render_width;
  viewport.dst_height = render_height;

  if (g_runtime_display_ini.has_gameplay_viewport) {
    viewport.active = true;
    viewport.source_width = g_runtime_display_ini.gameplay_viewport_width;
    viewport.source_height = g_runtime_display_ini.gameplay_viewport_height;
    viewport.pixel_width =
        g_runtime_display_ini.gameplay_pixel_width > 0 ? g_runtime_display_ini.gameplay_pixel_width : 1;
    viewport.pixel_height =
        g_runtime_display_ini.gameplay_pixel_height > 0 ? g_runtime_display_ini.gameplay_pixel_height : 1;
    viewport.dst_width = viewport.source_width * viewport.pixel_width;
    viewport.dst_height = viewport.source_height * viewport.pixel_height;
    viewport.dst_x = (render_width - viewport.dst_width) / 2;
    viewport.dst_y = (render_height - viewport.dst_height) / 2;
  }

  return viewport;
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
  WeaponVisualSet weapon_visuals;
  HudFont hud_font;
  MenuAssets menu_assets;
  RenderFramebuffer framebuffer;
  GloomZone *previous_zones = NULL;
  GloomZone *render_zones = NULL;
  uint64_t perf_frequency = 0;
  uint64_t perf_prev = 0;
  double accumulator = 0.0;
  double title_timer = 0.0;
  const double dt = 1.0 / (double)FIXED_TICK_HZ;
  bool running = true;
  bool mouse_captured = true;
  bool suppress_mouse_fire_until_button_up = false;
  double mouse_dx_accum = 0.0;
  int render_width = DEFAULT_WINDOW_WIDTH;
  int render_height = DEFAULT_WINDOW_HEIGHT;
  int render_scale = 1;
  int window_width = DEFAULT_WINDOW_WIDTH;
  int window_height = DEFAULT_WINDOW_HEIGHT;
#ifdef GLOOM_DOS_SDL3
  bool classic_viewport = true;
#else
  bool classic_viewport = false;
#endif
  bool explicit_resolution = false;
  bool barrel_projectile_origin = true;
  bool two_player_mode = false;
  bool combat_mode = false;
  bool skip_menu = false;
  uint8_t violence_mode = GLOOM_VIOLENCE_MEATY_MESSY;
  uint32_t profile_render_frames = 0u;
#ifdef GLOOM_DOS_SDL3
  uint32_t dos_event_poll_frame = 0u;
#endif
#if GLOOM_RUNTIME_HAS_AUTOSAVE
  char autosave_notice[32] = {0};
  double autosave_notice_timer = 0.0;
#endif
  bool level_transition_pending = false;
  bool combat_round_pending = false;
  AppState state;
  AppState previous_state;

  memset(&grid_offsets, 0, sizeof(grid_offsets));
  memset(&wall_textures, 0, sizeof(wall_textures));
  memset(&flat_textures, 0, sizeof(flat_textures));
  memset(&object_visuals, 0, sizeof(object_visuals));
  memset(&weapon_visuals, 0, sizeof(weapon_visuals));
  memset(&hud_font, 0, sizeof(hud_font));
  memset(&menu_assets, 0, sizeof(menu_assets));
  memset(&framebuffer, 0, sizeof(framebuffer));
  load_runtime_display_ini(&g_runtime_display_ini);
  if (g_runtime_display_ini.has_resolution) {
    window_width = g_runtime_display_ini.resolution_width;
    window_height = g_runtime_display_ini.resolution_height;
    render_width = window_width;
    render_height = window_height;
    explicit_resolution = true;
  }

  if (argc > 1 && strcmp(argv[1], "--version") == 0) {
    printf("%s\n", runtime_title());
    return 0;
  }

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

  if (argc > 1 && strcmp(argv[1], "--combat-selftest") == 0) {
    return run_combat_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--player-death-selftest") == 0) {
    return run_player_death_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--sfx-selftest") == 0) {
    return run_sfx_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--pickup-selftest") == 0) {
    return run_pickup_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--hud-selftest") == 0) {
    return run_hud_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--menu-selftest") == 0) {
    return run_menu_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--completion-selftest") == 0) {
    return run_completion_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--texture-source-selftest") == 0) {
    return run_texture_source_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--input-selftest") == 0) {
    return run_input_selftest();
  }

#if GLOOM_RUNTIME_HAS_AUTOSAVE
  if (argc > 1 && strcmp(argv[1], "--autosave-selftest") == 0) {
    return run_autosave_selftest();
  }
#endif

  if (argc > 1 && strcmp(argv[1], "--wall-selftest") == 0) {
    return run_wall_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--switch-selftest") == 0) {
    return run_switch_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--teleport-selftest") == 0) {
    return run_teleport_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--level-transition-selftest") == 0) {
    return run_level_transition_selftest();
  }

  if (argc > 1 && strcmp(argv[1], "--replay-selftest") == 0) {
    return run_replay_selftest(argc, argv);
  }

  if (argc > 1) {
    int argi = 0;

    for (argi = 1; argi < argc; ++argi) {
      if (strcmp(argv[argi], "--classic") == 0) {
        classic_viewport = true;
      } else if (strcmp(argv[argi], "--widescreen") == 0) {
        classic_viewport = false;
      } else if (strcmp(argv[argi], "--render-scale") == 0) {
        char *end = NULL;
        long parsed = 0;

        if (argi + 1 >= argc) {
          fprintf(stderr, "--render-scale requires an integer value from 1 to 6\n");
          return 1;
        }
        parsed = strtol(argv[++argi], &end, 10);
        if (end == argv[argi] || *end != '\0' || parsed < 1 || parsed > 6) {
          fprintf(stderr, "--render-scale requires an integer value from 1 to 6\n");
          return 1;
        }
        render_scale = (int)parsed;
      } else if (strcmp(argv[argi], "--window-size") == 0 || strcmp(argv[argi], "--resolution") == 0 ||
                 strcmp(argv[argi], "--boot-resolution") == 0) {
        char *end = NULL;
        long parsed_width = 0;
        long parsed_height = 0;

        if (argi + 1 >= argc) {
          fprintf(stderr, "%s requires WIDTHxHEIGHT, for example 1280x720\n", argv[argi]);
          return 1;
        }
        parsed_width = strtol(argv[++argi], &end, 10);
        if ((*end != 'x' && *end != 'X') || parsed_width < 320 || parsed_width > 7680) {
          fprintf(stderr, "%s requires WIDTHxHEIGHT, width 320..7680\n", argv[argi - 1]);
          return 1;
        }
        parsed_height = strtol(end + 1, &end, 10);
        if (*end != '\0' || parsed_height < 200 || parsed_height > 4320) {
          fprintf(stderr, "%s requires WIDTHxHEIGHT, height 200..4320\n", argv[argi - 1]);
          return 1;
        }
        window_width = (int)parsed_width;
        window_height = (int)parsed_height;
        render_width = window_width;
        render_height = window_height;
        explicit_resolution = true;
      } else if (strcmp(argv[argi], "--barrel-projectiles") == 0) {
        barrel_projectile_origin = true;
      } else if (strcmp(argv[argi], "--two-player") == 0 || strcmp(argv[argi], "--2p") == 0) {
        two_player_mode = true;
      } else if (strcmp(argv[argi], "--combat") == 0 || strcmp(argv[argi], "--two-player-combat") == 0) {
        two_player_mode = true;
        combat_mode = true;
      } else if (strcmp(argv[argi], "--skip-menu") == 0) {
        skip_menu = true;
      } else if (strcmp(argv[argi], "--profile-render") == 0) {
        char *end = NULL;
        long parsed = 0;

        if (argi + 1 >= argc) {
          fprintf(stderr, "--profile-render requires a frame count, for example --profile-render 60\n");
          return 1;
        }
        parsed = strtol(argv[++argi], &end, 10);
        if (end == argv[argi] || *end != '\0' || parsed < 1 || parsed > 10000) {
          fprintf(stderr, "--profile-render requires a frame count from 1 to 10000\n");
          return 1;
        }
        profile_render_frames = (uint32_t)parsed;
        skip_menu = true;
      } else if (strcmp(argv[argi], "--player-projectiles") == 0 ||
                 strcmp(argv[argi], "--legacy-projectiles") == 0) {
        barrel_projectile_origin = false;
      } else if (strcmp(argv[argi], "--meaty") == 0) {
        violence_mode = GLOOM_VIOLENCE_MEATY;
      } else if (strcmp(argv[argi], "--messy") == 0) {
        violence_mode = GLOOM_VIOLENCE_MESSY;
      } else if (strcmp(argv[argi], "--meaty-messy") == 0 || strcmp(argv[argi], "--gore") == 0) {
        violence_mode = GLOOM_VIOLENCE_MEATY_MESSY;
      } else {
        map_path = argv[argi];
      }
    }
  }

  if (resolve_runtime_file_path(map_path, map_path_buffer, sizeof(map_path_buffer))) {
    resolved_map_path = map_path_buffer;
  }

#if GLOOM_RUNTIME_IS_BINARY
  printf("%s\n", runtime_title());
#endif

  memset(&state, 0, sizeof(state));
  memset(&previous_state, 0, sizeof(previous_state));
  state.barrel_projectile_origin = barrel_projectile_origin;
  state.two_player_mode = two_player_mode;
  state.combat_mode = combat_mode;
  if (state.combat_mode) {
    initialize_combat_rotation_state(&state, 1u, 3);
    if (!resolve_next_combat_map(&state, map_path_buffer, sizeof(map_path_buffer))) {
      fprintf(stderr, "Failed to pick initial original combat map\n");
      return 1;
    }
    resolved_map_path = map_path_buffer;
#ifdef GLOOM_DOS_SDL3
    refresh_dos_index_palette(&framebuffer, window);
#endif
  }

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

  if (!load_weapon_visual_set(&weapon_visuals)) {
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  if (!load_hud_font(&hud_font)) {
    free_weapon_visual_set(&weapon_visuals);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
#ifdef GLOOM_DOS_SDL3
  register_dos_hud_font_palette(&hud_font);
#endif

  compute_map_bounds(&state);

  if (!initialize_camera_from_map_spawn(&state)) {
    fprintf(stderr, "No player spawn found in map event data\n");
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
  state.violence_mode = violence_mode;
  initialize_runtime_objects(&state);
  printf("Camera spawn: x=%.0f z=%.0f angle=%.3f\n", state.camera_x, state.camera_z, state.camera_angle);
  if (state.two_player_mode) {
    printf("Two-player mode: p1 x=%.0f z=%.0f, p2 x=%.0f z=%.0f\n", state.camera_x, state.camera_z,
           state.player2.camera_x, state.player2.camera_z);
  }
  printf("Projectile origin: %s\n", state.barrel_projectile_origin ? "barrel" : "player");
  printf("Violence model: %s\n", violence_mode_name(state.violence_mode));

  if (!load_grid_offset_set(&grid_offsets)) {
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
#ifdef GLOOM_DOS_SDL3
  rebuild_dos_runtime_index_mappings(&wall_textures, &flat_textures, &object_visuals, &weapon_visuals);
#endif

  if (state.map.zone_count > (size_t)GLOOM_MAX_RENDER_ZONES) {
    fprintf(stderr, "Cannot load %s: zone count %zu exceeds fixed render buffer capacity %u\n", resolved_map_path,
            state.map.zone_count, (unsigned)GLOOM_MAX_RENDER_ZONES);
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

  previous_zones = (GloomZone *)malloc((size_t)GLOOM_MAX_RENDER_ZONES * sizeof(*previous_zones));
  render_zones = (GloomZone *)malloc((size_t)GLOOM_MAX_RENDER_ZONES * sizeof(*render_zones));
  if (previous_zones == NULL || render_zones == NULL) {
    fprintf(stderr, "Out of memory while preparing fixed render interpolation zone buffers\n");
    free(previous_zones);
    free(render_zones);
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
  if (state.map.zone_count > 0u) {
    memcpy(previous_zones, state.map.zones, state.map.zone_count * sizeof(*previous_zones));
    memcpy(render_zones, state.map.zones, state.map.zone_count * sizeof(*render_zones));
  }

  memset(&g_audio, 0, sizeof(g_audio));
  dos_logf("DOS checkpoint: before SDL_Init");
#ifdef GLOOM_DOS_SDL3
  SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "soundblaster");
  SDL_SetHint(SDL_HINT_DOS_ALLOW_DIRECT_FRAMEBUFFER, "1");
  SDL_SetHint("SDL_DOS_MANUAL_AUDIO_FREQ", "22050");
  SDL_SetHint("SDL_DOS_MANUAL_AUDIO_PUMP", "1");
  SDL_SetHint("SDL_DOS_SKIP_EVENT_YIELD", "1");
  SDL_SetHint("SDL_DOS_SKIP_MOUSE_PUMP", "1");
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO) != 0) {
#else
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO | SDL_INIT_GAMECONTROLLER) != 0) {
#endif
    fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
    free(previous_zones);
    free(render_zones);
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
#ifdef GLOOM_DOS_SDL3
  dos_logf("DOS checkpoint: SDL_Init video/events ok");
  fprintf(stderr, "DOS checkpoint: SDL_Init video/events/audio ok\n");
  fflush(stderr);
#endif
  if (!explicit_resolution) {
    if (classic_viewport) {
      render_width = BASE_WIDTH;
#ifdef GLOOM_DOS_SDL3
      render_height = DOS_LOWEST_HEIGHT;
#else
      render_height = BASE_HEIGHT;
#endif
    } else {
#ifdef GLOOM_DOS_SDL3
      render_width = WIDESCREEN_WIDTH;
      render_height = DOS_LOWEST_HEIGHT;
#else
      select_aspect_720p_resolution(&render_width, &render_height);
#endif
    }
  }
  render_width *= render_scale;
  render_height *= render_scale;
  if (!explicit_resolution || render_scale > 1) {
    window_width = render_width;
    window_height = render_height;
  }
  configure_runtime_gameplay_viewport(render_width, render_height);
  gamepad_init();

#ifdef GLOOM_DOS_SDL3
  if (!audio_load_sfx_bank(&g_audio) || !audio_start(&g_audio)) {
#else
  if (!audio_load_sfx_bank(&g_audio) || !audio_start(&g_audio)) {
#endif
    fprintf(stderr, "Failed to initialize SDL Paula SFX port from original amiga/sfxs assets\n");
    audio_shutdown(&g_audio);
    gamepad_shutdown();
    SDL_Quit();
    free(previous_zones);
    free(render_zones);
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }

#ifdef GLOOM_DOS_SDL3
  (void)SDL_SetHint(SDL_HINT_DOS_ALLOW_DIRECT_FRAMEBUFFER, "1");
#endif

  window = SDL_CreateWindow(runtime_title(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, window_width,
                            window_height, SDL_WINDOW_RESIZABLE);
  if (window == NULL) {
    fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
    audio_shutdown(&g_audio);
    gamepad_shutdown();
    SDL_Quit();
    free(previous_zones);
    free(render_zones);
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
#ifdef GLOOM_DOS_SDL3
  dos_logf("DOS checkpoint: SDL_CreateWindow ok %dx%d render=%dx%d", window_width, window_height, render_width,
           render_height);
  fprintf(stderr, "DOS checkpoint: SDL_CreateWindow ok %dx%d render=%dx%d\n", window_width, window_height,
          render_width, render_height);
  fflush(stderr);
  configure_dos_index_display_mode(window, render_width, render_height);
#endif

  renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
  if (renderer == NULL) {
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_SOFTWARE);
  }
  if (renderer == NULL) {
    fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
    SDL_DestroyWindow(window);
    audio_shutdown(&g_audio);
    gamepad_shutdown();
    SDL_Quit();
    free(previous_zones);
    free(render_zones);
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
#ifdef GLOOM_DOS_SDL3
  dos_logf("DOS checkpoint: SDL_CreateRenderer ok");
  fprintf(stderr, "DOS checkpoint: SDL_CreateRenderer ok\n");
  fflush(stderr);
#endif
  runtime_emscripten_install_pointer_lock_listener();
  runtime_emscripten_install_fullscreen_listeners();

  (void)SDL_RenderSetIntegerScale(renderer, classic_viewport ? SDL_TRUE : SDL_FALSE);
  (void)SDL_RenderSetLogicalSize(renderer, render_width, render_height);
#ifdef GLOOM_DOS_SDL3
  dos_runtime_set_mouse_pump_enabled(false);
  apply_dos_index_palette_to_window(window);
#endif
  if (!ensure_render_framebuffer(renderer, &framebuffer, render_width, render_height)) {
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    audio_shutdown(&g_audio);
    gamepad_shutdown();
    SDL_Quit();
    free(previous_zones);
    free(render_zones);
    free_hud_font(&hud_font);
    free_weapon_visual_set(&weapon_visuals);
    free_grid_offset_set(&grid_offsets);
    free_object_visual_set(&object_visuals);
    free_flat_texture_set(&flat_textures);
    free_wall_texture_set(&wall_textures);
    gloom_map_free(&state.map);
    return 1;
  }
#ifdef GLOOM_DOS_SDL3
  dos_logf("DOS checkpoint: render framebuffer ok");
  fprintf(stderr, "DOS checkpoint: render framebuffer ok\n");
  fflush(stderr);
#endif

  if (!skip_menu) {
    GloomGameMode menu_game_mode =
        combat_mode ? GLOOM_GAME_MODE_COMBAT
                    : (two_player_mode ? GLOOM_GAME_MODE_TWO_PLAYER : GLOOM_GAME_MODE_ONE_PLAYER);
    uint8_t menu_violence_mode = violence_mode;
    char selected_map_path[1024] = "amiga/maps/map1_1";

    if (!load_menu_assets(&menu_assets)) {
      fprintf(stderr, "Failed to load original menu assets\n");
      free_render_framebuffer(&framebuffer);
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      audio_shutdown(&g_audio);
      gamepad_shutdown();
      SDL_Quit();
      free(previous_zones);
      free(render_zones);
      free_hud_font(&hud_font);
      free_weapon_visual_set(&weapon_visuals);
      free_grid_offset_set(&grid_offsets);
      free_object_visual_set(&object_visuals);
      free_flat_texture_set(&flat_textures);
      free_wall_texture_set(&wall_textures);
      gloom_map_free(&state.map);
      return 1;
    }
#ifdef GLOOM_DOS_SDL3
    dos_logf("DOS checkpoint: menu assets ok");
    fprintf(stderr, "DOS checkpoint: menu assets ok\n");
    fflush(stderr);
#endif

    if (run_start_menu(renderer, &framebuffer, &menu_assets, render_width, render_height, &menu_game_mode,
                       &menu_violence_mode, &g_control_config) != 0) {
      free_menu_assets(&menu_assets);
      free_render_framebuffer(&framebuffer);
      SDL_DestroyRenderer(renderer);
      SDL_DestroyWindow(window);
      audio_shutdown(&g_audio);
      gamepad_shutdown();
      SDL_Quit();
      free(previous_zones);
      free(render_zones);
      free_hud_font(&hud_font);
      free_weapon_visual_set(&weapon_visuals);
      free_grid_offset_set(&grid_offsets);
      free_object_visual_set(&object_visuals);
      free_flat_texture_set(&flat_textures);
      free_wall_texture_set(&wall_textures);
      gloom_map_free(&state.map);
      return 0;
    }

#ifdef GLOOM_DOS_SDL3
    refresh_dos_index_palette(&framebuffer, window);
#endif
    free_menu_assets(&menu_assets);
    two_player_mode = menu_game_mode != GLOOM_GAME_MODE_ONE_PLAYER;
    combat_mode = menu_game_mode == GLOOM_GAME_MODE_COMBAT;
    violence_mode = menu_violence_mode;
    state.two_player_mode = two_player_mode;
    state.combat_mode = combat_mode;
    state.violence_mode = violence_mode;
#if GLOOM_RUNTIME_HAS_AUTOSAVE
    if (menu_game_mode == GLOOM_GAME_MODE_LOAD_ONE_PLAYER || menu_game_mode == GLOOM_GAME_MODE_LOAD_TWO_PLAYER) {
      bool load_two_player = menu_game_mode == GLOOM_GAME_MODE_LOAD_TWO_PLAYER;

      if (!load_autosaved_runtime_game(load_two_player, renderer, &framebuffer, render_width, render_height, &state,
                                       &wall_textures, &flat_textures, &object_visuals, previous_zones, render_zones,
                                       &two_player_mode, &combat_mode, &violence_mode, map_path_buffer,
                                       sizeof(map_path_buffer), &resolved_map_path)) {
        fprintf(stderr, "Failed to load autosave from menu\n");
        free_render_framebuffer(&framebuffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        audio_shutdown(&g_audio);
        gamepad_shutdown();
        SDL_Quit();
        free(previous_zones);
        free(render_zones);
        free_hud_font(&hud_font);
        free_weapon_visual_set(&weapon_visuals);
        free_grid_offset_set(&grid_offsets);
        free_object_visual_set(&object_visuals);
        free_flat_texture_set(&flat_textures);
        free_wall_texture_set(&wall_textures);
        gloom_map_free(&state.map);
        return 1;
      }
      autosave_notice[0] = '\0';
      autosave_notice_timer = 0.0;
    } else
#endif
    if (combat_mode) {
      uint8_t series = 1u;
      int16_t lives = 3;

      if (run_combat_menu(renderer, &framebuffer, render_width, render_height, &series, &lives) != 0) {
        free_render_framebuffer(&framebuffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        audio_shutdown(&g_audio);
        gamepad_shutdown();
        SDL_Quit();
        free(previous_zones);
        free(render_zones);
        free_hud_font(&hud_font);
        free_weapon_visual_set(&weapon_visuals);
        free_grid_offset_set(&grid_offsets);
        free_object_visual_set(&object_visuals);
        free_flat_texture_set(&flat_textures);
        free_wall_texture_set(&wall_textures);
        gloom_map_free(&state.map);
        return 0;
      }
#ifdef GLOOM_DOS_SDL3
      refresh_dos_index_palette(&framebuffer, window);
#endif
      initialize_combat_rotation_state(&state, series, lives);
      if (!resolve_next_combat_map(&state, selected_map_path, sizeof(selected_map_path))) {
        fprintf(stderr, "Failed to pick original combat map\n");
        free_render_framebuffer(&framebuffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        audio_shutdown(&g_audio);
        gamepad_shutdown();
        SDL_Quit();
        free(previous_zones);
        free(render_zones);
        free_hud_font(&hud_font);
        free_weapon_visual_set(&weapon_visuals);
        free_grid_offset_set(&grid_offsets);
        free_object_visual_set(&object_visuals);
        free_flat_texture_set(&flat_textures);
        free_wall_texture_set(&wall_textures);
        gloom_map_free(&state.map);
        return 1;
      }
    }
    if (!combat_mode && menu_game_mode != GLOOM_GAME_MODE_LOAD_ONE_PLAYER &&
        menu_game_mode != GLOOM_GAME_MODE_LOAD_TWO_PLAYER) {
      ScriptLevelIntro intro;

      if (!resolve_script_level_intro_for_map(selected_map_path, &intro) ||
          !run_script_intro_screen(renderer, &framebuffer, &intro, render_width, render_height)) {
        free_render_framebuffer(&framebuffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        audio_shutdown(&g_audio);
        gamepad_shutdown();
        SDL_Quit();
        free(previous_zones);
        free(render_zones);
        free_hud_font(&hud_font);
        free_weapon_visual_set(&weapon_visuals);
        free_grid_offset_set(&grid_offsets);
        free_object_visual_set(&object_visuals);
        free_flat_texture_set(&flat_textures);
        free_wall_texture_set(&wall_textures);
        gloom_map_free(&state.map);
        return 1;
      }
#ifdef GLOOM_DOS_SDL3
      refresh_dos_index_palette(&framebuffer, window);
#endif
    }
    {
      if (menu_game_mode != GLOOM_GAME_MODE_LOAD_ONE_PLAYER && menu_game_mode != GLOOM_GAME_MODE_LOAD_TWO_PLAYER &&
          !load_runtime_level(selected_map_path, &state, &wall_textures, &flat_textures, &object_visuals,
                              previous_zones, render_zones, false, 0, 0, 0u, 0u, 0, 0, 0u, 0u,
                              barrel_projectile_origin, two_player_mode, violence_mode, map_path_buffer,
                              sizeof(map_path_buffer))) {
        fprintf(stderr, "Failed to load original selected menu map\n");
        free_render_framebuffer(&framebuffer);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        audio_shutdown(&g_audio);
        gamepad_shutdown();
        SDL_Quit();
        free(previous_zones);
        free(render_zones);
        free_hud_font(&hud_font);
        free_weapon_visual_set(&weapon_visuals);
        free_grid_offset_set(&grid_offsets);
        free_object_visual_set(&object_visuals);
        free_flat_texture_set(&flat_textures);
        free_wall_texture_set(&wall_textures);
        gloom_map_free(&state.map);
        return 1;
      }
      resolved_map_path = map_path_buffer;
    }
  }
  mouse_captured = set_gameplay_start_mouse_capture(window);

  previous_state = state;
  if (previous_zones != NULL) {
    previous_state.map.zones = previous_zones;
  }
  if (profile_render_frames > 0u) {
    memset(&g_profiler, 0, sizeof(g_profiler));
    g_profiler.enabled = true;
    g_profiler.target_frames = profile_render_frames;
    profile_logf("PROFILE begin frames=%u map=%s", (unsigned)profile_render_frames, resolved_map_path);
  }
  perf_frequency = SDL_GetPerformanceFrequency();
  perf_prev = SDL_GetPerformanceCounter();

  while (running) {
    SDL_Event event;
    AppState render_state;
    float render_alpha = 0.0f;
    bool pause_menu_requested = false;
    double profile_frame_start_ms = profile_now_ms();
    double profile_start_ms = 0.0;
    uint64_t perf_now = SDL_GetPerformanceCounter();
    double elapsed = (double)(perf_now - perf_prev) / (double)perf_frequency;
#ifdef GLOOM_DOS_SDL3
    bool poll_events_this_frame = dos_event_poll_frame == 0u;
#endif
    perf_prev = perf_now;

    if (elapsed > 0.25) {
      elapsed = 0.25;
    }
#if GLOOM_RUNTIME_HAS_AUTOSAVE
    if (autosave_notice_timer > 0.0) {
      autosave_notice_timer -= elapsed;
      if (autosave_notice_timer < 0.0) {
        autosave_notice_timer = 0.0;
      }
    }
#endif

    accumulator += elapsed;
    title_timer += elapsed;

#ifdef __EMSCRIPTEN__
    if (runtime_emscripten_consume_pointer_lock_lost()) {
      (void)set_runtime_mouse_capture(window, false);
      mouse_captured = false;
      mouse_dx_accum = 0.0;
      suppress_mouse_fire_until_button_up = false;
      pause_menu_requested = true;
      SDL_FlushEvent(SDL_MOUSEMOTION);
    }
    if (runtime_emscripten_consume_fullscreen_resize()) {
      SDL_PumpEvents();
      (void)SDL_RenderSetLogicalSize(renderer, render_width, render_height);
    }
#endif

    profile_start_ms = profile_now_ms();
#ifdef GLOOM_DOS_SDL3
    /* The patched DOS SDL pump skips the cooperative yield and mouse INT 33h work, so per-frame polling stays smooth. */
    dos_event_poll_frame = (dos_event_poll_frame + 1u) % (uint32_t)DOS_EVENT_POLL_INTERVAL_FRAMES;
    if (poll_events_this_frame) {
#endif
      while (SDL_PollEvent(&event) != 0) {
        gamepad_handle_event(&event);
        if (event.type == SDL_QUIT) {
          running = false;
        }
        if (event.type == SDL_KEYDOWN && event.key.repeat == 0) {
          if (event.key.keysym.sym == SDLK_ESCAPE) {
            pause_menu_requested = true;
            if (mouse_captured || g_runtime_mouse_capture_active || SDL_GetRelativeMouseMode()) {
              (void)set_runtime_mouse_capture(window, false);
              mouse_captured = false;
              mouse_dx_accum = 0.0;
              suppress_mouse_fire_until_button_up = false;
              SDL_FlushEvent(SDL_MOUSEMOTION);
            }
          } else if (event.key.keysym.sym == SDLK_p) {
            pause_menu_requested = true;
          } else if (event.key.keysym.sym == SDLK_F11) {
#ifdef __EMSCRIPTEN__
            runtime_emscripten_canvas_fullscreen_toggle();
#else
            Uint32 flags = SDL_GetWindowFlags(window);
            if ((flags & (SDL_WINDOW_FULLSCREEN | SDL_WINDOW_FULLSCREEN_DESKTOP)) != 0u) {
              (void)SDL_SetWindowFullscreen(window, 0);
            } else {
              (void)SDL_SetWindowFullscreen(window, SDL_WINDOW_FULLSCREEN_DESKTOP);
            }
#endif
          }
        }
        if (event.type == SDL_CONTROLLERBUTTONDOWN && event.cbutton.button == SDL_CONTROLLER_BUTTON_START) {
          pause_menu_requested = true;
        }
#ifndef GLOOM_DOS_SDL3
        if (event.type == SDL_MOUSEBUTTONDOWN && event.button.clicks >= 2) {
          if (!mouse_captured && set_runtime_mouse_capture(window, true)) {
            mouse_captured = true;
            mouse_dx_accum = 0.0;
            suppress_mouse_fire_until_button_up = event.button.button == SDL_BUTTON_LEFT;
            SDL_FlushEvent(SDL_MOUSEMOTION);
          }
        }
#endif
        if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_LEFT) {
          suppress_mouse_fire_until_button_up = false;
        }
        if (event.type == SDL_MOUSEMOTION && mouse_captured) {
#ifdef GLOOM_DOS_SDL3
          mouse_dx_accum += dos_runtime_mouse_motion_delta(window, &event.motion);
#else
          mouse_dx_accum += event.motion.xrel;
#endif
        }
      }
#ifdef GLOOM_DOS_SDL3
    }
#endif
    profile_add_elapsed(&g_profiler.events_ms, profile_start_ms);

    if (pause_menu_requested && running) {
      render_alpha = (float)(accumulator / dt);
      render_state =
          interpolate_render_state(&previous_state, &state, render_alpha, render_zones, state.map.zone_count);
      render(renderer, &framebuffer, &render_state, &grid_offsets, &wall_textures, &flat_textures, &object_visuals,
             &weapon_visuals, &hud_font, render_width, render_height,
#if GLOOM_RUNTIME_HAS_AUTOSAVE
             autosave_notice_timer > 0.0 ? autosave_notice : NULL
#else
             NULL
#endif
      );

      PauseMenuResult pause_result =
          run_pause_menu(window, renderer, &framebuffer, render_width, render_height, &mouse_captured, &mouse_dx_accum,
                         &suppress_mouse_fire_until_button_up, state.two_player_mode);

      perf_prev = SDL_GetPerformanceCounter();
      accumulator = 0.0;
      previous_state = state;
      if (previous_zones != NULL) {
        previous_state.map.zones = previous_zones;
      }

      if (pause_result == PAUSE_MENU_QUIT) {
        running = false;
        continue;
      }
      if (pause_result == PAUSE_MENU_MAIN_MENU) {
        if (mouse_captured && set_runtime_mouse_capture(window, false)) {
          mouse_captured = false;
          mouse_dx_accum = 0.0;
          suppress_mouse_fire_until_button_up = false;
          SDL_FlushEvent(SDL_MOUSEMOTION);
        }
        if (!run_menu_and_restart_game(renderer, &framebuffer, render_width, render_height, &state, &wall_textures,
                                       &flat_textures, &object_visuals, previous_zones, render_zones,
                                       barrel_projectile_origin, &two_player_mode, &combat_mode, &violence_mode,
                                       map_path_buffer, sizeof(map_path_buffer), &resolved_map_path)) {
          running = false;
          continue;
        }
#if GLOOM_RUNTIME_HAS_AUTOSAVE
        autosave_notice[0] = '\0';
        autosave_notice_timer = 0.0;
#endif
#ifdef GLOOM_DOS_SDL3
        refresh_dos_index_palette(&framebuffer, window);
#endif
        mouse_captured = set_gameplay_start_mouse_capture(window);
        previous_state = state;
        if (previous_zones != NULL) {
          previous_state.map.zones = previous_zones;
        }
        title_timer = 1.0;
        continue;
      }
#if GLOOM_RUNTIME_HAS_AUTOSAVE
      if (pause_result == PAUSE_MENU_LOAD_GAME) {
        if (mouse_captured && set_runtime_mouse_capture(window, false)) {
          mouse_captured = false;
          mouse_dx_accum = 0.0;
          suppress_mouse_fire_until_button_up = false;
          SDL_FlushEvent(SDL_MOUSEMOTION);
        }
        if (!load_autosaved_runtime_game(state.two_player_mode, renderer, &framebuffer, render_width, render_height,
                                         &state, &wall_textures, &flat_textures, &object_visuals, previous_zones,
                                         render_zones, &two_player_mode, &combat_mode, &violence_mode,
                                         map_path_buffer, sizeof(map_path_buffer), &resolved_map_path)) {
          running = false;
          continue;
        }
        autosave_notice[0] = '\0';
        autosave_notice_timer = 0.0;
#ifdef GLOOM_DOS_SDL3
        refresh_dos_index_palette(&framebuffer, window);
#endif
        mouse_captured = set_gameplay_start_mouse_capture(window);
        previous_state = state;
        if (previous_zones != NULL) {
          previous_state.map.zones = previous_zones;
        }
        accumulator = 0.0;
        title_timer = 1.0;
        continue;
      }
#endif
    }

    {
      int gamepad_dx = player1_gamepad_look_delta();
      bool keyboard_look_enabled = player1_receives_keyboard_input();
      int keyboard_dx = keyboard_look_enabled ? (int)mouse_dx_accum : 0;

      if (keyboard_dx != 0 || gamepad_dx != 0) {
        apply_mouse_look(&state, keyboard_dx + gamepad_dx);
      }
      mouse_dx_accum = keyboard_look_enabled ? mouse_dx_accum - (double)keyboard_dx : 0.0;
    }

    profile_start_ms = profile_now_ms();
    while (accumulator >= dt) {
      const uint8_t *keyboard = SDL_GetKeyboardState(NULL);
      bool mouse_fire = mouse_captured && !suppress_mouse_fire_until_button_up &&
                        ((SDL_GetMouseState(NULL, NULL) & SDL_BUTTON(SDL_BUTTON_LEFT)) != 0u);

      previous_state = state;
      if (previous_zones != NULL) {
        memcpy(previous_zones, state.map.zones, state.map.zone_count * sizeof(*previous_zones));
        previous_state.map.zones = previous_zones;
      }
      update(&state, keyboard, mouse_fire, &object_visuals);
      if (g_profiler.enabled) {
        g_profiler.update_steps += 1u;
      }
      if (state.finished == GLOOM_LEVEL_COMPLETE_FINISHED) {
        level_transition_pending = true;
        break;
      }
      if (state.finished == GLOOM_COMBAT_ROUND_FINISHED) {
        combat_round_pending = true;
        break;
      }
      accumulator -= dt;
    }
    profile_add_elapsed(&g_profiler.update_ms, profile_start_ms);
#ifdef GLOOM_DOS_SDL3
    audio_dos_pump(&g_audio);
#endif

    if (title_timer >= 1.0) {
#ifndef __EMSCRIPTEN__
      char title[160];
      if (state.two_player_mode) {
        (void)snprintf(title, sizeof(title),
                       "%s | 2P | P1 HP=%d L=%d | P2 HP=%d L=%d | zones=%zu", runtime_title(),
                       state.player_hitpoints, state.player_lives, state.player2.player_hitpoints,
                       state.player2.player_lives, state.map.zone_count);
      } else if (state.player_dead) {
        (void)snprintf(title, sizeof(title), "%s | DEAD | x=%.0f z=%.0f angle=%.2f | zones=%zu",
                       runtime_title(), state.camera_x, state.camera_z, state.camera_angle,
                       state.map.zone_count);
      } else {
        (void)snprintf(title, sizeof(title), "%s | HP=%d | x=%.0f z=%.0f angle=%.2f | zones=%zu",
                       runtime_title(), state.player_hitpoints, state.camera_x, state.camera_z, state.camera_angle,
                       state.map.zone_count);
      }
      SDL_SetWindowTitle(window, title);
#endif
      title_timer = 0.0;
    }

    profile_start_ms = profile_now_ms();
    render_alpha = (float)(accumulator / dt);
    render_state = interpolate_render_state(&previous_state, &state, render_alpha, render_zones, state.map.zone_count);
    profile_add_elapsed(&g_profiler.interpolate_ms, profile_start_ms);
    render(renderer, &framebuffer, &render_state, &grid_offsets, &wall_textures, &flat_textures, &object_visuals,
           &weapon_visuals, &hud_font, render_width, render_height,
#if GLOOM_RUNTIME_HAS_AUTOSAVE
           autosave_notice_timer > 0.0 ? autosave_notice : NULL
#else
           NULL
#endif
    );

    if (combat_round_pending) {
      char next_combat_map[1024] = {0};

      combat_round_pending = false;
      if (state.player_lives <= 0 || state.player2.player_lives <= 0) {
        const char *message = state.player_lives <= 0 ? "PLAYER TWO WINS COMBAT GAME!" : "PLAYER ONE WINS COMBAT GAME!";

        if (mouse_captured && set_runtime_mouse_capture(window, false)) {
          mouse_captured = false;
          mouse_dx_accum = 0.0;
          suppress_mouse_fire_until_button_up = false;
          SDL_FlushEvent(SDL_MOUSEMOTION);
        }
        if (!run_combat_result_screen(renderer, &framebuffer, render_width, render_height, message)) {
          running = false;
          continue;
        }
        if (!run_menu_and_restart_game(renderer, &framebuffer, render_width, render_height, &state, &wall_textures,
                                       &flat_textures, &object_visuals, previous_zones, render_zones,
                                       barrel_projectile_origin, &two_player_mode, &combat_mode, &violence_mode,
                                       map_path_buffer, sizeof(map_path_buffer), &resolved_map_path)) {
          running = false;
          continue;
        }
#ifdef GLOOM_DOS_SDL3
        refresh_dos_index_palette(&framebuffer, window);
#endif
        mouse_captured = set_gameplay_start_mouse_capture(window);
        previous_state = state;
        if (previous_zones != NULL) {
          previous_state.map.zones = previous_zones;
        }
        accumulator = 0.0;
        title_timer = 1.0;
        continue;
      }

      if (!resolve_next_combat_map(&state, next_combat_map, sizeof(next_combat_map))) {
        fprintf(stderr, "Cannot continue combat game: failed to pick original combat map\n");
        running = false;
        continue;
      }
      if (!load_runtime_level(next_combat_map, &state, &wall_textures, &flat_textures, &object_visuals, previous_zones,
                              render_zones, true, 0, state.player_lives, 0u, 0u, 0, state.player2.player_lives, 0u,
                              0u, barrel_projectile_origin, true, violence_mode, map_path_buffer,
                              sizeof(map_path_buffer))) {
        fprintf(stderr, "Cannot continue combat game: failed to load %s\n", next_combat_map);
        running = false;
        continue;
      }
      resolved_map_path = map_path_buffer;
#ifdef GLOOM_DOS_SDL3
      refresh_dos_index_palette(&framebuffer, window);
#endif
      previous_state = state;
      if (previous_zones != NULL) {
        previous_state.map.zones = previous_zones;
      }
      accumulator = 0.0;
      continue;
    }

    if (level_transition_pending) {
      char next_map_path[1024] = {0};
      ScriptCompletion completion;
      ScriptPlayNextResult next_result = SCRIPT_PLAY_NEXT_ERROR;
      int16_t preserved_hitpoints = state.player_hitpoints;
      int16_t preserved_lives = state.player_lives;
      uint8_t preserved_weapon = state.player_weapon;
      uint8_t preserved_reload = state.player_reload;
      int16_t preserved_p2_hitpoints = state.player2.player_hitpoints;
      int16_t preserved_p2_lives = state.player2.player_lives;
      uint8_t preserved_p2_weapon = state.player2.player_weapon;
      uint8_t preserved_p2_reload = state.player2.player_reload;
      bool transition_two_player_mode = state.two_player_mode;

      memset(&completion, 0, sizeof(completion));
      level_transition_pending = false;
      audio_pause_output(&g_audio, true, true);
      audio_pause_output(&g_audio, false, false);
      next_result =
          resolve_next_script_play_map_or_done(resolved_map_path, next_map_path, sizeof(next_map_path), &completion);
      if (next_result == SCRIPT_PLAY_NEXT_ERROR) {
        fprintf(stderr, "Cannot complete levelover: misc/script has no next play_ map or done_ after %s\n",
                resolved_map_path);
        running = false;
        continue;
      }

      if (next_result == SCRIPT_PLAY_NEXT_DONE) {
        if (mouse_captured && set_runtime_mouse_capture(window, false)) {
          mouse_captured = false;
          mouse_dx_accum = 0.0;
          suppress_mouse_fire_until_button_up = false;
          SDL_FlushEvent(SDL_MOUSEMOTION);
        }
        if (!run_completion_screen(renderer, &framebuffer, &completion, render_width, render_height)) {
          running = false;
          continue;
        }
        if (!run_menu_and_restart_game(renderer, &framebuffer, render_width, render_height, &state, &wall_textures,
                                       &flat_textures, &object_visuals, previous_zones, render_zones,
                                       barrel_projectile_origin, &two_player_mode, &combat_mode, &violence_mode,
                                       map_path_buffer, sizeof(map_path_buffer), &resolved_map_path)) {
          running = false;
          continue;
        }
#ifdef GLOOM_DOS_SDL3
        refresh_dos_index_palette(&framebuffer, window);
#endif
        mouse_captured = set_gameplay_start_mouse_capture(window);
        previous_state = state;
        if (previous_zones != NULL) {
          previous_state.map.zones = previous_zones;
        }
        accumulator = 0.0;
        title_timer = 1.0;
        continue;
      }

      if (transition_two_player_mode) {
        share_levelover_two_player_lives(&preserved_lives, &preserved_p2_lives);
      }

      {
        ScriptLevelIntro intro;

        if (!resolve_script_level_intro_for_map(next_map_path, &intro) ||
            !run_script_intro_screen(renderer, &framebuffer, &intro, render_width, render_height)) {
          running = false;
          continue;
        }
        if (mouse_captured) {
          mouse_dx_accum = 0.0;
          SDL_FlushEvent(SDL_MOUSEMOTION);
        }
      }
#ifdef GLOOM_DOS_SDL3
      refresh_dos_index_palette(&framebuffer, window);
#endif
#if GLOOM_RUNTIME_HAS_AUTOSAVE
      {
        uint32_t script_ordinal = 0u;

        if (script_play_ordinal_for_map(next_map_path, &script_ordinal) &&
            gloom_should_autosave_script_ordinal(script_ordinal) &&
            write_gloom_autosave_for_scriptplay(next_map_path, script_ordinal, transition_two_player_mode,
                                                violence_mode, barrel_projectile_origin, preserved_hitpoints,
                                                preserved_lives, preserved_weapon, preserved_reload,
                                                preserved_p2_hitpoints, preserved_p2_lives, preserved_p2_weapon,
                                                preserved_p2_reload)) {
          (void)snprintf(autosave_notice, sizeof(autosave_notice), "GAME SAVED");
          autosave_notice_timer = 2.0;
        }
      }
#endif

      if (!load_runtime_level(next_map_path, &state, &wall_textures, &flat_textures, &object_visuals,
                               previous_zones, render_zones, true, preserved_hitpoints, preserved_lives,
                               preserved_weapon, preserved_reload, preserved_p2_hitpoints, preserved_p2_lives,
                               preserved_p2_weapon, preserved_p2_reload, barrel_projectile_origin,
                               transition_two_player_mode, violence_mode, map_path_buffer, sizeof(map_path_buffer))) {
        fprintf(stderr, "Cannot complete levelover: failed to load next script map %s\n", next_map_path);
        running = false;
        continue;
      }

      resolved_map_path = map_path_buffer;
#ifdef GLOOM_DOS_SDL3
      refresh_dos_index_palette(&framebuffer, window);
#endif
      mouse_captured = set_gameplay_start_mouse_capture(window);
      previous_state = state;
      if (previous_zones != NULL) {
        previous_state.map.zones = previous_zones;
      }
      accumulator = 0.0;
    }

    if (g_profiler.enabled) {
      profile_add_elapsed(&g_profiler.frame_ms, profile_frame_start_ms);
      g_profiler.frames += 1u;
      if (g_profiler.frames >= g_profiler.target_frames) {
        running = false;
      }
    }
#ifdef __EMSCRIPTEN__
    if (!g_profiler.enabled) {
      uint64_t frame_end = SDL_GetPerformanceCounter();
      double frame_elapsed = perf_frequency > 0u ? (double)(frame_end - perf_now) / (double)perf_frequency : 0.0;
      int sleep_ms = 0;

      if (frame_elapsed < (1.0 / 60.0)) {
        sleep_ms = (int)(((1.0 / 60.0) - frame_elapsed) * 1000.0);
        if (sleep_ms < 1) {
          sleep_ms = 1;
        }
      }
      emscripten_sleep(sleep_ms);
    }
#endif
  }

  profile_report(render_width, render_height);
  free(previous_zones);
  free(render_zones);
  free_menu_assets(&menu_assets);
  free_hud_font(&hud_font);
  free_weapon_visual_set(&weapon_visuals);
  free_render_framebuffer(&framebuffer);
  free_grid_offset_set(&grid_offsets);
  free_object_visual_set(&object_visuals);
  free_flat_texture_set(&flat_textures);
  free_wall_texture_set(&wall_textures);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  audio_shutdown(&g_audio);
  gamepad_shutdown();
  SDL_Quit();
  gloom_map_free(&state.map);

  return 0;
}
