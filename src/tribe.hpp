#pragma once

#include <cstdint>
#include <iostream>
using std::int8_t;
using std::ostream;

// -----------------------------------------------------------------------------
// Tribes
// -----------------------------------------------------------------------------

enum class Tribe : unsigned char {
  None,
  Beast, Demon, Dragon, Mech, Murloc,
  All
};

constexpr bool has_tribe(Tribe t, Tribe query) {
  return t == Tribe::All || t == query;
}

extern const char* tribe_names[];
inline const char* name(Tribe t) { return tribe_names[static_cast<int>(t)]; }

inline ostream& operator << (ostream& s, Tribe t) {
  return s << name(t);
}

