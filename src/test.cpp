#include "battle.hpp"
#include <vector>
#include <algorithm>
using std::vector;

// -----------------------------------------------------------------------------
// Example games
// -----------------------------------------------------------------------------

// From https://www.youtube.com/watch?v=TV0HSwbhasQ
Battle game1(int turn) {
  Battle battle;
  Board& player = battle.board[0];
  Board& enemy = battle.board[1];
  if (turn == 1) {
    // Toki
    player.minions[0] = Minion(MinionType::VulgarHomunculus);
    // Yogg
    enemy.minions[0] = Minion(MinionType::RockpoolHunter);
    enemy.minions[0].buff(1,1);
    // outcome: 0
  } else if (turn == 2) {
    // Toki
    player.minions[0] = Minion(MinionType::VulgarHomunculus);
    // Rat King
    enemy.minions[0] = Minion(MinionType::DireWolfAlpha);
    enemy.minions[0].buff(1,2);
    // outcome: 0
  } else if (turn == 3) {
    // Toki
    player.minions[0] = Minion(MinionType::VulgarHomunculus);
    player.minions[1] = Minion(MinionType::AlleyCat);
    player.minions[2] = Minion(MinionType::AlleyCat);
    player.minions[3] = Minion(MinionType::TabbyCat);
    // Akazamzarak
    enemy.minions[0] = Minion(MinionType::HarvestGolem);
    enemy.minions[1] = Minion(MinionType::Voidwalker);
    // outcome: 0
  } else if (turn == 4) {
    player.minions[0] = Minion(MinionType::KaboomBot);
    player.minions[1] = Minion(MinionType::KaboomBot);
    player.minions[2] = Minion(MinionType::AlleyCat);
    player.minions[3] = Minion(MinionType::AlleyCat);
    player.minions[4] = Minion(MinionType::TabbyCat);
    player.minions[5] = Minion(MinionType::VulgarHomunculus);
    // Yogg
    enemy.minions[0] = Minion(MinionType::KaboomBot);
    enemy.minions[1] = Minion(MinionType::KaboomBot);
    enemy.minions[2] = Minion(MinionType::RockpoolHunter);
    enemy.minions[2].buff(2,2);
    enemy.minions[3] = Minion(MinionType::Zoobot);
    enemy.minions[4] = Minion(MinionType::VulgarHomunculus);
    enemy.minions[4].buff(1,1);
    // outcome: -1
  } else if (turn == 5) {
    player.minions[0] = Minion(MinionType::KaboomBot);
    player.minions[1] = Minion(MinionType::KaboomBot);
    player.minions[2] = Minion(MinionType::NightmareAmalgam);
    player.minions[3] = Minion(MinionType::MetaltoothLeaper);
    player.minions[4] = Minion(MinionType::AlleyCat);
    player.minions[5] = Minion(MinionType::AlleyCat);
    player.minions[6] = Minion(MinionType::VulgarHomunculus);
    player.minions[0].buff(2,0);
    player.minions[1].buff(2,0);
    player.minions[2].buff(2,0);
    // Akazamzarak
    enemy.minions[0] = Minion(MinionType::HarvestGolem);
    enemy.minions[1] = Minion(MinionType::NightmareAmalgam);
    enemy.minions[1].buff(3,3);
    enemy.minions[2] = Minion(MinionType::Zoobot);
    enemy.minions[3] = Minion(MinionType::Voidwalker);
    enemy.minions[4] = Minion(MinionType::NethrezimOverseer);
    // outcome: 0
  } else if (turn == 6) {
    player.minions[0] = Minion(MinionType::KaboomBot);
    player.minions[1] = Minion(MinionType::KaboomBot);
    player.minions[2] = Minion(MinionType::PilotedShredder);
    player.minions[3] = Minion(MinionType::NightmareAmalgam);
    player.minions[4] = Minion(MinionType::MetaltoothLeaper);
    player.minions[5] = Minion(MinionType::MetaltoothLeaper);
    player.minions[6] = Minion(MinionType::VulgarHomunculus);
    player.minions[0].buff(4,0);
    player.minions[1].buff(4,0);
    player.minions[2].buff(2,0);
    player.minions[3].buff(4,0);
    player.minions[4].buff(2,0);
    // Rat King
    enemy.minions[0] = Minion(MinionType::CobaltGuardian);
    enemy.minions[0].buff(1,1);
    enemy.minions[0].divine_shield = true;
    enemy.minions[1] = Minion(MinionType::Houndmaster);
    enemy.minions[2] = Minion(MinionType::KindlyGrandmother);
    enemy.minions[2].buff(2,1);
    enemy.minions[3] = Minion(MinionType::KindlyGrandmother);
    enemy.minions[4] = Minion(MinionType::NightmareAmalgam);
    enemy.minions[4].buff(6,5);
    enemy.minions[4].taunt = true;
    enemy.minions[4].deathrattle_microbots = 3;
    // outcome: 2
  } else if (turn == 7) {
    player.minions[0] = Minion(MinionType::CaveHydra);
    player.minions[1] = Minion(MinionType::KaboomBot);
    player.minions[2] = Minion(MinionType::KaboomBot);
    player.minions[3] = Minion(MinionType::PilotedShredder);
    player.minions[4] = Minion(MinionType::NightmareAmalgam);
    player.minions[5] = Minion(MinionType::MetaltoothLeaper);
    player.minions[1].buff(4,0);
    player.minions[2].buff(4,0);
    player.minions[3].buff(2,0);
    player.minions[4].buff(4,0);
    player.minions[5].buff(2,0);
    // bartendotron
    enemy.minions[0] = Minion(MinionType::GoldrinnTheGreatWolf);
    enemy.minions[1] = Minion(MinionType::PilotedShredder);
    enemy.minions[2] = Minion(MinionType::AlleyCat, true);
    enemy.minions[3] = Minion(MinionType::NightmareAmalgam);
    enemy.minions[3].buff(2,2);
    enemy.minions[3].taunt = true;
    enemy.minions[4] = Minion(MinionType::Houndmaster);
    // outcome 5
  } else if (turn == 8) {
    player.minions[0] = Minion(MinionType::CaveHydra);
    player.minions[1] = Minion(MinionType::KaboomBot);
    player.minions[2] = Minion(MinionType::KaboomBot);
    player.minions[3] = Minion(MinionType::NightmareAmalgam);
    player.minions[4] = Minion(MinionType::LightfangEnforcer);
    player.minions[5] = Minion(MinionType::ImpGangBoss);
    player.minions[0].buff(2,2);
    player.minions[1].buff(6,0);
    player.minions[2].buff(8,2);
    player.minions[3].buff(6,2);
    player.minions[5].buff(2,2);
    // afk
    enemy.minions[0] = Minion(MinionType::KaboomBot);
    enemy.minions[1] = Minion(MinionType::KaboomBot);
    enemy.minions[2] = Minion(MinionType::CobaltGuardian);
    enemy.minions[2].divine_shield = true;
    enemy.minions[3] = Minion(MinionType::SecurityRover);
    enemy.minions[4] = Minion(MinionType::MicroMachine);
    enemy.minions[4].buff(3,0);
    enemy.minions[5] = Minion(MinionType::Junkbot);
    enemy.minions[6] = Minion(MinionType::PsychOTron);
    enemy.minions[6].buff(2,2);
    // outcome: -9
  } else if (turn == 9) {
    player.append(Minion(MinionType::CaveHydra).set_stats(4,6));
    player.append(Minion(MinionType::KaboomBot, true).set_stats(23,9).add_deathrattle_microbots(3));
    player.append(Minion(MinionType::NightmareAmalgam).set_stats(16,9).add_deathrattle_microbots(3));
    player.append(Minion(MinionType::LightfangEnforcer));
    player.append(Minion(MinionType::ImpGangBoss).set_stats(6,8));
    // rat king
    enemy.append(Minion(MinionType::CobaltGuardian).set_stats(11,9).set_divine_shield());
    enemy.append(Minion(MinionType::BrannBronzebeard));
    enemy.append(Minion(MinionType::KindlyGrandmother,true).set_stats(7,8));
    enemy.append(Minion(MinionType::CobaltGuardian).set_divine_shield());
    enemy.append(Minion(MinionType::ScrewjankClunker));
    enemy.append(Minion(MinionType::OldMurkEye).set_stats(8,8,2,0));
    enemy.append(Minion(MinionType::NightmareAmalgam).set_stats(17,17).set_taunt().set_poison());
    // outcome: -8
  } else if (turn == 10) {
    player.append(Minion(MinionType::BrannBronzebeard));
    player.append(Minion(MinionType::CaveHydra).set_stats(8,10));
    player.append(Minion(MinionType::KaboomBot, true).set_stats(25,11).add_deathrattle_microbots(3));
    player.append(Minion(MinionType::NightmareAmalgam).set_stats(18,11).add_deathrattle_microbots(3));
    player.append(Minion(MinionType::LightfangEnforcer));
    player.append(Minion(MinionType::ImpGangBoss).set_stats(12,14));
    // dead player
    enemy.append(Minion(MinionType::GoldrinnTheGreatWolf));
    enemy.append(Minion(MinionType::PilotedShredder));
    enemy.append(Minion(MinionType::AlleyCat, true));
    enemy.append(Minion(MinionType::NightmareAmalgam).set_stats(5,6).set_taunt());
    enemy.append(Minion(MinionType::Houndmaster));
    // outcome: 17
  } else if (turn == 11) {
    player.append(Minion(MinionType::LightfangEnforcer));
    player.append(Minion(MinionType::KaboomBot, true).set_stats(33,21).set_taunt().set_divine_shield().add_deathrattle_microbots(3));
    player.append(Minion(MinionType::CaveHydra).set_stats(18,20));
    player.append(Minion(MinionType::BrannBronzebeard));
    player.append(Minion(MinionType::VirmenSensei));
    player.append(Minion(MinionType::NightmareAmalgam).set_stats(24,17).add_deathrattle_microbots(3));
    player.append(Minion(MinionType::ImpGangBoss).set_stats(15,16));
    // rat king
    enemy.append(Minion(MinionType::CobaltGuardian).set_stats(22,10).set_divine_shield().add_deathrattle_microbots(3));
    enemy.append(Minion(MinionType::OldMurkEye).set_stats(8,8,2,0));
    enemy.append(Minion(MinionType::KindlyGrandmother,true).set_stats(7,8));
    enemy.append(Minion(MinionType::NightmareAmalgam).set_stats(27,21).set_taunt().set_divine_shield().set_poison().add_deathrattle_microbots(3));
    enemy.append(Minion(MinionType::BrannBronzebeard));
    enemy.append(Minion(MinionType::Junkbot).set_stats(9,5));
    // outcome: 4
  }
  return battle;
}

