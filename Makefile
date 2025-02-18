# Copyright (c) 2018, Sergiy Yevtushenko
# Copyright (c) 2018-2019, Niklas Hauser
#
# This file is part of the modm project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.

BUILD_DIR = ../build/UvRgbConcentrator

CMAKE_GENERATOR = Unix Makefiles
CMAKE_FLAGS = -DCMAKE_EXPORT_COMPILE_COMMANDS:BOOL=ON -DCMAKE_RULE_MESSAGES:BOOL=ON -DCMAKE_VERBOSE_MAKEFILE:BOOL=OFF
.PHONY: build-release build-debug cmake clean cleanall upload-debug upload-release gdb gdb-release openocd killopenocd

.DEFAULT_GOAL := all

### Targets

all: cmake build-release

cmake:
	@cmake -E make_directory $(BUILD_DIR)/cmake-build-debug
	@cmake -E make_directory $(BUILD_DIR)/cmake-build-release
	@cd $(BUILD_DIR)/cmake-build-debug && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Debug -G "$(CMAKE_GENERATOR)" /mnt/d/Develop/Repo/Modm/Projects/UvRgbConcentrator
	@cd $(BUILD_DIR)/cmake-build-release && cmake $(CMAKE_FLAGS) -DCMAKE_BUILD_TYPE=Release -G "$(CMAKE_GENERATOR)" /mnt/d/Develop/Repo/Modm/Projects/UvRgbConcentrator

build-release:
	@cmake --build $(BUILD_DIR)/cmake-build-release

build-debug:
	@cmake --build $(BUILD_DIR)/cmake-build-debug

clean:
	@cmake --build $(BUILD_DIR)/cmake-build-release --target clean
	@cmake --build $(BUILD_DIR)/cmake-build-debug --target clean

cleanall:
	@rm -rf $(BUILD_DIR)/cmake-build-release
	@rm -rf $(BUILD_DIR)/cmake-build-debug

upload-debug: build-debug
	@openocd -f modm/openocd.cfg -c "modm_program $(BUILD_DIR)/cmake-build-release/UvRgbConcentrator.elf"

upload-release: build-release
	@openocd -f modm/openocd.cfg -c "modm_program $(BUILD_DIR)/cmake-build-release/UvRgbConcentrator.elf"

gdb: build-debug
	@arm-none-eabi-gdb -x modm/gdbinit -x modm/openocd_gdbinit $(BUILD_DIR)/cmake-build-debug/UvRgbConcentrator.elf
	@killall openocd || true

gdb-release: build-release
	@arm-none-eabi-gdb -x modm/gdbinit -x modm/openocd_gdbinit $(BUILD_DIR)/cmake-build-release/UvRgbConcentrator.elf
	@killall openocd || true

openocd:
	@openocd -f modm/openocd.cfg

killopenocd:
	@killall openocd || true
