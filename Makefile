GXX = g++
GXX_FLAGS = -Wall -Wextra -Wno-unused-parameter -pedantic -std=c++17 -O2 -flto
EMCC = emcc
EMCC_FLAGS = $(GXX_FLAGS) --bind -s FILESYSTEM=0

LIB_SOURCES = $(addprefix src/, enum_data.cpp minion_events.cpp hero_powers.cpp battle.cpp random.cpp)
SOURCES = $(LIB_SOURCES) src/repl.cpp

OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all web clean
all: hsbg

# Compiling

%.o: %.cpp src/*.hpp
	$(GXX) $(GXX_FLAGS) $< -c -o $@

hsbg: $(OBJECTS)
	$(GXX) $(GXX_FLAGS) $^ -o $@

debug: $(SOURCES)
	$(GXX) -Wall -Wextra -Wno-unused-parameter -pedantic -std=c++11 -g $^ -o hsbg

# Compiling for web

web: web/hsbg.js

web/hsbg.js: $(SOURCES)
	$(EMCC) $(EMCC_FLAGS) $(SOURCES) -o $@

# Generate enum data

hsdata/CardDefs.xml:
	git submodule init
	git submodule update

src/enums.hpp: src/enum_data.cpp
src/enum_data.cpp: scripts/generate_enum_data.py hsdata/CardDefs.xml
	python $<

# Generate board data

scripts/generate_board_data: src/generate_board_data.o src/enums.hpp src/board.hpp
	$(GXX) $(GXX_FLAGS) $(LIB_SOURCES:.cpp=.o) src/generate_board_data.o -o $@

BOARD_DATA = $(wildcard data/board*.txt)

.DELETE_ON_ERROR: src/board_data.cpp
src/board_data.cpp: scripts/generate_board_data $(BOARD_DATA)
	scripts/generate_board_data $(BOARD_DATA) > $@

# log parser

log_parser: $(LIB_SOURCES:.cpp=.o) src/log_parser.o
	$(GXX) $(GXX_FLAGS) $^ -o $@

# tournament

tournament: $(LIB_SOURCES:.cpp=.o) src/tournament.o
	$(GXX) $(GXX_FLAGS) $^ -o $@

benchmark: $(LIB_SOURCES:.cpp=.o) src/benchmark.o
	$(GXX) $(GXX_FLAGS) $^ -o $@

benchmark-profile: $(LIB_SOURCES) src/benchmark.cpp
	$(GXX) $(GXX_FLAGS) -g -pg $^ -o $@

variance-benchmark: $(LIB_SOURCES:.cpp=.o) src/variance_benchmark.o
	$(GXX) $(GXX_FLAGS) $^ -o $@

# Cleanup

clean:
	rm -rf src/*.o
	rm -rf web/hsbg.js
	rm -rf web/*.wasm

distclean: clean
	rm -rf src/enum_data.cpp src/enums.hpp src/board_data.cpp

