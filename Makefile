include sources.mk

PLATFORM = WINDOWS
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

ifeq ($(PLATFORM), LINUX)
CLEAN = rm -f build/obj/* build/bin/* 
else ifeq ($(PLATFORM), WINDOWS)
CLEAN = del /Q build\bin\* build\obj\*
else
$(error Platform type undefined. Possible types: LINUX, WINDOWS)
endif

CCFLAGS = -Wall -Wpedantic -std=c11 -I $(INC_DIR)
CPPFLAGS = -D PLATFORM=$(PLATFORM) $(CPP_DEFINE:%=-D %)
ARFLAGS = rcs

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(CC) -c $(CPPFLAGS) $(CCFLAGS) -o $@ $<

# Build executable
PHONY: build-bin
build-bin: $(OBJECTS)
	$(CC) $(CCFLAGS) -o $(BIN_DIR)/$(TARGET_NAME) $^

# Build library
PHONY: build-lib
build-lib: $(OBJECTS)
	$(AR) $(ARFLAGS) $(BIN_DIR)/lib$(TARGET_NAME).a $^

PHONY: clean
clean:
	$(CLEAN)