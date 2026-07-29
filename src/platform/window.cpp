#include "window.h"

#include <raylib.h>

namespace platform {

int runWindow(int width, int height, const char* title) {
    InitWindow(width, height, title);
    if (!IsWindowReady()) {
        return 1;
    }

    while (!WindowShouldClose()) {
        BeginDrawing();
        ClearBackground(Color{ 24, 24, 32, 255 });
        DrawText("doomer", 20, 20, 40, RAYWHITE);
        DrawText("scaffold - ADE-23", 20, 70, 20, GRAY);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}

} // namespace platform
