MODULE = fast-chess
C_INCLUDES = -Isrc -Ithird_party
SRC_FILES = src/*.cpp src/*/*.cpp src/*/*/*.cpp src/*/*/*/*.cpp

CFLAGS += -std=c++17 -O3 -flto
DFLAGS += -DNDEBUG
WFLAGS += -Wfatal-errors -Wall -Wextra
LFLAGS += -static-libstdc++

ifneq ($(filter $(COMP),gcc g++),)
	override COMP = g++
	CFLAGS += -march=native -flto-partition=one -fno-gcse
	DFLAGS += -D__BIONIC_VERSIONER
	WFLAGS += 
	LFLAGS += 
else
	override COMP = clang++
	CFLAGS += -march=armv8-a --target=aarch64-linux-android21
	DFLAGS += 
	WFLAGS += 
	LFLAGS += 
endif

build:
	$(COMP) $(CFLAGS) $(C_INCLUDES) $(DFLAGS) $(WFLAGS) $(SRC_FILES) $(LFLAGS) -o $(MODULE)
	strip $(MODULE)

clean:
	@rm -f $(MODULE)