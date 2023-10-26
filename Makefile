# This Makefile delegates to other Makefiles depending on the target.

.PHONY: all sdl playdate clean

# Attempt to get the processor count for multithreaded compilation.
PROC_COUNT := $(shell nproc)
ifeq ($(PROC_COUNT),)
# looks like we don't have nproc...
PROC_COUNT := 1
endif

# The first recipe is the default. here we make it fail so you are required to
# specify a target.
all:
	$(error "Please specify a target with 'make sdl' or 'make playdate'.")

sdl:
	$(MAKE) -f Makefile.sdl -j$(PROC_COUNT)

playdate:
	$(MAKE) -f Makefile.playdate -j$(PROC_COUNT)

clean:
	$(MAKE) -f Makefile.sdl clean
	$(MAKE) -f Makefile.playdate clean
