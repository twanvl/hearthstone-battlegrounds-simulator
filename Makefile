GXX = g++
GXX_FLAGS = -Wall -Wextra -Wno-unused-parameter -pedantic -std=c++11 -O2
EMCC = emcc
EMCC_FLAGS = $(GXX_FLAGS) --bind -s FILESYSTEM=0

LIB_SOURCES = $(addprefix src/, enum_data.cpp minion_events.cpp hero_powers.cpp battle.cpp xoroshiro128plus.cpp)
SOURCES = $(LIB_SOURCES) src/repl.cpp

OBJECTS = $(SOURCES:.cpp=.o)

.PHONY: all web clean
all: hsbg

# Compiling

%.o: %.cpp src/*.hpp
	$(GXX) $(GXX_FLAGS) $< -c -o $@

hsbg: $(OBJECTS)
	$(GXX) $(GXX_FLAGS) $^ -o $@

# Compiling for web

web: web/hsbg.js

web/hsbg.js: $(SOURCES)
	$(EMCC) $(EMCC_FLAGS) $(SOURCES) -o $@

# Generate enum data

hsdata/CardDefs.xml:
	git submodule update

src/enums.hpp: src/enum_data.cpp
src/enum_data.cpp: scripts/generate_enum_data.py hsdata/CardDefs.xml
	python $<

# Generate board data

.SECONDARY: scripts/generate_board_data
scripts/generate_board_data: src/enums.hpp src/board.hpp
	$(GXX) $(GXX_FLAGS) $(LIB_SOURCES:.cpp=.o) src/generate_board_data.o -o $@

BOARD_DATA = data/board*.txt

.DELETE_ON_ERROR: src/board_data.cpp
src/board_data.cpp: scripts/generate_board_data $(BOARD_DATA)
	scripts/generate_board_data $(BOARD_DATA) > $@

log_parser: src/log_parser.o *.hpp
	$(GXX) $(GXX_FLAGS) $(LIB_SOURCES) log_parser.cpp -O -o $@

# Cleanup

clean:
	rm -rf src/*.o
	rm -rf web/hsbg.js
	rm -rf web/*.wasm

