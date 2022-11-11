
BUILD_DIR=$(CURDIR)/build

MKSUNXI = mksunxi
MKLFS = mklfs

LFSSRC  = ../lib/lfs/lfs.c ../lib/lfs/lfs_util.c
CSRC    = mklfs.c
CXXSRC  =

COBJS   = $(subst ../lib/lfs,$(BUILD_DIR),$(LFSSRC:.c=.o))
COBJS  += $(addprefix $(BUILD_DIR)/,$(CSRC:.c=.o))
CXXOBJS = $(addprefix $(BUILD_DIR)/,$(CXXSRC:.cpp=.opp))

INCLUDES = -I includes -I ../lib
CFLAGS   = -O2 -std=gnu99 $(INCLUDES) -DLINUX -D__NO_INLINE__
CXXFLAGS = -O2 -std=gnu++11 $(INCLUDES)

CC  ?= gcc
CXX ?= g++

all: tools
tools: $(BUILD_DIR) $(MKSUNXI) $(MKLFS)

.SILENT:
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

clean:
	rm -rf build
	rm -f $(MKSUNXI)
	rm -f $(MKLFS)

$(BUILD_DIR)/%.o : ../lib/lfs/%.c
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

$(MKLFS): $(COBJS) $(CXXOBJS)
	g++ $(COBJS) $(CXXOBJS) $(CXXFLAGS) -o $@
