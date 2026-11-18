# tests/Makefile

ROOT_DIR := ..
SRC_DIR := $(ROOT_DIR)/src
SRC_COMMON_DIR := $(SRC_DIR)/common
BUILD_DIR := $(ROOT_DIR)/build/tests

include $(ROOT_DIR)/config.mk

OSSLINC = -I$(OPENSSLDIR)/include
OSSLIBS = -L$(OPENSSLDIR)/lib -lcrypto

CFLAGS := $(CFG_CFLAGS)
CFLAGS += -I$(SRC_COMMON_DIR)
CFLAGS += -Wall -Werror -I$(ROOT_DIR) $(OSSLINC)

LDFLAGS := $(CFG_LDFLAGS)

TARGET := $(BUILD_DIR)/pbkdf2-check-test

OBJS := \
    $(BUILD_DIR)/pbkdf2-check-test.o \
    $(BUILD_DIR)/pbkdf2-check.o \
    $(BUILD_DIR)/base64.o

.PHONY: all clean

all: $(TARGET)

VPATH = $(SRC_DIR) $(SRC_COMMON_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(BUILD_DIR)/pbkdf2-check-test.o: pbkdf2-check-test.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/pbkdf2-check.o: pbkdf2-check.c base64.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/base64.o: base64.c base64.h | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(OSSLIBS)

clean:
	rm -rf $(BUILD_DIR)
