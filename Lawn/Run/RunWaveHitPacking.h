#pragma once
#include <array>
#include <cstdint>

// The slot is already known at lookup, so storing its generation preserves the
// complete ID comparison in the existing 4096-byte field without changing saves.
namespace RunWaveHitPacking {
constexpr unsigned LegacySlots = 1024, Slots = 2048;
template<class Word> constexpr unsigned Generation(const Word* words, unsigned slot) {
    return slot < Slots ? (static_cast<uint32_t>(words[slot / 2]) >> ((slot % 2) * 16)) & 0xffffU : 0;
}
template<class Word> constexpr bool Has(const Word* words, uint32_t id) {
    return (id >> 16) != 0 && (id & 0xffffU) < Slots && Generation(words, id & 0xffffU) == (id >> 16);
}
template<class Word> constexpr bool Record(Word* words, uint32_t id) {
    const unsigned slot = id & 0xffffU, generation = id >> 16;
    if (slot >= Slots || !generation) return false;
    const unsigned shift = (slot % 2) * 16;
    const uint32_t old = static_cast<uint32_t>(words[slot / 2]);
    words[slot / 2] = static_cast<Word>((old & ~(0xffffU << shift)) | (generation << shift));
    return true;
}
template<class Word> constexpr uint32_t LegacyId(const Word* words, unsigned slot) {
    const unsigned generation = Generation(words, slot);
    return generation ? (generation << 16) | slot : 0;
}
template<class Word> constexpr void ConvertLegacy(Word* words) {
    std::array<uint32_t, LegacySlots> old{};
    for (unsigned i = 0; i < LegacySlots; ++i) {
        old[i] = static_cast<uint32_t>(words[i]);
        words[i] = static_cast<Word>(0);
    }
    for (unsigned slot = 0; slot < LegacySlots; ++slot)
        if ((old[slot] & 0xffffU) == slot) Record(words, old[slot]);
}
}
