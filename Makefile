# Compiler
CXX = g++
CXXFLAGS = -Wall -Wextra -std=c++17 -O2

# Directories
SRC_DIR = src
INCLUDE_DIR = include
BUILD_DIR = build
BIN_DIR = bin

# Executable name
TARGET = cph

# Source and object files
SRCS = $(wildcard $(SRC_DIR)/*.cpp $(SRC_DIR)/commands/*.cpp $(SRC_DIR)/utils/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(BUILD_DIR)/%.o, $(SRCS))

# Default rule: Build the CLI tool
all: $(TARGET)

# Link the final executable
$(TARGET): $(OBJS)
	@mkdir -p $(BIN_DIR)
	$(CXX) $(CXXFLAGS) -o $(BIN_DIR)/$(TARGET) $(OBJS)

# Compile source files into object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(BUILD_DIR)/commands $(BUILD_DIR)/utils
	$(CXX) $(CXXFLAGS) -I$(INCLUDE_DIR) -c $< -o $@

# Install command (copies binary to /usr/local/bin)
install: all
	@sudo cp $(BIN_DIR)/$(TARGET) /usr/local/bin/$(TARGET)
	@sudo chmod +x /usr/local/bin/$(TARGET)
	@echo "Installed $(TARGET) to /usr/local/bin"

# Uninstall command (removes binary)
uninstall:
	@sudo rm -f /usr/local/bin/$(TARGET)
	@echo "Uninstalled $(TARGET)"

# Clean all compiled files
clean:
	@rm -rf $(BUILD_DIR) $(BIN_DIR)

# Debugging version (without optimizations)
debug: CXXFLAGS = -Wall -Wextra -std=c++17 -g
debug: clean all

.PHONY: all clean install uninstall debug

