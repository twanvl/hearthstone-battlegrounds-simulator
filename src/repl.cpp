#include "battle.hpp"
#include "simulation.hpp"
#include "parser.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <memory>
using namespace std;

// -----------------------------------------------------------------------------
// REPL class
// -----------------------------------------------------------------------------

struct REPL {
  // board
  Board players[2];
  int current_player = 0;
  bool used = false;

  // reporting
  vector<int> actual_outcomes;

  // single stepping
  unique_ptr<Battle> active_battle;
  vector<unique_ptr<Battle>> history;

  // simulating
  int default_num_runs = DEFAULT_NUM_RUNS;
  Objective optimization_objective = Objective::DamageTaken;

  // error messages
  ErrorHandler error;

  // Repl
  ostream& out;
  void repl(istream&, bool prompts);
  REPL(ostream& out, const char* filename = "")
    : error(out, filename)
    , out(out)
  {}
  REPL(istream& in, ostream& out, bool prompts, const char* filename = "")
    : error(out, filename)
    , out(out)
  {
    repl(in,prompts);
  }

  // Parser
  void parse_line(std::string const& line);
  void parse_line(StringParser& in);

  // Commands
  void do_help();
  void do_quit();
  void do_board(int player);
  void do_swap();
  void do_show();
  void do_reset();
  void do_step();
  void do_trace();
  void do_back();
  void do_list_minions();
  void do_list_hero_powers();
  void do_list_objectives();
  void do_run(int runs = -1);
  void do_optimize_order(Objective objective, int runs = -1);
  void do_optimize_buff_placement(Minion const& buff, Objective objective, int runs = -1);
  void do_add_minion(Minion const&);
  void do_buff_minion(bool enemy, MinionRef const& ref, Minion const& buff);
  void do_end_input();
};

// -----------------------------------------------------------------------------
// Command parser
// -----------------------------------------------------------------------------

void REPL::repl(istream& in, bool prompt) {
  while (in.good()) {
    if (prompt) {
      out << "> " << flush;
    } else {
      error.line_number++;
    }
    std::string line;
    getline(in,line);
    parse_line(line);
  }
  do_end_input();
}

void REPL::parse_line(std::string const& line) {
  StringParser in(line.c_str(), error);
  parse_line(in);
}

