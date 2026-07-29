#include "game/save.h"

#include <nlohmann/json.hpp>

namespace game {
namespace {

using nlohmann::json;

std::unexpected<SaveError> fail(SaveErrorKind kind, std::string message) {
    return std::unexpected(SaveError{ kind, std::move(message) });
}

} // namespace

std::string toJson(const SaveData& save) {
    json inventory = json::array();
    for (const ItemStack& stack : save.inventory.stacks) {
        inventory.push_back({ { "item", stack.item.value }, { "count", stack.count } });
    }

    const json document = {
        { "version", kCurrentSaveVersion },
        { "player",
          {
              { "health", save.player.health },
              { "maxHealth", save.player.maxHealth },
              { "armor", save.player.armor },
          } },
        { "inventory", inventory },
        { "progress",
          {
              { "currentLevel", save.progress.currentLevel },
              { "completedLevels", save.progress.completedLevels },
          } },
        { "currency", save.currency },
        // As a string: JSON numbers are doubles, which cannot hold a full 64-bit
        // seed without losing low bits. Losing them would silently change what a
        // loaded run replays.
        { "rngSeed", std::to_string(save.rngSeed) },
    };

    return document.dump(2);
}

LoadResult fromJson(std::string_view json_text) {
    json document = json::parse(json_text, nullptr, /*allow_exceptions=*/false);

    if (document.is_discarded() || !document.is_object()) {
        return fail(SaveErrorKind::Malformed, "save is not a JSON object");
    }

    // Validate the version before reading anything else. Interpreting fields
    // under the wrong schema is worse than refusing to load.
    if (!document.contains("version") || !document["version"].is_number_integer()) {
        return fail(SaveErrorKind::MissingVersion,
                    "save has no integer version field and cannot be migrated");
    }

    const int version = document["version"].get<int>();
    if (version > kCurrentSaveVersion || version < 1) {
        return fail(SaveErrorKind::UnsupportedVersion,
                    "save version " + std::to_string(version) + " is not supported by this build (current is " +
                        std::to_string(kCurrentSaveVersion) + ")");
    }

    // No migrations exist yet: version 1 is the only schema. When
    // kCurrentSaveVersion becomes 2, the chain applies here, oldest first.

    SaveData save;
    save.version = version;

    // Missing optional fields fall back to defaults. This is the migration path
    // for an additive change, and it is deliberate rather than incidental.
    if (const auto player = document.find("player"); player != document.end() && player->is_object()) {
        save.player.health = player->value("health", save.player.health);
        save.player.maxHealth = player->value("maxHealth", save.player.maxHealth);
        save.player.armor = player->value("armor", save.player.armor);
    }

    if (const auto inventory = document.find("inventory");
        inventory != document.end() && inventory->is_array()) {
        for (const auto& entry : *inventory) {
            if (!entry.is_object()) {
                continue;
            }
            ItemStack stack;
            stack.item.value = entry.value("item", 0u);
            stack.count = entry.value("count", 0);
            save.inventory.stacks.push_back(stack);
        }
    }

    if (const auto progress = document.find("progress");
        progress != document.end() && progress->is_object()) {
        save.progress.currentLevel = progress->value("currentLevel", save.progress.currentLevel);
        if (const auto completed = progress->find("completedLevels");
            completed != progress->end() && completed->is_array()) {
            save.progress.completedLevels = completed->get<std::vector<int>>();
        }
    }

    save.currency = document.value("currency", save.currency);

    if (const auto seed = document.find("rngSeed"); seed != document.end() && seed->is_string()) {
        try {
            save.rngSeed = std::stoull(seed->get<std::string>());
        } catch (const std::exception&) {
            // A corrupt seed is not worth refusing the whole save over: the run
            // simply will not replay identically. Default and continue.
            save.rngSeed = 0;
        }
    }

    return save;
}

} // namespace game
