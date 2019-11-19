#include "battle.hpp"
#include "simulation.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <algorithm>
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
  bool battle_started = false;
  Battle step_battle;
  vector<Battle> history;

  // simulating
  int default_num_runs = DEFAULT_NUM_RUNS;

  // error messages
  string filename;
  int line_number = 0;
  ostream& error();

  // Repl
  ostream& out;
  void repl(istream&, bool prompts);
  REPL(ostream& out, string const& filename = "")
    : filename(filename)
    , out(out)
  {}
  REPL(istream& in, ostream& out, bool prompts, string const& filename = "")
    : filename(filename)
    , out(out)
  {
    repl(in,prompts);
  }

  // Parser
  void parse_line(std::string const& line);
  istream& parse_minion(istream&, Minion&);
  void parse_add_minion(istream&);
  MinionType find_minion(std::string const& name);
  HeroPower find_hero_power(std::string const& name);

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
  void do_run(int runs = -1);
  void do_optimize_order(int runs = -1);
  void do_add_minion(Minion const&);
  void do_end_input();
};

// -----------------------------------------------------------------------------
// Parser
// -----------------------------------------------------------------------------

struct skip {
  const char* text;
  skip(const char* text) : text(text) {}
};

istream& operator >> (istream& in, const skip& x) {
  ios_base::fmtflags f = in.flags();
  in >> noskipws;
  char c;
  const char * text = x.text;
  while (in && *text) {
    in >> c;
    if (c != *text) {
      in.setstate(ios::failbit);
      return in;
    }
    text++;
  }
  in.flags(f);
  return in;
}

void lower(string& str) {
  for (char& c : str) {
    c = tolower(c);
  }
}

istream& REPL::parse_minion(istream& in, Minion& m) {
  int attack = -1, health = -1;
  bool golden = false;
  // read stats
  in >> ws;
  char c = in.peek();
  if (isdigit(c)) {
    in >> attack >> skip("/") >> health;
    if (!in) {
      error() << "Expected attack/health" << endl;
      return in;
    }
  }
  // read name
  string name;
  string word;
  while (in.good()) {
    in >> word;
    lower(word);
    if (word == "gold" || word == "golden") {
      golden = true;
    } else if (word == "with" || word == "+") {
      break;
    } else if (!word.empty()) {
      bool last = false;
      if (!word.empty() && word.back() == ',') {
        word.pop_back();
        last = true;
      }
      if (!word.empty()) {
        if (!name.empty()) name += " ";
        name += word;
      }
      if (last) break;
    }
  }
  // find minion by name
  MinionType type = find_minion(name);
  m = Minion(type, golden);
  if (attack != -1) m.attack = attack;
  if (health != -1) m.health = health;
  // read keywords
  while (in.good()) {
    in >> ws;
    if (in.peek() == '+' || in.peek() == '-') {
      int attack = 0;
      in >> attack;
      if (in.peek() == '/') {
        int health = 0;
        in >> skip("/") >> health;
        if (in) {
          m.buff(attack,health);
        } else {
          error() << "Unknown minion modifier" << endl;
        }
      } else {
        in >> word;
        lower(word);
        if (word == "attack") {
          m.buff(attack,0);
        } else if (word == "health") {
          m.buff(0,health);
        } else {
          error() << "Unknown minion modifier: " << attack << word << endl;
        }
      }
      continue;
    }
    in >> word;
    lower(word);
    if (word.empty()) continue;
    if (word.back() == ',') word.pop_back();
    if (word == "taunt" || word == "t") {
      m.taunt = true;
    } else if (word == "divine" || word == "shield" || word == "d") {
      m.divine_shield = true;
    } else if (word == "poison" || word == "poisonous" || word == "p") {
      m.poison = true;
    } else if (word == "windfury" || word == "w") {
      m.windfury = true;
    } else if (word == "microbots") {
      m.add_deathrattle_microbots();
    } else if (word == "+") {
      // ignore
    } else {
      error() << "Unknown minion modifier: " << word << endl;
    }
  }
  if (!m.exists()) {
    in.setstate(ios::failbit);
  }
  return in;
}

void REPL::repl(istream& in, bool prompt) {
  while (in.good()) {
    if (prompt) {
      out << "> " << flush;
    } else {
      line_number++;
    }
    std::string line;
    getline(in,line);
    parse_line(line);
  }
  do_end_input();
}

std::string read_command(istream& in) {
  std::string cmd;
  in >> cmd;
  lower(cmd);
  if (!cmd.empty() && cmd.back() == ':') {
    cmd.pop_back();
  }
  return cmd;
}

