#include "parser.hpp"
#include "simulation.hpp"
#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <memory>
using namespace std;

// -----------------------------------------------------------------------------
// Simulation stuff
// -----------------------------------------------------------------------------

void print_stats(ostream& out, ScoreSummary const& stats, vector<int> const& results) {
  out << "win: " << percentage(stats.win_rate(0)) << ", ";
  out << "tie: " << percentage(stats.draw_rate()) << ", ";
  out << "lose: " << percentage(stats.win_rate(1)) << endl;
  out.precision(3);
  out << "mean score: " << stats.mean_score();
  out << ", median score: " << results[results.size()/2] << endl;
  int steps = 10;
  int n = (int)results.size() - 1;
  out << "percentiles: ";
  for (int i=0; i <= steps; ++i) {
    out << results[i*(n-1)/steps] << " ";
  }
  out << endl;
}

void print_outcome_percentile(ostream& out, int outcome, vector<int> const& results) {
  int p = percentile(outcome,results);
  out << "actual outcome: " << outcome << ", is at the " << p << "-th percentile"
      << (p < 15 ? ", you got unlucky" : p > 85 ? ", you got lucky" : "") << endl;
}

void print_damage_taken(ostream& out, ScoreSummary const& stats, int health, int player) {
  double dmg = stats.mean_damage_taken(player);
  out.precision(3);
  out << "mean damage " << (player == 0 ? "taken" : "dealt") << ": " << dmg << endl;
  if (health > 0) {
    out << (player == 0 ? "your" : "their") << " expected health afterwards: " << (health - dmg);
    out << ", " << percentage(stats.death_rate(player)) << " chance to die" << endl;
  }
}

void do_run(ostream& out, Board players[2], int n = DEFAULT_NUM_RUNS) {
  vector<int> results;
  ScoreSummary stats = simulate(players[0], players[1], n, &results);
  out << "--------------------------------" << endl;
  print_stats(out, stats, results);
  print_damage_taken(out, stats, players[0].health, 0);
  print_damage_taken(out, stats, players[1].health, 1);
  out << "--------------------------------" << endl;
}

// -----------------------------------------------------------------------------
// Hearthstone state
// -----------------------------------------------------------------------------

enum class EntityType {
  UNKNOWN,
  GAME,
  PLAYER,
  MINION,
  HERO_POWER,
  HERO,
  ENCHANTMENT,
  MOVE_MINION_HOVER_TARGE,
};

enum class Zone {
  UNKNOWN,
  PLAY,
  HAND,
  GRAVEYARD,
  SETASIDE,
  REMOVEDFROMGAME,
};

struct Entity {
  std::string hs_id;
  EntityType type = EntityType::UNKNOWN;
  Zone zone = Zone::UNKNOWN;
  // for minions
  //Minion minion;
  int attack = 0;
  int health = 0;
  int tech_level = 0;
  int premium = 0;
  int zone_position = 0;
  int controller = 0;
  int entity_id = 0;
  int taunt = 0, divine_shield = 0, poisonous = 0, windfury = 0;
  // for heros(/opponents)
  int player_id = 0;
  int is_dummy_player = 0;
  int damage = 0;
  //int PLAYER_LEADERBOARD_PLACE = 0;
  // for controllers
  // NEXT_OPPONENT_PLAYER_ID
  // for hero powers
  int used = 0; // tag 1398
  // for enchantments
  int attached = 0;
  std::vector<Entity*> enchantments;
  Entity(int entity_id) : entity_id(entity_id) {}
};
using Entities = std::unordered_map<int, unique_ptr<Entity>>;

struct HSGame {
  bool in_battlegrounds = false;
  std::unordered_map<int, unique_ptr<Entity>> entities;
  Entity* game_entity = nullptr;
  std::vector<Entity*> players;
  int turn=0;

  void clear() {
    entities.clear();
    game_entity = nullptr;
    players.clear();
  }

  // returns false for player, true for enemy
  bool decode_controller(int controller) {
    if (players.empty()) return false;
    return (controller == players[0]->controller) == players[0]->is_dummy_player;
  }
};

