# The game binary. Only built by the windows-game preset.
#
# raylib is linked into the platform layer only. Nothing in src/game/ may see it
# (engine_api_protocol.md); keeping the link private to this target is what makes
# that structural rather than a convention.

add_library(platform STATIC
  platform/window.cpp
)
target_include_directories(platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(platform PRIVATE raylib)

add_executable(doomer
  main.cpp
)
target_link_libraries(doomer PRIVATE platform)

if(WIN32)
  target_link_libraries(doomer PRIVATE opengl32 gdi32 winmm)
endif()
