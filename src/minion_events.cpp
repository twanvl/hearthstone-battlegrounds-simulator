#include "battle.hpp"

// -----------------------------------------------------------------------------
// Aura buffs
// -----------------------------------------------------------------------------

void Battle::recompute_aura_from(Minion& m, int player, int pos) {
  int factor = m.golden ? 2 : 1;
  switch (m.type) {
    case MinionType::DireWolfAlpha:
      board[player].aura_buff_adjacent(factor, factor, pos);
      break;
    case MinionType::MurlocWarleader:
      board[player].aura_buff_others_if(double_if_golden(2,m.golden), 0, pos, [](Minion const& to){ return to.has_tribe(Tribe::Murloc); });
      break;
    case MinionType::OldMurkEye: {
      // count murlocs for both players
      int count = 0;
      for (int who=0; who<2; ++who) {
        board[who].for_each_with_pos([=,&count](int i, Minion const& to) {
          if ((who != player || pos != i) && to.has_tribe(Tribe::Murloc)) count++;
        });
      }
      m.buff(factor*count,0);
      break;
    }
    case MinionType::PhalanxCommander:
      board[player].aura_buff_others_if(double_if_golden(2,m.golden), 0, pos, [](Minion const& to){ return to.taunt; });
      break;
    case MinionType::Siegebreaker:
      board[player].aura_buff_others_if(double_if_golden(1,m.golden), 0, pos, [](Minion const& to){ return to.has_tribe(Tribe::Demon); });
      break;
    case MinionType::MalGanis:
      board[player].aura_buff_others_if(double_if_golden(2,m.golden), double_if_golden(2,m.golden), pos,
        [](Minion const& to){ return to.has_tribe(Tribe::Demon); });
      break;
    default:; // nothing
  }
}

// -----------------------------------------------------------------------------
// Deathrattles
// -----------------------------------------------------------------------------

#define TWICE_IF_GOLDEN() for(int i=0;i<(m.golden?2:1);++i)

void Battle::do_base_deathrattle(Minion const& m, int player, int pos) {
  switch (m.type) {
    // Tier 1
    case MinionType::Mecharoo:
      summon(Minion(MinionType::JoEBot,m.golden), player, pos);
      break;
    case MinionType::SelflessHero:
      TWICE_IF_GOLDEN() {
        board[player].give_random_minion_divine_shield();
      }
      break;
    // Tier 2
    case MinionType::HarvestGolem:
      summon(Minion(MinionType::DamagedGolem,m.golden), player, pos);
      break;
    case MinionType::KaboomBot:
      TWICE_IF_GOLDEN() {
        damage_random_minion(1-player, 4);
      }
      // don't need to call check_for_deaths();
      break;
    case MinionType::KindlyGrandmother:
      summon(Minion(MinionType::BigBadWolf,m.golden), player, pos);
      break;
    case MinionType::MountedRaptor:
      TWICE_IF_GOLDEN() {
        summon(random_one_cost_minion(), player, pos);
      }
      break;
    case MinionType::RatPack:
      summon_many(m.attack, Minion(MinionType::Rat,m.golden), player, pos);
      break;
    case MinionType::SpawnOfNZoth:
      board[player].buff_all(double_if_golden(1,m.golden), double_if_golden(1,m.golden));
      break;
    // Tier 3
    case MinionType::InfestedWolf:
      summon_many(2, Minion(MinionType::Spider,m.golden), player, pos);
      break;
    case MinionType::PilotedShredder:
      TWICE_IF_GOLDEN() {
        summon(random_two_cost_minion(), player, pos);
      }
      break;
    case MinionType::ReplicatingMenace:
      summon_many(3, Minion(MinionType::Microbot,m.golden), player, pos);
      break;
    case MinionType::TortollanShellraiser: {
      int amount = double_if_golden(1,m.golden);
      board[player].buff_random_minion(amount,amount);
      break;
    }
    // Tier 4
    case MinionType::PilotedSkyGolem:
      TWICE_IF_GOLDEN() {
        summon(random_four_cost_minion(), player, pos);
      }
      break;
    case MinionType::TheBeast:
      summon_for_opponent(MinionType::FinkleEinhorn, player);
      break;
    // Tier 5
    case MinionType::GoldrinnTheGreatWolf: {
      int amount = double_if_golden(4,m.golden);
      board[player].buff_all_if(amount, amount, [](Minion const& x){return x.has_tribe(Tribe::Beast);});
      break;
    }
    case MinionType::MechanoEgg:
      summon(Minion(MinionType::Robosaur,m.golden), player, pos);
      break;
    case MinionType::SatedThreshadon:
      summon_many(3, Minion(MinionType::MurlocScout,m.golden), player, pos);
      break;
    case MinionType::SavannahHighmane:
      summon_many(2, Minion(MinionType::Hyena,m.golden), player, pos);
      break;
    // Tier 6
    case MinionType::Ghastcoiler:
      for (int i=0; i<double_if_golden(2,m.golden); ++i) {
        summon(random_deathrattle_minion(), player, pos);
      }
      break;
    case MinionType::KangorsApprentice:
      for (int i=0; i<double_if_golden(2,m.golden) && mechs_that_died[player][i].exists(); ++i) {
        summon(mechs_that_died[player][i].new_copy(), player, pos);
      }
      break;
    case MinionType::SneedsOldShredder:
      TWICE_IF_GOLDEN() {
        summon(random_legendary_minion(), player, pos);
      }
      break;
    default:;
  }
}

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