void REPL::parse_line(std::string const& line) {
  // first word
  std::istringstream in(line);
  in >> std::ws;
  int c = in.peek();
  if (c == EOF) {
    return;
  } else if (isdigit(c)) {
    parse_add_minion(in);
    return;
  } else if (c == '#') {
    // comment, ignore
    return;
  } else if (c == '*') {
    in.ignore();
    parse_add_minion(in);
    return;
  } else if (c == '=') {
    do_end_input();
    out << endl;
    return;
  }
  // a command
  std::string cmd = read_command(in);
  if (!in) return;
  in >> std::ws;
  if (cmd.empty()) {
    return;
  } else if (cmd == "quit" || cmd == "q") {
    do_quit();
  } else if (cmd == "help" || cmd == "h" || cmd == "?") {
    do_help();
  } else if (cmd == "board" || cmd == "clear") {
    do_board(0);
  } else if (cmd == "vs") {
    do_board(1);
  } else if (cmd == "swap") {
    do_swap();
  } else if (cmd == "info" || cmd == "msg" || cmd == "message" || cmd == "print" || cmd == "echo") {
    std::string msg;
    getline(in,msg);
    out << msg << endl;
  } else if (cmd == "hp" || cmd == "hero-power" || cmd == "heropower") {
    string hp;
    getline(in,hp);
    players[current_player].hero_power = find_hero_power(hp);
  } else if (cmd == "actual" || cmd == "outcome") {
    int n = 0;
    if (in >> n) {
      actual_outcomes.push_back(n);
    } else {
      error() << "Expected outcome value, usage:" << endl
              << "  actual <score>" << endl;
    }
  } else if (cmd == "run" || cmd == "simulate") {
    int n = -1;
    in >> n;
    do_run(n);
  } else if (cmd == "optimize") {
    do_optimize_order();
  } else if (cmd == "runs") {
    int n = DEFAULT_NUM_RUNS;
    if (in >> n && n > 0) {
      default_num_runs = n;
    } else {
      error() << "Please specify number of runs" << endl;
    }
  } else if (cmd == "level") {
    int n = 0;
    if (in >> n && n >= 0) {
      players[current_player].level = n;
    } else {
      error() << "Expected number" << endl;
    }
  } else if (cmd == "health") {
    int n = 0;
    if (in >> n && n >= 0) {
      players[current_player].health = n;
    } else {
      error() << "Expected number" << endl;
    }
  } else if (cmd == "show") {
    do_show();
  } else if (cmd == "minion") {
    Minion m;
    if (parse_minion(in,m)) {
      out << m << endl;
    }
  } else if (cmd == "list" || cmd == "minions") {
    do_list_minions();
  } else if (cmd == "heropowers") {
    do_list_hero_powers();
  } else if (cmd == "step") {
    do_step();
  } else if (cmd == "reset") {
    do_reset();
  } else if (cmd == "steps" || cmd == "trace") {
    do_trace();
  } else if (cmd == "back") {
    do_back();
  } else {
    error() << "Unknown command: " << cmd << endl;
  }
}

void REPL::parse_add_minion(istream& in) {
  Minion m;
  if (parse_minion(in,m)) {
    do_add_minion(m);
  } else {
    error() << "Expected minion, see help command for the syntax" << endl;
  }
}

// -----------------------------------------------------------------------------
// minion types
// -----------------------------------------------------------------------------

bool ignored_char(char x) {
  return !isalpha(x);
}

// compare strings, ignoring case and non-alphabetic characters
bool equal_names(string const& a, string const& b) {
  size_t i=0, j=0;
  while (i < a.size() && ignored_char(a[i])) i++;
  while (j < b.size() && ignored_char(b[j])) j++;
  while (i < a.size() && j < b.size()) {
    if (tolower(a[i]) != tolower(b[j])) return false;
    i++; j++;
    while (i < a.size() && ignored_char(a[i])) i++;
    while (j < b.size() && ignored_char(b[j])) j++;
  }
  return i == a.size() && j == b.size();
}

MinionType REPL::find_minion(std::string const& name) {
  for (int i=0; i < static_cast<int>(MinionType::COUNT); ++i) {
    if (equal_names(name,minion_info[i].name)) {
      return static_cast<MinionType>(i);
    }
  }
  error() << "Unknown minion: " << name << endl;
  return MinionType::NONE;
}

HeroPower REPL::find_hero_power(std::string const& name) {
  for (int i=0; i < static_cast<int>(HeroPower::COUNT); ++i) {
    if (equal_names(name,hero_power_names[i])) {
      return static_cast<HeroPower>(i);
    }
  }
  error() << "Unknown hero power: " << name << endl;
  return HeroPower::None;
}

// -----------------------------------------------------------------------------
// Commands
// -----------------------------------------------------------------------------

