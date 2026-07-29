#include "platform/raylib/raylib_audio_device.h"
#include "platform/raylib/raylib_input.h"
#include "platform/raylib/raylib_window.h"

// Bootstrap. This is the one place concrete platform implementations are
// chosen; everything above receives them as interfaces.
//
// The fixed-step loop, the world, and the systems arrive with ADE-10. What is
// here is the minimum that proves the seam is wired: acquire, run frames,
// release - with every acquire matched, including on the failure path.
int main() {
    platform::RaylibWindow window;
    if (!window.open(1280, 720, "doomer")) {
        window.close();
        return 1;
    }

    platform::RaylibAudioDevice audio;
    if (!audio.open()) {
        // Not fatal. Running silent is a valid response to a missing device,
        // and that call belongs to the game rather than the platform layer.
        audio.close();
    }

    platform::RaylibInput input;

    while (!window.shouldClose()) {
        input.poll();

        window.beginFrame();
        window.endFrame();
    }

    // Release in reverse order of acquisition.
    audio.close();
    window.close();
    return 0;
}
