all: hsbg
web: ../web/hsbg.js

GXX = g++
GXX_FLAGS = -Wall -std=c++11
EMCC = emcc
EMCC_FLAGS = $(GXX_FLAGS) --bind -O2 -s FILESYSTEM=0
LIB_SOURCES = minion_info.cpp minion_events.cpp hero_powers.cpp battle.cpp
SOURCES = $(LIB_SOURCES) repl.cpp

hsbg: $(SOURCES) *.hpp
	$(GXX) $(GXX_FLAGS) $(SOURCES) -O -o $@

test: $(SOURCES) *.hpp
	$(GXX) $(GXX_FLAGS) $(LIB_SOURCES) test.cpp -g -o $@

../web/hsbg.js: $(SOURCES) *.hpp
	$(EMCC) $(EMCC_FLAGS) $(SOURCES) -o $@

