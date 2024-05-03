# general
CXX = g++-13
CXXFLAGS = -std=c++23 -Wall -fopenmp

SRC_DIR = src
BUILD_DIR = .build
OBJS_DIR = $(BUILD_DIR)/objs
DEPS_DIR = $(BUILD_DIR)/deps

EXE = $(BUILD_DIR)/exe

SRCS = $(wildcard $(SRC_DIR)/*.cpp) $(wildcard $(SRC_DIR)/utils/*.cpp)
OBJS = $(patsubst $(SRC_DIR)/%.cpp, $(OBJS_DIR)/%.o, $(SRCS))
DEPS = $(patsubst $(SRC_DIR)/%.cpp, $(DEPS_DIR)/%.d, $(SRCS))

PRE_DIRS = $(patsubst %/, %, $(sort $(dir $(OBJS) $(DEPS))))

# flags
DEBUG ?= 0
ifeq ($(DEBUG), 1)
	CXXFLAGS += -g -O0
else
	CXXFLAGS += -DNDEBUG -O3 -march=native
endif

# targets
all: $(EXE)

-include $(DEPS)

$(EXE): $(OBJS) | $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJS_DIR)/%.o: $(SRC_DIR)/%.cpp | $(PRE_DIRS)
	$(CXX) $(CXXFLAGS) -MD -MP -MF $(DEPS_DIR)/$*.d -c $< -o $@

$(BUILD_DIR) $(PRE_DIRS): ; mkdir -p $@

clean:
	rm -rf $(BUILD_DIR)

.PHONY: all clean
