#include "battle.hpp"
using std::cout;

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

void Battle::do_hero_power(HeroPower hp, int player) {
  switch(hp) {
    case HeroPower::None:
      break;
    case HeroPower::Nefarian:
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
    default:;
  }
}

