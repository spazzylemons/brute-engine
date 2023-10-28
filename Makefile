# This Makefile delegates to other Makefiles depending on the target.

.PHONY: all emscripten playdate sdl clean

# Attempt to get the processor count for multithreaded compilation.
PROC_COUNT := $(shell nproc)
ifeq ($(PROC_COUNT),)
# looks like we don't have nproc...
PROC_COUNT := 1
endif

# The first recipe is the default. here we make it fail so you are required to
# specify a target.
all:
	$(error "Please specify a target.")

emscripten: assetextract
	$(MAKE) -f Makefile.emscripten -j$(PROC_COUNT)

playdate: assetextract
	$(MAKE) -f Makefile.playdate -j$(PROC_COUNT)

sdl: assetextract
	$(MAKE) -f Makefile.sdl -j$(PROC_COUNT)

clean:
	$(MAKE) -f Makefile.emscripten clean
	$(MAKE) -f Makefile.playdate clean
	$(MAKE) -f Makefile.sdl clean
	-rm -rf Source/assets.bingit 

assetextract:
	tools/map_converter.py assets/ Source/assets.bin
