# Headless test binaries. Nothing here may link raylib or open a window: the
# linux-test preset does not fetch raylib, so an attempt fails at configure time.

add_executable(engine_tests
  engine/dependencies_test.cpp
  engine/physics_test.cpp
)

target_link_libraries(engine_tests PRIVATE
  GTest::gtest_main
  GTest::gmock
  nlohmann_json::nlohmann_json
  box3d::box3d
)

include(GoogleTest)
gtest_discover_tests(engine_tests)
