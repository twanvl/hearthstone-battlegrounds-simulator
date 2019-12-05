#include "board_parser.hpp"
#include "simulation.hpp"
#include <iomanip>
using namespace std;

// -----------------------------------------------------------------------------
// Pair off a bunch of boards
// -----------------------------------------------------------------------------

void tournament(Boards const& boards) {
  cerr << boards.size() << " boards" << endl;
  int n = (int)boards.size();
  // square array of stats
  vector<ScoreSummary> the_stats;
  the_stats.resize(n*n);
  auto stats = [&](int i, int j) -> ScoreSummary& {
    return the_stats[i*n+j];
  };
  for (int i=0; i<n; ++i) {
    cout << "turn " << boards[i].turn;
    cout << "\t" << boards[i].turn;
    cout << "\t" << boards[i].board.total_stats();
    for (int j=0; j<i; ++j) {
      cout << "\t" << setprecision(3) << stats(i,j).damage_score();
    }
    for (int j=i; j<n; ++j) {
      ScoreSummary result = simulate(boards[i].board, boards[j].board);
      cout << "\t" << setprecision(3) << result.damage_score();
      stats(i,j) = result;
      stats(j,i) = result.flipped();
    }
    cout << endl;
  }
  // damage taken
  {
    ofstream fout("damage_taken.csv");
    for (int i=0; i<n; ++i) {
      for (int j=0; j<n; ++j) {
        if (j>0) fout << "\t";
        fout << stats(i,j).mean_damage_taken(0);
      }
      fout << endl;
    }
  }
  // output
  {
    ofstream fout("winrates.csv");
    for (int i=0; i<n; ++i) {
      for (int j=0; j<n; ++j) {
        if (j>0) fout << "\t";
        fout << stats(i,j).win_rate(0);
      }
      fout << endl;
    }
  }
}

// -----------------------------------------------------------------------------
// Main function
// -----------------------------------------------------------------------------

int main(int argc, char const** argv) {
  if (argc <= 1) {
    cout << "Usage: " << argv[0] << " <board files>" << endl;
  } else {
    Boards boards;
    if (!load_boards(argc, argv, boards)) return 1;
    tournament(boards);
  }
  return 0;
}
