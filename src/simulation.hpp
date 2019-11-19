#include "battle.hpp"
#include <vector>
#include <array>
#include <algorithm>
using std::vector;

// -----------------------------------------------------------------------------
// Simulation
// -----------------------------------------------------------------------------

const int DEFAULT_NUM_RUNS = 1000;

inline int simulate_single(Battle const& battle) {
  Battle copy(battle);
  copy.verbose = 0;
  copy.run();
  return copy.score();
}

vector<int> simulate(Battle const& b, int n = DEFAULT_NUM_RUNS) {
  vector<int> results;
  results.reserve(n);
  for (int i=0; i<n; ++i) {
    results.push_back(simulate_single(b));
  }
  std::sort(results.begin(), results.end());
  return results;
}

struct ScoreSummary {
  int total_score = 0;
  int num_wins = 0;
  int num_draws = 0;
  int num_losses = 0;

  ScoreSummary() {}
  ScoreSummary(vector<int> const& scores) {
    add_scores(scores);
  }

  int num_runs() const { return num_wins + num_draws + num_losses; }
  double win_rate() const { return (double)num_wins / num_runs(); }
  double draw_rate() const { return (double)num_draws / num_runs(); }
  double loss_rate() const { return (double)num_losses / num_runs(); }
  double mean_score() const { return (double)total_score / num_runs(); }
  // for optimization use this score:
  inline double optimization_score() {
    return win_rate() + draw_rate() * 0.5;
  }

  void add_score(int score) {
    total_score += score;
    if (score > 0)       num_wins++;
    else if (score == 0) num_draws++;
    else                 num_losses++;
  }
  void add_scores(vector<int> const& scores) {
    for (auto x : scores) add_score(x);
  }
};

ScoreSummary simulate_summary(Battle const& battle, int runs = DEFAULT_NUM_RUNS) {
  ScoreSummary results;
  for (int i=0; i<runs; ++i) {
    results.add_score(simulate_single(battle));
  }
  return results;
}

double simulate_optimization_score(Battle const& battle, int runs = DEFAULT_NUM_RUNS) {
  return simulate_summary(battle,runs).optimization_score();
}

// -----------------------------------------------------------------------------
// Statistics
// -----------------------------------------------------------------------------

double mean(vector<int> const& xs) {
  // work around emscripten bug (missing accumulate function)
  //return std::accumulate(xs.begin(), xs.end(), 0.) / xs.size();
  double sum = 0.;
  for(int x : xs) sum += x;
  return sum / xs.size();
}

double mean_damage_taken(vector<int> const& xs, int enemy_level, int sign = 1) {
  double sum = 0.;
  for(int x : xs) if (sign*x < 0) sum += enemy_level - sign*x;
  return sum / xs.size();
}

double mean_damage_dealt(vector<int> const& xs, int level) {
  return mean_damage_taken(xs,level,-1);
}

double death_rate(vector<int> const& results, int enemy_level, int health, int sign = 1) {
  int deaths = 0;
  for(int x : results) if (sign*x < 0 && (enemy_level - sign*x) >= health) deaths++;
  return (double)deaths / results.size();
}

int percentile(int i, vector<int> const& results) {
  auto bounds = equal_range(results.begin(), results.end(), i);
  int a = static_cast<int>(bounds.first - results.begin());
  int b = static_cast<int>(bounds.second - results.begin());
  return 100 * (a + b) / 2 / (results.size() - 1);
}

// -----------------------------------------------------------------------------
// Optimization
// -----------------------------------------------------------------------------

void permute_minions(Board& board, Minion const original[], int const perm[], int n) {
  // note: original != board.minions
  for (int i=0; i<n; ++i) {
    board.minions[i] = original[perm[i]];
  }
}

struct OptimizeMinionOrder {
  std::array<int,BOARDSIZE> best_order;
  double current_score;
  double best_score;
  int n;
  
  OptimizeMinionOrder(Board const& board, Board const& enemy, int runs = DEFAULT_NUM_RUNS) {
    n = board.size();
    // current situation
    std::array<int,BOARDSIZE> order;
    for (int i=0; i<n; ++i) order[i] = i;
    // optimize
    bool current = true;
    do {
      // check
      Battle battle(board, enemy);
      permute_minions(battle.board[0], board.minions, order.data(), n);
      double score = simulate_optimization_score(battle, runs);
      if (current) { // first permutation = current situation
        current = false;
        current_score = score;
        best_score = score;
        best_order = order;
      } else if (score > best_score) {
        best_score = score;
        best_order = order;
      }
    } while (std::next_permutation(order.begin(), order.begin() + n));
  }
};

