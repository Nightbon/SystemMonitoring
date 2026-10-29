.PHONY: all clean dist style test gcov_report check valgrind cppcheck install uninstall dvi agents

BUILD_DIR = ../build
PREFIX ?= ../build
INSTALL_DIR = $(PREFIX)/SystemMonitor

agents:
	$(MAKE) -C ./agents all

all: build

build:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DBUILD_TESTING=OFF -DBUILD_DVI=OFF -DRelease=ON && make

test:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DBUILD_TESTING=ON -DBUILD_DVI=OFF -DRelease=OFF && make && ./run_tests --gtest_color=yes

gcov_report: clean
	mkdir -p $(BUILD_DIR) && mkdir -p $(BUILD_DIR)/report && cd $(BUILD_DIR) && cmake .. -DBUILD_TESTING=ON -DBUILD_DVI=OFF -DRelease=OFF -DCMAKE_CXX_FLAGS="--coverage" && make && ./run_tests 
	gcovr -r . \
	--html --html-details -o $(BUILD_DIR)/report/index.html \
	--filter "model/.*\.(cc|h)" \
	--exclude "model/agent_config.h" \
	--print-summary \
	--exclude-throw-branches --exclude-unreachable-branches \
	$(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)
	rm -rf ../dist

install: all
	@if [ ! -d $(INSTALL_DIR) ]; then mkdir -p $(INSTALL_DIR); fi
	install -d $(INSTALL_DIR)
	install -m 755 $(BUILD_DIR)/SystemMonitor $(INSTALL_DIR)
	@if [ -d ./agents ]; then cp -r ./agents $(INSTALL_DIR)/; fi
	@echo "Installed to $(INSTALL_DIR)"

uninstall:
	rm -f $(INSTALL_DIR)/SystemMonitor
	@echo "Uninstalled from $(INSTALL_DIR)"

dvi:
	mkdir -p $(BUILD_DIR) && cd $(BUILD_DIR) && cmake .. -DBUILD_DVI=ON -DRelease=OFF -DBUILD_TESTING=OFF && make doxygen
	@echo "Documentation ready at $(BUILD_DIR)/docs/html/index.html"

dist:
	mkdir -p ../dist
	cp -r ../src ../CMakeLists.txt ../dist/
	cd ../dist && tar -czf ../dist/SystemMonitor.tar.gz src CMakeLists.txt
	rm -rf ../dist/src ../dist/CMakeLists.txt

style:
	find ../src -name "*.cc" -o -name "*.h" | xargs clang-format -n --style=Google

cppcheck:
	cppcheck --library=qt --enable=all --suppress=missingIncludeSystem --suppress=missingInclude ../src/model ../src/controller ../src/view ../src/main.cc ../src/agents

valgrind: test
	cd $(BUILD_DIR) && valgrind --leak-check=full ./run_tests

check: clean gcov_report style cppcheck valgrind
	@echo "All checks passed!"
