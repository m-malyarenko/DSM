include sources.mk

OS = WINDOWS
BUILD_TYPE = DEBUG
TARGET_NAME = dsm

SRC_DIR = ./src
INC_DIR = ./inc
OBJ_DIR = ./build/obj
BIN_DIR = ./build/bin

OBJECTS = $(SOURCES:%.c=$(OBJ_DIR)/%.o)

CC = gcc
AR = ar

ifeq ($(BUILD_TYPE), DEBUG)
CPP_DEFINE = DEBUG
else ifeq ($(BUILD_TYPE), RELEASE)
CPP_DEFINE = NDEBUG
else
$(error Build type undefined. Possible types: DEBUG, RELEASE)
endif

CCFLAGS = -Wall -Wpedantic -ggdb -std=c11 -I $(INC_DIR)
CPPFLAGS = $(CPP_DEFINE:%=-D %)

ifeq ($(OS), LINUX)
CLEAN = rm -f build/obj/* build/bin/* 
else ifeq ($(OS), WINDOWS)
CLEAN = del /Q build\bin\* build\obj\*
else
$(error Platform type undefined. Possible types: LINUX, WINDOWS)
endif

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CPPFLAGS) $(CCFLAGS) -o $@ $<

# Build executable
PHONY: build-bin
build-bin: $(OBJECTS)
	$(CC) $(CCFLAGS) -o $(BIN_DIR)/$(TARGET_NAME) $^

# Build library
PHONY: build-lib
build-lib: $(OBJECTS)
	$(AR) rcs $(BIN_DIR)/lib$(TARGET_NAME).a $^

PHONY: clean
clean:
	$(CLEAN)