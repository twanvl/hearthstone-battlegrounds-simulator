#include "battle.hpp"
using std::cout;

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

void Battle::do_hero_power(HeroType hp, int player) {
  switch(hp) {
    case HeroType::None:
      break;
    case HeroType::Nefarian:
      damage_all(1-player, 1);
      break;
    case HeroType::RagnarosTheFirelord:
      damage_random_minion(1-player, 8);
      damage_random_minion(1-player, 8);
      break;
    case HeroType::PatchesThePirate:
      damage_random_minion(1-player, 4);
      damage_random_minion(1-player, 4);
      break;
    case HeroType::TheLichKing: {
      int i = board[player].minions.size() - 1;
      if (i >= 0) {
        board[player].minions[i].reborn = true;
      }
      break;
    }
    case HeroType::Giantfin:
      board[player].minions.for_each([](Minion& m){
        m.deathrattle_murlocs = 1;
      });
      break;
    case HeroType::ProfessorPutricide:
      if (board[player].minions.contains(0)) {
        board[player].minions[0].attack += 10;
      }
      break;
    default:;
  }
}

