#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <memory>

// -----------------------------------------------------------------------------
// Random number generator
// -----------------------------------------------------------------------------

class RNG {
private:
  uint64_t s[2] = {1234567891234567890u,9876543210987654321u};
public:
  RNG() {}
  RNG(uint64_t s[2]) : s{s[0],s[1]} {}

  uint64_t next();
  void jump();
  void long_jump();

  inline uint64_t random(uint64_t range) {
    // TODO: make sure range evenly divides 2^64
    return next() % range;
  }

  inline int random(int range) {
    return (int)random((uint64_t)range);
  }

  inline RNG next_rng() {
    RNG out = {s};
    jump();
    return out;
  }

  template <typename T>
  void shuffle(T* data, int n) {
    for (int i=1; i<n; ++i) {
      int j = random(i+1);
      if (i != j) std::swap(data[i], data[j]);
    }
  }
};

// -----------------------------------------------------------------------------
// Lowering variance
// -----------------------------------------------------------------------------

// Random number generator that avoids making the same choice twice in different runs.
// This reduces variance between simulation runs, and should give numbers closer to the expected value
//
// The class keeps track of a tree of random choices that have been made.
// When a new random number is requested, a permutation of all possible results is made,
// and on the next visit to a node only other choices can be visited until we have visited them all.
//
// We keep a budget (in terms of state space size) so that the table doesn't become too large.
// After the budget is exhausted falls back to a normal rng
class LowVarianceRNG {
private:
  int budget, initial_budget;
  RNG& rng;
  struct Entry;
  struct Tree {
    size_t i; // index of next child to use when visiting this node
    std::vector<std::unique_ptr<Entry>> children;
  };
  struct Entry {
    int value;
    Tree tree;
    Entry(int v) : value(v) {}
  };
  Tree root;
  Tree* cur;
public:
  LowVarianceRNG(RNG& rng, int budget = 10000)
    : budget(budget), initial_budget(budget)
    , rng(rng)
    , cur(&root)
  {}

  // Start a new run
  void start() {
    cur = &root;
    budget = initial_budget;
  }

  int random(int n);
};


// -----------------------------------------------------------------------------
// The RNG to use in battles
// -----------------------------------------------------------------------------

#define LOW_VARIANCE_RNG 0
extern RNG global_rng;

#if LOW_VARIANCE_RNG
  using BattleRNG = LowVarianceRNG;
  extern BattleRNG global_battle_rng;
#else
  using BattleRNG = RNG;
  #define global_battle_rng global_rng
#endif

