# Notes
#
# TARGETS
#
# The default TARGET is set to DEVEL with all warnings cranked up to maximum.
# This should be the mode used during development / debug cycle.
#
# RELEASE is for the final binary release, it is also a good practice to strip
# the resulting binaries using: 'strip --strip-unneeded <binary>'
#
# Profiled builds are a bit trickier, one has to compile twice:
# 1. first compile with TARGET := $(GENERATE_PROFILE) and -lgcov linker flag
# 2. run your application to collect peformance samples (.gcda files in the
# build directory)
# 3. run make clean to delete old .o object files
# 4. now run make again with TARGET := $(PROFILED_RELEASE) to compile a new
# binary using .gcda files for extra level of optimization
# Warning! The default flags -march=native and -march=tune will likely make the
# binary non backwards compatible CPU-wise, but this will squeeze out the best
# performance for the particular machine we are building on.
#
# Static Code Analysis
# this will require 'flawfinder' and 'cppcheck' tools:
# sudo apt install flawfinder cppcheck
# now run 'make static-analysis' rule to make sure the code is squeaky clean.

#
# name of the resulting binary file
#
BUILD_ARTIFACT := contro

SHELL := /usr/bin/env bash
CC := gcc

#
# color output
#
PRINT = @echo -e "\e[1;32mBuilding $<\e[0m"

#
# TARGET can be only one of the below
#
DEVEL := 1
RELEASE := 2
GENERATE_PROFILE := 3
PROFILED_RELEASE := 4

#
# select one only at a time
#
TARGET := $(DEVEL)
# TARGET := $(RELEASE)
# TARGET := $(GENERATE_PROFILE)
# TARGET := $(PROFILED_RELEASE)

#
# -I, -D preprocessor options
#
CPPFLAGS = 
ifneq ($(TARGET), $(DEVEL))
	CPPFLAGS += -DNDEBUG
endif

#
# debugging and optimization options for the C and C++ compilers
#
ifeq ($(TARGET), $(DEVEL))

	# basic warnings
	CFLAGS := -std=c90 -pedantic -Og -g3 -Wall -pipe
	CFLAGS += -Werror

    # extended warning and debug options
	CFLAGS += -Wlogical-op -Wdouble-promotion -Wshadow -Wformat=2
	CFLAGS += -Wcast-align -Winvalid-pch -Wmissing-include-dirs
	CFLAGS += -Wredundant-decls -Wswitch-default -Wswitch-enum
	CFLAGS += -Wmissing-prototypes -Wconversion -Wcast-qual
	CFLAGS += -Wduplicated-cond -Wduplicated-branches -Wrestrict
	CFLAGS += -Wnull-dereference -Wjump-misses-init
	CFLAGS += -Wmissing-declarations -Wodr 

	# debugger experience
	CFLAGS += -Og -g3 -fno-omit-frame-pointer -fno-strict-aliasing
	CFLAGS += -fno-optimize-sibling-calls -fasynchronous-unwind-tables
	CFLAGS += -fexceptions

    # security options
	CPPFLAGS += -D_FORTIFY_SOURCE=2
	CFLAGS += -fstack-protector-strong
	CFLAGS += -fPIE -fPIC -pie -Wl,-pie -Wl,-z,defs -Wl,-z,now -Wl,-z,relro

    # sanitizers, some require clang
	CFLAGS += -fsanitize=address
	CFLAGS += -fsanitize=leak
	# CFLAGS += -fsanitize=address -fsanitize=pointer-compare
	# CFLAGS += -fsanitize=pointer-subtract
	# run with: ASAN_OPTIONS=detect_invalid_pointer_pairs=2: ./a.out
	CFLAGS += -fsanitize=undefined
	# CFLAGS += -fsanitize=thread
	# CFLAGS += -fsanitize=memory
    
else ifeq ($(TARGET), $(RELEASE))
	CFLAGS := -O2
	# CFLAGS += -fopt-info-vec-all
	# CFLAGS += -Q --help=target --help=optimizers
else ifeq ($(TARGET), $(GENERATE_PROFILE))
	CFLAGS := -Ofast -march=native -mtune=native -flto -fprofile-generate -pipe
else ifeq ($(TARGET), $(PROFILED_RELEASE))
	CFLAGS := -Ofast -march=native -mtune=native -flto -fprofile-use -pipe
	# CFLAGS += -fprofile-correction
endif

#
# -L options for the linker
#
LDFLAGS := -L.

#
# -l options to pass to the linker
#
LDLIBS := -lSDL2 -lSDL2_image
ifeq ($(TARGET), $(DEVEL))
	LDLIBS += -fsanitize=address -static-libasan
	LDLIBS += -fsanitize=undefined -static-libubsan
endif
ifeq ($(TARGET),$(filter $(TARGET),$(GENERATE_PROFILE) $(PROFILED_RELEASE)))
	LDLIBS += -lgcov
endif

#
# linking
#
OBJECTS := main.o
all: $(OBJECTS)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $(BUILD_ARTIFACT)

#
# compiling
#
main.o: main.c
	$(PRINT)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< $(LDFLAGS) $(LDLIBS)

clean:
	rm -f *.o $(BUILD_ARTIFACT)
	rm -rf "./infer-out"

very-clean:
	rm -f *.o *.gcda $(BUILD_ARTIFACT)

static-analysis:
	@echo
	@echo -e "\e[1;33mAnalazing: cppcheck... \e[0m"
	cppcheck $(CPPFLAGS) std=89 *.c *.h
	@echo
	@echo -e "\e[1;33mAnalazing: infer... \e[0m"	
	infer run -- gcc -c main.c
	@echo
	@echo -e "\e[1;33mAnalazing: cpd... \e[0m"
	run.sh cpd --language c --minimum-tokens 50 --files main.c	
	@echo
	@echo -e "\e[1;33mAnalazing: flawfinder... \e[0m"	
	flawfinder main.c
	@echo
	@echo -e "\e[1;33mAnalazing: splint... \e[0m"	
	splint --weak $(CPPFLAGS) main.c
	@echo
	@echo -e "\e[1;33mAnalazing: oclint... \e[0m"
	oclint main.c -- -c

