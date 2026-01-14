CXX := g++
CXXFLAGS := -std=c++17 -Wall -Wextra -pedantic -O2

BIN_DIR := bin
TARGET := $(BIN_DIR)/elevator_sim
SRC := elevator_sim.cpp

.PHONY: all clean run

all: $(TARGET)

$(TARGET): $(SRC) | $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

$(BIN_DIR):
	mkdir -p $(BIN_DIR)

run: $(TARGET)
	./$(TARGET)

clean:
	rm -rf $(BIN_DIR) *.o *.out a.out
