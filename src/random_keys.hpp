#pragma once
#include "minion.hpp"

// -----------------------------------------------------------------------------
// Random number generator keys
// -----------------------------------------------------------------------------

enum class RNGType : unsigned char {
  OneCostMinion = 1, TwoCostMinion, FourCostMinion, LegendaryMinion, DeathrattleMinion,
  FirstPlayer,
  Damage,
  Attack,
  GiveDivineShield,
  Buff,
};

inline RNGKey rng_key(RNGType type) {
  return {(int)type};
}
inline RNGKey rng_key(RNGType type, int player) {
  return {(int)type ^ (player<<8)};
}
inline RNGKey rng_key(RNGType type, int player, int amount) {
  return {(int)type ^ (player<<8) ^ (amount<<9)};
}
inline RNGKey rng_key(RNGType type, int player, Minion const& attacker) {
  return {(int)type ^ (player<<8) ^ ((int)attacker.type<<9) ^ ((int)attacker.golden<<9)};
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

// utilities for picking random elements

template <typename A, int N>
constexpr int array_size(A(&)[N]) { return N; }

template <typename A, int N, typename Key>
A random_element(A(& list)[N], BattleRNG& rng, Key key) {
  return list[rng.random(array_size(list), key)];
}

