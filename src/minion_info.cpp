#include "minion_info.hpp"
#include "board.hpp" // for random

// -----------------------------------------------------------------------------
// Tribe names
// -----------------------------------------------------------------------------

const char* tribe_names[] = {"None","Beast","Demon","Dragon","Mech","Murloc","All"};

// -----------------------------------------------------------------------------
// Minion info table
// -----------------------------------------------------------------------------

const MinionInfo minion_info[] = {
  {"(none)",0,Tribe::None,0,0,false,false,false,false},
  // Tier 1
  {"Alley Cat",1,Tribe::Beast,1,1,false,false,false,false},
  {"Dire Wolf Alpha",1,Tribe::Beast,2,2,false,false,false,false},
  {"Mecharoo",1,Tribe::Mech,1,1,false,false,false,false},
  {"Micro Machine",1,Tribe::Mech,1,2,false,false,false,false},
  {"Murloc Tidecaller",1,Tribe::Murloc,1,2,false,false,false,false},
  {"Murloc Tidehunter",1,Tribe::Murloc,2,1,false,false,false,false},
  {"Righteous Protector",1,Tribe::None,1,1,true,true,false,false},
  {"Rockpool Hunter",1,Tribe::Murloc,2,3,false,false,false,false},
  {"Selfless Hero",1,Tribe::None,2,1,false,false,false,false},
  {"Voidwalker",1,Tribe::Demon,1,3,true,false,false,false},
  {"Vulgar Homunculus",1,Tribe::Demon,2,4,true,false,false,false},
  {"Wrath Weaver",1,Tribe::Demon,1,1,false,false,false,false},
  // Tier 1 tokens
  {"Tabby Cat",1,Tribe::Beast,1,1,false,false,false,false},
  {"Jo-E Bot",1,Tribe::Mech,1,1,false,false,false,false},
  {"Murloc Scout",1,Tribe::Murloc,1,1,false,false,false,false},
  {"Amalgam",1,Tribe::All,1,1,false,false,false,false},
  {"Plant",1,Tribe::None,1,1,false,false,false,false},
  // Tier 2
  {"Annoy-o-Tron",2,Tribe::Mech,1,2,true,true,false,false},
  {"Harvest Golem",2,Tribe::Mech,2,3,false,false,false,false},
  {"Kaboom Bot",2,Tribe::Mech,2,2,false,false,false,false},
  {"Kindly Grandmother",2,Tribe::Beast,1,1,false,false,false,false},
  {"Metaltooth Leaper",2,Tribe::Mech,3,3,false,false,false,false},
  {"Mounted Raptor",2,Tribe::Beast,3,2,false,false,false,false},
  {"Murloc Warleader",2,Tribe::Murloc,3,3,false,false,false,false},
  {"Nethrezim Overseer",2,Tribe::Demon,2,4,false,false,false,false},
  {"Nightmare Amalgam",2,Tribe::All,3,4,false,false,false,false},
  {"Old Murk-Eye",2,Tribe::Murloc,2,4,false,false,false,false},
  {"Pogo-Hopper",2,Tribe::Mech,1,1,false,false,false,false},
  {"Rat Pack",2,Tribe::Beast,2,2,false,false,false,false},
  {"Scavenging Hyena",2,Tribe::Beast,2,2,false,false,false,false},
  {"Shielded Minibot",2,Tribe::Mech,2,2,true,false,false,false},
  {"Spawn of N'Zoth",2,Tribe::None,2,2,false,false,false,false},
  {"Zoobot",2,Tribe::Mech,3,3,false,false,false,false},
  // Tier 2 tokens
  {"Damaged Golem",1,Tribe::Mech,2,1,false,false,false,false},
  {"Big Bad Wolf",1,Tribe::Beast,3,2,false,false,false,false},
  {"Rat",1,Tribe::Beast,1,1,false,false,false,false},
  // Tier 3
  {"Cobalt Guardian",3,Tribe::Mech,6,3,false,false,false,false},
  {"Coldlight Seer",3,Tribe::Murloc,2,3,false,false,false,false},
  {"Crowd Favorite",3,Tribe::None,4,4,false,false,false,false},
  {"Crystalweaver",3,Tribe::None,5,4,false,false,false,false},
  {"Houndmaster",3,Tribe::None,4,3,false,false,false,false},
  {"Imp Gang Boss",3,Tribe::Demon,2,4,false,false,false,false},
  {"Infested Wolf",3,Tribe::Beast,3,3,false,false,false,false},
  {"Khadgar",3,Tribe::None,2,2,false,false,false,false},
  {"Pack Leader",3,Tribe::None,3,3,false,false,false,false},
  {"Phalanx Commander",3,Tribe::None,4,5,false,false,false,false},
  {"Piloted Shredder",3,Tribe::Mech,4,3,false,false,false,false},
  {"Psych-o-Tron",3,Tribe::Mech,3,4,true,true,false,false},
  {"Replicating Menace",3,Tribe::Mech,3,1,false,false,false,false},
  {"ScrewjankClunker",3,Tribe::Mech,2,5,false,false,false,false},
  {"ShifterZerus",3,Tribe::None,1,1,false,false,false,false},
  {"Soul Juggler",3,Tribe::None,3,3,false,false,false,false},
  {"Tortollan Shellraiser",3,Tribe::None,0,0,true,false,false,false},
  // Tier 3 tokens
  {"Microbot",1,Tribe::Mech,1,1,false,false,false,false},
  {"Spider",1,Tribe::Beast,1,1,false,false,false,false},
  {"Imp",1,Tribe::Demon,1,1,false,false,false,false},
  // Tier 4
  {"Annoy-o-Module",4,Tribe::Mech,2,4,true,true,false,false},
  {"Bolvar Fireblood",4,Tribe::None,1,6,false,true,false,false},
  {"Cave Hydra",4,Tribe::Beast,2,4,false,false,false,false,true},
  {"Defender of Argus",4,Tribe::None,2,3,false,false,false,false},
  {"Festeroot Hulk",4,Tribe::None,2,7,false,false,false,false},
  {"Iron Sensei",4,Tribe::Mech,2,2,false,false,false,false},
  {"Junkbot",4,Tribe::Mech,1,5,false,false,false,false},
  {"Managerie Magician",4,Tribe::None,4,4,false,false,false,false},
  {"Piloted Sky Golem",4,Tribe::Mech,6,4,false,false,false,false},
  {"Security Rover",4,Tribe::Mech,2,6,false,false,false,false},
  {"Siegebreaker",4,Tribe::Demon,5,8,true,false,false,false},
  {"The Beast",4,Tribe::Beast,9,7,false,false,false,false},
  {"Toxfin",4,Tribe::Murloc,1,2,false,false,false,false},
  {"VirmenSensei",4,Tribe::None,4,5,false,false,false,false},
  // Tier 4 tokens
  {"Guard Bot",1,Tribe::Mech,2,3,true,false,false,false},
  {"Finkle Einhorn",1,Tribe::None,3,3,false,false,false,false},
  // Tier 5
  {"Annihilan Battlemaster",5,Tribe::Demon,3,1,false,false,false,false},
  {"Baron Rivendare",5,Tribe::None,1,7,false,false,false,false},
  {"Brann Bronzebeard",5,Tribe::None,2,4,false,false,false,false},
  {"Goldrinn the GreatWolf",5,Tribe::Beast,4,4,false,false,false,false},
  {"Ironhide Direhorn",5,Tribe::Beast,7,7,false,false,false,false},
  {"Lightfang Enforcer",5,Tribe::None,2,2,false,false,false,false},
  {"Mal'Ganis",5,Tribe::Demon,9,7,false,false,false,false},
  {"Mechano Egg",5,Tribe::Mech,0,8,false,false,false,false},
  {"Primalfin Lookout",5,Tribe::Murloc,3,2,false,false,false,false},
  {"Sated Threshadon",5,Tribe::Beast,5,7,false,false,false,false},
  {"Savannah Highmane",5,Tribe::Beast,6,5,false,false,false,false},
  {"Strongshell Scavenger",5,Tribe::None,2,3,false,false,false,false},
  {"The Boogeymonster",5,Tribe::None,6,7,false,false,false,false},
  // Tier 5 tokens
  {"Ironhide Runt",1,Tribe::Beast,5,5,false,false,false,false},
  {"Robosaur",1,Tribe::Mech,8,8,false,false,false,false},
  {"Hyena",1,Tribe::Beast,2,2,false,false,false,false},
  // Tier 6
  {"Foe Reaper 4000",6,Tribe::Mech,6,9,false,false,false,false,true},
  {"Gentle Megasaur",6,Tribe::Beast,5,4,false,false,false,false},
  {"Ghastcoiler",6,Tribe::Beast,7,7,false,false,false,false},
  {"Kangor's Apprentice",6,Tribe::None,3,6,false,false,false,false},
  {"Maexxna",6,Tribe::Beast,2,8,false,false,true,false},
  {"Mama Bear",6,Tribe::Beast,4,4,false,false,false,false},
  {"Pre-nerf Mama Bear",6,Tribe::Beast,5,5,false,false,false,false},
  {"Sneed's Old Shredder",6,Tribe::Mech,5,7,false,false,false,false},
  {"Voidlord",6,Tribe::Demon,3,9,false,false,false,false},
  {"Zapp Slywick",6,Tribe::None,0,0,false,false,false,true,false},
  //
  //{"MISSING",0,Tribe::None,0,0,false,false,false,false},
};

