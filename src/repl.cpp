#include "battle.hpp"
#include <vector>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
using namespace std;

// -----------------------------------------------------------------------------
// REPL class
// -----------------------------------------------------------------------------

const int DEFAULT_RUNS = 1000;

struct REPL {
  Board players[2];
  int current_player = 0;
  bool used = false;

  // reporting
  vector<int> actual_outcomes;

  // single stepping
  bool battle_started = false;
  Battle step_battle;
  vector<Battle> history;

  // error messages
  string filename;
  int line_number = 0;
  ostream& error();

  // Repl
  void repl(istream&, bool prompts);

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
  void do_show();
  void do_reset();
  void do_step();
  void do_trace();
  void do_back();
  void do_run(int runs = DEFAULT_RUNS);
  void do_add_minion(Minion const&);
  void do_end_input();
};
/*
// -----------------------------------------------------------------------------
// Parser
// -----------------------------------------------------------------------------

struct MinionFilter {
  enum FilterType {
    POS,TRIBE,TAUNT,ALL,LEFT,RIGHT
  };
  int pos;
  Tribe tribe;
};
*/
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
      cout << "> " << flush;
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
  in >> cmd >> ws;
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
    cout << endl;
    return;
  }
  // a command
  std::string cmd = read_command(in);
  if (!in) return;
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
  } else if (cmd == "info" || cmd == "msg" || cmd == "message" || cmd == "print" || cmd == "echo") {
    std::string msg;
    getline(in,msg);
    cout << msg << endl;
  } else if (cmd == "hp" || cmd == "hero-power" || cmd == "heropower") {
    string hp;
    getline(in,hp);
    players[current_player].hero_power = find_hero_power(hp);
  } else if (cmd == "actual" || cmd == "outcome") {
    int n = 0;
    if (in >> n) {
      actual_outcomes.push_back(n);
    } else {
      cout << "Error: Expected outcome value, usage:" << endl;
      cout << "  actual <score>" << endl;
    }
  } else if (cmd == "run" || cmd == "simulate") {
    int n = DEFAULT_RUNS;
    in >> n;
    do_run(n);
  } else if (cmd == "show") {
    do_show();
  } else if (cmd == "minion") {
    Minion m;
    if (parse_minion(in,m)) {
      cout << m << endl;
    }
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
    return cout << filename << ":" << line_number << ": Error: ";
  } else {
    return cout << "Error: ";
  }
}


