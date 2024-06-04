# Define compiler and flags
CC = gcc
CFLAGS = -Wall -Iapp/inc -Iinfra/inc
LDFLAGS = -lpthread -lrt

# Define source directories
APP_DIR = app
INFRA_DIR = infra

# Define output directories
BUILD_DIR = build
RELEASE_DIR = $(BUILD_DIR)/release
DEBUG_DIR = $(BUILD_DIR)/debug

# Define source files
APP_SRCS = $(APP_DIR)/clicmds.c $(APP_DIR)/main.c
INFRA_SRCS = $(INFRA_DIR)/cli.c $(INFRA_DIR)/cli_task.c $(INFRA_DIR)/text_utils.c

# Define object files
RELEASE_OBJS = $(APP_SRCS:%.c=$(RELEASE_DIR)/%.o) $(INFRA_SRCS:%.c=$(RELEASE_DIR)/%.o)
DEBUG_OBJS = $(APP_SRCS:%.c=$(DEBUG_DIR)/%.o) $(INFRA_SRCS:%.c=$(DEBUG_DIR)/%.o)

# Define targets
TARGET = cli_demo

.PHONY: all release debug clean

all: release debug

release: CFLAGS += -O2
release: $(RELEASE_DIR)/$(TARGET)

debug: CFLAGS += -g
debug: $(DEBUG_DIR)/$(TARGET)

$(RELEASE_DIR)/$(TARGET): $(RELEASE_OBJS)
	@mkdir -p $(RELEASE_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(DEBUG_DIR)/$(TARGET): $(DEBUG_OBJS)
	@mkdir -p $(DEBUG_DIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(RELEASE_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(DEBUG_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(BUILD_DIR)