using namespace std;

vector<int> simulate(Battle const& b, int n) {
  vector<int> results;
  results.reserve(n);
  for (int i=0; i<n; ++i) {
    Battle copy = b;
    copy.turn = random(2);
    copy.verbose = 0;
    copy.run();
    results.push_back(copy.score());
  }
  sort(results.begin(), results.end());
  return results;
}

double mean(vector<int> const& x) {
  return accumulate(x.begin(), x.end(), 0.) / x.size();
}
void print_stats(vector<int> const& results) {
  int wins   = count_if(results.begin(), results.end(), [](int i) {return i > 0;});
  int losses = count_if(results.begin(), results.end(), [](int i) {return i < 0;});
  int ties = (int)results.size() - wins - losses;
  //double n = (double)results.size();
  cout << "win: " << wins << ", tie: " << ties << ", lose: " << losses << endl;
  cout << "mean score: " << mean(results) << endl;
  cout << "median score: " << results[results.size()/2] << endl;
  int steps = 10, n = static_cast<int>(results.size()) - 1;
  cout << "percentiles: ";
  for (int i=0; i <= steps; ++i) {
    cout << results[i*n/steps] << " ";
  }
  cout << endl;
}

int main() {
  // Test minion IO:
  

  //for (int i=0; i<5; ++i) {
  int i=11;{
  //for (int i=3; i<4; ++i) {
    Battle b = game1(i);
    b.turn = 0;
    b.verbose = 2;
    cout << "Turn " << i << endl;
    cout << b;
    cout << endl;
    while (!b.done()) {
      cout << "Attack by " << b.turn << "." << b.board[b.turn].next_attacker << endl;
      b.attack_round();
      cout << b;
      cout << endl;
    }
    cout << "score: " << b.score() << endl;
    cout << "----------------------------------" << endl;
    print_stats(simulate(game1(i),1000));
    cout << "----------------------------------" << endl;
  }
  cout << sizeof(Battle) << endl;
}

