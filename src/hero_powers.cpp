#include "hero_powers.hpp"
#include "battle.hpp"
using std::cout;

// -----------------------------------------------------------------------------
// Names
// -----------------------------------------------------------------------------

const char* hero_power_names[] = {"None", "Neffarian", "Ragnaros the Firelord", "Patches the Pirate","The Lich King", "Giantfin", "Professor Putricide"};

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

void Battle::do_hero_power(HeroPower hp, int player) {
  if (verbose >= 2 && hp != HeroPower::None) {
    cout << "Hero power " << hp << " for " << player << endl;
  }
  switch(hp) {
    case HeroPower::None:
      break;
    case HeroPower::Neffarian:
      damage_all(1-player, 1);
      break;
    case HeroPower::RagnarosTheFirelord:
      damage_random_minion(1-player, 8);
      damage_random_minion(1-player, 8);
      break;
    case HeroPower::PatchesThePirate:
      damage_random_minion(1-player, 3);
      damage_random_minion(1-player, 3);
      break;
    case HeroPower::TheLichKing: {
      int i = board[player].size() - 1;
      if (i >= 0) {
        board[player].minions[i].reborn = true;
      }
      break;
    }
    case HeroPower::Giantfin:
      board[player].for_each([](Minion& m){
        m.deathrattle_murlocs = 1;
      });
      break;
    case HeroPower::ProfessorPutricide:
      if (board[player].minions[0].exists()) {
        board[player].minions[0].attack += 10;
      }
      break;
  }
}

