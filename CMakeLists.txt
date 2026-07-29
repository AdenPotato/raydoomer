cmake_minimum_required(VERSION 3.24)
project(doomer CXX)

# Locked in docs/reference/locked_decisions.md. Extensions off so the build
# fails loudly rather than silently relying on a GNU extension.
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(FetchContent)

# ---------------------------------------------------------------------------
# Dependencies
#
# Two build configurations (see CMakePresets.json):
#   linux-test    native host compiler, builds and runs the headless tests
#   windows-game  MinGW-w64 cross-compile, builds doomer.exe
#
# raylib is only ever needed by the game binary. The test configuration does not
# fetch it at all, which is what makes "tests never need a window" structural
# rather than a convention (game_test_protocol.md).
# ---------------------------------------------------------------------------

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

# Box3D publishes no release tags, so pin a full commit SHA. A moving branch
# would make builds unreproducible, which for a physics engine also means
# non-reproducible simulation - the determinism the tests assert would silently
# depend on whatever was on main that day.
set(BOX3D_SAMPLES OFF CACHE BOOL "" FORCE)
set(BOX3D_BENCHMARKS OFF CACHE BOOL "" FORCE)
set(BOX3D_UNIT_TESTS OFF CACHE BOOL "" FORCE)
set(BOX3D_DOCS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  box3d
  GIT_REPOSITORY https://github.com/erincatto/box3d.git
  GIT_TAG        781673be801569a94be7942848755fc0baad3653
)
FetchContent_MakeAvailable(box3d)

if(NOT CMAKE_CROSSCOMPILING)
  set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
  FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        v1.15.2
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(googletest)

  # platform_core only; src/CMakeLists.txt stops before the raylib targets when
  # not cross-compiling, so nothing here can pull in a window.
  add_subdirectory(src)

  enable_testing()
  add_subdirectory(tests)
else()
  set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
  set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    raylib
    GIT_REPOSITORY https://github.com/raysan5/raylib.git
    GIT_TAG        5.5
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(raylib)

  add_subdirectory(src)
endif()