ostream& REPL::error() {
  if (line_number) {
    return out << filename << ":" << line_number << ": Error: ";
  } else {
    return out << "Error: ";
  }
}


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
  /*
  out << "-- Modifying the board stepwise" << endl;
  out << "move <i> to <j> = move a minion from position i to position j" << endl;
  out << "sell <i> = sell minion(s) with position/condition i" << endl;
  out << "play <minion> [at <i>]" << endl;
  out << "give <i> <buff> = buff minion(s) i with one or more buffs" << endl;
  out << endl;
  */
  out << "-- Running simulations" << endl;
  out << "actual <i> = tell about actual outcome (used in simulation display)" << endl;
  out << "run [<n>]  = run n simulations (default: 100)" << endl;
  out << "optimize   = optimize the minion order to maximize winrate" << endl;
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
  out << endl;
  out << "-- Minion format" << endl;
  out << "Minions are specified as" << endl;
  out << "  [attack/health] [golden] <name>, <buff>, <buff>, .." << endl;
  out << "For example" << endl;
  out << " * 10/12 Nightmare Amalgam" << endl;
  out << " * Golden Murloc Tidecaller, poisonous, divine shield, taunt, windfury, +12 attack" << endl;
  /*
  out << endl;
  out << "-- Refering to a minion" << endl;
  out << "You can refer to a minion with an index (1 to 7), a name, a tribe, or all" << endl;
  out << "For example" << endl;
  out << "  give all +1/+1" << endl;
  out << "  give Mechs divine shield, windfury" << endl;
  */
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
    used = false;
  }
}

void print_stats(ostream& out, vector<int> const& results) {
  ScoreSummary summary(results);
  int n = summary.num_runs();
  out.precision(1);
  out.setf(std::ios::fixed, std:: ios::floatfield);
  out << "win: " << 100*summary.win_rate() << "%, ";
  out << "tie: " << 100*summary.draw_rate() << "%, ";
  out << "lose: " << 100*summary.loss_rate() << "%" << endl;
  out.precision(3);
  out << "mean score: " << summary.mean_score();
  out << ", median score: " << results[results.size()/2] << endl;
  int steps = 10;
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

void print_damage_taken(ostream& out, int enemy_level, int health, vector<int> const& results, int sign = 1) {
  if (enemy_level <= 0) return;
  double dmg = mean_damage_taken(results, enemy_level, sign);
  out.precision(3);
  out << "mean damage " << (sign == 1 ? "taken" : "dealt") << ": " << dmg << endl;
  if (health > 0) {
    if (sign < 0) out << "their ";
    out << "expected health afterwards: " << (health - dmg);
    out << ", " << death_rate(results, enemy_level, health, sign) << "% chance to die" << endl;
  }
}

void REPL::do_run(int n) {
  if (n <= 0) n = default_num_runs;
  vector<int> results = simulate(Battle(players[0], players[1]), n);
  out << "--------------------------------" << endl;
  print_stats(out, results);
  for (int o : actual_outcomes) {
    print_outcome_percentile(out, o, results);
  }
  print_damage_taken(out, players[1].level, players[0].health, results);
  print_damage_taken(out, players[0].level, players[1].health, results, -1);
  out << "--------------------------------" << endl;
  used = true;
}

void REPL::do_optimize_order(int n) {
  if (n <= 0) n = default_num_runs;
  OptimizeMinionOrder opt(players[0], players[1], n);
  if (opt.current_score >= opt.best_score) {
    out << "Your winrate cannot be improved by reordering your minions" << endl;
  } else {
    out.precision(1);
    out.setf(std::ios::fixed, std:: ios::floatfield);
    out << "Your winrate can be improved from " << 100*opt.current_score << "% to " << 100*opt.best_score << "% by reordering your minions:" << endl;
    Board new_board = players[0];
    permute_minions(new_board, players[0].minions, opt.best_order.data(), opt.n);
    out << new_board;
    // TODO: significance test
  }
  used = true;
}

void REPL::do_show() {
  if (!battle_started) {
    step_battle = Battle(players[0], players[1], &out);
  }
  out << step_battle;
}

void REPL::do_reset() {
  battle_started = false;
  history.clear();
}

void REPL::do_step() {
  if (!battle_started) {
    history.clear();
    step_battle = Battle(players[0], players[1], &out);
    step_battle.verbose = 2;
    history.push_back(step_battle);
    step_battle.start();
    battle_started = true;
  } else if (!step_battle.done()) {
    history.push_back(step_battle);
    step_battle.attack_round();
  } else {
    out << "Battle is done, score: " << step_battle.score() << endl;
    return;
  }
  out << step_battle << endl;
}

void REPL::do_trace() {
  if (!battle_started) do_step();
  while (!step_battle.done()) do_step();
  do_step();
}

void REPL::do_back() {
  if (!history.empty()) {
    step_battle = history.back();
    history.pop_back();
    if (history.empty()) battle_started = false;
    out << step_battle << endl;
  } else {
    error() << "History is empty" << endl;
  }
}

void REPL::do_list_minions() {
  for (int i=1; i < static_cast<int>(MinionType::COUNT); ++i) {
    out << minion_info[i].name << endl;
  }
}

void REPL::do_list_hero_powers() {
  for (int i=1; i < static_cast<int>(HeroPower::COUNT); ++i) {
    out << hero_power_names[i] << endl;
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

