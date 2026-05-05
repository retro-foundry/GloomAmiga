cmake_minimum_required(VERSION 3.24)
project(GloomPC VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

include(FetchContent)

option(GLOOM_FETCH_SDL "Fetch SDL2 from source" ON)
option(GLOOM_ENABLE_WEB_ASSETS "Preload game assets when targeting Emscripten" ON)
set(GLOOM_BINARY_VERSION "${PROJECT_VERSION}" CACHE STRING "Version string embedded in native/DOS binary builds")

if(NOT EMSCRIPTEN AND GLOOM_FETCH_SDL)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST OFF CACHE BOOL "" FORCE)

  FetchContent_Declare(
    SDL2
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-2.30.12
    GIT_SHALLOW TRUE
  )

  FetchContent_MakeAvailable(SDL2)
endif()

add_executable(gloom_pc
  src/main.c
  src/map.c
  src/iff.c
)

set_target_properties(gloom_pc PROPERTIES
  VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:gloom_pc>"
  VS_DPI_AWARE OFF
)

if(EMSCRIPTEN)
  set(CMAKE_EXECUTABLE_SUFFIX ".html")
  target_compile_options(gloom_pc PRIVATE
    "SHELL:-sUSE_SDL=2"
  )
  target_link_options(gloom_pc PRIVATE
    "SHELL:-s USE_SDL=2"
    "SHELL:-s DISABLE_DEPRECATED_FIND_EVENT_TARGET_BEHAVIOR=0"
    "SHELL:-sALLOW_MEMORY_GROWTH=1"
    "SHELL:-sINITIAL_MEMORY=134217728"
    "SHELL:-sSTACK_SIZE=4194304"
    "SHELL:-sASYNCIFY"
    "SHELL:-sEXPORTED_FUNCTIONS=_main,_SetWebRTCGuestConnected,_SetWebRTCGuestPlayer2Input"
  )
  if(GLOOM_ENABLE_WEB_ASSETS)
    target_link_options(gloom_pc PRIVATE
      "SHELL:--preload-file ${CMAKE_SOURCE_DIR}/amiga@/amiga"
      "SHELL:--shell-file ${CMAKE_SOURCE_DIR}/package/template.html"
    )
    set_property(TARGET gloom_pc APPEND PROPERTY LINK_DEPENDS
      "${CMAKE_SOURCE_DIR}/package/template.html"
    )
    add_custom_command(TARGET gloom_pc POST_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/package/favicon.ico"
        "$<TARGET_FILE_DIR:gloom_pc>/"
      COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/package/icon.png"
        "$<TARGET_FILE_DIR:gloom_pc>/"
      COMMAND ${CMAKE_COMMAND} -E copy
        "${CMAKE_SOURCE_DIR}/package/icon256.png"
        "$<TARGET_FILE_DIR:gloom_pc>/"
      COMMENT "Copying web shell icons"
    )
  endif()
else()
target_compile_definitions(gloom_pc PRIVATE
  GLOOM_BINARY_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
  GLOOM_BINARY_VERSION_MINOR=${PROJECT_VERSION_MINOR}
  GLOOM_BINARY_VERSION_PATCH=${PROJECT_VERSION_PATCH}
)
add_custom_command(TARGET gloom_pc POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/data"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/txts"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/data/txts"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/objs"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/data/objs"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/prog/objs"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/misc"
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:gloom_pc>/amiga/sfxs"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_SOURCE_DIR}/amiga/combat.iff"
    "${CMAKE_SOURCE_DIR}/amiga/blackmagic.iff"
    "${CMAKE_SOURCE_DIR}/amiga/bullet1.bin"
    "${CMAKE_SOURCE_DIR}/amiga/bullet2.bin"
    "${CMAKE_SOURCE_DIR}/amiga/bullet3.bin"
    "${CMAKE_SOURCE_DIR}/amiga/bullet4.bin"
    "${CMAKE_SOURCE_DIR}/amiga/bullet5.bin"
    "${CMAKE_SOURCE_DIR}/amiga/sparks1.bin"
    "${CMAKE_SOURCE_DIR}/amiga/sparks2.bin"
    "${CMAKE_SOURCE_DIR}/amiga/sparks3.bin"
    "${CMAKE_SOURCE_DIR}/amiga/sparks4.bin"
    "${CMAKE_SOURCE_DIR}/amiga/sparks5.bin"
    "${CMAKE_SOURCE_DIR}/amiga/gridoffs4.bin"
    "${CMAKE_SOURCE_DIR}/amiga/theend.iff"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/maps"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/maps"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/data/maps"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/data/maps"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/txts"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/txts"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/data/txts"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/data/txts"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/objs"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/objs"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/data/objs"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/data/objs"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/prog/objs"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/prog/objs"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/misc"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/misc"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/sfxs"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/sfxs"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${CMAKE_SOURCE_DIR}/amiga/ggfx"
    "$<TARGET_FILE_DIR:gloom_pc>/amiga/ggfx"
  COMMENT "Copying runtime map/texture/object data to output directory"
)
endif()

target_include_directories(gloom_pc PRIVATE src)

if(NOT EMSCRIPTEN)
  if(TARGET SDL2::SDL2-static)
    target_link_libraries(gloom_pc PRIVATE SDL2::SDL2-static)
  elseif(TARGET SDL2::SDL2)
    target_link_libraries(gloom_pc PRIVATE SDL2::SDL2)
  else()
    message(FATAL_ERROR "SDL2 target not found. Enable GLOOM_FETCH_SDL or provide SDL2.")
  endif()

  if(TARGET SDL2::SDL2main)
    target_link_libraries(gloom_pc PRIVATE SDL2::SDL2main)
  endif()
endif()

if(MSVC)
  target_compile_definitions(gloom_pc PRIVATE _CRT_SECURE_NO_WARNINGS)
  target_compile_options(gloom_pc PRIVATE /W4)
else()
  target_compile_options(gloom_pc PRIVATE -Wall -Wextra -Wpedantic)
endif()

set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT gloom_pc)
