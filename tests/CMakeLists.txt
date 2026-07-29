# Headless test binaries. Nothing here may link raylib or open a window: the
# linux-test preset does not fetch raylib, so an attempt fails at configure time.

add_executable(engine_tests
  engine/dependencies_test.cpp
  engine/physics_test.cpp
  engine/fixed_step_test.cpp
  engine/simulation_constants_test.cpp
  engine/event_dispatch_test.cpp
  game/world_test.cpp
  platform/clock_test.cpp
  platform/file_system_test.cpp
  platform/seam_test.cpp
)

# Test-support headers (fakes, the manual clock) are included as "support/...".
target_include_directories(engine_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

target_link_libraries(engine_tests PRIVATE
  GTest::gtest_main
  GTest::gmock
  nlohmann_json::nlohmann_json
  box3d::box3d
  platform_core
  engine
  game
)

include(GoogleTest)
gtest_discover_tests(engine_tests)
