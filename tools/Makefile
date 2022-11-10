
BUILD_DIR=$(CURDIR)/build

MKSUNXI = mksunxi
MKLITTLEFS = mklittlefs

CSRC    = ../lib/littlefs/lfs.c ../lib/littlefs/lfs_util.c
CXXSRC  = mklittlefs.cpp

COBJS   = $(subst ../lib/littlefs,$(BUILD_DIR),$(CSRC:.c=.o))
CXXOBJS = $(addprefix $(BUILD_DIR)/,$(CXXSRC:.cpp=.opp))

INCLUDES = -I includes -I ../lib
CFLAGS   = -O2 -std=gnu99 $(INCLUDES)
CXXFLAGS = -O2 -std=gnu++11 $(INCLUDES)

CC  ?= gcc
CXX ?= g++

all: tools
tools: $(BUILD_DIR) $(MKSUNXI) $(MKLITTLEFS)

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf build
	rm -f $(MKSUNXI)
	rm -f $(MKLITTLEFS)

$(BUILD_DIR)/%.o : ../lib/littlefs/%.c
	echo "  CC    $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.o : %.c
	echo "  CC    $<"
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.opp : %.cpp
	echo "  CXX   $<"
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MKSUNXI):
	$(CC) $(CFLAGS) mksunxi.c -o $(MKSUNXI)

$(MKLITTLEFS): $(COBJS) $(CXXOBJS)
	g++ $(COBJS) $(CXXOBJS) $(CXXFLAGS) -o $@
