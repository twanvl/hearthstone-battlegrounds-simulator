GXX = g++
GXX_FLAGS = -Wall -std=c++11
EMCC = emcc
EMCC_FLAGS = $(GXX_FLAGS) --bind -O2 -s FILESYSTEM=0
LIB_SOURCES = minion_info.cpp minion_events.cpp hero_powers.cpp battle.cpp xoroshiro128plus.cpp
SOURCES = $(LIB_SOURCES) repl.cpp

.PHONY: all web clean
all: hsbg
web: ../web/hsbg.js

hsbg: $(SOURCES) *.hpp
	$(GXX) $(GXX_FLAGS) $(SOURCES) -O -o $@

debug: $(SOURCES) *.hpp
	$(GXX) $(GXX_FLAGS) $(SOURCES) -g -o hsbg

test: $(SOURCES) *.hpp
	$(GXX) $(GXX_FLAGS) $(LIB_SOURCES) test.cpp -g -o $@

../web/hsbg.js: $(SOURCES) *.hpp
	$(EMCC) $(EMCC_FLAGS) $(SOURCES) -o $@


.SECONDARY: generate_board_data
generate_board_data: minion_types.hpp hero_powers.hpp board.hpp
	$(GXX) $(GXX_FLAGS) $(LIB_SOURCES) generate_board_data.cpp -O -o $@

BOARD_DATA = ../data/board*.txt

.DELETE_ON_ERROR: board_data.cpp
board_data.cpp: generate_board_data $(BOARD_DATA)
	./generate_board_data $(BOARD_DATA) > $@
