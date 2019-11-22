#pragma once
#include "minion_info.hpp"
#include <algorithm>
using std::min;

// -----------------------------------------------------------------------------
// Minion instances
// -----------------------------------------------------------------------------

struct Minion {
  int attack : 16, health : 16;
  MinionType type;
  bool golden : 1;
  bool taunt : 1, divine_shield : 1, poison : 1, windfury : 1;
  bool reborn : 1; // for lich king
  unsigned deathrattle_murlocs : 1; // for giantfin
  unsigned deathrattle_microbots : 3;
  unsigned deathrattle_golden_microbots : 3;
  unsigned deathrattle_plants  : 3; // from adapt
  int attack_buff : 8, health_buff : 8; // temporary buffs from auras, at most 7*4

  Minion() {
    this->type = MinionType::NONE;
    this->attack = -1;
    this->health = -1;
    this->taunt  = false;
    this->divine_shield = false;
    this->poison = false;
    this->windfury = false;
    this->reborn = false;
    this->deathrattle_murlocs = 0;
    this->deathrattle_microbots = 0;
    this->deathrattle_golden_microbots = 0;
    this->deathrattle_plants = 0;
  }
  Minion(MinionType type, bool golden = false) {
    this->type = type;
    this->golden = golden;
    MinionInfo const& info = ::info(type);
    this->attack = info.attack_for(golden);
    this->health = info.health_for(golden);
    this->attack_buff = 0;
    this->health_buff = 0;
    this->taunt  = info.taunt;
    this->divine_shield = info.divine_shield;
    this->poison = info.poison;
    this->windfury = info.windfury;
    this->reborn = false;
    this->deathrattle_murlocs = 0;
    this->deathrattle_microbots = 0;
    this->deathrattle_golden_microbots = 0;
    this->deathrattle_plants = 0;
  }

  Minion new_copy() const {
    return Minion(type, golden);
  }
  Minion reborn_copy() const {
    Minion copy(type, golden);
    copy.health = 1;
    return copy;
  }

  const char* name() const { return info(type).name; }
  int stars() const { return info(type).stars; }
  Tribe tribe() const { return info(type).tribe; }
  bool cleave() const { return info(type).cleave; }
  bool has_tribe(Tribe query) const { return ::has_tribe(tribe(), query); }

  bool exists() const { return type != MinionType::NONE; }
  bool dead() const { return health <= 0; }
  bool alive() const { return exists() && !dead(); }

  void clear() {
    this->type = MinionType::NONE;
  }
  void buff(int attack, int health) {
    this->attack += attack;
    this->health += health;
  }
  void aura_buff(int attack, int health) {
    this->attack += attack;
    this->health += health;
    this->attack_buff += attack;
    this->health_buff += health;
  }
  void clear_aura_buff() {
    this->attack -= this->attack_buff;
    this->health -= this->health_buff;
    this->attack_buff = 0;
    this->health_buff = 0;
  }

  Minion& set_stats(int attack, int health, int attack_buff=0, int health_buff=0) {
    this->attack = attack;
    this->health = health;
    this->attack_buff = attack_buff;
    this->health_buff = health_buff;
    return *this;
  }
  Minion& set_taunt() {
    this->taunt = true;
    return *this;
  }
  Minion& set_divine_shield() {
    this->divine_shield = true;
    return *this;
  }
  Minion& set_poison() {
    this->poison = true;
    return *this;
  }
  Minion& add_deathrattle_microbots(int n=3) {
    this->deathrattle_microbots = min(this->deathrattle_microbots+n,7);
    return *this;
  }
  Minion& add_deathrattle_golden_microbots(int n=3) {
    this->deathrattle_golden_microbots = min(this->deathrattle_golden_microbots+n,7);
    return *this;
  }
  Minion& add_deathrattle_plants(int n=2) {
    this->deathrattle_plants = min(this->deathrattle_plants+n,7);
    return *this;
  }

  // full dump/construction

  constexpr Minion(MinionType type, bool golden, int attack, int health, bool taunt, bool divine_shield, bool poison, bool windfury, bool reborn, int deathrattle_murlocs, int deathrattle_microbots, int deathrattle_golden_microbots, int deathrattle_plants)
    : attack(attack), health(health)
    , type(type), golden(golden)
    , taunt(taunt), divine_shield(divine_shield), poison(poison), windfury(windfury), reborn(reborn)
    , deathrattle_murlocs(deathrattle_murlocs), deathrattle_microbots(deathrattle_microbots)
    , deathrattle_golden_microbots(deathrattle_golden_microbots), deathrattle_plants(deathrattle_plants)
    , attack_buff(0)
    , health_buff(0)
  {}
};

inline ostream& operator << (ostream& s, Minion const& minion) {
  s << minion.attack << "/" << minion.health << " ";
  if (minion.golden) s << "Golden ";
  s << minion.name();
  if (false) {
    s << " (";
    for (int i = 0 ; i < minion.stars(); ++i) s << "*";
    s << ")";
  }
  if (minion.taunt) s << ", taunt";
  if (minion.divine_shield) s << ", divine shield";
  if (minion.poison) s << ", poisonous";
  if (minion.windfury) s << ", windfury";
  return s;
}

