CXX      := g++
CXXFLAGS := -std=c++17 -O2 -Wall -Wextra -Iinclude
TARGET   := tpcli
SRC      := src/tpcli.cpp

all: $(TARGET)

$(TARGET): $(SRC) $(wildcard include/*.h)
	$(CXX) $(CXXFLAGS) -o $@ $(SRC)

clean:
	rm -f $(TARGET)

.PHONY: all clean