void REPL::parse_line(StringParser& in) {
  // first word
  in.skip_ws();
  if (in.match_end()) {
    return; // empty line or comment
  } else if (in.match("*")) {
    // define minion
    Minion m;
    if (parse_minion(in,m) && in.parse_end()) {
      do_add_minion(m);
    }
    return;
  } else if (in.peek() == '=') {
    // board separator
    do_end_input();
    out << endl;
    return;
  } else if (in.match("quit") || in.match("q")) {
    in.parse_end();
    do_quit();
  } else if (in.match("help") || in.match("?")) {
    in.parse_end();
    do_help();
  } else if (in.match("board") || in.match("clear")) {
    in.match(":"); // optional
    in.parse_end();
    do_board(0);
  } else if (in.match("vs")) {
    in.match(":"); // optional
    in.parse_end();
    do_board(1);
  } else if (in.match("swap")) {
    in.parse_end();
    do_swap();
  } else if (in.match("info") || in.match("msg") || in.match("message")) {
    in.match(":"); // optional
    in.skip_ws();
    out << in.str << endl;
  } else if (in.match("hp") || in.match("hero-power")) {
    in.match(":"); // optional
    HeroPower hp;
    if (parse_hero_power(in, hp) && in.parse_end()) {
      players[current_player].hero_power = hp;
    }
  } else if (in.match("actual") || in.match("outcome")) {
    in.match(":"); // optional
    int n = 0;
    if (in.parse_int(n) && in.parse_end()) {
      actual_outcomes.push_back(n);
    }
  } else if (in.match("give")) {
    MinionRef ref;
    Minion buff;
    bool enemy = in.match("enemy");
    if (parse_minion_ref(in,ref) && parse_buffs(in,buff) && in.parse_end()) {
      do_buff_minion(enemy,ref,buff);
    }
  } else if (in.match("run") || in.match("simulate")) {
    in.match(":"); // optional
    int n = -1;
    in.match_int(n); // optional
    do_run(n);
  } else if (in.match("objective")) {
    in.match(":"); // optional
    Objective obj;
    if (parse_objective(in, obj) && in.parse_end()) {
      optimization_objective = obj;
    }
  } else if (in.match("optimize")) {
    in.match(":"); // optional
    Objective obj = optimization_objective;
    match_objective(in,obj); // optional
    if (in.match("buff")) {
      Minion buff;
      if (parse_buffs(in,buff) && in.parse_end()) {
        do_optimize_buff_placement(buff,obj);
      }
    } else {
      in.parse_end();
      do_optimize_order(obj);
    }
  } else if (in.match("runs")) {
    in.match(":"); // optional
    int n = DEFAULT_NUM_RUNS;
    if (in.parse_positive(n) && in.parse_end()) {
      default_num_runs = n;
    }
  } else if (in.match("level")) {
    in.match(":"); // optional
    int n = 0;
    if (in.parse_non_negative(n) && in.parse_end()) {
      players[current_player].level = n;
    }
  } else if (in.match("health")) {
    in.match(":"); // optional
    int n = 0;
    if (in.parse_non_negative(n) && in.parse_end()) {
      players[current_player].health = n;
    }
  } else if (in.match("show")) {
    in.parse_end();
    do_show();
  } else if (in.match("minion")) {
    in.match(":"); // optional
    Minion m;
    if (parse_minion(in,m) && in.parse_end()) {
      out << m << endl;
    }
  } else if (in.match("list") || in.match("minions")) {
    in.parse_end();
    do_list_minions();
  } else if (in.match("heropowers")) {
    in.parse_end();
    do_list_hero_powers();
  } else if (in.match("objectives")) {
    in.parse_end();
    do_list_objectives();
  } else if (in.match( "step")) {
    in.parse_end();
    do_step();
  } else if (in.match("reset")) {
    in.parse_end();
    do_reset();
  } else if (in.match("steps") || in.match("trace")) {
    in.parse_end();
    do_trace();
  } else if (in.match("back")) {
    in.parse_end();
    do_back();
  } else {
    in.unknown("command");
  }
}

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

