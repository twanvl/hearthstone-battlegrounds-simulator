#include "board_parser.hpp"
#include "simulation.hpp"
#include <iomanip>
#include <chrono>
using namespace std;

// -----------------------------------------------------------------------------
// Pair off a bunch of boards
// -----------------------------------------------------------------------------

void tournament_benchmark(Boards const& boards) {
  using namespace std::chrono;
  int n = (int)boards.size();
  int runs = 5000;
  vector<double> wr;
  auto start = high_resolution_clock::now();
  for (int i=0; i<n; ++i) {
    double w = 0;
    for (int j=0; j<n; ++j) {
      auto stats = simulate(boards[i].board, boards[j].board, runs);
      w += stats.win_rate(0);
    }
    wr.push_back(w);
  }
  auto end = high_resolution_clock::now();
  duration<double> t = end-start;
  cout << "Time: " << setprecision(5) << t.count();
  cout << "    (";
  for (auto w : wr) {
    cout << " " << w;
  }
  cout << ")" << endl;
}

// -----------------------------------------------------------------------------
// Main function
// -----------------------------------------------------------------------------

int main(int argc, char const** argv) {
  Boards boards;
  if (!load_boards("examples/benchmark-boards.txt", boards)) return 1;
  for (int rep=0; rep<3; ++rep) {
    tournament_benchmark(boards);
  }
}
