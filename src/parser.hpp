#pragma once

#include "minion.hpp"
#include "simulation.hpp"
#include <cstring>

// -----------------------------------------------------------------------------
// Error handling
// -----------------------------------------------------------------------------

struct StringParser;

struct ErrorHandler {
  ostream& out;
  const char* filename;
  int line_number = 0;

  ErrorHandler(ostream& out, const char* filename) : out(out), filename(filename) {}

  ostream& operator() () const {
    if (line_number) {
      return out << filename << ":" << line_number << ": Error: ";
    } else {
      return out << "Error: ";
    }
  }
};

// -----------------------------------------------------------------------------
// Name lookup utilities
// -----------------------------------------------------------------------------

bool ignored_char(char x) {
  return !isalpha(x) && !isdigit(x) && x != '/';
  //return isspace(x) || x == '\'' || x == '-' || x == '_' || x == ':';
}
bool is_word_end(char x) {
  return x == 0 || isspace(x) || x == ',' || x == ':';
}
bool is_token_end(char x) {
  return x == 0 || x == ',';
}

// compare strings, ignoring case and allow skipping of joining characters
// b is the query
bool match_name(const char* a, const char* b) {
  while (*a && ignored_char(*a)) ++a;
  while (*b && ignored_char(*b)) ++b;
  while (*a && *b) {
    if (tolower(*a) != tolower(*b)) return false;
    ++a; ++b;
    while (*a && ignored_char(*a)) ++a;
    while (*b && ignored_char(*b)) ++b;
  }
  return !*a && !*b;
}

// compare strings, allow a to be longer (ending in a word ending charater)
// ignore joining characters in the middle of the query, and space at the start
// return point in a, or nullptr if no match
const char* match_name_or_prefix(const char* a, const char* b) {
  while (*a && isspace(*a)) ++a;
  while (*b && isspace(*b)) ++b;
  while (*a && *b) {
    if (tolower(*a) != tolower(*b)) return nullptr;
    ++a; ++b;
    if (*b) {
      while (*a && ignored_char(*a)) ++a;
      while (*b && ignored_char(*b)) ++b;
    }
  }
  if (!*b && is_word_end(*a)) return a;
  else return nullptr;
}

struct StringParser {
  const char* str;
  ErrorHandler const& error;

  StringParser(const char* str, ErrorHandler const& error)
    : str(str), error(error)
  {}

  // Matching/parsing functions
  // Convention: match_X does nothing on failure
  // parse_X shows error message on failure

  // try to match a string, if there is a match, advance
  bool match(const char* query) {
    const char* after = match_name_or_prefix(str, query);
    if (after) {
      str = after;
      return true;
    } else {
      return false;
    }
  }
  bool match_exact(const char* query) {
    const char* after = str;
    while (*query) {
      if (*after != *query) {
        return false;
      }
      ++after; ++query;
    }
    str = after;
    return true;
  }
  bool match_int(int& out) {
    int n = 0;
    if (sscanf(str, "%d%n", &out, &n) >= 1) {
      str += n;
      return true;
    } else {
      return false;
    }
  }
  bool match_string(std::string& out, char end=0, bool allow_empty=true) {
    const char* after = str;
    while (*after && *after != end) ++after;
    if (allow_empty || after != str) {
      out.assign(str,after);
      str = after;
      return true;
    } else {
      return false;
    }
  }
  bool match_end() {
    const char* at = str;
    while (*str) {
      if (*str == '#') break; // comment
      if (!isspace(*str)) {
        str = at;
        return false;
      }
      ++str;
    }
    return true;
  }

  void skip_ws() {
    while (isspace(*str)) ++str;
  }
  bool skip_until(const char* query) {
    const char* pos = strstr(str, query);
    if (pos) {
      str = pos + strlen(query);
      return true;
    } else {
      return false;
    }
  }
  // skip all characters up to and including the query string
  bool skip_until(char query) {
    const char* pos = strchr(str,query);
    if (pos) {
      str = pos + 1;
      return true;
    } else {
      return false;
    }
  }

  // Error raising versions

