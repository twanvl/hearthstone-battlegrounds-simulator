#include "board_parser.hpp"
#include "simulation.hpp"
#include <iomanip>
#include <chrono>
using namespace std;

// -----------------------------------------------------------------------------
// Pair off a bunch of boards
// -----------------------------------------------------------------------------

void rng_variance_test(Boards const& boards) {
  using namespace std::chrono;
  int runs = 1000;
  int reps = 10;
  int n = (int)boards.size();
  auto start = high_resolution_clock::now();
  for (int i=0; i<n; ++i) {
    for (int j=0; j<n; ++j) {
      if (abs(boards[i].board.total_stats() - boards[j].board.total_stats()) > 11) continue;
      vector<double> winrates;
      cout << i << " vs " << j << " ";
      cout << "(" << boards[i].board.total_stats() << " vs " << boards[j].board.total_stats() << ") ";
      for (int rep=0; rep<reps; ++rep) {
        auto stats = simulate(boards[i].board, boards[j].board, runs);
        winrates.push_back(stats.win_rate(0));
      }
      double m = mean(winrates);
      double v = variance(winrates);
      cout << " m:" << setprecision(5) << m << " v:" << v << endl;
    }
  }
  auto end = high_resolution_clock::now();
  duration<double> t = end-start;
  cout << "Time: " << setprecision(5) << t.count() << endl;
}

// -----------------------------------------------------------------------------
// Main function
// -----------------------------------------------------------------------------

int main(int argc, char const** argv) {
  Boards boards;
  if (!load_boards("examples/variance-benchmark-boards.txt", boards)) return 1;
  rng_variance_test(boards);
}
