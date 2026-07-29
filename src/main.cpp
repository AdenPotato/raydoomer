#include "engine/fixed_step.h"
#include "engine/simulation.h"
#include "game/world.h"
#include "platform/clock.h"
#include "platform/raylib/raylib_audio_device.h"
#include "platform/raylib/raylib_input.h"
#include "platform/raylib/raylib_window.h"

// Bootstrap and the frame loop.
//
// This is the one place concrete platform implementations are chosen; everything
// above receives them as interfaces. The loop itself owns no rules: it decides
// only *when* the simulation advances, and hands presentation an interpolation
// alpha to draw with.
//
// Simulation runs on the fixed step; presentation on the variable one. Nothing
// smoothed for the eye may feed back into the rules (engine_protocol.md).
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
    platform::SystemClock clock;

    engine::FixedStepAccumulator accumulator{ engine::SIM_TICK_SECONDS,
                                              engine::MAX_CATCHUP_TICKS };
    game::World world;

    double previousSeconds = clock.nowSeconds();

    while (!window.shouldClose()) {
        // Time enters the program here and nowhere else. Everything downstream
        // receives it as a parameter (seam 1, game_test_protocol.md).
        const double now = clock.nowSeconds();
        const double frameDelta = now - previousSeconds;
        previousSeconds = now;

        input.poll();

        const engine::StepResult step = accumulator.advance(frameDelta);
        for (int i = 0; i < step.ticks; ++i) {
            world.tick(static_cast<float>(engine::SIM_TICK_SECONDS));
        }

        window.beginFrame();
        // Presentation draws here, interpolating by step.alpha. Nothing renders
        // yet; the renderer arrives with the sprite and HUD work.
        (void)step.alpha;
        window.endFrame();
    }

    // Release in reverse order of acquisition.
    audio.close();
    window.close();
    return 0;
}