void REPL::do_help() {
  out << "Commands:" << endl;
  out << endl;
  out << "-- Defining the board" << endl;
  out << "board      = begin defining player board" << endl;
  out << "vs         = begin defining opposing board" << endl;
  //out << "swap       = swap with enemy board" << endl;
  out << "* <minion> = give the next minion" << endl;
  out << "HP <hero>  = tell that a hero power is used" << endl;
  out << "level <n>  = give the level of a player" << endl;
  out << "health <n> = give the health of a player" << endl;
  //out << "secret <secret> = tell that a secret is in play" << endl;
  //out << "after-auras = tell the program that the stats given are after taking auras into account (default = true)" << endl;
  out << endl;
  out << "-- Modifying the board" << endl;
  /*
  out << "move <i> to <j> = move a minion from position i to position j" << endl;
  out << "sell <i> = sell minion(s) with position/condition i" << endl;
  out << "play <minion> [at <i>]" << endl;
  */
  out << "give <m> <buff> = buff minion(s) m with one or more buffs" << endl;
  out << endl;
  out << "-- Running simulations" << endl;
  out << "actual <i> = tell about actual outcome (used in simulation display)" << endl;
  out << "run [<n>]  = run n simulations (default: 100)" << endl;
  out << "optimize   = optimize the minion order to maximize some objective" << endl;
  out << "objective  = set the optimization objective (default: minimize damage taken)" << endl;
  out << endl;
  out << "-- Stepping through a single battle" << endl;
  out << "show       = show the board state" << endl;
  out << "reset      = reset battle" << endl;
  out << "step       = do 1 attack step, or start if battle not started yet" << endl;
  out << "trace      = do steps until the battle ends" << endl;
  out << "back       = step backward. can be used to re-roll RNG" << endl;
  out << endl;
  out << "-- Other" << endl;
  out << "info       = show a message" << endl;
  out << "help       = show this help message" << endl;
  out << "quit       = quit the simulator" << endl;
  out << "minions    = list all minions" << endl;
  out << "heropowers = list all hero powers" << endl;
  out << "objectives = list all optimization objectives" << endl;
  out << endl;
  out << "-- Minion format" << endl;
  out << "Minions are specified as" << endl;
  out << "  [attack/health] [golden] <name>, <buff>, <buff>, .." << endl;
  out << "For example" << endl;
  out << " * 10/12 Nightmare Amalgam" << endl;
  out << " * Golden Murloc Tidecaller, poisonous, divine shield, taunt, windfury, +12 attack" << endl;
  out << endl;
  out << "-- Minion buffs" << endl;
  out << " * +<n> attack = buff attack by this much" << endl;
  out << " * +<n> health = buff health by this much" << endl;
  out << " * +<a>/+<h>   = buff attack and health" << endl;
  out << " * taunt, divine shield, poisonous, windfury = the obvious" << endl;
  out << " * microbots   = deathrattle: summon 3 1/1 Microbots" << endl;
  out << " * golden microbots = deathrattle: summon 3 2/2 Microbots" << endl;
  out << " * plants      = deathrattle: summon 2 1/1 Plants" << endl;
  out << " * <minion>    = magnetize given minion" << endl;
  out << endl;
  out << "-- Refering to a minion" << endl;
  out << "You can refer to a minion with an index (1 to 7), a name, a tribe, or all" << endl;
  out << "For example" << endl;
  out << "  give all +1/+1" << endl;
  out << "  give 2 poisonous  # buffs the second minion" << endl;
  out << "  give Mech divine shield, windfury" << endl;
  out << "  give Cave Hydra +10 health" << endl;
  out << "By default this refers to your side, to modify the enemy:" << endl;
  out << "  give enemy all taunt" << endl;
}

void REPL::do_quit() {
  exit(0);
}

void REPL::do_end_input() {
  if (!used && !players[0].empty()) {
    do_run();
  }
  actual_outcomes.clear();
  do_reset();
  current_player = 0;
}

void REPL::do_board(int player) {
  players[player] = Board();
  current_player = player;
  actual_outcomes.clear();
  do_reset();
  used = false;
}

void REPL::do_swap() {
  std::swap(players[0], players[1]);
}

void REPL::do_add_minion(Minion const& m) {
  if (players[current_player].full()) {
    error() << "Player already has a full board" << endl;
  } else {
    players[current_player].append(m);
    active_battle.reset();
    used = false;
  }
}

void REPL::do_buff_minion(bool enemy, MinionRef const& ref, Minion const& buff) {
  int n = 0;
  ref.for_each(players[enemy ? 1 : 0], [&](Minion& m){
    m.buff(buff);
    n++;
  });
  out << "Modified " << n << " minion" << (n == 1 ? "" : "s") << endl;
  active_battle.reset();
}

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

void REPL::do_run(int n) {
  if (n <= 0) n = default_num_runs;
  vector<int> results;
  ScoreSummary stats = simulate(players[0], players[1], n, &results);
  out << "--------------------------------" << endl;
  print_stats(out, stats, results);
  for (int o : actual_outcomes) {
    print_outcome_percentile(out, o, results);
  }
  print_damage_taken(out, stats, players[0].health, 0);
  print_damage_taken(out, stats, players[1].health, 1);
  out << "--------------------------------" << endl;
  used = true;
}

