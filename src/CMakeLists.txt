# The platform layer is split in two, and the split is the seam.
#
#   platform_core   interfaces + implementations with no third-party dependency.
#                   Builds in BOTH configurations, so the headless tests can
#                   link it and exercise the real clock and filesystem.
#
#   platform_raylib the raylib-backed implementations. Windows build only,
#                   because linux-test never fetches raylib.
#
# A test can therefore depend on the interfaces without any possibility of
# dragging in a window or a GPU: the backend simply does not exist in that
# configuration.

add_library(platform_core STATIC
  platform/clock.cpp
  platform/file_system.cpp
  platform/violation.cpp
)
target_include_directories(platform_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

# Engine: no third-party dependency, no game nouns. Builds in both
# configurations so the headless tests exercise the real thing.
add_library(engine STATIC
  engine/billboard.cpp
  engine/camera.cpp
  engine/debug_renderer.cpp
  engine/event_dispatch.cpp
  engine/rng.cpp
  engine/fixed_step.cpp
  engine/physics.cpp
)
target_include_directories(engine PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
# engine -> platform is the correct direction. platform must never link engine.
target_link_libraries(engine PUBLIC platform_core PRIVATE box3d::box3d)

# Game: the rules. Depends on engine, never on platform or a third-party
# library - the layer-boundary gate enforces that.
add_library(game STATIC
  game/player_controller.cpp
  game/quake_movement.cpp
  game/save.cpp
  game/world.cpp
)
target_include_directories(game PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(game PUBLIC engine PRIVATE nlohmann_json::nlohmann_json)

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

add_library(platform_raylib STATIC
  platform/raylib/raylib_window.cpp
  platform/raylib/raylib_input.cpp
  platform/raylib/raylib_renderer.cpp
  platform/raylib/raylib_audio_device.cpp
)
target_link_libraries(platform_raylib
  PUBLIC  platform_core
  PRIVATE raylib          # PRIVATE: raylib does not escape this target
)

add_executable(doomer
  main.cpp
)
target_link_libraries(doomer PRIVATE platform_raylib game)

if(WIN32)
  target_link_libraries(doomer PRIVATE opengl32 gdi32 winmm)
endif()
