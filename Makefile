# VirtMonitors Makefile wrapper around CMake

BUILD_DIR ?= build
BUILD_TYPE ?= Release
JOBS ?= $(shell nproc 2>/dev/null || echo 1)

# Default PREFIX:
# If PREFIX is explicitly provided (e.g. make install PREFIX=/usr), use it.
# Otherwise, if running as root, default to /usr/local.
# If running as normal user, default to ~/.local (matches ~/.local/bin/virtmonitors).
ifeq ($(origin PREFIX), undefined)
  ifeq ($(shell id -u), 0)
    PREFIX := /usr/local
  else
    PREFIX := $(HOME)/.local
  endif
endif

.PHONY: all build install uninstall test check clean distclean help

all: build

$(BUILD_DIR)/CMakeCache.txt: CMakeLists.txt src/CMakeLists.txt
	@cmake -B $(BUILD_DIR) \
		-DCMAKE_BUILD_TYPE=$(BUILD_TYPE) \
		-DCMAKE_INSTALL_PREFIX=$(PREFIX)

build: $(BUILD_DIR)/CMakeCache.txt
	@cmake --build $(BUILD_DIR) -j$(JOBS)
	@ln -sf $(BUILD_DIR)/src/virtmonitors virtmonitors
	@echo "Build complete. Executable available at: ./virtmonitors and $(BUILD_DIR)/src/virtmonitors"

install: build
	@echo "Installing virtmonitors to $(if $(DESTDIR),$(DESTDIR),)$(PREFIX)/bin..."
	@DESTDIR=$(DESTDIR) cmake --install $(BUILD_DIR) --prefix $(PREFIX)
	@echo "Installation complete: $(if $(DESTDIR),$(DESTDIR),)$(PREFIX)/bin/virtmonitors"

uninstall:
	@echo "Removing $(if $(DESTDIR),$(DESTDIR),)$(PREFIX)/bin/virtmonitors..."
	@rm -f $(if $(DESTDIR),$(DESTDIR),)$(PREFIX)/bin/virtmonitors
	@echo "Uninstall complete."

test check: build
	@ctest --test-dir $(BUILD_DIR) --output-on-failure

clean:
	@if [ -d "$(BUILD_DIR)" ]; then \
		cmake --build $(BUILD_DIR) --target clean 2>/dev/null || true; \
	fi
	@rm -f virtmonitors
	@echo "Cleaned build artifacts."

distclean:
	@rm -rf $(BUILD_DIR) virtmonitors
	@echo "Removed build directory and symlink."

help:
	@echo "VirtMonitors build system"
	@echo ""
	@echo "Targets:"
	@echo "  make              - Configure and build the project (creates ./virtmonitors symlink)"
	@echo "  make install      - Install virtmonitors to PREFIX/bin (default: ~/.local or /usr/local if root)"
	@echo "  make uninstall    - Remove virtmonitors from PREFIX/bin"
	@echo "  make test         - Run regression and integration tests with CTest"
	@echo "  make clean        - Clean built objects"
	@echo "  make distclean    - Remove build directory completely"
	@echo ""
	@echo "Variables:"
	@echo "  PREFIX=<path>     - Target install prefix (current default: $(PREFIX))"
	@echo "  BUILD_TYPE=<type> - CMake build type (Release, Debug, RelWithDebInfo; default: $(BUILD_TYPE))"
	@echo "  JOBS=<num>        - Parallel build jobs (default: $(JOBS))"
