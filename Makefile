# Define compiler and flags
CC = gcc
CFLAGS = -Wall -Isrc/inc -Isrc/infra/inc
LDFLAGS = -lpthread -lrt

# Define source directories
SRC_DIR = src
INFRA_DIR = src/infra

# Define output directories
BUILD_DIR = build
RELEASE_DIR = $(BUILD_DIR)/release
DEBUG_DIR = $(BUILD_DIR)/debug

# Define source files
SRC_SRCS = $(SRC_DIR)/clicmds.c $(SRC_DIR)/main.c
INFRA_SRCS = $(INFRA_DIR)/cli.c $(INFRA_DIR)/cli_task.c $(INFRA_DIR)/text_utils.c

# Define object files
RELEASE_OBJS = $(SRC_SRCS:%.c=$(RELEASE_DIR)/%.o) $(INFRA_SRCS:%.c=$(RELEASE_DIR)/%.o)
DEBUG_OBJS = $(SRC_SRCS:%.c=$(DEBUG_DIR)/%.o) $(INFRA_SRCS:%.c=$(DEBUG_DIR)/%.o)

# Define targets
TARGET = cli_demo

# Default target
.PHONY: release
release: CFLAGS += -O2
release: $(RELEASE_DIR)/$(TARGET)

.PHONY: all release debug clean

all: release debug

debug: CFLAGS += -g
debug: $(DEBUG_DIR)/$(TARGET)

$(RELEASE_DIR)/$(TARGET): $(RELEASE_OBJS)
	@mkdir -p $(RELEASE_DIR)
	@echo "Linking $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo

$(DEBUG_DIR)/$(TARGET): $(DEBUG_OBJS)
	@mkdir -p $(DEBUG_DIR)
	@echo "Linking $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)
	@echo

$(RELEASE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Building $<"
	@$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	@echo "Building $<"
	@$(CC) $(CFLAGS) -c $< -o $@

clean:
	@echo "Cleaning up..."
	@rm -rf $(BUILD_DIR)
	@echo
