#!/usr/bin/env bash
# Enforces the one-way dependency: game -> engine -> platform.
#
# The layer rule is stated in engine_api_protocol.md and core_protocol.md, and
# CMake's PRIVATE linkage already stops most violations at link time. This
# catches the rest - a stray #include that compiles because a header happened to
# be reachable - and turns an unverifiable convention into a blocking gate.
#
# Runs in CI and locally. Run it before pushing.

set -uo pipefail
cd "$(dirname "$0")/.."

fail=0

report() {
  local rule="$1" dir="$2" pattern="$3" hits="$4"
  if [ -n "$hits" ]; then
    echo "VIOLATION: $rule"
    echo "  $dir must not include $pattern"
    echo "$hits" | sed 's/^/    /'
    echo
    fail=1
  fi
}

# 1. Gameplay may not see third-party libraries at all. It calls the engine API
#    and nothing below it (gameplay_protocol.md).
if [ -d src/game ]; then
  hits=$(grep -rn -E '#include\s*[<"](raylib\.h|rlgl\.h|box3d/)' src/game 2>/dev/null || true)
  report "gameplay reaches a third-party library" "src/game/" "raylib or box3d headers" "$hits"

  # 2. Gameplay may not reach past the engine into the platform layer.
  hits=$(grep -rn -E '#include\s*[<"]platform/' src/game 2>/dev/null || true)
  report "gameplay reaches the platform layer" "src/game/" "platform/ headers" "$hits"
fi

# 3. raylib is a platform concern. The engine wraps it via platform, never
#    directly, so swapping the windowing library stays platform-local.
if [ -d src/engine ]; then
  hits=$(grep -rn -E '#include\s*[<"](raylib\.h|rlgl\.h)' src/engine 2>/dev/null || true)
  report "engine reaches raylib directly" "src/engine/" "raylib headers" "$hits"
fi

# 4. Tests must run headless, in CI, with no window and no GPU
#    (game_test_protocol.md). The linux-test preset does not even fetch raylib,
#    so this is belt-and-braces, but it fails with a clear message rather than
#    an opaque "file not found".
hits=$(grep -rn -E '#include\s*[<"](raylib\.h|rlgl\.h)' tests 2>/dev/null || true)
report "a test reaches raylib" "tests/" "raylib headers" "$hits"

if [ "$fail" -eq 0 ]; then
  echo "layer boundaries OK"
fi
exit "$fail"
