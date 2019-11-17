#include "battle.hpp"
using std::cout;
using std::endl;

// -----------------------------------------------------------------------------
// Course of battle, attacking
// -----------------------------------------------------------------------------

void Battle::run() {
  while (!done()) {
    attack_round();
  }
}

void Battle::start() {
  turn = random(2);
  do_hero_powers();
}

void Battle::attack_round() {
  // attacker
  Board& active = board[turn];
  if (active.next_attacker >= BOARDSIZE || !active.minions[active.next_attacker].exists()) {
    active.next_attacker = 0;
  }
  int from = active.next_attacker;
  active.next_attacker++;
  bool windfury = active.minions[from].windfury;
  active.track_pos[0] = from; // track if this minion stays alive
  // do attack
  single_attack_by(turn, from);
  // windfury?
  if (windfury && active.track_pos[0] > 0) {
    from = active.track_pos[0]; // minions might have been inserted/removed
    single_attack_by(turn, from);
  }
  // switch players
  turn = 1 - turn;
}

void Battle::single_attack_by(int player, int from) {
  Minion& attacker = board[player].minions[from];
  bool cleave = attacker.cleave();
  // find a target
  Board& enemy = board[1-player];
  int target = attacker.type == MinionType::ZappSlywick
                 ? enemy.lowest_attack_target() : enemy.random_attack_target();
  if (verbose) {
    cout << "attack by " << player << "." << from << ", " << attacker << (cleave ? "[C]" : "") << " to " << target << endl;
  }
  // Make a snapshot of the defending minion, so we know attack values
  Minion defender_snapshot = enemy.minions[target];
  // minions might move during all of this because of damage triggers
  enemy.track_pos[0] = target;
  if (cleave) {
    enemy.track_pos[1] = target-1;
    enemy.track_pos[2] = target+1;
  }
  // damage enemy minion(s)
  int kill = 0, overkill = 0;
  for (int i=0; i < (cleave ? 3 : 1) ; ++i) {
    int pos = enemy.track_pos[i];
    if (pos >= 0 && pos < BOARDSIZE && enemy.minions[pos].exists()) {
      damage(attacker, 1-player, pos);
      pos = enemy.track_pos[i];
      if (enemy.minions[pos].dead()) {
        kill++;
        if (enemy.minions[pos].health < 0) {
          overkill++;
        }
      }
    }
  }
  damage(defender_snapshot, player, from);
  // after attack triggers
  if (kill) {
    on_attack_and_kill(attacker, player, from, overkill);
  }
  on_after_friendly_attack(attacker, player);
  // remove dead minions and run deathrattles
  check_for_deaths();
}

void Battle::on_after_friendly_attack(Minion const& attacker, int player) {
  board[player].for_each_alive([&](Minion& m) {
    on_after_friendly_attack(m, attacker);
  });
}

// -----------------------------------------------------------------------------
// Damage
// -----------------------------------------------------------------------------

bool Battle::damage(int player, int pos, int amount, bool poison) {
  if (amount <= 0) return false;
  Minion& m = board[player].minions[pos];
  if (verbose >= 2) {
    cout << "damage of " << amount << (poison ? "[P]" : "") << " to " << player << "." << pos << ", " << m << endl;
  }
  if (m.divine_shield) {
    m.divine_shield = false;
    on_break_divine_shield(player);
    return false;
  } else {
    m.health -= amount;
    if (m.health > 0 && poison) {
      m.health = 0;
    }
    on_damaged(m, player, pos);
    // if (m.health <= 0) destroy_minion(player, pos);
    return true;
  }
}

bool Battle::damage(Minion const& attacker, int player, int pos) {
  if (verbose >= 4) {
    cout << "damage by " << attacker << " to " << player << "." << pos << endl;
  }
  return damage(player, pos, attacker.attack, attacker.poison);
}

void Battle::damage_random_minion(int player, int amount) {
  int i = board[player].random_living_minion();
  if (i != -1) {
    damage(player, i, amount);
  }
}

void Battle::damage_all(int player, int amount) {
  board[player].for_each_with_pos([&](int pos, Minion&) {
    damage(player, pos, amount);
  });
}

void Battle::on_break_divine_shield(int player) {
  board[player].for_each_alive([&](Minion& m) {
    on_break_friendly_divine_shield(m, player);
  });
}

// -----------------------------------------------------------------------------
// Dying minions
// -----------------------------------------------------------------------------