void Battle::on_friendly_summon(Minion& m, Minion& summoned, int player) {
  switch (m.type) {
    case MinionType::MurlocTidecaller:
      if (summoned.has_tribe(Tribe::Murloc)) {
        m.buff(double_if_golden(1,m.golden), 0);
      }
      break;
    case MinionType::CobaltGuardian:
      if (summoned.has_tribe(Tribe::Mech)) {
        m.divine_shield = true;
      }
      break;
    case MinionType::PackLeader:
      if (summoned.has_tribe(Tribe::Beast)) {
        summoned.buff(double_if_golden(3,m.golden), 0);
      }
      break;
    case MinionType::MamaBear:
      if (summoned.has_tribe(Tribe::Beast)) {
        summoned.buff(double_if_golden(4,m.golden), double_if_golden(4,m.golden));
      }
      break;
    case MinionType::OldMamaBear:
      if (summoned.has_tribe(Tribe::Beast)) {
        summoned.buff(double_if_golden(5,m.golden), double_if_golden(5,m.golden));
      }
      break;
    default:;
  }
}

void Battle::on_friendly_death(Minion& m, Minion const& dead_minion, int player) {
  switch (m.type) {
    case MinionType::ScavengingHyena:
      if (dead_minion.has_tribe(Tribe::Beast)) {
        m.buff(double_if_golden(2,m.golden), double_if_golden(1,m.golden));
      }
      break;
    case MinionType::SoulJuggler:
      if (dead_minion.has_tribe(Tribe::Demon)) {
        damage_random_minion(1-player, double_if_golden(3,m.golden));
        // already in a loop that does check_for_deaths();
      }
      break;
    case MinionType::Junkbot:
      if (dead_minion.has_tribe(Tribe::Mech)) {
        m.buff(double_if_golden(2,m.golden), double_if_golden(2,m.golden));
      }
      break;
    default:;
  }
}

void Battle::on_damaged(Minion const& m, int player, int pos) {
  switch (m.type) {
    case MinionType::ImpGangBoss:
      // Note: summons to the right
      summon(Minion(MinionType::Imp,m.golden), player, pos+1);
      break;
    case MinionType::SecurityRover:
      summon(Minion(MinionType::GuardBot,m.golden), player, pos+1);
      break;
    default:;
  }
}

void Battle::on_attack_and_kill(Minion& m, int player, int pos, bool overkill) {
  switch (m.type) {
    case MinionType::IronhideDirehorn:
      if (overkill) {
        summon(Minion(MinionType::IronhideRunt, m.golden), player, pos+1);
      }
      break;
    case MinionType::TheBoogeymonster:
      m.buff(double_if_golden(2,m.golden),double_if_golden(2,m.golden));
      break;
    default:;
  }
}

void Battle::on_after_friendly_attack(Minion& m, Minion const& attacker) {
  switch (m.type) {
    case MinionType::FesterootHulk:
      m.buff(double_if_golden(1,m.golden),0);
      break;
    default:;
  }
}

void Battle::on_break_friendly_divine_shield(Minion& m, int player) {
  switch (m.type) {
    case MinionType::BolvarFireblood:
      m.buff(double_if_golden(2,m.golden),0);
      break;
    default:;
  }
}

// -----------------------------------------------------------------------------
// Battlecries
// -----------------------------------------------------------------------------

