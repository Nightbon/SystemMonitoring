.PHONY: all clean

CXX = g++
CXXFLAGS = -std=c++20 -Wall -Wextra -fPIC -I../
LDFLAGS = -shared

BUILD_DIR = ../../build/agents

AGENTS = $(BUILD_DIR)/cpu_agent.so $(BUILD_DIR)/memory_agent.so $(BUILD_DIR)/network_agent.so $(BUILD_DIR)/additional_agent.so

all: $(AGENTS)

$(BUILD_DIR)/%.so: %.cc 
	mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $< -o $@
	cp $*.conf $(BUILD_DIR)/

clean:
	rm -rf ../../build/agents