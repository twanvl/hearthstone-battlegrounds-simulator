#pragma once
#include "minion.hpp"
using std::ostream;
using std::endl;

// -----------------------------------------------------------------------------
// Array of at most N minions
// -----------------------------------------------------------------------------

#if 1

template <int N>
struct MinionArray {
private:
  // List of minions.
  // Invariants:
  //  the first size() elements are valid, all other elements have !.exists()
  Minion minions[N];

public:
  MinionArray() {}
  MinionArray(std::initializer_list<Minion> minions) {
    for (size_t i=0; i<minions.size() && i<7; ++i) {
      this->minions[i] = minions.begin()[i];
    }
  }

  // Access

  Minion& operator [] (int i) {
    return minions[i];
  }

  Minion const& operator [] (int i) const {
    return minions[i];
  }

  // Queries

  int size() const {
    for (int i=0; i<N; ++i) {
      if (!minions[i].exists()) return i;
    }
    return N;
  }

  bool empty() const {
    return !minions[0].exists();
  }

  bool full() const {
    return minions[N-1].exists();
  }

  bool contains(int pos) const {
    return pos >= 0 && pos < N && minions[pos].exists();
  }

  // Modification

  void clear() {
    for (int i=0; i<N; ++i) {
      minions[i].clear();
    }
  }

  int append(Minion const& minion) {
    for (int i=0; i<N; ++i) {
      if (!minions[i].exists()) {
        minions[i] = minion;
        return i;
      }
    }
    return N;
  }

  bool insert(int pos, Minion const& minion) {
    if (full()) return false;
    std::move_backward(&minions[pos], &minions[N-1], &minions[N]);
    minions[pos] = minion;
    return true;
  }

  void remove(int pos) {
    std::move(&minions[pos+1], &minions[N], &minions[pos]);
    minions[N-1].clear();
  }

  void remove_all_from(int pos) {
    for (;pos < N && minions[pos].exists(); ++pos) {
      minions[pos].clear();
    }
  }

  // Iterators

  using iterator = Minion*;
  using reference = Minion&;
  using const_iterator = Minion const*;
  using const_reference = Minion const&;

  struct end_iterator {
    Minion const* ptr;
  };
  iterator begin() {
    return minions;
  }
  const_iterator begin() const {
    return minions;
  }
  end_iterator end() const {
    return &minions[N];
  }

  // Iteration

  template <typename F>
  void for_each(F fun) {
    for (int i=0; i<N && minions[i].exists(); ++i) {
      fun(minions[i]);
    }
  }
  template <typename F>
  void for_each(F fun) const {
    for (int i=0; i<N && minions[i].exists(); ++i) {
      fun(minions[i]);
    }
  }

  template <typename F>
  void for_each_alive(F fun) {
    for (int i=0; i<N && minions[i].exists(); ++i) {
      if (!minions[i].dead()) {
        fun(minions[i]);
      }
    }
  }
  template <typename F>
  void for_each_alive(F fun) const {
    for (int i=0; i<N && minions[i].exists(); ++i) {
      if (!minions[i].dead()) {
        fun(minions[i]);
      }
    }
  }

  template <typename F>
  void for_each_with_pos(F fun) {
    for (int i=0; i<N && minions[i].exists(); ++i) {
      fun(i, minions[i]);
    }
  }
  template <typename F>
  void for_each_with_pos(F fun) const {
    for (int i=0; i<N && minions[i].exists(); ++i) {
      fun(i, minions[i]);
    }
  }

  template <typename F>
  int count_if(F fun) const {
    int count = 0;
    for_each([&count,fun](Minion const& m) { if (fun(m)) count++; });
    return count;
  }
};

// -----------------------------------------------------------------------------
// Vector as minion array
// -----------------------------------------------------------------------------
#else

#include <vector>

template <int N>
struct MinionArray : public std::vector<Minion> {
  MinionArray() {}
  MinionArray(std::initializer_list<Minion> minions)
    : std::vector<Minion>(minions)
  {}

  // Queries

  bool full() const {
    return size() == N;
  }

  bool contains(int pos) const {
    return (size_t)pos < size();
  }

  // Modification

  int append(Minion const& minion) {
    push_back(minion);
    return (int)size() - 1;
  }

  bool insert(int pos, Minion const& minion) {
    if (full()) return false;
    std::vector<Minion>::insert(begin() + pos, minion);
    return true;
  }

  void remove(int pos) {
    std::vector<Minion>::erase(begin() + pos);
  }

  void remove_all_from(int pos) {
    resize(pos);
  }

  // Iteration

  template <typename F>
  void for_each(F fun) {
    std::for_each(begin(), end(), fun);
  }
  template <typename F>
  void for_each(F fun) const {
    std::for_each(begin(), end(), fun);
  }

  template <typename F>
  void for_each_alive(F fun) {
    for (Minion& m : *this) {
      if (!m.dead()) {
        fun(m);
      }
    }
  }
  template <typename F>
  void for_each_alive(F fun) const {
    for (Minion const& m : *this) {
      if (!m.dead()) {
        fun(m);
      }
    }
  }

  template <typename F>
  void for_each_with_pos(F fun) {
    for (size_t i=0; i<size(); ++i) {
      fun((int)i, (*this)[i]);
    }
  }
  template <typename F>
  void for_each_with_pos(F fun) const {
    for (size_t i=0; i<size(); ++i) {
      fun((int)i, (*this)[i]);
    }
  }

  template <typename F>
  int count_if(F fun) const {
    int count = 0;
    for_each([&count,fun](Minion const& m) { if (fun(m)) count++; });
    return count;
  }
};


#endif

// -----------------------------------------------------------------------------
// Output
// -----------------------------------------------------------------------------

template <int N>
inline std::ostream& operator << (std::ostream& s, MinionArray<N> const& b) {
  b.for_each([&s](Minion const& m) {
    s << "* " << m << std::endl;
  });
  return s;
}

