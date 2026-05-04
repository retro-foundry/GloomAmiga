#ifndef GLOOM_SDL_DOS_COMPAT_H
#define GLOOM_SDL_DOS_COMPAT_H

#define SDL_DISABLE_OLD_NAMES 1
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <sys/nearptr.h>

extern int __djgpp_nearptr_enable(void);

#define SDL_INIT_GAMECONTROLLER SDL_INIT_GAMEPAD

#define SDL_QUIT SDL_EVENT_QUIT
#define SDL_KEYDOWN SDL_EVENT_KEY_DOWN
#define SDL_MOUSEMOTION SDL_EVENT_MOUSE_MOTION
#define SDL_MOUSEBUTTONDOWN SDL_EVENT_MOUSE_BUTTON_DOWN
#define SDL_MOUSEBUTTONUP SDL_EVENT_MOUSE_BUTTON_UP
#define SDL_CONTROLLERBUTTONDOWN SDL_EVENT_GAMEPAD_BUTTON_DOWN
#define SDL_CONTROLLERDEVICEADDED SDL_EVENT_GAMEPAD_ADDED
#define SDL_CONTROLLERDEVICEREMOVED SDL_EVENT_GAMEPAD_REMOVED

#define SDL_BUTTON(X) SDL_BUTTON_MASK(X)
#define SDL_QUERY -1
#define SDL_ENABLE 1
#define SDL_DISABLE 0
#define SDL_TRUE true
#define SDL_FALSE false
#define AUDIO_F32SYS SDL_AUDIO_F32
#define SDL_RENDERER_ACCELERATED 0u
#define SDL_RENDERER_PRESENTVSYNC 0u
#define SDL_RENDERER_SOFTWARE 0u
#define SDL_WINDOW_FULLSCREEN_DESKTOP SDL_WINDOW_FULLSCREEN
#define SDL_ScaleModeNearest SDL_SCALEMODE_NEAREST
#define SDLK_p SDLK_P
#define SDLK_s SDLK_S
#define SDLK_w SDLK_W

#define SDL_CONTROLLER_BUTTON_A SDL_GAMEPAD_BUTTON_SOUTH
#define SDL_CONTROLLER_BUTTON_B SDL_GAMEPAD_BUTTON_EAST
#define SDL_CONTROLLER_BUTTON_START SDL_GAMEPAD_BUTTON_START
#define SDL_CONTROLLER_BUTTON_BACK SDL_GAMEPAD_BUTTON_BACK
#define SDL_CONTROLLER_BUTTON_DPAD_UP SDL_GAMEPAD_BUTTON_DPAD_UP
#define SDL_CONTROLLER_BUTTON_DPAD_DOWN SDL_GAMEPAD_BUTTON_DPAD_DOWN
#define SDL_CONTROLLER_BUTTON_DPAD_LEFT SDL_GAMEPAD_BUTTON_DPAD_LEFT
#define SDL_CONTROLLER_BUTTON_DPAD_RIGHT SDL_GAMEPAD_BUTTON_DPAD_RIGHT
#define SDL_CONTROLLER_BUTTON_RIGHTSHOULDER SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER
#define SDL_CONTROLLER_AXIS_LEFTX SDL_GAMEPAD_AXIS_LEFTX
#define SDL_CONTROLLER_AXIS_LEFTY SDL_GAMEPAD_AXIS_LEFTY
#define SDL_CONTROLLER_AXIS_RIGHTX SDL_GAMEPAD_AXIS_RIGHTX
#define SDL_CONTROLLER_AXIS_TRIGGERRIGHT SDL_GAMEPAD_AXIS_RIGHT_TRIGGER
#define SDL_GameController SDL_Gamepad
#define SDL_GameControllerButton SDL_GamepadButton

static SDL_Window *gloom_dos_window = NULL;

static inline int GloomDos_Init(SDL_InitFlags flags) {
  SDL_SetMainReady();
  if (__djgpp_nearptr_enable() == 0) {
    SDL_SetError("DOSVESA: __djgpp_nearptr_enable() failed");
    return -1;
  }
  return SDL_Init(flags) ? 0 : -1;
}

typedef struct {
  SDL_Keycode sym;
} GloomDosKeySym;

typedef struct {
  SDL_EventType type;
  Uint32 reserved;
  Uint64 timestamp;
  SDL_WindowID windowID;
  SDL_KeyboardID which;
  SDL_Scancode scancode;
  GloomDosKeySym keysym;
  SDL_Keymod mod;
  Uint16 raw;
  bool down;
  bool repeat;
} GloomDosKeyboardEvent;