  bool parse_exact(const char* query) {
    if (match_exact(query)) return true;
    expected(query);
    return false;
  }
  bool parse_end() {
    if (match_end()) return true;
    expected("end of line");
    return false;
  }
  bool parse_int(int& out) {
    if (match_int(out)) return true;
    expected("number");
    return false;
  }
  bool parse_positive(int& out) {
    const char* at = str;
    if (match_int(out) && out > 0) return true;
    str = at;
    expected("positive number");
    return false;
  }
  bool parse_non_negative(int& out) {
    const char* at = str;
    if (match_int(out) && out >= 0) return true;
    str = at;
    expected("non-negative number");
    return false;
  }
  bool parse_string(std::string& out, char end=0) {
    if (match_string(out,end)) return true;
    expected("string");
    return false;
  }

  // Errors

  void unknown(const char* what) const {
    error() << "Unknown " << what << ": " << next_token() << endl;
  }
  void expected(const char* what) const {
    error() << "Expected " << what << ", instead of " << next_token() << endl;
  }

  // Peeking/queries

  char peek() const {
    return *str;
  }
  bool end() const {
    return !*str;
  }
  std::string next_token(char end_char=0) const {
    const char* start = str;
    while (isspace(*start)) ++start;
    if (!*start) return "<end of line>";
    const char* end = start;
    while (!is_token_end(*end) && *end != end_char) ++end;
    return std::string(start,end);
  }
};

// -----------------------------------------------------------------------------
// Parsers
// -----------------------------------------------------------------------------

bool match_tribe(StringParser& in, Tribe& out) {
  for (int i=0; i < Tribe_count; ++i) {
    if (in.match(tribe_names[i])) {
      out = static_cast<Tribe>(i);
      return true;
    }
  }
  return false;
}

bool match_minion_type(StringParser& in, MinionType& out) {
  for (int i=0; i < MinionType_count; ++i) {
    if (in.match(minion_info[i].name)) {
      out = static_cast<MinionType>(i);
      return true;
    }
  }
  return false;
}

bool match_hero_type(StringParser& in, HeroType& out) {
  for (int i=0; i < HeroType_count; ++i) {
    if (in.match(hero_info[i].name) || in.match(hero_info[i].hero_power.name)) {
      out = static_cast<HeroType>(i);
      return true;
    }
  }
  return false;
}

bool match_objective(StringParser& in, Objective& out) {
  for (int i=0; i < NUM_OBJECTIVES; ++i) {
    if (in.match(name(static_cast<Objective>(i)))) {
      out = static_cast<Objective>(i);
      return true;
    }
  }
  return false;
}

bool parse_minion_type(StringParser& in, MinionType& out) {
  if (!match_minion_type(in,out)) {
    in.unknown("minion type");
    return false;
  }
  return true;
}

bool parse_hero_type(StringParser& in, HeroType& out) {
  if (!match_hero_type(in,out)) {
    in.unknown("hero / hero power");
    return false;
  }
  return true;
}

bool parse_objective(StringParser& in, Objective& out) {
  if (!match_objective(in,out)) {
    in.unknown("objective");
    return false;
  }
  return true;
}


// Parse one or more buffs, apply them to a minion
bool parse_buffs(StringParser& in, Minion& m) {
  // read buffs
  while (true) {
    in.skip_ws();
    if (in.peek() == '+' || in.peek() == '-') {
      const char* at = in.str;
      int attack = 0;
      if (!in.parse_int(attack)) {
        return false;
      }
      if (in.match_exact("/")) {
        int health = 0;
        if (in.match_int(health)) {
          m.buff(attack,health);
        } else {
          in.str = at;
          in.unknown("minion buff");
          return false;
        }
      } else if (in.match("attack")) {
        m.buff(attack,0);
      } else if (in.match("health")) {
        m.buff(0,attack);
      } else {
        in.str = at;
        in.unknown("minion buff");
        return false;
      }
    } else if (in.match("taunt")) {
      m.taunt = true;
    } else if (in.match("divine shield")) {
      m.divine_shield = true;
    } else if (in.match("poison") || in.match("poisonous")) {
      m.poison = true;
    } else if (in.match("windfury")) {
      m.windfury = true;
    } else if (in.match("reborn")) {
      m.reborn = true;
    } else if (in.match("microbots")) {
      m.add_deathrattle_microbots();
    } else if (in.match("golden microbots")) {
      m.add_deathrattle_golden_microbots();
    } else if (in.match("plants")) {
      m.add_deathrattle_plants();
    } else if (in.match("Annoy-o-Module")) {
      m.taunt = true;
      m.divine_shield = true;
      m.buff(2,4);
    } else if (in.match("golden Annoy-o-Module")) {
      m.taunt = true;
      m.divine_shield = true;
      m.buff(2*2,2*4);
    } else if (in.match("Replicating Menace")) {
      m.buff(3,1);
      m.add_deathrattle_microbots(3);
    } else if (in.match("golden Replicating Menace")) {
      m.buff(2*3,2*1);
      m.add_deathrattle_golden_microbots(3);
    } else if (in.end()) {
      in.expected("a buff");
      return false;
    } else {
      in.unknown("minion buff");
      return false;
    }
    if (!in.match(",")) {
      return true;
    }
  }
}

