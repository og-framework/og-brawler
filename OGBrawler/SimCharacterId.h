#pragma once
// SPDX-License-Identifier: BUSL-1.1
// docs/SimCharacterId-rationale.md · docs/SimCharacterId-guards.md

#include <cstdint>
#include <limits>
#include <type_traits>

enum class SimCharacterId : std::uint8_t
{
    None = 0u,
};

static_assert(sizeof(SimCharacterId) == 1u && std::is_trivially_copyable_v<SimCharacterId>,
    "SimCharacterId is one raw byte wherever it is serialized, so a synced field of this type "
    "costs exactly 1 B on the correction wire. Widening the underlying type is a wire-format "
    "change and a ruling (R2), not an edit.");

constexpr unsigned int toStorageKey(SimCharacterId id)
{
    return static_cast<unsigned int>(id);
}

class SimCharacterIdAllocator
{
public:
    static constexpr std::uint8_t kLastAssignable =
        std::numeric_limits<std::underlying_type_t<SimCharacterId>>::max();

    SimCharacterIdAllocator() = default;
    SimCharacterIdAllocator(const SimCharacterIdAllocator&) = delete;
    SimCharacterIdAllocator& operator=(const SimCharacterIdAllocator&) = delete;
    SimCharacterIdAllocator(SimCharacterIdAllocator&&) = delete;
    SimCharacterIdAllocator& operator=(SimCharacterIdAllocator&&) = delete;

    // ⛔G-01  docs/SimCharacterId-guards.md
    SimCharacterId allocate()
    {
        if (m_lastAssigned == kLastAssignable)
            return SimCharacterId::None;
        ++m_lastAssigned;
        return static_cast<SimCharacterId>(m_lastAssigned);
    }

    bool isExhausted() const { return m_lastAssigned == kLastAssignable; }

    unsigned int assignedCount() const { return static_cast<unsigned int>(m_lastAssigned); }

private:
    std::uint8_t m_lastAssigned = 0u;
};

static_assert(!std::is_copy_assignable_v<SimCharacterIdAllocator>
        && !std::is_move_assignable_v<SimCharacterIdAllocator>
        && !std::is_copy_constructible_v<SimCharacterIdAllocator>
        && !std::is_move_constructible_v<SimCharacterIdAllocator>,
    "SimCharacterIdAllocator must be neither assignable nor copyable. An assignment from a fresh "
    "allocator RESETS the count and re-issues id 1 while a character still holds it, and a copy "
    "gives two allocators that both issue the same next id. Either one breaks 'never reused' "
    "(guard G-01 of docs/SimCharacterId-guards.md) without touching allocate().");
