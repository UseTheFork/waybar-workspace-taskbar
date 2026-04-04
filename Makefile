BUILD_DIR := build
LOCAL_DIR := local

.PHONY: all setup build clean rebuild

all: build

setup:
	meson setup $(BUILD_DIR)

build: $(BUILD_DIR)/build.ninja
	ninja -C $(BUILD_DIR)

$(BUILD_DIR)/build.ninja:
	meson setup $(BUILD_DIR)

clean:
	ninja -C $(BUILD_DIR) clean

rebuild:
	rm -rf $(BUILD_DIR)
	meson setup $(BUILD_DIR)
	ninja -C $(BUILD_DIR)

run:
	waybar -c $(LOCAL_DIR)/config.jsonc -s $(LOCAL_DIR)/style.css

run_interactive:
	GTK_DEBUG=interactive waybar -c $(LOCAL_DIR)/config.jsonc -s $(LOCAL_DIR)/style.css
