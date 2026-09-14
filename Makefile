# Convenience wrapper around the CMake build. CMake stays the build system;
# this only remembers the usual options and targets.
#
#   make            build (Release, ./build)
#   make test       build, then run the unit tests
#   make lint       lint the shipped layouts
#   make install    install to PREFIX (default /usr/local)
#   make clean      remove the build tree
#
# Overridable: BUILD_DIR, BUILD_TYPE, PREFIX, JOBS, CMAKE_FLAGS.

BUILD_DIR   ?= build
BUILD_TYPE  ?= Release
PREFIX      ?= /usr/local
JOBS        ?= $(shell nproc 2>/dev/null || echo 1)
CMAKE       ?= cmake
CMAKE_FLAGS ?= -DCMAKE_BUILD_TYPE=$(BUILD_TYPE) -DCMAKE_INSTALL_PREFIX=$(PREFIX)

.DEFAULT_GOAL := all
.PHONY: all configure build test check lint install clean help

all: build

# cmake -S/-B is cheap and keeps BUILD_TYPE/PREFIX changes in sync.
configure:
	$(CMAKE) -S . -B $(BUILD_DIR) $(CMAKE_FLAGS)

build: configure
	$(CMAKE) --build $(BUILD_DIR) -j$(JOBS)

test check: build
	ctest --test-dir $(BUILD_DIR) --output-on-failure

lint: build
	$(BUILD_DIR)/tabletkeyboard --check-layout data/layouts/us.json
	$(BUILD_DIR)/tabletkeyboard --check-layout data/layouts/jp106.json

install: build
	$(CMAKE) --install $(BUILD_DIR)

clean:
	rm -rf $(BUILD_DIR)

help:
	@echo "targets: all build test lint install clean"
	@echo "vars:    BUILD_DIR=$(BUILD_DIR) BUILD_TYPE=$(BUILD_TYPE) PREFIX=$(PREFIX) JOBS=$(JOBS)"