typedef union GloomDosEvent {
  SDL_EventType type;
  SDL_CommonEvent common;
  SDL_DisplayEvent display;
  SDL_WindowEvent window;
  GloomDosKeyboardEvent key;
  SDL_TextEditingEvent edit;
  SDL_TextInputEvent text;
  SDL_MouseMotionEvent motion;
  SDL_MouseButtonEvent button;
  SDL_MouseWheelEvent wheel;
  SDL_JoyAxisEvent jaxis;
  SDL_JoyBallEvent jball;
  SDL_JoyHatEvent jhat;
  SDL_JoyButtonEvent jbutton;
  SDL_JoyDeviceEvent jdevice;
  SDL_GamepadAxisEvent gaxis;
  SDL_GamepadButtonEvent cbutton;
  SDL_GamepadDeviceEvent cdevice;
  SDL_QuitEvent quit;
  SDL_UserEvent user;
  SDL_TouchFingerEvent tfinger;
  SDL_PenProximityEvent pproximity;
  SDL_PenTouchEvent ptouch;
  SDL_PenMotionEvent pmotion;
  SDL_PenButtonEvent pbutton;
  SDL_PenAxisEvent paxis;
  SDL_RenderEvent render;
  SDL_DropEvent drop;
  Uint8 padding[sizeof(SDL_Event)];
} GloomDosEvent;

static inline int GloomDos_PollEvent(GloomDosEvent *event) {
  SDL_Event raw;
  if (!SDL_PollEvent(&raw)) {
    return 0;
  }
  SDL_memset(event, 0, sizeof(*event));
  SDL_memcpy(event, &raw, sizeof(raw));
  if (raw.type == SDL_EVENT_KEY_DOWN || raw.type == SDL_EVENT_KEY_UP) {
    event->key.keysym.sym = raw.key.key;
    event->key.scancode = raw.key.scancode;
    event->key.mod = raw.key.mod;
    event->key.raw = raw.key.raw;
    event->key.down = raw.key.down;
    event->key.repeat = raw.key.repeat;
  }
  return 1;
}

static inline int GloomDos_CreateRenderer(SDL_Window *window, int index, Uint32 flags) {
  (void)index;
  (void)flags;
  return SDL_CreateRenderer(window, NULL) != NULL;
}

static inline SDL_Renderer *GloomDos_CreateRendererPtr(SDL_Window *window, int index, Uint32 flags) {
  (void)index;
  (void)flags;
  return SDL_CreateRenderer(window, NULL);
}

static inline SDL_Window *GloomDos_CreateWindow(const char *title, int x, int y, int w, int h, Uint32 flags) {
  (void)x;
  (void)y;
  gloom_dos_window = SDL_CreateWindow(title, w, h, flags);
  return gloom_dos_window;
}

static inline int GloomDos_RenderSetLogicalSize(SDL_Renderer *renderer, int w, int h) {
  return SDL_SetRenderLogicalPresentation(renderer, w, h, SDL_LOGICAL_PRESENTATION_LETTERBOX) ? 0 : -1;
}

static inline int GloomDos_RenderSetIntegerScale(SDL_Renderer *renderer, bool enabled) {
  (void)renderer;
  (void)enabled;
  return 0;
}

static inline int GloomDos_RenderCopy(SDL_Renderer *renderer, SDL_Texture *texture, const SDL_Rect *src,
                                      const SDL_Rect *dst) {
  SDL_FRect fsrc;
  SDL_FRect fdst;
  const SDL_FRect *fsrc_ptr = NULL;
  const SDL_FRect *fdst_ptr = NULL;
  if (src != NULL) {
    fsrc.x = (float)src->x;
    fsrc.y = (float)src->y;
    fsrc.w = (float)src->w;
    fsrc.h = (float)src->h;
    fsrc_ptr = &fsrc;
  }
  if (dst != NULL) {
    fdst.x = (float)dst->x;
    fdst.y = (float)dst->y;
    fdst.w = (float)dst->w;
    fdst.h = (float)dst->h;
    fdst_ptr = &fdst;
  }
  return SDL_RenderTexture(renderer, texture, fsrc_ptr, fdst_ptr) ? 0 : -1;
}

static inline int GloomDos_LockTexture(SDL_Texture *texture, const SDL_Rect *rect, void **pixels, int *pitch) {
  return SDL_LockTexture(texture, rect, pixels, pitch) ? 0 : -1;
}

static inline int GloomDos_UpdateTexture(SDL_Texture *texture, const SDL_Rect *rect, const void *pixels, int pitch) {
  return SDL_UpdateTexture(texture, rect, pixels, pitch) ? 0 : -1;
}