ostream& operator << (ostream& out, Entity const& e) {
  return out << "[" << e.entity_id << ":" << (int)e.type << ":" << e.hs_id << " " << e.attack << "/" << e.health << "]";
}

// -----------------------------------------------------------------------------
// Conversion to board state
// -----------------------------------------------------------------------------

MinionType to_minion_type(std::string const& hs_id, bool& golden_out) {
  for (int i=0; i<MinionType_count; ++i) {
    for (int g=0; g<2; ++g) {
      if (minion_info[i].hs_id[g] && hs_id == minion_info[i].hs_id[g]) {
        golden_out = g > 0;
        return static_cast<MinionType>(i);
      }
    }
  }
  cerr << "Unknown minion type: " << hs_id << endl;
  return MinionType::None;
}

Minion convert_to_minion(Entity const& entity) {
  bool golden;
  MinionType type = to_minion_type(entity.hs_id, golden);
  golden = golden || entity.premium;
  Minion minion(type,golden);
  minion.attack = entity.attack;
  minion.health = entity.health;
  minion.invalid_aura = true; // stats include aura's
  if (entity.taunt) minion.taunt = true;
  if (entity.divine_shield) minion.divine_shield = true;
  if (entity.poisonous) minion.poison = true;
  if (entity.windfury) minion.windfury = true;
  for (Entity* e : entity.enchantments) {
    if (e && e->zone == Zone::PLAY) {
      // deathrattles
      if (e->hs_id == "BOT_312e") {
        minion.add_deathrattle_microbots();
      } else if (e->hs_id == "TB_BaconUps_032e") {
        minion.add_deathrattle_golden_microbots();
      } else if (e->hs_id == "UNG_999t2e") {
        minion.add_deathrattle_plants();
      }
      // other enchantments are included in the stats
    }
  }
  return minion;
}

HeroType to_hero_type(std::string const& hs_id) {
  for (int i=0; i < HeroType_count; ++i) {
    if (hero_info[i].hs_id && hs_id == hero_info[i].hs_id) {
      return static_cast<HeroType>(i);
    } else if (hero_info[i].hero_power.hs_id && hs_id == hero_info[i].hero_power.hs_id) {
      return static_cast<HeroType>(i);
    }
  }
  return HeroType::None;
}

struct PreBattleState {
  Board players[2];
};

PreBattleState to_board_state(HSGame& game) {
  PreBattleState out;
  for (auto const& x : game.entities) {
    Entity const& e = *x.second;
    int who = game.decode_controller(e.controller);
    if (e.zone == Zone::PLAY) {
      if (e.type == EntityType::MINION) {
        int pos = e.zone_position - 1;
        if (pos >= 0 && pos < BOARDSIZE) {
          out.players[who].minions[pos] = convert_to_minion(e);
        }
      } else if (e.type == EntityType::HERO_POWER) {
        out.players[who].use_hero_power = e.used;
      } else if (e.type == EntityType::HERO) {
        out.players[who].level = e.tech_level;
        out.players[who].health = e.health - e.damage;
        out.players[who].hero = to_hero_type(e.hs_id);
      }
    }
  }
  for (int player=0; player<2; ++player) {
    out.players[player].recompute_auras(&out.players[1-player]);
  }
  return out;
}

// -----------------------------------------------------------------------------
// Log parser
// -----------------------------------------------------------------------------

struct HearthstoneLogHandler {
  void on_tag();
  void on_battle_start();
};

// -----------------------------------------------------------------------------
// Log parser: utilities
// -----------------------------------------------------------------------------

// parse a timestamp (out becomes time in seconds)
bool parse_timestamp(StringParser& in, double& out) {
  int h,m,n=0;
  double s;
  if (scanf(in.str,"%d:$d:%f%n",&h,&m,&s,&n) >= 3) {
    in.str += n;
    out = 60*(60*h+m) + s;
    return true;
  } else {
    return false;
  }
}

