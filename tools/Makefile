
BUILD_DIR=build

MKSUNXI = mksunxi

CSRC    = mksunxi.c
CXXSRC  =

COBJS   = $(addprefix $(BUILD_DIR)/,$(CSRC:.c=.o))
CXXOBJS = $(addprefix $(BUILD_DIR)/,$(CXXSRC:.cpp=.opp))

INCLUDES = -I includes -I ../lib
CFLAGS   = -O2 -std=gnu99 $(INCLUDES)
CXXFLAGS = -O2 -std=gnu++11 $(INCLUDES)

CC  ?= gcc
CXX ?= g++

all: tools
tools: $(MKSUNXI)

.PHONY: all tools clean
.SILENT:

clean:
	rm -rf build
	rm -f $(MKSUNXI)

$(BUILD_DIR)/%.o : %.c
	echo "  CC    $@"
	mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.opp : %.cpp
	echo "  CXX   $@"
	mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MKSUNXI): $(COBJS)
	echo "  LD    $@"
	$(CC) $(CFLAGS) $(COBJS) -o $(MKSUNXI)
