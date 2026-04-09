# Amiga Engine — C++20 core (host tests) + optional Amiga C bridge.
# Cross C++: install demoscene-toolchain with m68k-amigaos-g++ and run `make cross`.

TOPDIR := $(realpath ..)

ifndef DEMOSCENE_ROOT
$(error Run 'source activate' from the repository root (see README.md))
endif

HOST_CXX ?= g++
HOST_CXXFLAGS := -std=c++20 -O2 -Wall -Wextra -Werror -pedantic \
	-I$(CURDIR)/include -DAMIGA_ENGINE_HOST_BUILD

HOST_SRCS := \
	src/cxx/copper_ops.cpp \
	src/cxx/scene.cpp \
	src/cxx/blitter_queue.cpp \
	src/cxx/blitter_sim.cpp \
	tests/host/test_runner.cpp

HOST_OBJS := $(HOST_SRCS:.cpp=.host.o)

.PHONY: all build clean host-test cross libamigaengine-c.a

all: host-test

# Invoked by the repository root `make build` recurse.
build: all

host-test: tests-host-runner
	./tests-host-runner

tests-host-runner: $(HOST_OBJS)
	$(HOST_CXX) $(HOST_CXXFLAGS) -o $@ $^

%.host.o: %.cpp
	$(HOST_CXX) $(HOST_CXXFLAGS) -c -o $@ $<

# Static library for Amiga effects (C, links with m68k-amigaos-gcc).
CC := m68k-amigaos-gcc
AR := m68k-amigaos-ar
CPPFLAGS := -I$(CURDIR)/include -I$(TOPDIR)/include
CFLAGS := -m68000 -mregparm=2 -freg-struct-return -O2 -Wall -Werror \
	-ffreestanding -fno-common -ggdb3
CFLAGS += -D__CONSTLIBBASEDECL__=const

C_BRIDGE_SRCS := c/ae_amiga_bridge.c
C_BRIDGE_OBJS := $(C_BRIDGE_SRCS:.c=.amiga.o)

cross: libamigaengine-c.a

libamigaengine-c.a: $(C_BRIDGE_OBJS)
	$(AR) rcs $@ $^

%.amiga.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(HOST_OBJS) tests-host-runner $(C_BRIDGE_OBJS) libamigaengine-c.a