void REPL::do_help() {
  cout << "Commands:" << endl;
  cout << endl;
  cout << "-- Defining the board" << endl;
  cout << "board      = begin defining player board" << endl;
  cout << "vs         = begin defining opposing board" << endl;
  //cout << "swap       = swap with enemy board" << endl;
  cout << "* <minion> = give the next minion" << endl;
  cout << "HP <hero>  = tell that a hero power is used" << endl;
  //cout << "secret <secret> = tell that a secret is in play" << endl;
  //cout << "after-auras = tell the program that the stats given are after taking auras into account (default = true)" << endl;
  cout << endl;
  /*
  cout << "-- Modifying the board stepwise" << endl;
  cout << "move <i> to <j> = move a minion from position i to position j" << endl;
  cout << "sell <i> = sell minion(s) with position/condition i" << endl;
  cout << "play <minion> [at <i>]" << endl;
  cout << "give <i> <buff> = buff minion(s) i with one or more buffs" << endl;
  */
  cout << endl;
  cout << "-- Running simulations" << endl;
  cout << "actual <i> = tell about actual outcome (used in simulation display)" << endl;
  cout << "run [<n>]  = run n simulations (default: 100)" << endl;
  cout << endl;
  cout << "-- Stepping through a single battle" << endl;
  cout << "show       = show the board state" << endl;
  cout << "reset      = reset battle" << endl;
  cout << "step       = do 1 attack step, or start if battle not started yet" << endl;
  cout << "trace      = do steps until the battle ends" << endl;
  cout << "back       = step backward. can be used to re-roll RNG" << endl;
  cout << endl;
  cout << "-- Other" << endl;
  cout << "info       = show a message" << endl;
  cout << "help       = show this help message" << endl;
  cout << "quit       = quit the simulator" << endl;
  //cout << "minions    = list all minions" << endl;
  cout << endl;
  cout << "-- Minion format" << endl;
  cout << "Minions are specified as" << endl;
  cout << "  [attack/health] [golden] <name>, <buff>, <buff>, .." << endl;
  cout << "For example" << endl;
  cout << " * 10/12 Nightmare Amalgam" << endl;
  cout << " * Golden Murloc Tidecaller, poisonous, divine shield, taunt, windfury, +12 attack" << endl;
  /*
  cout << endl;
  cout << "-- Refering to a minion" << endl;
  cout << "You can refer to a minion with an index (1 to 7), a name, a tribe, or all" << endl;
  cout << "For example" << endl;
  cout << "  give all +1/+1" << endl;
  cout << "  give Mechs divine shield, windfury" << endl;
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

void REPL::do_add_minion(Minion const& m) {
  if (players[current_player].full()) {
    error() << "Player already has a full board" << endl;
  } else {
    players[current_player].append(m);
    used = false;
  }
}

vector<int> simulate(Battle const& b, int n) {
  vector<int> results;
  results.reserve(n);
  for (int i=0; i<n; ++i) {
    Battle copy = b;
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
  int n = static_cast<int>(results.size()) - 1;
  cout << "win: " << (wins*100/n) << "%, tie: " << (ties*100)/n << "%, lose: " << (losses*100)/n << "%" << endl;
  cout << "mean score: " << mean(results);
  cout << ", median score: " << results[results.size()/2] << endl;
  int steps = 10;
  cout << "percentiles: ";
  for (int i=0; i <= steps; ++i) {
    cout << results[i*n/steps] << " ";
  }
  cout << endl;
}

int percentile(int i, vector<int> const& results) {
  auto bounds = equal_range(results.begin(), results.end(), i);
  int a = static_cast<int>(bounds.first - results.begin());
  int b = static_cast<int>(bounds.second - results.begin());
  return 100 * (a + b) / 2 / (results.size() - 1);
}

void REPL::do_run(int n) {
  vector<int> results = simulate(Battle(players[0], players[1]), n);
  cout << "--------------------------------" << endl;
  print_stats(results);
  for (int o : actual_outcomes) {
    int p =percentile(o,results);
    cout << "actual outcome: " << o << ", is at the " << p << "-th percentile"
         << (p < 15 ? ", you got unlucky" : p > 85 ? ", you got lucky" : "") << endl;
  }
  cout << "--------------------------------" << endl;
  used = true;
}

void REPL::do_show() {
  if (!battle_started) {
    step_battle = Battle(players[0], players[1]);
  }
  cout << step_battle;
}

void REPL::do_reset() {
  battle_started = false;
  history.clear();
}

void REPL::do_step() {
  if (!battle_started) {
    history.clear();
    step_battle = Battle(players[0], players[1]);
    step_battle.verbose = 2;
    history.push_back(step_battle);
    step_battle.start();
    battle_started = true;
  } else if (!step_battle.done()) {
    history.push_back(step_battle);
    step_battle.attack_round();
  } else {
    cout << "Battle is done, score: " << step_battle.score() << endl;
    return;
  }
  cout << step_battle << endl;
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
    cout << step_battle << endl;
  } else {
    error() << "History is empty" << endl;
  }
}

// -----------------------------------------------------------------------------
// Main function
// -----------------------------------------------------------------------------

int main(int argc, char const** argv) {
  if (argc <= 1) {
    REPL repl;
    repl.repl(cin, true);
  } else {
    for (int i=1; i<argc; ++i) {
      ifstream is(argv[i]);
      if (!is) {
        cerr << "Error loading file " << argv[i] << endl;
        return 1;
      }
      REPL repl;
      repl.filename = argv[i];
      repl.repl(is,false);
    }
  }
  return 0;
}

