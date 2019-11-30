#pragma once

#include "enums.hpp"
#include "random.hpp"
#include <ostream>
using std::ostream;

// -----------------------------------------------------------------------------
// Tribe utilities and info
// -----------------------------------------------------------------------------

constexpr bool has_tribe(Tribe t, Tribe query) {
  return t == Tribe::All || t == query;
}

extern const char* tribe_names[Tribe_count];

inline const char* name(Tribe t) {
  return tribe_names[static_cast<int>(t)];
}

inline ostream& operator << (ostream& s, Tribe t) {
  return s << name(t);
}

// -----------------------------------------------------------------------------
// Minion type info
// -----------------------------------------------------------------------------

constexpr int double_if_golden(int x, bool golden) {
  return golden ? 2*x : x;
}

// Minion info
struct MinionInfo {
  const char* name;
  const char* hs_id[2]; // internal id used by hearthstone, for normal and golden
  int stars;
  Tribe tribe;
  int attack, health;
  bool taunt, divine_shield, poison, windfury, cleave;
  //bool battlecry;
  //bool affects_auras;

  constexpr int attack_for(bool golden) const {
    return double_if_golden(attack, golden);
  }
  constexpr int health_for(bool golden) const {
    return double_if_golden(health, golden);
  }
};

// All information on the minions
extern const MinionInfo minion_info[MinionType_count];

inline MinionInfo const& info(MinionType type) {
  return minion_info[static_cast<int>(type)];
}

// Query information about known minions

inline const char* name(MinionType type) { return info(type).name; }
inline Tribe tribe(MinionType type) { return info(type).tribe; }
inline bool has_tribe(MinionType type, Tribe query) { return has_tribe(tribe(type), query); }
inline int stars(MinionType type) { return info(type).stars; }

// random minion spawning

MinionType random_one_cost_minion(RNG& rng);
MinionType random_two_cost_minion(RNG& rng);
MinionType random_four_cost_minion(RNG& rng);
MinionType random_deathrattle_minion(RNG& rng);
MinionType random_legendary_minion(RNG& rng);

// -----------------------------------------------------------------------------
// Hero power info
// -----------------------------------------------------------------------------

struct HeroPowerInfo {
  const char* name;
  const char* hs_id;
};

struct HeroInfo {
  const char* name;
  const char* hs_id;
  HeroPowerInfo hero_power;
};

extern const HeroInfo hero_info[HeroPower_count];

inline const char* name(HeroPower x) {
  return hero_info[static_cast<int>(x)].name;
}

inline ostream& operator << (ostream& s, HeroPower x) {
  return s << name(x);
}

// -----------------------------------------------------------------------------
// Utilities
// -----------------------------------------------------------------------------

// utilities for random minions

template <typename A, int N>
constexpr int array_size(A(&)[N]) { return N; }

template <typename A, int N>
A random_element(A(& list)[N], RNG& rng) {
  return list[rng.random(array_size(list))];
}
