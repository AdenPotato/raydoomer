#include "game/world.h"

#include "engine/event_dispatch.h"
#include "engine/rng.h"

namespace game {

void World::tick(float dt, engine::Rng& rng, engine::EventDispatcher& events) {
    // Systems are called from here, in an explicit fixed order, from ADE-12
    // onward. Until one exists, taking the context without using it is what
    // keeps the contract honest: the signature is locked, and a system added
    // later needs no change to the loop that drives it.
    (void)rng;
    (void)events;

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
