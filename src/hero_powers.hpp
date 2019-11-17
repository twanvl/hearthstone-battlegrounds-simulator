#pragma once

#include <cstdint>
#include <iostream>
using std::int8_t;
using std::ostream;

// -----------------------------------------------------------------------------
// Hero powers
// -----------------------------------------------------------------------------

enum class HeroPower : unsigned char {
  None,
  Neffarian,           // deal 1 damage to all enemies
  RagnarosTheFirelord, // deal 8 damage to two random enemies
  PatchesThePirate,    // deal 3 damage to two random enemies
  TheLichKing,         // give right-most minion reborn
  Giantfin,            // give minions "deathrattle summon a 1/1 murloc"
  ProfessorPutricide,  // give left-most minion +10 attack
  COUNT,
};

extern const char* hero_power_names[];
inline const char* name(HeroPower t) { return hero_power_names[static_cast<int>(t)]; }

inline ostream& operator << (ostream& s, HeroPower t) {
  return s << name(t);
}