// -----------------------------------------------------------------------------
// Tables of minions at specific cost/rarity/keyword
// -----------------------------------------------------------------------------

const MinionType one_cost_minions[] = {
  MinionType::AlleyCat,
  MinionType::Mecharoo,
  MinionType::MurlocTidecaller,
  MinionType::RighteousProtector,
  MinionType::SelflessHero,
  MinionType::Voidwalker,
  MinionType::PogoHopper,
  MinionType::ShifterZerus,
  MinionType::Toxfin,
};
const MinionType two_cost_minions[] = {
  MinionType::DireWolfAlpha,
  MinionType::MicroMachine,
  MinionType::MurlocTidehunter,
  MinionType::RockpoolHunter,
  MinionType::VulgarHomunculus,
  MinionType::AnnoyOTron,
  MinionType::KindlyGrandmother,
  MinionType::ScavengingHyena,
  MinionType::ShieldedMinibot,
  MinionType::Khadgar,
};
const MinionType four_cost_minions[] = {
  MinionType::OldMurkEye,
  MinionType::CrowdFavorite,
  MinionType::Crystalweaver,
  MinionType::Houndmaster,
  MinionType::InfestedWolf,
  MinionType::PilotedShredder,
  MinionType::ReplicatingMenace,
  MinionType::ScrewjankClunker,
  MinionType::TortollanShellraiser,
  MinionType::AnnoyOModule,
  MinionType::DefenderOfArgus,
  MinionType::BaronRivendare,
  MinionType::StrongshellScavenger,
  MinionType::GentleMegasaur,
};
const MinionType deathrattle_minions[] = {
  MinionType::Mecharoo,
  MinionType::SelflessHero,
  MinionType::HarvestGolem,
  MinionType::KaboomBot,
  MinionType::KindlyGrandmother,
  MinionType::MountedRaptor,
  MinionType::RatPack,
  MinionType::SpawnOfNZoth,
  MinionType::InfestedWolf,
  MinionType::PilotedShredder,
  MinionType::ReplicatingMenace,
  MinionType::TortollanShellraiser,
  MinionType::PilotedSkyGolem,
  MinionType::TheBeast,
  MinionType::GoldrinnTheGreatWolf,
  MinionType::MechanoEgg,
  MinionType::SatedThreshadon,
  MinionType::SavannahHighmane,
  MinionType::Ghastcoiler,
  MinionType::KangorsApprentice,
  MinionType::SneedsOldShredder,
  MinionType::Voidlord,
};
const MinionType legendary_minions[] = {
  MinionType::OldMurkEye,
  MinionType::ShifterZerus,
  MinionType::BolvarFireblood,
  MinionType::BaronRivendare,
  MinionType::BrannBronzebeard,
  MinionType::GoldrinnTheGreatWolf,
  MinionType::MalGanis,
  MinionType::TheBoogeymonster,
  MinionType::FoeReaper4000,
  MinionType::Maexxna,
  MinionType::SneedsOldShredder,
  MinionType::ZappSlywick,
};

// -----------------------------------------------------------------------------
// Random sampling minions
// -----------------------------------------------------------------------------

template <typename A, int N>
constexpr int array_size(A(&)[N]) { return N; }

template <typename A, int N>
A random_element(A(& list)[N]) {
  return list[random(array_size(list))];
}

MinionType random_one_cost_minion() {
  return random_element(one_cost_minions);
}
MinionType random_two_cost_minion() {
  return random_element(two_cost_minions);
}
MinionType random_four_cost_minion() {
  return random_element(four_cost_minions);
}
MinionType random_legendary_minion() {
  return random_element(legendary_minions);
}
MinionType random_deathrattle_minion() {
  return random_element(deathrattle_minions);
}

