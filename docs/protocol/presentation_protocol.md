# Presentation Protocol

The job description for **presentation work**: everything the player sees and hears. Rendering, the camera, sprites, the HUD, and audio output.

Cross-cutting rules (folder structure, naming, code quality, commits, TDD loop, branching, docstrings) live in the shared core [core_protocol.md](core_protocol.md). Engine-layer rules are [engine_protocol.md](engine_protocol.md); the rules being presented are [gameplay_protocol.md](gameplay_protocol.md); visual tokens are [style_guide.md](../design/style_guide.md); human verification is [qa_protocol.md](qa_protocol.md).

> **The defining constraint:** presentation **observes** the simulation. It never feeds back into it. Every rule below follows from that.

---

## Presentation observes; it never decides

- **No gameplay rule lives here.** If removing the renderer would change what the game does, a rule leaked into presentation.
- **Presentation never writes simulation state.** It reads. A smoothing value, a camera shake, an animation timer: none of it may reach back into the rules ([gameplay_protocol.md](gameplay_protocol.md)).
- **Presentation runs on the variable step**, simulation on the fixed one. Anything smoothed for the eye belongs here and stays here.
- **Draw from interpolated state.** Positions come from the two most recent simulation states blended by the render alpha, never from raw current state, which visibly stutters when the tick and the frame disagree.

> **What not to do:** do not sneak a gameplay decision into a draw call because the data was conveniently in hand. "Only draw the hit spark if the shot actually connected" is a rule; it belongs in the simulation, emitting an event.

---

## The camera

- **The camera is presentation, not simulation.** Its position may derive from the player's simulated position, but nothing in the rules may read the camera back.
- **Camera feel values are tunables in data** - follow distance, smoothing, shake magnitude, field of view ([gameplay_protocol.md](gameplay_protocol.md)).
- **Frame-rate independent.** Smoothing scales with delta time; a per-frame lerp constant is a different camera at 60 and 144 fps.

## Sprites

Actors are 2D billboards, not 3D models ([locked_decisions.md](../reference/locked_decisions.md)).

- **Billboards face the camera on the yaw axis only.** Rolling a sprite to face a pitched camera looks wrong for this style.
- **Directional frames come from the angle between the viewer and the actor's facing,** resolved through one shared helper rather than reimplemented per entity type.
- **Depth sorting is explicit and stable.** Two sprites at equal depth must resolve in a defined order; unstable sorting flickers.
- **Animation is driven by simulated time,** not by frame count. "Advance a frame every N frames" is a different animation at a different refresh rate.

## Audio output

Audio is presentation. It is triggered by simulation events and never by an entity reaching for a device.

- **Gameplay emits an event; a presentation handler plays the sound.** No entity calls the audio API.
- **Device access sits in `platform/`** behind the seam like any other device ([engine_protocol.md](engine_protocol.md)).
- **Volumes and audio feel values are tunables in data.**
- **Never block the frame on audio.** Loading and decoding happen at level load, not mid-tick.

---

## HUD and styling

- **All color, spacing, and type values come from tokens** in [style_guide.md](../design/style_guide.md). Never a raw hex or pixel literal in HUD code.
- **The token tables and component taxonomy are canonical in the style guide.** Do not redefine them here or in a component.
- **Resolution independence.** The HUD scales with the viewport; a layout tuned for one resolution and hardcoded in pixels breaks on every other one. Verify the **smallest supported resolution** first, because that is where a layout overflows.
- **Extract repeated HUD data to a config,** not inline constants in the drawing code. Render from one table.

## Accessibility

These are hard constraints, not polish.

- **Color is never the sole indicator of state.** Low health, damage direction, and item rarity each pair color with a shape, an icon, or text. A colorblind player must be able to play.
- **Contrast holds against the game world, not against a flat background.** WCAG AA (4.5:1 normal text, 3:1 large text and UI) is the target, and the worst case is bright HUD text over a bright wall.
- **Text is legible at the smallest supported resolution,** verified there rather than assumed from a desktop-sized window.

---

## Testing

Rendering is the one genuinely unobservable output, which is what golden images are for. Everything around it is ordinary testable logic.

| What                                   | The test written first                                                                   |
|----------------------------------------|------------------------------------------------------------------------------------------|
| Billboard angle, frame selection, sort order | Unit test over plain inputs. No window, no device.                                 |
| Interpolation math                     | Unit test: two states plus an alpha produce the expected blend.                          |
| HUD layout logic                       | Unit test at several viewport sizes, asserting computed positions.                       |
| Audio triggering                       | gmock the audio device; assert **which** sounds were requested, never real playback.     |
| Rendered output                        | Golden-image test against a committed reference, within a documented threshold.          |
| Visual quality beyond a golden         | **Not a test.** A playtest with written criteria ([qa_protocol.md](qa_protocol.md)).     |

**Golden images are deterministic or they are worthless:** fixed camera, fixed seed, fixed simulated time, no animation sampled from the wall clock. A re-baked reference is a reviewed change and states its reason in the commit ([game_test_protocol.md](game_test_protocol.md)).

> **What not to do:** do not test rendering by eyeballing a screenshot in review. That is a playtest wearing a test's clothing. Commit a golden, or call it a playtest.

---

## Never claim a visual outcome you did not observe

You cannot see the game run. "The muzzle flash reads better now" is a hypothesis, not a result. Say what the code draws and what a playtest would need to confirm ([gameplay_protocol.md](gameplay_protocol.md)).

---

## When you are doing presentation work, you own

- A presentation layer that reads simulation state and never writes it.
- Interpolated drawing, so motion is smooth regardless of the frame rate.
- Tokens rather than literals, with the style guide as the single source.
- Accessibility as a constraint: never color alone, contrast that holds against the world.
- Deterministic goldens, re-baked only with a stated reason.
- An honest account of what you changed versus what you verified.
