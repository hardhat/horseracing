ROOT ?= ../..
CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -g -I$(ROOT)/src
BIN_DIR := bin
RACE_SIM_BIN := $(BIN_DIR)/test_race_sim
WAGER_MATH_BIN := $(BIN_DIR)/test_wager_math
SESSION_LOGIC_BIN := $(BIN_DIR)/test_session_logic
RACE_SIM_OBJ := $(BIN_DIR)/race_sim.o $(BIN_DIR)/test_race_sim.o
WAGER_MATH_OBJ := $(BIN_DIR)/wager_math.o $(BIN_DIR)/test_wager_math.o
SESSION_LOGIC_OBJ := $(BIN_DIR)/wager_math.o $(BIN_DIR)/session_logic.o $(BIN_DIR)/test_session_logic.o

.PHONY: all run clean

all: run

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/race_sim.o: $(ROOT)/src/race_sim.c $(ROOT)/src/race_sim.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/test_race_sim.o: test_race_sim.c $(ROOT)/src/race_sim.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/wager_math.o: $(ROOT)/src/wager_math.c $(ROOT)/src/wager_math.h $(ROOT)/src/game.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/test_wager_math.o: test_wager_math.c $(ROOT)/src/wager_math.h $(ROOT)/src/game.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/session_logic.o: $(ROOT)/src/session_logic.c $(ROOT)/src/session_logic.h $(ROOT)/src/wager_math.h $(ROOT)/src/game.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/test_session_logic.o: test_session_logic.c $(ROOT)/src/session_logic.h $(ROOT)/src/wager_math.h $(ROOT)/src/game.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(RACE_SIM_BIN): $(RACE_SIM_OBJ)
	$(CC) -o $@ $(RACE_SIM_OBJ)

$(WAGER_MATH_BIN): $(WAGER_MATH_OBJ)
	$(CC) -o $@ $(WAGER_MATH_OBJ)

$(SESSION_LOGIC_BIN): $(SESSION_LOGIC_OBJ)
	$(CC) -o $@ $(SESSION_LOGIC_OBJ)

run: $(RACE_SIM_BIN) $(WAGER_MATH_BIN) $(SESSION_LOGIC_BIN)
	./$(RACE_SIM_BIN)
	./$(WAGER_MATH_BIN)
	./$(SESSION_LOGIC_BIN)

clean:
	-rm -f $(BIN_DIR)/*.o $(RACE_SIM_BIN) $(WAGER_MATH_BIN) $(SESSION_LOGIC_BIN)