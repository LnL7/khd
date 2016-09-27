FRAMEWORKS     = -framework Carbon -framework Cocoa
BUILD_PATH     = ./bin
BUILD_FLAGS    = -Wall -g
KHD_SRC        = ./src/khd.cpp ./src/hotkey.cpp ./src/parse.cpp\
                 ./src/tokenize.cpp ./src/locale.cpp ./src/daemon.cpp\
                 ./src/sharedworkspace.mm
BINS           = $(BUILD_PATH)/khd

all: $(BINS)

.PHONY: all clean install

install: BUILD_FLAGS=-O2
install: clean $(BINS)

$(BINS): | $(BUILD_PATH)

$(BUILD_PATH):
	mkdir -p $(BUILD_PATH)

clean:
	rm -rf $(BUILD_PATH)

$(BUILD_PATH)/khd: $(KHD_SRC)
	g++ $^ $(BUILD_FLAGS) $(FRAMEWORKS) -o $@
