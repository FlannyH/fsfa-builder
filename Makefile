PROJECT = fsfa_builder

CC = g++
CFLAGS = -O2 -Wall

SRCS = 	fsfa_builder.cpp

TARGET_DIR = bin

all: $(TARGET_DIR)/$(PROJECT)

$(TARGET_DIR):
	mkdir -p $(TARGET_DIR)

$(TARGET_DIR)/$(PROJECT): $(SRCS) $(HEADERS) | $(TARGET_DIR)
	$(CC) $(CFLAGS) -o $(TARGET_DIR)/$(PROJECT) $(SRCS)

clean:
	rm -rf $(TARGET_DIR)/$(PROJECT)

.PHONY: all clean
