#include "engine/debug_renderer.h"
#include "engine/event_dispatch.h"
#include "engine/fixed_step.h"
#include "engine/rng.h"
#include "engine/simulation.h"
#include "game/world.h"
#include "platform/clock.h"
#include "platform/raylib/raylib_audio_device.h"
#include "platform/raylib/raylib_input.h"
#include "platform/raylib/raylib_renderer.h"
#include "platform/raylib/raylib_window.h"

namespace {

/// Puts something in the physics world so the debug view has something to show.
///
/// **Temporary.** This is scaffolding until the brush level loader exists; when
/// it lands, levels come from data and this function is deleted along with its
/// call (Enforcement Rule 10 - a replacement removes what it supersedes).
void buildDemoScene(engine::PhysicsWorld& physics) {
    engine::BodyDef floorDef;
    floorDef.type = engine::BodyType::Static;
    floorDef.position = engine::Point3{ 0.0, 0.0, 0.0 };
    floorDef.shape = engine::BoxShape{ engine::Vec3{ 8.0f, 0.5f, 8.0f } };
    floorDef.reportsContacts = false; // geometry does not care that it was hit
    physics.createBody(floorDef);

    // A few spheres at different heights, so they land at different times and
    // the interpolation is visible rather than instantaneous.
    for (int i = 0; i < 4; ++i) {
        engine::BodyDef ball;
        ball.type = engine::BodyType::Dynamic;
        ball.position = engine::Point3{ static_cast<double>(i) * 1.5 - 2.0,
                                        6.0 + static_cast<double>(i) * 2.0, 0.0 };
        ball.shape = engine::SphereShape{ 0.5f };
        physics.createBody(ball);
    }
}

} // namespace

// Bootstrap and the frame loop.
//
// The one place concrete platform implementations are chosen; everything above
// receives them as interfaces. The loop owns no rules: it decides only *when*
// the simulation advances, and hands presentation an interpolation alpha.
int main() {
    platform::RaylibWindow window;
    if (!window.open(1280, 720, "doomer")) {
        window.close();
        return 1;
    }

    platform::RaylibAudioDevice audio;
    if (!audio.open()) {
        // Not fatal. Running silent is a valid response to a missing device, and
        // that call belongs to the game rather than the platform layer.
        audio.close();
    }

    platform::RaylibInput input;
    platform::RaylibRenderer renderer;
    platform::SystemClock clock;

    engine::FixedStepAccumulator accumulator{ engine::SIM_TICK_SECONDS,
                                              engine::MAX_CATCHUP_TICKS };

    // Randomness and the event sink are constructed once, here, and handed to
    // the simulation. Nothing downstream fetches either of them.
    //
    // A fixed seed for now: a new run picks one and stores it in the save so the
    // run replays identically. Choosing it belongs to a run-start flow that does
    // not exist yet.
    constexpr uint64_t kProvisionalSeed = 0x5EED;
    engine::Rng rng{ kProvisionalSeed };

    engine::EventDispatcher events;
    // Systems register here, in a fixed order, before the first publish. None
    // exist yet, so the dispatcher is frozen empty.
    events.freeze();

    game::World world;
    buildDemoScene(world.physics());

    // On in a development build, off in a release build. Toggled with Interact.
    engine::DebugRenderer debugRenderer;

    // Edge detection for the debug controls, tracked here rather than by adding
    // an isKeyPressed to the input seam: three callers in one place do not
    // justify growing the interface.
    bool viewWasDown = false;
    bool modeWasDown = false;
    bool overlayWasDown = false;

    double previousSeconds = clock.nowSeconds();

    while (!window.shouldClose()) {
        // Time enters the program here and nowhere else. Everything downstream
        // receives it as a parameter (seam 1, game_test_protocol.md).
        const double now = clock.nowSeconds();
        const double frameDelta = now - previousSeconds;
        previousSeconds = now;

        input.poll();

        // F1 toggles the view, F2 cycles wireframe/solid/both, F3 toggles the
        // solver overlay.
        const bool viewIsDown = input.isKeyDown(platform::Key::DebugView);
        if (viewIsDown && !viewWasDown) {
            debugRenderer.setEnabled(!debugRenderer.isEnabled());
        }
        viewWasDown = viewIsDown;

        const bool modeIsDown = input.isKeyDown(platform::Key::DebugCycleMode);
        if (modeIsDown && !modeWasDown) {
            debugRenderer.cycleDrawMode();
        }
        modeWasDown = modeIsDown;

        const bool overlayIsDown = input.isKeyDown(platform::Key::DebugOverlay);
        if (overlayIsDown && !overlayWasDown) {
            debugRenderer.setSolverOverlayEnabled(!debugRenderer.isSolverOverlayEnabled());
        }
        overlayWasDown = overlayIsDown;

        const engine::StepResult step = accumulator.advance(frameDelta);
        for (int i = 0; i < step.ticks; ++i) {
            world.tick(static_cast<float>(engine::SIM_TICK_SECONDS), rng, events);
        }

        window.beginFrame();
        debugRenderer.draw(renderer, world.physics(), step.alpha);
        window.endFrame();
    }

    // Release in reverse order of acquisition.
    audio.close();
    window.close();
    return 0;
}
