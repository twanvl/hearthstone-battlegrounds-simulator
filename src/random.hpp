#pragma once

#include <cstdint>
#include <utility>
#include <vector>
#include <unordered_map>
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

  void start() {} // for compatability with low variance RNG

  // for compatability with keyed RNG
  template <typename Key>
  inline int random(int n, Key key) {
    return random(n);
  }

  template <typename T>
  void shuffle(T* data, int n) {
    for (int i=1; i<n; ++i) {
      int j = random(i+1);
      if (i != j) std::swap(data[i], data[j]);
    }
  }
  template <typename T>
  void shuffle(std::vector<T>& data) {
    shuffle(data.data(), static_cast<int>(data.size()));
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

  template <typename Key>
  inline int random(int n, Key key) {
    return random(n);
  }
};

class FastLowVarianceRNG {
private:
  int prev_entry = -1, cur_entry = -1;
  int budget, initial_budget;
  RNG& rng;
  /*
  We store a table of permutations of numbers 0..n-1.
  Each permutation has a header (n,i) followed by n entries (value,next)
  where
   n = size of this permutation
   i = next index to use from this permutation
   value = value to return
   next = index into table for the next permutation after yielding this value (or -1 if not yet visited)
  */
  // header
  struct Header {
    int n;
    int i;
  };
  // entry
  struct Entry {
    int value;
    int next;
  };
  std::vector<Entry> entries;
public:
  FastLowVarianceRNG(RNG& rng, int budget = 10000)
    : budget(budget), initial_budget(budget)
    , rng(rng)
  {}

  // Start a new run
  void start() {
    prev_entry = -1;
    cur_entry = entries.empty() ? -1 : 0;
    budget = initial_budget;
  }

  int random(int n);

  template <typename Key>
  inline int random(int n, Key key) {
    return random(n);
  }
};


// -----------------------------------------------------------------------------
// Keyed rng
// -----------------------------------------------------------------------------

// Random number generator that uses permutations to reduce variance (as above)
// but to detect 'the same' call to .random(), we use a caller provided key
//
// So for each (key,n) we keep a sperate permutation.
// 
template <typename Key>
class KeyedRNG {
private:
  struct Header {
    Key key;
    int n;
    inline bool operator == (Header const& that) const noexcept {
      return key == that.key && n == that.n;
    }
  };
  struct Entry {
    int i; // number of items from the permutation used
    std::vector<int> perm; // permutation of [0..n-1]
  };
  struct KeyEntry {
    size_t times_used = 0; // number of times this key has been used in this run
    std::vector<Entry> entries;
  };
  struct HeaderHash {
    inline size_t operator()(Header const&) const noexcept;
  };
  std::unordered_map<Header,KeyEntry,HeaderHash> table;
  RNG& rng;
public:
  KeyedRNG(RNG& rng) : rng(rng) {}
  void start();
  int random(int n, Key key);
};

template <class T>
inline void hash_combine(std::size_t& seed, const T& v) {
  std::hash<T> hasher;
  seed ^= hasher(v) + 0x9e3779b9 + (seed<<6) + (seed>>2);
}

// -----------------------------------------------------------------------------
// global RNG
// -----------------------------------------------------------------------------

extern RNG global_rng;

// -----------------------------------------------------------------------------
// RNG keys
// -----------------------------------------------------------------------------

struct RNGKey{ int key; };

// -----------------------------------------------------------------------------
// The RNG to use in battles
// -----------------------------------------------------------------------------

#define LOW_VARIANCE_RNG 0
#define KEYED_RNG 1

#if LOW_VARIANCE_RNG
  using BattleRNG = FastLowVarianceRNG;
  extern BattleRNG global_battle_rng;
#elif KEYED_RNG
  using BattleRNG = KeyedRNG<RNGKey>;
  extern BattleRNG global_battle_rng;
#else
  using BattleRNG = RNG&;
  #define global_battle_rng global_rng
#endif

