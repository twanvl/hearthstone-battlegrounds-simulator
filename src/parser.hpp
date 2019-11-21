
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
  return isspace(x) || x == '\'' || x == '-' || x == '_' || x == ':';
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
  bool exact_match(const char* query) {
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
  void skip_ws() {
    while (isspace(*str)) ++str;
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

  // Error raising versions

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
  std::string next_token() const {
    const char* start = str;
    while (isspace(*start)) ++start;
    if (!*start) return "<end of line>";
    const char* end = start;
    while (!is_token_end(*end)) ++end;
    return std::string(start,end);
  }
};

// -----------------------------------------------------------------------------
// Parsers
// -----------------------------------------------------------------------------

bool match_minion_type(StringParser& in, MinionType& out) {
  for (int i=0; i < static_cast<int>(MinionType::COUNT); ++i) {
    if (in.match(minion_info[i].name)) {
      out = static_cast<MinionType>(i);
      return true;
    }
  }
  return false;
}

bool match_hero_power(StringParser& in, HeroPower& out) {
  for (int i=0; i < static_cast<int>(HeroPower::COUNT); ++i) {
    if (in.match(hero_power_names[i])) {
      out = static_cast<HeroPower>(i);
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

bool parse_hero_power(StringParser& in, HeroPower& out) {
  if (!match_hero_power(in,out)) {
    in.unknown("hero power");
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
      if (in.exact_match("/")) {
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
    if (!(in.match_int(attack) && in.exact_match("/") && in.match_int(health))) {
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
  if (attack != -1) m.attack = attack;
  if (health != -1) m.health = health;
  // buff?
  if (in.match(",")) {
    if (!parse_buffs(in, m)) return false;
  }
  return true;
}

