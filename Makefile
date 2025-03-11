BIN_DIR := ./bin
BUILD_DIR := ./build
SRC_DIR := ./src
INC_DIR := ./include

SERVER_OBJS := $(BUILD_DIR)/utils.o $(BUILD_DIR)/queue.o $(BUILD_DIR)/jobs.o $(BUILD_DIR)/jobexecutorserver.o
COMMANDER_OBJS := $(BUILD_DIR)/utils.o $(BUILD_DIR)/jobcommander.o

CC = gcc
CFLAGS = -g -Wall -Wextra -pedantic $(addprefix -I,$(INC_DIR))

all: $(BIN_DIR)/jobExecutorServer $(BIN_DIR)/jobCommander

$(BIN_DIR)/jobExecutorServer: $(SERVER_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@ -lpthread

$(BIN_DIR)/jobCommander: $(COMMANDER_OBJS)
	@mkdir -p $(dir $@)
	$(CC) $^ -o $@

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm -r $(BIN_DIR) $(BUILD_DIR)