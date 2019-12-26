#pragma once
#include "minion.hpp"
#include "minion_array.hpp"
#include "enums.hpp"
#include "random.hpp"
#include "random_keys.hpp"
#include <iostream>
#include <cstdlib>
#include <limits>
#include <algorithm>
using std::ostream;
using std::endl;
using std::max;

// -----------------------------------------------------------------------------
// Board state (for a single player)
// -----------------------------------------------------------------------------

const int BOARDSIZE = 7;
const int MAX_HAND_SIZE = 10;
const int NUM_EXTRA_POS = 3;

// Board state
struct Board {
  MinionArray<BOARDSIZE> minions;

  // position of next minion to attack
  int next_attacker = 0;
  // extra positions to keep track of
  // we need enough for cleave attacks
  int track_pos[NUM_EXTRA_POS] = {-1};

  // hero power to start battle with
  HeroType hero = HeroType::None;
  bool use_hero_power = false;
  // level of the player (or 0 if unknown)
  int level = 0;
  // health of the player
  int health = 0;
  // cards in hand
  std::vector<Minion> hand;
  // bob's minions
  // during battle turns, this contains only frozen minions
  std::vector<Minion> bobs_minions;
  // available gold
  int gold = 0;
private:
  // are there (possibly) any auras?
  bool any_auras = false;
public:

  Board() {}
  Board(std::initializer_list<Minion> minions, HeroType hero, bool use_hero_power, int level, int health)
    : minions(minions)
    , hero(hero)
    , use_hero_power(use_hero_power)
    , level(level)
    , health(health)
  {}

  // Modifying minions

  bool insert(int pos, Minion const& minion) {
    if (!minions.insert(pos,minion)) return false;
    if (next_attacker > pos) {
      // Note: inserting just before the next attacker makes inserted minion the next attacker
      next_attacker++;
    }
    for (int i=0; i<NUM_EXTRA_POS; ++i) {
      if (pos >= track_pos[i]) track_pos[i]++;
    }
    any_auras = any_auras || is_aura_minion(minion.type) || minion.invalid_aura;
    return true;
  }

  int append(Minion const& minion) {
    int pos = minions.append(minion);
    any_auras = any_auras || is_aura_minion(minion.type) || minion.invalid_aura;
    return pos;
  }

  void remove(int pos) {
    minions.remove(pos);
    if (pos < next_attacker) {
      next_attacker--;
    }
    for (int i=0; i<NUM_EXTRA_POS; ++i) {
      if (pos < track_pos[i]) track_pos[i]--;
      else if (pos == track_pos[i]) track_pos[i] = -1;
    }
  }

  // Targeting
  
  // target to attack
  int random_attack_target(BattleRNG& rng, RNGKey key) const {
    int num_taunts = 0, num_minions = 0;
    minions.for_each([&](Minion const& m) {
      if (m.taunt) num_taunts++;
      num_minions++;
    });
    if (num_taunts > 0) {
      int pick = rng.random(num_taunts, key);
      for (int i=0; minions.contains(i); ++i) {
        if (minions[i].taunt) {
          if (pick-- == 0) return i;
        }
      }
      return -1;
    } else if (num_minions == 0) {
      return -1;
    } else {
      return rng.random(num_minions, key);
    }
  }

  // minion with the lowest attack
  int lowest_attack_target(BattleRNG& rng, RNGKey key) const {
    int num_lowest = 0, lowest_attack = std::numeric_limits<int>::max();
    for (int i=0; minions.contains(i); ++i) {
      if (minions[i].attack < lowest_attack) {
        num_lowest = 1;
        lowest_attack = minions[i].attack;
      } else if (minions[i].attack == lowest_attack) {
        num_lowest++;
      }
    }
    int pick = rng.random(num_lowest, key);
    for (int i=0; minions.contains(i); ++i) {
      if (minions[i].attack == lowest_attack) {
        if (pick-- == 0) return i;
      }
    }
    return -1;
  }

  int random_living_minion(BattleRNG& rng, RNGKey key) const {
    int num_alive = 0;
    for (int i=0; minions.contains(i); ++i) {
      if (!minions[i].dead()) {
        num_alive++;
      }
    }
    if (num_alive == 0) return -1;
    int pick = rng.random(num_alive, key);
    for (int i=0; minions.contains(i); ++i) {
      if (!minions[i].dead()) {
        if (pick-- == 0) return i;
      }
    }
    return -1;
  }

  // Permanent buffs

  template <typename Condition>
  void buff_all_if(int attack, int health, Condition c) {
    minions.for_each_alive([&](Minion& m){
      if (c(m)) m.buff(attack,health);
    });
  }
  void buff_all(int attack, int health) {
    minions.for_each_alive([&](Minion& m){
      m.buff(attack,health);
    });
  }

  // Specific buffs

  template <typename F>
  void for_random_living_minion(F fun, BattleRNG& rng, RNGKey key) {
    int i = random_living_minion(rng,key);
    if (i != -1) fun(minions[i]);
  }
  void give_random_minion_divine_shield(BattleRNG& rng, int player) {
    for_random_living_minion([](Minion& m) { m.divine_shield = true; }, rng, rng_key(RNGType::GiveDivineShield,player));
  }
  void buff_random_minion(int attack, int health, BattleRNG& rng, int player) {
    for_random_living_minion([=](Minion& m) { m.buff(attack, health); }, rng, rng_key(RNGType::Buff,player,attack+(health<<8)));
  }

  // Duplication effects (Brann/Rivendare/Khadgar)

  int extra_summon_count() const {
    return has_minion(MinionType::Khadgar) + 1;
  }
  int extra_deathrattle_count() const {
    return has_minion(MinionType::BaronRivendare) + 1;
  }
  int extra_battlecry_count() const {
    return has_minion(MinionType::BrannBronzebeard) + 1;
  }
  // is the minion present? return 0 if not, 1 if yes, 2 if golden
  int has_minion(MinionType type) const {
    int have = 0;
    minions.for_each_alive([type,&have](Minion const& m) {
      if (m.type == type) have = max(have, m.golden ? 2 : 1);
    });
    return have;
  }

  // Properties of minions array

  bool full() const {
    return minions.full();
  }

  int total_stars() const {
    int total = 0;
    minions.for_each([&total](Minion const& m) {
      total += m.stars();
    });
    return total;
  }

  int total_stats() const {
    int total = 0;
    minions.for_each([&total](Minion const& m) {
      total += m.attack + m.health;
    });
    return total;
  }

  // Auras

  void recompute_auras(Board const* enemy_board = nullptr);

  template <typename Condition>
  void aura_buff_others_if(int attack, int health, int pos, Condition c) {
    // Note: also buff "dead" minions, they might not die because of the buff
    minions.for_each_with_pos([=](int i,Minion& m) {
      if (i != pos && c(m)) m.aura_buff(attack,health);
    });
  }

  void aura_buff_adjacent(int attack, int health, int pos) {
    if (minions.contains(pos-1)) minions[pos-1].aura_buff(attack,health);
    if (minions.contains(pos+1)) minions[pos+1].aura_buff(attack,health);
  }

};

inline ostream& operator << (ostream& s, Board const& b) {
  if (b.level) {
    s << "level " << b.level << endl;
  }
  if (b.health) {
    s << "health " << b.health << endl;
  }
  if (b.use_hero_power) {
    s << "heropower " << b.hero << endl;
  }
  s << b.minions;
  return s;
}

