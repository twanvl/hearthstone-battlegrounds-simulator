#pragma once

#include "minion_types.hpp"
#include "tribe.hpp"

// -----------------------------------------------------------------------------
// Minion type info
// -----------------------------------------------------------------------------

inline int double_if_golden(int x, bool golden) {
  return golden ? 2*x : x;
}

// Minion info
struct MinionInfo {
  const char* name;
  int stars;
  Tribe tribe;
  int attack, health;
  bool taunt, divine_shield, poison, windfury, cleave;
  bool battlecry;
  //bool affects_auras;

  inline int attack_for(bool golden) const {
    return double_if_golden(attack, golden);
  }
  inline int health_for(bool golden) const {
    return double_if_golden(health, golden);
  }
};

// All information on the minions
extern const MinionInfo minion_info[static_cast<int>(MinionType::COUNT)];

inline MinionInfo const& info(MinionType type) {
  return minion_info[static_cast<int>(type)];
}

// Query information about known minions

inline const char* name(MinionType type) { return info(type).name; }
inline Tribe tribe(MinionType type) { return info(type).tribe; }
inline bool has_tribe(MinionType type, Tribe query) { return has_tribe(tribe(type), query); }
inline int stars(MinionType type) { return info(type).stars; }

// random minion spawning

MinionType random_one_cost_minion();
MinionType random_two_cost_minion();
MinionType random_four_cost_minion();
MinionType random_deathrattle_minion();
MinionType random_legendary_minion();

MinionType type_by_name(std::string const& name);

// events