// -----------------------------------------------------------------------------
// Log parser: tag/value pairs
// -----------------------------------------------------------------------------

bool parse_tag_value(StringParser& in, int& out) {
  return in.parse_int(out);
}

#define PARSE_ENUM(cls,name) \
  if (in.match_exact(#name)) { out = cls::name; return true; }
bool parse_tag_value(StringParser& in, EntityType& out) {
  PARSE_ENUM(EntityType, MINION)
  PARSE_ENUM(EntityType, HERO_POWER)
  PARSE_ENUM(EntityType, HERO)
  PARSE_ENUM(EntityType, ENCHANTMENT)
  PARSE_ENUM(EntityType, GAME)
  PARSE_ENUM(EntityType, PLAYER)
  PARSE_ENUM(EntityType, MOVE_MINION_HOVER_TARGE)
  //in.expected("Entity type");
  out = EntityType::UNKNOWN;
  return false;
}
bool parse_tag_value(StringParser& in, Zone& out) {
  PARSE_ENUM(Zone, SETASIDE)
  PARSE_ENUM(Zone, PLAY)
  PARSE_ENUM(Zone, HAND)
  PARSE_ENUM(Zone, GRAVEYARD)
  PARSE_ENUM(Zone, REMOVEDFROMGAME)
  //in.expected("Zone");
  out = Zone::UNKNOWN;
  return false;
}

#define PARSE_TAG(name, field) \
  if (in.match_exact(name " value=")) { \
    if (!parse_tag_value(in, field)) return false; \
    return true; \
  }

bool parse_tag_and_value(StringParser& in, Entity& entity, HSGame& game) {
  PARSE_TAG("HEALTH", entity.health);
  PARSE_TAG("ATK", entity.attack);
  PARSE_TAG("TAUNT", entity.taunt);
  PARSE_TAG("DIVINE_SHIELD", entity.divine_shield);
  PARSE_TAG("POISONOUS", entity.poisonous);
  PARSE_TAG("WINDFURY", entity.windfury);
  PARSE_TAG("PREMIUM", entity.premium);
  PARSE_TAG("CARDTYPE", entity.type);
  PARSE_TAG("ZONE_POSITION", entity.zone_position);
  PARSE_TAG("ZONE", entity.zone);
  PARSE_TAG("CONTROLLER", entity.controller);
  PARSE_TAG("PLAYER_ID", entity.player_id);
  PARSE_TAG("BACON_DUMMY_PLAYER", entity.is_dummy_player);
  PARSE_TAG("1398", entity.used); // hero power used
  PARSE_TAG("TECH_LEVEL", entity.tech_level);
  PARSE_TAG("PLAYER_TECH_LEVEL", entity.tech_level);
  PARSE_TAG("DAMAGE", entity.damage);
  if (in.match_exact("ATTACHED" " value=")) {
    int attach;
    if (!parse_tag_value(in, attach)) return false;
    if (attach != entity.attached) {
      entity.attached = attach;
      // store attachement
      Entities::iterator it = game.entities.find(attach);
      if (it != game.entities.end() && it->second) {
        it->second->enchantments.push_back(&entity);
      }
    }
    return true;
  }
  return false;
}

bool parse_existing_entity_id(StringParser& in, int& entity_id) {
  // parse something like "[entityName=.. id=7679 zone=.. zonePos=4 cardId=.. player=..]"
  if (!in.match_exact("[")) return false;
  if (!in.skip_until(" id=")) return false;
  if (!in.parse_int(entity_id)) return false;
  if (!in.skip_until(']')) return false;
  return true;
}

/*
enchantments:
 e.g.
  * BOT_312e = replicating manace attached to something
  * CFM_610e = "Serrated Shadows" = +1/+1
    (caused by CFM_610, Crystal Weaver)


attaching done inside
  BLOCK_START BlockType=POWER

playing cards:
  BLOCK_START BlockType=PLAY

turn starts: STEP = MAIN_READY
  Player.Id = _matchInfo.LocalPlayer.Id;
	Opponent.Id = _matchInfo.OpposingPlayer.Id;
  var opponentHero = game.Entities.Values
					  .Where(x => x.IsHero && x.IsInZone(PLAY) && x.IsControlledBy(game.Opponent.Id))
					  .FirstOrDefault();
  opponentHero.CardId != NonCollectible.Neutral.BobsTavernTavernBrawl

Note: Odd turns are in the tavern, even turns are battles

Note2: one of the players will have BACON_DUMMY_PLAYER=1

Battle start:
  * actual minions are changed to REMOVEDFROMGAME inside a 
    TRIGGER block by BaconShop8PlayerEnchant
  * copies of minions for battle are created inside a 
  BLOCK_START BlockType=TRIGGER Entity=[entityName=BaconShop8PlayerEnchant id=43 zone=PLAY zonePos=0 cardId=TB_BaconShop_8P_PlayerE player=1] EffectCardId= EffectIndex=-1 Target=0 SubOption=-1 TriggerKeyword=0
  * Then STEP=MAIN_READY is set
  * Then inside another TRIGGER block of BaconShop8PlayerEnchant, the ATTACK blocks happen; and DEATHS blocks happen; and TRIGGER blocks happen for things like Deathrattles

At end of battle:
  * winning hero gets ATK, does damage to loser, resets ATK
  * in a TRIGGER:
     * copied minions are moved to REMOVEDFROMGAME
     * controls are created
     * original minions are moved to PLAY zone
  * next turn, and MAIN_READY


Board state change:
 * BLOCK_START BlockType=PLAY Entity=[entityName=Drag To Sell ..]
 
*/

// -----------------------------------------------------------------------------
// Log parser
// -----------------------------------------------------------------------------

struct LogParser {
  HSGame game;
  Entity* current_entity = nullptr;
  bool verbose = false;

  bool parse_power_log_line(StringParser& in);
  void parse(istream&, const char* filename);
  bool parse_existing_entity(StringParser& in, Entity*& entity);
  bool parse_create_entity(StringParser& in);
  bool parse_game_entity_tag_change(StringParser& in);

  void on_step(StringParser& in);
};

bool LogParser::parse_power_log_line(StringParser& in) {
  // Log line looks like:
  //  "D 01:39:40.8734632 GameState.DebugPrintPower() - ..."
  if (!in.match("D")) return false;
  in.skip_ws();
  std::string timestamp;
  in.match_string(timestamp,' ');
  in.skip_ws();
  // log line type
  // Note: The GameState.DebugPrintPower() and PowerTaskList.DebugPrintPower() lines are mixed
  // so all events appear twice, making a mess of the log
  if (in.match_exact("GameState.DebugPrintPower()")) {
    // skip intil '-'
    if (!in.skip_until('-')) return false;
    in.skip_ws();
    
    // log line body
    if (current_entity && in.match_exact("tag=")) {
      // part of a FULL_ENTITY block
      return parse_tag_and_value(in,*current_entity,game);
    }
    // any other log line causes the FULL_ENTITY block to end
    current_entity = nullptr;
    // other bodies
    if (in.match_exact("FULL_ENTITY") || in.match_exact("SHOW_ENTITY")) {
      in.parse_exact(" - ");
      // "FULL_ENTITY - Creating ID=1731 CardID=CFM_315"
      //"FULL_ENTITY - Updating [...] CardID=TB_BaconShop_HERO_22"
      if (in.match_exact("Creating ID=")) {
        if (!parse_create_entity(in)) return false;
        in.skip_ws();
        if (!in.parse_exact("CardID=")) return false;
        if (!in.parse_string(current_entity->hs_id,'\r')) return false;
        in.parse_end();
        return true;
      } else if (in.match_exact("Updating ")) {
        in.parse_exact("Entity="); // either "FULL_ENTITY - Updating [id]" or "SHOW_ENTITY - Updating Entity=id"
        if (parse_existing_entity(in,current_entity)) {
          in.skip_ws();
          if (!in.parse_exact("CardID=")) return false;
          // note: CardID might not have been set before
          if (!in.parse_string(current_entity->hs_id,'\r')) return false;
          return true;
        } else {
          return false;
        }
      } else {
        in.expected("Creating or Updating");
        return false;
      }
    } else if (in.match_exact("TAG_CHANGE")) {
      // "TAG_CHANGE Entity=[entityName=.. id=7679 zone=PLAY zonePos=4 cardId=EX1_506 player=14] tag=1254 value=17 "
      // "TAG_CHANGE Entity=GameEntity tag=STEP value=BEGIN_MULLIGAN"
      // "TAG_CHANGE Entity=GameEntity tag=PROPOSED_ATTACKER value=5289"
      if (!in.parse_exact(" Entity=")) return false;
      Entity* entity = nullptr;
      if (!parse_existing_entity(in,entity)) return false;
      in.skip_ws();
      if (!in.parse_exact("tag=")) return false;
      if (entity == game.game_entity) {
        return parse_game_entity_tag_change(in);
      } else {
        return parse_tag_and_value(in,*entity,game);
      }
    } else if (in.match_exact("BLOCK_START")) {
      if (!in.parse_exact(" BlockType=")) return false;
      //cout << "BLOCK_START " << timestamp << " " << in.error.line_number << " " << in.next_token(' ') << endl;
      // "BLOCK_START BlockType=PLAY"
    } else if (in.match_exact("BLOCK_END")) {
    } else if (in.match_exact("CREATE_GAME")) {
      // ignore
      if (verbose) cout << "CREATE_GAME " << in.error.line_number << endl;
      game.clear();
      current_entity = nullptr;
    } else if (in.match_exact("Player EntityID=")) {
      if (!parse_create_entity(in)) return false;
      game.players.push_back(current_entity);
    } else if (in.match_exact("GameEntity EntityID=")) {
      if (!parse_create_entity(in)) return false;
      game.game_entity = current_entity;
    } else {
      //cout << " // " << in.error.line_number << ":" << in.next_token(' ') << endl;
      //"PlayerID=6, PlayerName=twanvl#2307"
      // ignore
      return false;
    }
  } else if (in.match_exact("GameState.DebugPrintGame()")) {
    if (!in.skip_until('-')) return false;
    in.skip_ws();
    if (in.match_exact("GameType=")) {
      game.in_battlegrounds = in.match_exact("GT_BATTLEGROUNDS");
    } else if (in.match_exact("PlayerID=")) {
      int id;
      if (!in.parse_int(id)) return false;
      if (!in.parse_exact(", PlayerName=")) return false;
      // todo: name of player can be used as entity name
    } else {
      return false;
    }
  }
  return true;
}

bool LogParser::parse_existing_entity(StringParser& in, Entity*& entity) {
  int id;
  if (parse_existing_entity_id(in,id) || in.match_int(id)) {
    Entities::iterator it = game.entities.find(id);
    if (it != game.entities.end() && it->second.get()) {
      entity = it->second.get();
      return true;
    } else {
      if (verbose) in.error() << "Entity not found: " << id << endl;
      entity = nullptr;
      return false;
    }
  } else if (in.match_exact("GameEntity") && game.game_entity) {
    entity = game.game_entity;
    return true;
  } else if ((in.match_exact("Bob's Tavern") || in.match_exact("The Innkeeper")) && game.players.size() >= 2) {
    entity = game.players[1];
    return true;
  } else {
    // is this the player?
    if (verbose) in.error() << "Entity not found:  " << in.next_token() << endl;
    return false;
  }
}

bool LogParser::parse_create_entity(StringParser& in) {
  int id;
  if (!in.parse_int(id)) return false;
  if (game.entities.count(id)) {
    in.error() << "Entity already exists " << id << endl;
  }
  unique_ptr<Entity> e(new Entity(id));
  current_entity = e.get();
  game.entities[id] = std::move(e);
  return true;
}

bool LogParser::parse_game_entity_tag_change(StringParser& in) {
  if (in.match_exact("STEP ")) {
    if (!in.parse_exact("value=")) return false;
    on_step(in);
  } else if (in.match_exact("NEXT_STEP ")) {
    // ignore
  } else if (in.match_exact("STATE ")) {
    if (!in.parse_exact("value=")) return false;
    if (in.match("RUNNING")) {
      return true; // 
    } else if (in.match("COMPLETE")) {
      game.in_battlegrounds = false;
    }
    //cout << "STATE " << in.error.line_number << " " << in.next_token() << endl;
  } else {
    PARSE_TAG("TURN", game.turn);
    if (verbose) in.error() << "Unknown GameEntity tag " << in.next_token() << endl;
    return false;
  }
  return true;
}

void LogParser::on_step(StringParser& in) {
  if (verbose) cout << in.error.line_number << ": STEP " << in.next_token() << endl;
  if (in.match_exact("MAIN_READY") || true) {
    // MAIN_READY indicates the start of a battle
    // unless there is a MAIN_START_TRIGGERS phase after it
    // I don't know the logic behind whether or not MAIN_START_TRIGGERS happens.
    // dump board state
    /*
    for (auto const& x : game.entities) {
      Entity const& e = *x.second;
      if (e.zone == Zone::PLAY && e.type == EntityType::MINION) {
        cout << " * " << game.decode_controller(e.controller) << "." << e.zone_position << ": " << e.hs_id << " " << e.attack << "/" << e.health << endl;
        cout << "   = " << convert_to_minion(e) << endl;
        for (const Entity* enchant : e.enchantments) {
          cout << "   + " << enchant->hs_id << " " << enchant->attack << "/" << enchant->health << endl;
        }
      }
    }
    */
    /*
    for (auto const& x : game.entities) {
      Entity const& e = *x.second;
      if (e.zone == Zone::PLAY && e.type == EntityType::HERO_POWER) {
        cout << " HERO_POWER " << game.decode_controller(e.controller) << "." << e.zone_position << ": " << e.hs_id << " " << to_hero_type(e.hs_id) << " " << e.used << endl;
      }
      if (e.zone == Zone::PLAY && e.type == EntityType::HERO) {
        cout << " HERO " << game.decode_controller(e.controller) << "." << e.zone_position << ": " << e.hs_id << " " << to_hero_type(e.hs_id) << " " << e.health << "-" << e.damage << ", " << e.tech_level << endl;
      }
    }
    */
    // Note: stats are including auras
    PreBattleState boards = to_board_state(game);
    /*
    cout << in.error.line_number << ": " << boards.players[0].hero << " vs " << boards.players[1].hero << endl;
    cout << Battle(boards.players[0], boards.players[1]) << endl;
    */
    if (boards.players[1].hero != HeroType::None) {
      cout << "turn " << (game.turn / 2) << endl;
      cout << boards.players[0] << endl;
      cout << "===================================" << endl;
      cout << "turn " << (game.turn / 2) << " enemy " << boards.players[1].hero << endl;
      cout << boards.players[1] << endl;
      cout << "===================================" << endl;
      //do_run(cout, boards.players);
    }
  }
}

void LogParser::parse(istream& lines, const char* filename) {
  ErrorHandler error(cout, filename);
  while (lines.good()) {
    // get line
    error.line_number++;
    std::string line;
    getline(lines,line);
    StringParser in(line.c_str(), error);
    // parse
    parse_power_log_line(in);
  }
}

// -----------------------------------------------------------------------------
// Main function
// -----------------------------------------------------------------------------

int main(int argc, char const** argv) {
  if (argc <= 1) {
    cout << "Usage: " << argv[0] << " <logfiles>" << endl;
  } else {
    LogParser parser;
    for (int i=1; i<argc; ++i) {
      ifstream in(argv[i]);
      if (!in) {
        cerr << "Error loading file " << argv[i] << endl;
        return 1;
      }
      parser.parse(in, argv[i]);
    }
  }
  return 0;
}

