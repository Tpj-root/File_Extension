CXX = g++
CXXFLAGS = -std=c++17 -O2 -Wall -Wextra

TARGET = tpcodec
SOURCE = tpcodec.cpp

CSV = frxXAUUSD_1790922600_2026-10-02.csv
TP  = frxXAUUSD_1790922600_2026-10-02.tp

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SOURCE)

build: $(TARGET)

convert: $(TARGET)
	./$(TARGET) $(CSV) $(TP)

read: $(TARGET)
	./$(TARGET) -r $(TP)

clean:
	rm -f $(TARGET)

rebuild: clean all

.PHONY: all build convert read clean rebuild