#include "battle.hpp"
using std::endl;

// -----------------------------------------------------------------------------
// Course of battle, attacking
// -----------------------------------------------------------------------------

void Battle::run() {
  start();
  int round = 0;
  bool missed_prev = 0;
  while (!done()) {
    round++;
    bool ok = attack_round();
    if (missed_prev && !ok) {
      turn = 2; // indicate battle is done
      return;
    }
    missed_prev = !ok;
    // track battles that never end
    if (round > 1000000) {
      std::cerr << "ERROR: infinite loop?" << endl;
      std::cerr << *this;
      break;
    }
  }
}

void Battle::start() {
  if (turn >= 0) return;
  // player with most minions attacks first
  int n0 = board[0].minions.size(), n1 = board[1].minions.size();
  if (n0 > n1) {
    turn = 0;
  } else if (n0 < n1) {
    turn = 1;
  } else {
    turn = rng.random(2, rng_key(RNGType::FirstPlayer));
  }
  board[0].next_attacker = 0;
  board[1].next_attacker = 0;
  do_hero_powers();
}

int find_attacker(Board const& board) {
  int from = board.next_attacker;
  for (int tries=0; tries<BOARDSIZE; ++tries) {
    if (from >= BOARDSIZE || !board.minions.contains(from)) {
      from = 0;
    }
    if (board.minions.contains(from) && board.minions[from].attack > 0) {
      return from;
    }
  }
  return -1;
}

bool Battle::attack_round() {
  // find attacker
  Board& active = board[turn];
  int from = find_attacker(active);
  if (from == -1) {
    // no minions that can attack, switch players
    turn = 1 - turn;
    return false;
  }
  // info on attacker
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
  return true;
}

void Battle::single_attack_by(int player, int from) {
  Minion& attacker = board[player].minions[from];
  bool cleave = attacker.cleave();
  // find a target
  Board& enemy = board[1-player];
  if (enemy.minions.empty()) return;
  int target = attacker.type == MinionType::ZappSlywick
                 ? enemy.lowest_attack_target(rng, rng_key(RNGType::Attack,player,attacker))
                 : enemy.random_attack_target(rng, rng_key(RNGType::Attack,player,attacker));
  if (verbose && log) {
    *log << "attack by " << player << "." << from << ", " << attacker << (cleave ? "[C]" : "") << " to " << target << endl;
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
    if (enemy.minions.contains(pos)) {
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
  board[player].minions.for_each_alive([&](Minion& m) {
    on_after_friendly_attack(m, attacker);
  });
}

// -----------------------------------------------------------------------------
// Damage
// -----------------------------------------------------------------------------

bool Battle::damage(int player, int pos, int amount, bool poison) {
  if (amount <= 0) return false;
  Minion& m = board[player].minions[pos];
  if (verbose >= 2 && log) {
    *log << "damage of " << amount << (poison ? "[P]" : "") << " to " << player << "." << pos << ", " << m << endl;
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
  if (verbose >= 4 && log) {
    *log << "damage by " << attacker << " to " << player << "." << pos << endl;
  }
  return damage(player, pos, attacker.attack, attacker.poison);
}

void Battle::damage_random_minion(int player, int amount) {
  int i = board[player].random_living_minion(rng, rng_key(RNGType::Damage,player,amount));
  if (i != -1) {
    damage(player, i, amount);
  }
}

void Battle::damage_all(int player, int amount) {
  board[player].minions.for_each_with_pos([&](int pos, Minion& m) {
    if (!m.dead()) damage(player, pos, amount);
  });
}

void Battle::on_break_divine_shield(int player) {
  board[player].minions.for_each_alive([&](Minion& m) {
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
      for (int i=0; board.minions.contains(i); ++i) {
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
      board.minions.remove_all_from(next);
    }
    if (num_dead[0] == 0 && num_dead[1] == 0) return;
    // run death triggers
    // Note: the order here can matter for the outcome of battles.
    // Normal hearthstone uses the order in which minions are played, but it is not clear how that works in battlegrounds
    // to remain balanced, always run attacking player first
    recompute_auras();
    for (int player=turn; player>=0 && player<2; turn ? --player : ++player) {
      for (int i=0; i<num_dead[player]; ++i) {
        on_death(dead_minions[player][i], player, positions[player][i]);
      }
    }
    // we might have lost some auras, causing more things to die, so loop
  }
}

void Battle::on_death(Minion const& dead_minion, int player, int pos) {
  if (verbose && log) {
    *log << "death: " << dead_minion << " at " << player << "." << pos << endl;
  }
  do_deathrattle(dead_minion, player, pos);
  board[player].minions.for_each_alive([&,player](Minion& m) {
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
  for (int i=0; i<count && !board[player].minions.full(); ++i) {
    board[player].insert(pos, m);
    on_summoned(board[player].minions[pos], player);
  }
  recompute_auras();
}

void Battle::summon_for_opponent(Minion const& m, int player) {
  int count = board[player].extra_summon_count();
  for (int i=0; i<count && !board[1-player].minions.full(); ++i) {
    int pos = board[1-player].append(m);
    on_summoned(board[1-player].minions[pos], player);
  }
  recompute_auras();
}

void Battle::on_summoned(Minion& summoned, int player) {
  // summon triggers
  board[player].minions.for_each_alive([&](Minion& m) {
    on_friendly_summon(m, summoned, player);
  });
}

// -----------------------------------------------------------------------------
// Hero powers
// -----------------------------------------------------------------------------

void Battle::do_hero_powers() {
  for (int player=0; player<2; ++player) {
    if (board[player].use_hero_power) {
      if (verbose >= 2 && log) {
        *log << "Hero power " << board[player].hero << " for " << player << endl;
      }
      do_hero_power(board[player].hero, player);
      board[player].use_hero_power = false;
    }
  }
}

// -----------------------------------------------------------------------------
// Auras
// -----------------------------------------------------------------------------

// As an optimization, we track if there are any minions with auras on the board
// if there are none, we can skip this step.

void Battle::recompute_auras() {
  for (int player=0; player<2; ++player) {
    recompute_auras(player);
  }
}

void Battle::recompute_auras(int player) {
  board[player].recompute_auras(&board[1-player]);
}

bool recompute_aura_from(Minion& m, int pos, Board& board, Board const* enemy_board);

void Board::recompute_auras(Board const* enemy_board) {
  if (!any_auras) return;
  // clear auras
  minions.for_each([&](Minion& m) {
    m.clear_aura_buff();
  });
  any_auras = false;
  // recompute auras
  minions.for_each_with_pos([this,enemy_board](int pos, Minion& m) {
    if (recompute_aura_from(m, pos, *this, enemy_board)) {
      any_auras = true;
    }
  });
  // handle invalid auras
  minions.for_each([](Minion& m) {
    if (m.invalid_aura) {
      m.invalid_aura = false;
      // stats didn't include aura, now that we have recomputed aura effects, we can compensate
      m.attack -= m.attack_aura;
      m.health -= m.health_aura;
    }
  });
}

