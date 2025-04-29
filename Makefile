MIK32_HAL_DIR=hardware/mik32-hal
MIK32_SHARED_DIR=hardware/mik32v2-shared

BUILD_DIR=build

SERIAL_PORT?=/dev/ttyUSB0
SERIAL_BOUDRATE?=115200

ROM_SECTIONS?=

.PHONY: clean flash monitor size_analyze_html size_analyze_rom size_analyze_ram size_analyze_sections

build_app: update_submodules $(BUILD_DIR)
	cmake --build $(BUILD_DIR)

update_submodules: $(MIK32_HAL_DIR)/README.md $(MIK32_SHARED_DIR)/README.md

$(MIK32_HAL_DIR)/README.md:
	git submodule update --init hardware/mik32-hal

$(MIK32_SHARED_DIR)/README.md:
	git submodule update --init hardware/mik32v2-shared

$(BUILD_DIR):
	cmake -G Ninja -B $(BUILD_DIR)

tests: update_submodules
	ceedling test:all

clean:
	rm -rf $(BUILD_DIR)

flash:
	python3 $(MIK32_UPLOADER_DIR)/mik32_upload.py build/app/base_project.hex --run-openocd \
	--openocd-exec /usr/bin/openocd \
	--openocd-target $(MIK32_UPLOADER_DIR)/openocd-scripts/target/mik32.cfg \
	--openocd-interface $(MIK32_UPLOADER_DIR)/openocd-scripts/interface/ftdi/mikron-link.cfg \
	--adapter-speed 500 --mcu-type MIK32V2

monitor:
	picocom $(SERIAL_PORT) -b $(SERIAL_BOUDRATE) --omap crcrlf --echo

size_analyze_html: build_app
	elf-size-analyze -t $(MIK32_TOOLCHAIN_DIR)/riscv-none-elf- -Ha --rom -W $(BUILD_DIR)/app/base_project.elf > size-analyze-rom.html
	elf-size-analyze -t $(MIK32_TOOLCHAIN_DIR)/riscv-none-elf- -Ha --ram -W $(BUILD_DIR)/app/base_project.elf > size-analyze-ram.html

size_analyze_rom: build_app
	elf-size-analyze -t $(MIK32_TOOLCHAIN_DIR)/riscv-none-elf- -w 120 -Ha --rom $(BUILD_DIR)/app/base_project.elf

size_analyze_ram: build_app
	elf-size-analyze -t $(MIK32_TOOLCHAIN_DIR)/riscv-none-elf- -w 120 -Ha --ram $(BUILD_DIR)/app/base_project.elf

size_analyze_sections: build_app
	elf-size-analyze -t $(MIK32_TOOLCHAIN_DIR)/riscv-none-elf- -w 120 -Ha -P $(BUILD_DIR)/app/base_project.elf