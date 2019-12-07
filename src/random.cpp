#include "random.hpp"
#include "random_keys.hpp"

// -----------------------------------------------------------------------------
// Random number generator: xoroshiro128+
// -----------------------------------------------------------------------------

/*  Written in 2016-2018 by David Blackman and Sebastiano Vigna (vigna@acm.org)

To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide. This software is distributed without any warranty.

See <http://creativecommons.org/publicdomain/zero/1.0/>. */

/* Modified by Twan van Laarhoven
*/

/* This is xoroshiro128+ 1.0, our best and fastest small-state generator
   for floating-point numbers. We suggest to use its upper bits for
   floating-point generation, as it is slightly faster than
   xoroshiro128**. It passes all tests we are aware of except for the four
   lower bits, which might fail linearity tests (and just those), so if
   low linear complexity is not considered an issue (as it is usually the
   case) it can be used to generate 64-bit outputs, too; moreover, this
   generator has a very mild Hamming-weight dependency making our test
   (http://prng.di.unimi.it/hwd.php) fail after 5 TB of output; we believe
   this slight bias cannot affect any application. If you are concerned,
   use xoroshiro128++, xoroshiro128** or xoshiro256+.

   We suggest to use a sign test to extract a random Boolean value, and
   right shifts to extract subsets of bits.

   The state must be seeded so that it is not everywhere zero. If you have
   a 64-bit seed, we suggest to seed a splitmix64 generator and use its
   output to fill s. 

   NOTE: the parameters (a=24, b=16, b=37) of this version give slightly
   better results in our test than the 2016 version (a=55, b=14, c=36).
*/

static inline uint64_t rotl(const uint64_t x, int k) {
	return (x << k) | (x >> (64 - k));
}


uint64_t RNG::next() {
	const uint64_t s0 = s[0];
	uint64_t s1 = s[1];
	const uint64_t result = s0 + s1;

	s1 ^= s0;
	s[0] = rotl(s0, 24) ^ s1 ^ (s1 << 16); // a, b
	s[1] = rotl(s1, 37); // c

	return result;
}


/* This is the jump function for the generator. It is equivalent
   to 2^64 calls to next(); it can be used to generate 2^64
   non-overlapping subsequences for parallel computations. */

void RNG::jump() {
	static const uint64_t JUMP[] = { 0xdf900294d8f554a5, 0x170865df4b3201fc };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	for(unsigned int i = 0; i < sizeof JUMP / sizeof *JUMP; i++)
		for(int b = 0; b < 64; b++) {
			if (JUMP[i] & UINT64_C(1) << b) {
				s0 ^= s[0];
				s1 ^= s[1];
			}
			next();
		}

	s[0] = s0;
	s[1] = s1;
}


/* This is the long-jump function for the generator. It is equivalent to
   2^96 calls to next(); it can be used to generate 2^32 starting points,
   from each of which jump() will generate 2^32 non-overlapping
   subsequences for parallel distributed computations. */

void RNG::long_jump() {
	static const uint64_t LONG_JUMP[] = { 0xd2a98b26625eee7b, 0xdddf9b1090aa7ac1 };

	uint64_t s0 = 0;
	uint64_t s1 = 0;
	for(unsigned int i = 0; i < sizeof LONG_JUMP / sizeof *LONG_JUMP; i++)
		for(int b = 0; b < 64; b++) {
			if (LONG_JUMP[i] & UINT64_C(1) << b) {
				s0 ^= s[0];
				s1 ^= s[1];
			}
			next();
		}

	s[0] = s0;
	s[1] = s1;
}

// -----------------------------------------------------------------------------
// Low variance RNG
// -----------------------------------------------------------------------------

int LowVarianceRNG::random(int n) {
  if (n <= 1) return 0;
  if (budget < n) {
    // don't keep subdividing
    return rng.random(n);
  } else {
    if (cur->children.empty()) {
      // initialize children
      cur->i = n;
      for (int i=0; i<n; ++i) {
        cur->children.emplace_back(new Entry(i));
      }
    } else if (cur->children.size() != (size_t)n) {
      //std::cerr << "requesting different random number range than previous runs" << std::endl;
      return rng.random(n);
    }
    if (cur->i >= cur->children.size()) {
      // reshuffle children
      rng.shuffle(cur->children);
      cur->i = 0;
    }
    // take next item from permutation
    Entry& e = *cur->children[cur->i++];
    cur = &e.tree;
    budget /= n; // decrease budget
    return e.value;
  }
}

// -----------------------------------------------------------------------------
// Fast low variance RNG
// -----------------------------------------------------------------------------

int FastLowVarianceRNG::random(int n) {
  if (n <= 1) return 0;
  if (budget < n) {
    // don't keep subdividing
    return rng.random(n);
  } else {
    if (cur_entry < 0) {
      // initialize new permutation entry
      cur_entry = (int)entries.size();
      if (prev_entry >= 0) entries[prev_entry].next = cur_entry;
      entries.push_back({n,n}); // header
      for (int i=0; i<n; ++i) {
        entries.push_back({i,-1}); // entry
      }
    }
    Header& cur = reinterpret_cast<Header&>(entries[cur_entry]);
    if (cur.n != n) {
      //std::cerr << "requesting different random number range than previous runs" << std::endl;
      return rng.random(n);
    }
    if (cur.i >= cur.n) {
      // reshuffle
      rng.shuffle(&entries[cur_entry + 1], n);
      cur.i = 0;
    }
    // take next item from permutation
    int p = cur_entry + 1 + cur.i++;
    prev_entry = p;
    cur_entry = entries[p].next;
    budget /= n; // decrease budget
    return entries[p].value;
  }
}

// -----------------------------------------------------------------------------
// Keyed rng
// -----------------------------------------------------------------------------

inline bool operator == (RNGKey a, RNGKey b) {
  return a.key == b.key;
}
namespace std {
  template <> struct hash<RNGKey> {
    inline size_t operator() (RNGKey k) const noexcept {
      return hash<int>()(k.key);
    }
  };
}

template <typename Key>
size_t KeyedRNG<Key>::HeaderHash::operator()(KeyedRNG<Key>::Header const& x) const noexcept {
  size_t seed = 0;
  hash_combine(seed, x.key);
  hash_combine(seed, x.n);
  return seed;
}

template <typename Key>
void KeyedRNG<Key>::start() {
  for (auto& entry : table) {
    entry.second.times_used = 0;
  }
}

template <typename Key>
int KeyedRNG<Key>::random(int n, Key key) {
  if (n <= 1) return 0;
  KeyEntry& ke = table[{key,n}];
  size_t times_used = ke.times_used++;
  if (times_used >= ke.entries.size()) {
    Entry new_entry;
    new_entry.perm.reserve(n);
    for (int i=0; i<n; ++i) {
      new_entry.perm.push_back(i);
    }
    new_entry.i = n;
    ke.entries.emplace_back(std::move(new_entry));
    if (times_used > 30) {
      exit(1);
    }
  }
  Entry& e = ke.entries[times_used];
  if (e.i >= n) {
    rng.shuffle(e.perm);
    e.i = 0;
  }
  return e.perm[e.i++];
}

template class KeyedRNG<RNGKey>;

// -----------------------------------------------------------------------------
// Global rng
// -----------------------------------------------------------------------------

RNG global_rng;

#if LOW_VARIANCE_RNG
BattleRNG global_battle_rng(global_rng,0);
#elif KEYED_RNG
BattleRNG global_battle_rng(global_rng);
#endif
