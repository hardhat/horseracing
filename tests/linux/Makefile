ROOT ?= ../..
CC ?= gcc
CFLAGS ?= -std=c11 -Wall -Wextra -pedantic -g -I$(ROOT)/src
BIN_DIR := bin
BIN := $(BIN_DIR)/test_race_sim
OBJ := $(BIN_DIR)/race_sim.o $(BIN_DIR)/test_race_sim.o

.PHONY: all run clean

all: run

$(BIN_DIR):
	@mkdir -p $(BIN_DIR)

$(BIN_DIR)/race_sim.o: $(ROOT)/src/race_sim.c $(ROOT)/src/race_sim.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN_DIR)/test_race_sim.o: test_race_sim.c $(ROOT)/src/race_sim.h | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BIN): $(OBJ)
	$(CC) -o $@ $(OBJ)

run: $(BIN)
	./$(BIN)

clean:
	-rm -f $(BIN_DIR)/*.o $(BIN)