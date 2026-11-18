# tests/Makefile

ROOT_DIR := ../..
SRC_DIR := $(ROOT_DIR)/src
SRC_COMMON_DIR := $(SRC_DIR)/common
BUILD_DIR := $(ROOT_DIR)/build
OBJ_DIR := $(BUILD_DIR)/tools/np

include $(ROOT_DIR)/config.mk

OSSLINC = -I$(OPENSSLDIR)/include
OSSLIBS = -L$(OPENSSLDIR)/lib -lcrypto

CFLAGS := $(CFG_CFLAGS)
CFLAGS += -I$(SRC_COMMON_DIR)
CFLAGS += -Wall -Werror -I$(ROOT_DIR) $(OSSLINC)

LDFLAGS := $(CFG_LDFLAGS)

TARGET := $(BUILD_DIR)/np

OBJS := \
    $(OBJ_DIR)/np.o \
    $(OBJ_DIR)/base64.o

.PHONY: all clean

all: $(TARGET)

VPATH = $(SRC_DIR) $(SRC_COMMON_DIR)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

$(OBJ_DIR):
	mkdir -p $(OBJ_DIR)

$(OBJ_DIR)/np.o: np.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJ_DIR)/base64.o: base64.c base64.h | $(OBJ_DIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(TARGET): $(OBJS) | $(BUILD_DIR)
	$(CC) $(LDFLAGS) -o $@ $(OBJS) $(OSSLIBS)

clean:
	rm -rf $(OBJ_DIR) $(TARGET)