static inline int GloomDos_SetTextureScaleMode(SDL_Texture *texture, SDL_ScaleMode scaleMode) {
  return SDL_SetTextureScaleMode(texture, scaleMode) ? 0 : -1;
}

static inline int GloomDos_SetTextureBlendMode(SDL_Texture *texture, SDL_BlendMode blendMode) {
  return SDL_SetTextureBlendMode(texture, blendMode) ? 0 : -1;
}

static inline SDL_FRect GloomDos_FRectFromRect(const SDL_Rect *rect) {
  SDL_FRect frect;
  frect.x = rect != NULL ? (float)rect->x : 0.0f;
  frect.y = rect != NULL ? (float)rect->y : 0.0f;
  frect.w = rect != NULL ? (float)rect->w : 0.0f;
  frect.h = rect != NULL ? (float)rect->h : 0.0f;
  return frect;
}

static inline int GloomDos_RenderFillRect(SDL_Renderer *renderer, const SDL_Rect *rect) {
  if (rect == NULL) {
    return SDL_RenderFillRect(renderer, NULL) ? 0 : -1;
  }
  {
    SDL_FRect frect = GloomDos_FRectFromRect(rect);
    return SDL_RenderFillRect(renderer, &frect) ? 0 : -1;
  }
}

static inline int GloomDos_RenderDrawRect(SDL_Renderer *renderer, const SDL_Rect *rect) {
  if (rect == NULL) {
    return SDL_RenderRect(renderer, NULL) ? 0 : -1;
  }
  {
    SDL_FRect frect = GloomDos_FRectFromRect(rect);
    return SDL_RenderRect(renderer, &frect) ? 0 : -1;
  }
}

static inline int GloomDos_RenderDrawLine(SDL_Renderer *renderer, int x1, int y1, int x2, int y2) {
  return SDL_RenderLine(renderer, (float)x1, (float)y1, (float)x2, (float)y2) ? 0 : -1;
}

static inline int GloomDos_SetRelativeMouseMode(bool enabled) {
  SDL_Window *window = gloom_dos_window != NULL ? gloom_dos_window : SDL_GetKeyboardFocus();
  if (window == NULL) {
    SDL_SetError("No DOS SDL window for relative mouse mode");
    return -1;
  }
  return SDL_SetWindowRelativeMouseMode(window, enabled) ? 0 : -1;
}

static inline bool GloomDos_GetRelativeMouseMode(void) {
  SDL_Window *window = gloom_dos_window != NULL ? gloom_dos_window : SDL_GetKeyboardFocus();
  return window != NULL && SDL_GetWindowRelativeMouseMode(window);
}

static inline int GloomDos_CaptureMouse(bool enabled) {
  return SDL_CaptureMouse(enabled) ? 0 : -1;
}

static inline void GloomDos_SetWindowGrab(SDL_Window *window, bool grabbed) {
  (void)SDL_SetWindowMouseGrab(window, grabbed);
}

static inline int GloomDos_ShowCursor(int toggle) {
  if (toggle == SDL_ENABLE) {
    return SDL_ShowCursor() ? SDL_ENABLE : SDL_DISABLE;
  }
  if (toggle == SDL_DISABLE) {
    return SDL_HideCursor() ? SDL_DISABLE : SDL_ENABLE;
  }
  return SDL_CursorVisible() ? SDL_ENABLE : SDL_DISABLE;
}

static inline Uint32 GloomDos_GetMouseState(int *x, int *y) {
  float fx = 0.0f;
  float fy = 0.0f;
  Uint32 buttons = SDL_GetMouseState(&fx, &fy);
  if (x != NULL) *x = (int)fx;
  if (y != NULL) *y = (int)fy;
  return buttons;
}

static inline int GloomDos_SetWindowFullscreen(SDL_Window *window, Uint32 flags) {
  return SDL_SetWindowFullscreen(window, flags != 0u) ? 0 : -1;
}

static inline const char *GloomDos_GetBasePath(void) {
  const char *path = SDL_GetBasePath();
  return SDL_strdup(path != NULL ? path : "");
}

static inline void GloomDos_free(void *ptr) {
  SDL_free(ptr);
}

static inline const Uint8 *GloomDos_GetKeyboardState(int *numkeys) {
  return (const Uint8 *)SDL_GetKeyboardState(numkeys);
}

static inline int GloomDos_GetCurrentDisplayMode(SDL_DisplayID displayID, SDL_DisplayMode *mode) {
  const SDL_DisplayMode *current = SDL_GetCurrentDisplayMode(displayID);
  if (current == NULL || mode == NULL) {
    return -1;
  }
  *mode = *current;
  return 0;
}

