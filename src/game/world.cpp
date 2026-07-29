#include "game/world.h"

namespace game {

void World::tick(float dt) {
    ++tickCount_;
    elapsedSeconds_ += dt;
}

} // namespace game