void REPL::do_optimize_order(Objective objective, int n) {
  if (n <= 0) n = default_num_runs;
  OptimizeMinionOrder opt(players[0], players[1], objective, n);
  if (opt.current_score >= opt.best_score) {
    out << "Your " << name(objective) << " cannot be improved by reordering your minions" << endl;
  } else {
    out << "Your " << name(objective) << " can be improved from ";
    display_objective_value(out, objective, opt.current_score);
    out << " to ";
    display_objective_value(out, objective, opt.best_score);
    out << " by reordering your minions:" << endl;
    Board new_board = players[0];
    permute_minions(new_board, players[0].minions, opt.best_order.data(), opt.n);
    out << new_board;
    // TODO: significance test?
  }
  used = true;
}

void REPL::do_optimize_buff_placement(Minion const& buff, Objective objective, int n) {
  if (n <= 0) n = default_num_runs;
  // TODO
  OptimizeMinionBuffPlacement opt(players[0], players[1], buff, objective, n);
  out << "Current " << name(objective) << " is ";
  display_objective_value(out, objective, opt.current_score);
  out << endl;
  players[0].for_each_with_pos([&](int i, Minion const& m) {
    out << "Buffing " << m << "; " << name(objective) << " becomes ";
    display_objective_value(out, objective, opt.scores[i]);
    if (opt.scores[i] >= opt.best_score) {
      out << ". This is the best.";
    }
    out << endl;
  });
}

void REPL::do_show() {
  if (!active_battle) {
    active_battle.reset(new Battle(players[0], players[1], &out));
    active_battle->verbose = 2;
  }
  out << *active_battle;
}


void REPL::do_reset() {
  active_battle.reset();
  history.clear();
}

void REPL::do_step() {
  if (!active_battle) {
    history.clear();
    active_battle.reset(new Battle(players[0], players[1], &out));
    active_battle->verbose = 2;
  } else if (!active_battle->started()) {
    history.push_back(unique_ptr<Battle>(new Battle(*active_battle)));
    active_battle->start();
  } else if (!active_battle->done()) {
    history.push_back(unique_ptr<Battle>(new Battle(*active_battle)));
    active_battle->attack_round();
  } else {
    out << "Battle is done, score: " << active_battle->score() << endl;
    return;
  }
  out << *active_battle << endl;
}

void REPL::do_trace() {
  if (!active_battle) do_step();
  while (!active_battle->done()) do_step();
  do_step();
}

void REPL::do_back() {
  if (!history.empty()) {
    active_battle = move(history.back());
    history.pop_back();
    out << *active_battle << endl;
  } else {
    error() << "History is empty" << endl;
  }
}

void REPL::do_list_minions() {
  for (int i=1; i < MinionType_count; ++i) {
    out << minion_info[i].name << endl;
  }
}

void REPL::do_list_hero_powers() {
  for (int i=1; i < HeroPower_count; ++i) {
    out << hero_info[i].name << endl;
  }
}

void REPL::do_list_objectives() {
  for (int i=0; i < NUM_OBJECTIVES; ++i) {
    out << name(static_cast<Objective>(i)) << endl;
  }
}

// -----------------------------------------------------------------------------
// Main function
// -----------------------------------------------------------------------------

#if !__EMSCRIPTEN__

int main(int argc, char const** argv) {
  if (argc <= 1) {
    REPL repl(cin, cout, true, "");
  } else {
    for (int i=1; i<argc; ++i) {
      ifstream in(argv[i]);
      if (!in) {
        cerr << "Error loading file " << argv[i] << endl;
        return 1;
      }
      REPL repl(in, cout, false, argv[i]);
    }
  }
  return 0;
}

#endif

// -----------------------------------------------------------------------------
// JS interface
// -----------------------------------------------------------------------------

#if __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#include <emscripten/bind.h>

string run_repl(string const& input) {
  ostringstream out;
  istringstream in(input);
  REPL repl(in, out, false, "input");
  return out.str();
}

EMSCRIPTEN_BINDINGS(hsbg) {
  emscripten::function("run", &run_repl);
}

#endif