bool parse_minion(StringParser& in, Minion& m) {
  int attack = -1, health = -1;
  bool golden = false;
  // read stats
  in.skip_ws();
  if (isdigit(in.peek())) {
    const char* at = in.str;
    if (!(in.match_int(attack) && in.match_exact("/") && in.match_int(health))) {
      in.str = at;
      in.expected("'attack/health'");
      return false;
    }
  }
  // golden?
  if (in.match("gold") || in.match("golden")) {
    golden = true;
  }
  // find minion by name
  MinionType type;
  in.skip_ws();
  if (in.end()) {
    in.error() << "Expected minion, see help command for the syntax" << endl;
    return false;
  }
  if (!parse_minion_type(in, type)) {
    return false;
  }
  // construct
  m = Minion(type, golden);
  if (attack != -1) {
    m.attack = attack;
    m.health = health;
    m.invalid_aura = true;
  }
  // buff?
  if (in.match(",")) {
    if (!parse_buffs(in, m)) return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
// References to a minion or a group of minions
// -----------------------------------------------------------------------------

struct MinionRef {
  enum class Type {
    Position,
    Last,
    Tribe,
    MinionType,
  } type;
  union {
    int pos;
    Tribe tribe;
    MinionType minion_type;
  };
  MinionRef() : type(Type::Tribe), tribe(Tribe::None) {}
  MinionRef(int i) : type(Type::Position), pos(i) {}
  MinionRef(Tribe t) : type(Type::Tribe), tribe(t) {}
  MinionRef(MinionType t) : type(Type::MinionType), minion_type(t) {}

  static MinionRef last() {
    MinionRef r;
    r.type = Type::Last;
    return r;
  }

  template <typename Fun>
  void for_each(Board& b, Fun fun) const {
    switch (type) {
      case Type::Position:
        if (b.minions.contains(pos)) {
          fun(b.minions[pos]);
        }
        return;
      case Type::Last: {
        int n = b.minions.size();
        if (n > 0) {
          fun(b.minions[n-1]);
        }
        return;
      }
      case Type::Tribe:
        b.minions.for_each([&](Minion& m) {
          if (tribe == Tribe::All || m.has_tribe(tribe)) {
            fun(m);
          }
        });
        return;
      case Type::MinionType:
        b.minions.for_each([&](Minion& m) {
          if (m.type == minion_type) {
            fun(m);
          }
        });
        return;
    }
  }
};

struct MinionAndSideRef {
  bool enemy = false;
  MinionRef ref;

  template <typename Fun>
  void for_each(Board& player, Board& enemy, Fun fun) const {
    ref.for_each(this->enemy ? enemy : player, fun);
  }
  template <typename Fun>
  void for_each(Board players[2], Fun fun) const {
    ref.for_each(players[this->enemy ? 1 : 0], fun);
  }
};

bool parse_minion_ref(StringParser& in, MinionRef& out) {
  Tribe tribe;
  if (match_tribe(in, tribe)) {
    out = MinionRef(tribe);
    return true;
  }
  MinionType type;
  if (match_minion_type(in, type)) {
    out = MinionRef(type);
    return true;
  }
  int i;
  if (in.match_int(i) && i > 0 && i-1 < BOARDSIZE) {
    out = MinionRef(i-1); // Note: 1 based indexing in ui
    return true;
  }
  if (in.match("start") || in.match("first")) {
    out = MinionRef(0);
    return true;
  }
  if (in.match("last")) {
    out = MinionRef::last();
    return true;
  }
  in.expected("minion reference (position, tribe or name)");
  return false;
}

bool parse_minion_and_side_ref(StringParser& in, MinionAndSideRef& out) {
  out.enemy = false;
  if (in.match("enemy")) out.enemy = true;
  if (!parse_minion_ref(in,out.ref)) return false;
  return true;
}

