#include "engine/event_dispatch.h"
#include "engine/fixed_step.h"
#include "engine/rng.h"
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

    // Randomness and the event sink are constructed once, here, and handed to
    // the simulation. Nothing downstream fetches either of them.
    //
    // A fixed seed for now: a new run picks one and stores it in the save so the
    // run replays identically. Choosing it is part of the run-start flow, which
    // does not exist yet.
    constexpr uint64_t kProvisionalSeed = 0x5EED;
    engine::Rng rng{ kProvisionalSeed };

    engine::EventDispatcher events;
    // Systems register here, in a fixed order, before the first publish. None
    // exist yet, so the dispatcher is frozen empty.
    events.freeze();

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
            world.tick(static_cast<float>(engine::SIM_TICK_SECONDS), rng, events);
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
