#include "game/world.h"

#include "engine/event_dispatch.h"
#include "engine/rng.h"
#include "game/player_controller.h"

namespace game {

engine::BodyHandle World::spawnPlayer(engine::Point3 position) {
    engine::BodyDef def;
    def.type = engine::BodyType::Dynamic;
    def.position = position;
    // A sphere for now. A capsule is the usual character shape and Box3D has
    // one; switching is a shape change, not a controller change.
    def.shape = engine::SphereShape{ movement_.characterRadius };
    def.lockRotation = true;
    // The controller owns friction. Surface friction here would fight it, and
    // the player would never reach its configured speed.
    def.friction = 0.0f;

    playerBody_ = physics_.createBody(def);
    return playerBody_;
}

void World::tick(float dt, const InputSnapshot& input, engine::Rng& rng,
                 engine::EventDispatcher& events) {
    // Gameplay systems are called from here, in an explicit fixed order, from
    // ADE-12 onward. Until one exists, taking the RNG without using it keeps the
    // contract honest: the signature is locked, so a system added later needs no
    // change to the loop that drives it.
    (void)rng;

    // Systems run in an explicit, fixed order. The order lives here and nowhere
    // else (runtime_architecture.md).
    if (playerBody_.generation != 0) {
        stepPlayer(physics_, playerBody_, input, dt, movement_);
    }

    // Exactly one physics step per simulation tick, never per rendered frame.
    // Contacts publish through the same dispatcher the gameplay systems use.
    physics_.step(dt, events);

    ++tickCount_;
    elapsedSeconds_ += dt;
}

SaveData World::toSave(uint64_t rngSeed) const {
    SaveData save;
    save.player = player_;
    save.inventory = inventory_;
    save.progress = progress_;
    save.currency = currency_;
    save.rngSeed = rngSeed;
    return save;
}

void World::restore(const SaveData& save) {
    player_ = save.player;
    inventory_ = save.inventory;
    progress_ = save.progress;
    currency_ = save.currency;

    // Level-scoped state does not survive a load. A save is a level boundary,
    // so the incoming level starts from zero rather than resuming a partial one.
    tickCount_ = 0;
    elapsedSeconds_ = 0.0f;
}

} // namespace game