static inline int GloomDos_NumJoysticks(void) {
  int count = 0;
  SDL_JoystickID *ids = SDL_GetJoysticks(&count);
  SDL_free(ids);
  return count;
}

static inline bool GloomDos_IsGameController(int device_index) {
  int count = 0;
  bool result = false;
  SDL_JoystickID *ids = SDL_GetJoysticks(&count);
  if (ids != NULL && device_index >= 0 && device_index < count) {
    result = SDL_IsGamepad(ids[device_index]);
  }
  SDL_free(ids);
  return result;
}

static inline SDL_JoystickID GloomDos_JoystickGetDeviceInstanceID(int device_index) {
  int count = 0;
  SDL_JoystickID result = 0;
  SDL_JoystickID *ids = SDL_GetJoysticks(&count);
  if (ids != NULL && device_index >= 0 && device_index < count) {
    result = ids[device_index];
  }
  SDL_free(ids);
  return result;
}

static inline SDL_Gamepad *GloomDos_GameControllerOpen(int device_index) {
  SDL_JoystickID id = GloomDos_JoystickGetDeviceInstanceID(device_index);
  return id != 0 ? SDL_OpenGamepad(id) : NULL;
}

static inline void GloomDos_LockAudioDevice(SDL_AudioDeviceID device) {
  (void)device;
}

static inline void GloomDos_UnlockAudioDevice(SDL_AudioDeviceID device) {
  (void)device;
}

static inline void GloomDos_PauseAudioDevice(SDL_AudioDeviceID device, int paused) {
  (void)device;
  (void)paused;
}

#define SDL_Init GloomDos_Init
#define SDL_PollEvent GloomDos_PollEvent
#define SDL_CreateWindow GloomDos_CreateWindow
#define SDL_CreateRenderer GloomDos_CreateRendererPtr
#define SDL_RenderSetLogicalSize GloomDos_RenderSetLogicalSize
#define SDL_RenderSetIntegerScale GloomDos_RenderSetIntegerScale
#define SDL_RenderCopy GloomDos_RenderCopy
#define SDL_RenderFillRect GloomDos_RenderFillRect
#define SDL_RenderDrawRect GloomDos_RenderDrawRect
#define SDL_RenderDrawLine GloomDos_RenderDrawLine
#define SDL_LockTexture GloomDos_LockTexture
#define SDL_UpdateTexture GloomDos_UpdateTexture
#define SDL_SetTextureScaleMode GloomDos_SetTextureScaleMode
#define SDL_SetTextureBlendMode GloomDos_SetTextureBlendMode
#define SDL_SetRelativeMouseMode GloomDos_SetRelativeMouseMode
#define SDL_GetRelativeMouseMode GloomDos_GetRelativeMouseMode
#define SDL_CaptureMouse GloomDos_CaptureMouse
#define SDL_SetWindowGrab GloomDos_SetWindowGrab
#define SDL_ShowCursor GloomDos_ShowCursor
#define SDL_GetMouseState GloomDos_GetMouseState
#define SDL_SetWindowFullscreen GloomDos_SetWindowFullscreen
#define SDL_GetBasePath GloomDos_GetBasePath
#define SDL_free GloomDos_free
#define SDL_GetKeyboardState GloomDos_GetKeyboardState
#define SDL_GetCurrentDisplayMode GloomDos_GetCurrentDisplayMode
#define SDL_NumJoysticks GloomDos_NumJoysticks
#define SDL_IsGameController GloomDos_IsGameController
#define SDL_JoystickGetDeviceInstanceID GloomDos_JoystickGetDeviceInstanceID
#define SDL_GameControllerOpen GloomDos_GameControllerOpen
#define SDL_LockAudioDevice GloomDos_LockAudioDevice
#define SDL_UnlockAudioDevice GloomDos_UnlockAudioDevice
#define SDL_PauseAudioDevice GloomDos_PauseAudioDevice
#define SDL_GameControllerName SDL_GetGamepadName
#define SDL_GameControllerClose SDL_CloseGamepad
#define SDL_GameControllerGetJoystick SDL_GetGamepadJoystick
#define SDL_JoystickInstanceID SDL_GetJoystickID
#define SDL_GameControllerEventState(X) ((void)0)
#define SDL_GameControllerGetButton SDL_GetGamepadButton
#define SDL_GameControllerGetAxis SDL_GetGamepadAxis
#define SDL_Event GloomDosEvent

#endif
