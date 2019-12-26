#include "battle.hpp"

// -----------------------------------------------------------------------------
// Aura buffs
// -----------------------------------------------------------------------------

bool recompute_aura_from(Minion& m, int pos, Board& board, Board const* enemy_board) {
  switch (m.type) {
    case MinionType::DireWolfAlpha:
      board.aura_buff_adjacent(m.double_if_golden(1), 0, pos);
      return true;
    case MinionType::MurlocWarleader:
      board.aura_buff_others_if(m.double_if_golden(2), 0, pos, [](Minion const& to){ return to.has_tribe(Tribe::Murloc); });
      return true;
    case MinionType::OldMurkEye: {
      // count murlocs for both players
      int count = pos >= 0 ? -1 : 0; // exclude self
      count += board.count_if([](Minion const& to) { return to.has_tribe(Tribe::Murloc); });
      if (enemy_board) {
        count += enemy_board->count_if([](Minion const& to) { return to.has_tribe(Tribe::Murloc); });
      }
      m.aura_buff(m.double_if_golden(count),0);
      return true;
    }
    case MinionType::PhalanxCommander:
      board.aura_buff_others_if(m.double_if_golden(2), 0, pos, [](Minion const& to){ return to.taunt; });
      return true;
    case MinionType::Siegebreaker:
      board.aura_buff_others_if(m.double_if_golden(1), 0, pos, [](Minion const& to){ return to.has_tribe(Tribe::Demon); });
      return true;
    case MinionType::MalGanis:
      board.aura_buff_others_if(m.double_if_golden(2), m.double_if_golden(2), pos,
        [](Minion const& to){ return to.has_tribe(Tribe::Demon); });
      return true;
    default:;
      return false;
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
        board[player].give_random_minion_divine_shield(rng, player);
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
        summon(random_one_cost_minion(rng, player), player, pos);
      }
      break;
    case MinionType::RatPack:
      summon_many(m.attack, Minion(MinionType::Rat,m.golden), player, pos);
      break;
    case MinionType::SpawnOfNZoth:
      board[player].buff_all(m.double_if_golden(1), m.double_if_golden(1));
      break;
    // Tier 3
    case MinionType::InfestedWolf:
      summon_many(2, Minion(MinionType::Spider,m.golden), player, pos);
      break;
    case MinionType::PilotedShredder:
      TWICE_IF_GOLDEN() {
        summon(random_two_cost_minion(rng, player), player, pos);
      }
      break;
    case MinionType::ReplicatingMenace:
      summon_many(3, Minion(MinionType::Microbot,m.golden), player, pos);
      break;
    case MinionType::TortollanShellraiser: {
      int amount = m.double_if_golden(1);
      board[player].buff_random_minion(amount,amount,rng,player);
      break;
    }
    // Tier 4
    case MinionType::PilotedSkyGolem:
      TWICE_IF_GOLDEN() {
        summon(random_four_cost_minion(rng, player), player, pos);
      }
      break;
    case MinionType::TheBeast:
      summon_for_opponent(MinionType::FinkleEinhorn, player);
      break;
    // Tier 5
    case MinionType::GoldrinnTheGreatWolf: {
      int amount = m.double_if_golden(4);
      board[player].buff_all_if(amount, amount, [](Minion const& x){return x.has_tribe(Tribe::Beast);});
      break;
    }
    case MinionType::KingBagurgle: {
      int amount = m.double_if_golden(2);
      board[player].buff_all_if(amount, amount, [](Minion const& x){return x.has_tribe(Tribe::Murloc);});
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
      for (int i=0; i<m.double_if_golden(2); ++i) {
        summon(random_deathrattle_minion(rng, player), player, pos);
      }
      break;
    case MinionType::KangorsApprentice:
      for (int i=0; i<m.double_if_golden(2) && mechs_that_died[player][i].exists(); ++i) {
        summon(mechs_that_died[player][i].new_copy(), player, pos);
      }
      break;
    case MinionType::SneedsOldShredder:
      TWICE_IF_GOLDEN() {
        summon(random_legendary_minion(rng, player), player, pos);
      }
      break;
    default:;
  }
}

// -----------------------------------------------------------------------------
// Events
// -----------------------------------------------------------------------------

void on_friendly_summon(Minion& m, Minion& summoned) {
  switch (m.type) {
    case MinionType::MurlocTidecaller:
      if (summoned.has_tribe(Tribe::Murloc)) {
        m.buff(m.double_if_golden(1), 0);
      }
      break;
    case MinionType::CobaltGuardian:
      if (summoned.has_tribe(Tribe::Mech)) {
        m.divine_shield = true;
      }
      break;
    case MinionType::PackLeader:
      if (summoned.has_tribe(Tribe::Beast)) {
        summoned.buff(m.double_if_golden(3), 0);
      }
      break;
    case MinionType::MamaBear:
      if (summoned.has_tribe(Tribe::Beast)) {
        summoned.buff(m.double_if_golden(4), m.double_if_golden(4));
      }
      break;
    case MinionType::PreNerfMamaBear:
      if (summoned.has_tribe(Tribe::Beast)) {
        summoned.buff(m.double_if_golden(5), m.double_if_golden(5));
      }
      break;
    default:;
  }
}

void Battle::on_friendly_summon(Minion& m, Minion& summoned, int player) {
  ::on_friendly_summon(m, summoned);
}

void Battle::on_friendly_death(Minion& m, Minion const& dead_minion, int player) {
  switch (m.type) {
    case MinionType::ScavengingHyena:
      if (dead_minion.has_tribe(Tribe::Beast)) {
        m.buff(m.double_if_golden(2), m.double_if_golden(1));
      }
      break;
    case MinionType::SoulJuggler:
      if (dead_minion.has_tribe(Tribe::Demon)) {
        damage_random_minion(1-player, m.double_if_golden(3));
        // already in a loop that does check_for_deaths();
      }
      break;
    case MinionType::Junkbot:
      if (dead_minion.has_tribe(Tribe::Mech)) {
        m.buff(m.double_if_golden(2), m.double_if_golden(2));
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
      m.buff(m.double_if_golden(2),m.double_if_golden(2));
      break;
    default:;
  }
}

void Battle::on_after_friendly_attack(Minion& m, Minion const& attacker) {
  switch (m.type) {
    case MinionType::FesterootHulk:
      m.buff(m.double_if_golden(1),0);
      break;
    default:;
  }
}

void Battle::on_break_friendly_divine_shield(Minion& m, int player) {
  switch (m.type) {
    case MinionType::BolvarFireblood:
      m.buff(m.double_if_golden(2),0);
      break;
    default:;
  }
}

// -----------------------------------------------------------------------------
// Battlecries
// -----------------------------------------------------------------------------

