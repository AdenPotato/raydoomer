#include "platform/window.h"

// Bootstrap. The fixed-step loop, world, and systems arrive with ADE-10;
// this only proves the cross-compiled binary reaches a window on the target.
int main() {
    return platform::runWindow(1280, 720, "doomer");
}