void Battle::check_for_deaths() {
  // Two step algorithm
  //  first: find dead minions, put them in a list and remove from board
  //  then: run their deathrattles and other triggers
  Minion dead_minions[2][BOARDSIZE];
  int positions[2][BOARDSIZE];
  int num_dead[2];
  while(true) {
    for (int player=0; player<2; ++player) {
      // filter to keep only alive minions
      Board& board = this->board[player];
      num_dead[player] = 0;
      int next = 0; // positions in cleaned up board
      for (int i=0; i<BOARDSIZE && board.minions[i].exists(); ++i) {
        if (board.next_attacker == i) {
          board.next_attacker = next;
        }
        if (board.minions[i].dead()) {
          positions[player][num_dead[player]] = next;
          dead_minions[player][num_dead[player]] = board.minions[i];
          // update tracked positions: this minion is dead
          for (int j=0; j<NUM_EXTRA_POS; ++j) {
            if (board.track_pos[j] == i) board.track_pos[j] = -1;
          }
          num_dead[player]++;
        } else {
          if (next < i) {
            board.minions[next] = board.minions[i];
            // update tracked positions: this minion has moved
            for (int j=0; j<NUM_EXTRA_POS; ++j) {
              if (board.track_pos[j] == i) board.track_pos[j] = next;
            }
          }
          next++;
        }
      }
      for (;next<BOARDSIZE && board.minions[next].exists(); ++next) {
        board.minions[next].clear();
      }
    }
    if (num_dead[0] == 0 && num_dead[1] == 0) return;
    // run death triggers
    for (int player=0; player<2; ++player) {
      board[player].recompute_auras();
      for (int i=0; i<num_dead[player]; ++i) {
        on_death(dead_minions[player][i], player, positions[player][i]);
      }
    }
    // we might have lost some auras, causing more things to die, so loop
  }
}

/*
void Battle::destroy_minion(int player, int pos) {
  Minion m = board[player].minions[pos]; // copy
  board[player].remove(pos);
  on_death(m, player, pos);
}
*/

void Battle::on_death(Minion const& dead_minion, int player, int pos) {
  if (verbose) {
    cout << "death: " << dead_minion << " at " << player << "." << pos << endl;
  }
  do_deathrattle(dead_minion, player, pos);
  board[player].for_each_alive([&,player](Minion& m) {
    on_friendly_death(m, dead_minion, player);
  });
  // track mechs that died
  if (dead_minion.has_tribe(Tribe::Mech)) {
    if (!mechs_that_died[player].full()) {
      mechs_that_died[player].append(dead_minion);
    }
  }
}

void Battle::do_deathrattle(Minion const& dead_minion, int player, int pos) {
  int count = board[player].extra_deathrattle_count();
  for (int i=0; i<count; ++i) {
    do_base_deathrattle(dead_minion, player, pos);
    summon_many(dead_minion.deathrattle_murlocs, MinionType::MurlocScout, player, pos);
    summon_many(dead_minion.deathrattle_microbots, MinionType::Microbot, player, pos);
    summon_many(dead_minion.deathrattle_golden_microbots, Minion(MinionType::Microbot,true), player, pos);
    summon_many(dead_minion.deathrattle_plants, MinionType::Plant, player, pos);
    if (dead_minion.reborn) {
      summon(dead_minion.reborn_copy(), player, pos);
    }
  }
}

// -----------------------------------------------------------------------------
// Summoning
// -----------------------------------------------------------------------------

void Battle::summon(Minion const& m, int player, int pos) {
  summon_many(1, m, player, pos);
}

void Battle::summon_many(int count, Minion const& m, int player, int pos) {
  if (count == 0) return;
  count *= board[player].extra_summon_count();
  for (int i=0; i<count && !board[player].full(); ++i) {
    board[player].insert(pos, m);
    on_summoned(board[player].minions[pos], player);
  }
  board[player].recompute_auras();
}

void Battle::summon_for_opponent(Minion const& m, int player) {
  int count = board[player].extra_summon_count();
  for (int i=0; i<count && !board[1-player].full(); ++i) {
    int pos = board[1-player].append(m);
    on_summoned(board[1-player].minions[pos], player);
  }
  board[1-player].recompute_auras();
}

void Battle::on_summoned(Minion& summoned, int player) {
  // summon triggers
  board[player].for_each_alive([&](Minion& m) {
    on_friendly_summon(m, summoned, player);
  });
}

// -----------------------------------------------------------------------------
// Hero powers
// -----------------------------------------------------------------------------

void Battle::do_hero_powers() {
  for (int player=0; player<2; ++player) {
    do_hero_power(board[player].hero_power, player);
    board[player].hero_power = HeroPower::None;
  }
}

