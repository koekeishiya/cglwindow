BUILD_FLAGS		= -O0 -g -Wall -std=c99
BUILD_PATH		= ./bin
SRC				= ./src/main.c
BINS			= $(BUILD_PATH)/main
INCLUDE         = -I/opt/local/include
LINK			= -L/opt/local/lib -framework Carbon -framework ApplicationServices -framework OpenGL

all: $(BINS)

install: BUILD_FLAGS=-O2 -std=c99 -Wall
install: clean $(BINS)

.PHONY: all clean install

$(BUILD_PATH):
	mkdir -p $(BUILD_PATH)

clean:
	rm -f $(BUILD_PATH)/main

$(BUILD_PATH)/main: $(SRC) | $(BUILD_PATH)
	clang $^ $(BUILD_FLAGS) -o $@ $(INCLUDE) $(LINK)
