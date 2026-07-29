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
)
target_include_directories(platform_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})

if(NOT CMAKE_CROSSCOMPILING)
  return()
endif()

add_library(platform_raylib STATIC
  platform/raylib/raylib_window.cpp
  platform/raylib/raylib_input.cpp
  platform/raylib/raylib_audio_device.cpp
)
target_link_libraries(platform_raylib
  PUBLIC  platform_core
  PRIVATE raylib          # PRIVATE: raylib does not escape this target
)

add_executable(doomer
  main.cpp
)
target_link_libraries(doomer PRIVATE platform_raylib)

if(WIN32)
  target_link_libraries(doomer PRIVATE opengl32 gdi32 winmm)
endif()
