#pragma once

#include <cstdint>
#include <vector>

namespace game {

/// An opaque reference to an item definition in `data/`.
///
/// @remarks
/// The definition itself - name, rarity, affixes, stats - lives in a content
/// file and is loaded by the content layer. The inventory stores only the
/// reference, so a balance change to an item never requires touching a save.
struct ItemId {
    uint32_t value = 0;

    friend bool operator==(const ItemId&, const ItemId&) = default;
};

/// A quantity of one item.
struct ItemStack {
    ItemId item;
    int count = 0;

    friend bool operator==(const ItemStack&, const ItemStack&) = default;
};

/// What the player is carrying.
///
/// @remarks
/// Intentionally a flat list. Capacity limits, sorting, equipment slots, and
/// stacking rules are ADE-17 and ADE-18; adding them here before there is a
/// system that needs them would be speculative generality (Enforcement Rule 5).
///
/// Order is preserved and meaningful: it is what the inventory UI will display,
/// and an unstable order would make the save round-trip non-deterministic.
struct Inventory {
    std::vector<ItemStack> stacks;

    friend bool operator==(const Inventory&, const Inventory&) = default;
};

} // namespace game
