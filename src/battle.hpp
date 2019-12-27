#pragma once
#include "enums.hpp"
#include "board.hpp"
#include <iostream>
#include <cstdlib>
using std::ostream;
using std::endl;

// -----------------------------------------------------------------------------
// Battle state
// -----------------------------------------------------------------------------

const int MAX_MECHS_THAT_DIED = 4;

struct Battle {
  int turn = -1; // player to attack next
  Board board[2];
  // randomness
  BattleRNG& rng;
  // mechs that died for each player
  MinionArray<MAX_MECHS_THAT_DIED> mechs_that_died[2];
  // logging
  int verbose = 0;
  ostream* log;

  Battle(Board const& b0, Board const& b1, ostream* log = nullptr, BattleRNG& rng = global_battle_rng)
    : board{b0,b1}
    , rng(rng)
    , log(log)
  {
    recompute_auras();
  }

  bool started() const {
    return turn >= 0;
  }
  bool done() const {
    return board[0].minions.empty() || board[1].minions.empty() || turn >= 2;
  }

  // Positive: player 0 won, negative, player 1 won, score is total stars remaining
  int score() const {
    int stars0 = board[0].total_stars();
    int stars1 = board[1].total_stars();
    if (stars0 > 0 && stars1 > 0) return 0;
    return stars0 - stars1;
  }

  // Simulate a battle
  void run();
  // pre start: decide who goes first, run hero powers
  void start();

  // Attacking
  bool attack_round(); // return true if an attack happened
  void single_attack_by(int player, int pos);

  // Summon minions
  void summon(Minion const& m, int player, int pos);
  void summon_many(int count, Minion const& m, int player, int pos);
  void summon_for_opponent(Minion const& m, int player);

  // Damage

  // deal damage to a minion
  // note: doesn't check for deaths!
  // returns true if target was damaged
  bool damage(int player, int pos, int damage, bool poison=false);
  bool damage(Minion const& attacker, int player, int pos);
  void damage_random_minion(int player, int damage); // doesn't check for deaths
  void damage_all(int player, int damage); // doesn't check for deaths
  void check_for_deaths();

  // Events during battle
  void on_death(Minion const& m, int player, int pos);
  void on_summoned(Minion& summoned, int player, int pos, bool played);
  void on_after_friendly_attack(Minion const& attacker, int player);
  void on_break_divine_shield(int player);

  // Hero powers
  void do_hero_powers();
  void do_hero_power(HeroType, int player);

  // Auras
  void recompute_aura_from(Minion& m, int player, int pos);
  void recompute_auras(int player);
  void recompute_auras();
};

inline ostream& operator << (ostream& s, Battle const& b) {
  s << b.board[0];
  s << "VS" << endl;
  s << b.board[1];
  return s;
}

