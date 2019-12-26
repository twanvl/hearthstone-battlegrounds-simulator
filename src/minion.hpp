#pragma once
#include "minion_info.hpp"
#include <algorithm>
using std::min;
using std::max;

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
  int attack_aura : 8; // temporary buffs from auras, at most 6*4+13*4
  int health_aura : 7;
  bool invalid_aura : 1; // the stats are forced to a given value which includes auras

  constexpr Minion()
    : attack(0)
    , health(0)
    , type(MinionType::None)
    , golden(false)
    , taunt(false)
    , divine_shield(false)
    , poison(false)
    , windfury(false)
    , reborn(false)
    , deathrattle_murlocs(0)
    , deathrattle_microbots(0)
    , deathrattle_golden_microbots(0)
    , deathrattle_plants(0)
    , attack_aura(0)
    , health_aura(0)
    , invalid_aura(false)
  {}
  Minion(MinionType type, bool golden = false)
    : Minion(type, info(type), golden)
  {}
private:
  inline constexpr Minion(MinionType type, MinionInfo const& info, bool golden)
    : attack(info.attack_for(golden))
    , health(info.health_for(golden))
    , type(type)
    , golden(golden)
    , taunt(info.taunt)
    , divine_shield(info.divine_shield)
    , poison(info.poison)
    , windfury(info.windfury)
    , reborn(false)
    , deathrattle_murlocs(0)
    , deathrattle_microbots(0)
    , deathrattle_golden_microbots(0)
    , deathrattle_plants(0)
    , attack_aura(0)
    , health_aura(0)
    , invalid_aura(false)
  {}
public:

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

  bool exists() const { return type != MinionType::None; }
  bool dead() const { return health <= 0; }
  bool alive() const { return exists() && !dead(); }

  int double_if_golden(int x) const { return ::double_if_golden(x, golden); }

  void clear() {
    this->type = MinionType::None;
  }

  void buff(int attack, int health) {
    this->attack += attack;
    this->health += health;
  }
  void buff(Minion const& b) {
    // buff by the stats of the reference minion (want b.type == None)
    this->attack += b.attack;
    this->health += b.health;
    this->taunt = this->taunt || b.taunt;
    this->divine_shield = this->divine_shield || b.divine_shield;
    this->poison = this->poison || b.poison;
    this->windfury = this->windfury || b.windfury;
    this->reborn = this->reborn || b.reborn;
    this->attack_aura += b.attack_aura;
    this->health_aura += b.health_aura;
    this->deathrattle_murlocs = max(this->deathrattle_murlocs, b.deathrattle_murlocs);
    this->add_deathrattle_microbots(b.deathrattle_microbots);
    this->add_deathrattle_golden_microbots(b.deathrattle_golden_microbots);
    this->add_deathrattle_plants(b.deathrattle_plants);
  }

  void aura_buff(int attack, int health) {
    this->attack += attack;
    this->health += health;
    this->attack_aura += attack;
    this->health_aura += health;
  }
  void clear_aura_buff() {
    this->attack -= this->attack_aura;
    this->health -= this->health_aura;
    this->attack_aura = 0;
    this->health_aura = 0;
  }

  void add_deathrattle_microbots(int n=3) {
    this->deathrattle_microbots = min(this->deathrattle_microbots+n,7);
  }
  void add_deathrattle_golden_microbots(int n=3) {
    this->deathrattle_golden_microbots = min(this->deathrattle_golden_microbots+n,7);
  }
  void add_deathrattle_plants(int n=2) {
    this->deathrattle_plants = min(this->deathrattle_plants+n,7);
  }

  // full dump/construction

  constexpr Minion(MinionType type, bool golden, int attack, int health, bool taunt, bool divine_shield, bool poison, bool windfury, bool reborn, int deathrattle_murlocs, int deathrattle_microbots, int deathrattle_golden_microbots, int deathrattle_plants)
    : attack(attack), health(health)
    , type(type), golden(golden)
    , taunt(taunt), divine_shield(divine_shield), poison(poison), windfury(windfury), reborn(reborn)
    , deathrattle_murlocs(deathrattle_murlocs), deathrattle_microbots(deathrattle_microbots)
    , deathrattle_golden_microbots(deathrattle_golden_microbots), deathrattle_plants(deathrattle_plants)
    , attack_aura(0)
    , health_aura(0)
    , invalid_aura(false)
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
  if (minion.reborn) s << ", reborn";
  for (int i=0; i < minion.deathrattle_microbots; i+=3) s << ", microbots";
  for (int i=0; i < minion.deathrattle_golden_microbots; i+=3) s << ", golden microbots";
  for (int i=0; i < minion.deathrattle_plants; i+=2) s << ", plants";
  return s;
}

