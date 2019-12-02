#pragma once
#include "board.hpp"
#include "parser.hpp"
#include <vector>
#include <algorithm>
#include <fstream>
using std::istream;

// -----------------------------------------------------------------------------
// Board parser
// -----------------------------------------------------------------------------

struct BoardWithLabel {
  Board board;
  int turn = -1;
  std::string label;
};
using Boards = std::vector<BoardWithLabel>;

bool operator < (BoardWithLabel const& a, BoardWithLabel const& b) {
  return a.turn < b.turn || (a.turn == b.turn && a.board.total_stats() < b.board.total_stats());
}

// -----------------------------------------------------------------------------
// Board parser
// -----------------------------------------------------------------------------

bool parse_board_definition(StringParser& in, Board& board) {
  if (in.match_end()) {
    // empty line or comment
  } else if (in.match("*")) {
    // define minion
    Minion m;
    if (parse_minion(in,m) && in.parse_end()) {
      board.append(m);
    }
  } else if (in.match("board")) {
  } else if (in.match("hp") || in.match("hero-power")) {
    in.match(":"); // optional
    HeroType hero;
    if (parse_hero_type(in, hero) && in.parse_end()) {
      board.hero = hero;
      board.use_hero_power = true;
    }
  } else if (in.match("level")) {
    in.match(":"); // optional
    int n = 0;
    if (in.parse_non_negative(n) && in.parse_end()) {
      board.level = n;
    }
  } else if (in.match("health")) {
    in.match(":"); // optional
    int n = 0;
    if (in.parse_non_negative(n) && in.parse_end()) {
      board.health = n;
    }
  } else {
    return false;
  }
  return true;
}

bool parse_board_with_label(StringParser& in, BoardWithLabel& board) {
  if (in.match("turn")) {
    in.match(":"); // optional
    int n = 0;
    if (in.parse_non_negative(n)) {
      in.skip_ws();
      board.label = in.str;
      board.turn = n;
    }
    return true;
  }
  return parse_board_definition(in, board.board);
}

void load_boards(istream& lines, const char* filename, Boards& boards) {
  ErrorHandler error(std::cerr, filename);

  BoardWithLabel board;
  
  while (lines.good()) {
    // get line
    error.line_number++;
    std::string line;
    getline(lines,line);
    StringParser in(line.c_str(), error);

    // parse line
    if (parse_board_with_label(in, board)) {
      continue;
    } else if (in.peek() == '=') {
      if (board.turn > 0) {
        boards.push_back(board);
      }
      board = BoardWithLabel();
      // board separator
    } else {
      in.unknown("command");
    }
  }
}

bool load_boards(int argc, char const** argv, Boards& boards) {
  for (int i=1; i<argc; ++i) {
    std::ifstream in(argv[i]);
    if (!in) {
      std::cerr << "Error loading file " << argv[i] << endl;
      return false;
    }
    load_boards(in, argv[i], boards);
  }
  std::sort(boards.begin(), boards.end());
  return true;
}

