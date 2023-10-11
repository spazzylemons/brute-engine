HEAP_SIZE      = 8388208
STACK_SIZE     = 61800

PRODUCT = Brute.pdx

# Locate the SDK
SDK = ${PLAYDATE_SDK_PATH}
ifeq ($(SDK),)
	SDK = $(shell egrep '^\s*SDKRoot' ~/.Playdate/config | head -n 1 | cut -c9-)
endif

ifeq ($(SDK),)
$(error SDK path not found; set ENV value PLAYDATE_SDK_PATH)
endif

# WADs from which map files are derived
WADFILES = $(wildcard maps/*.wad)
MAPDIRS = $(patsubst maps/%.wad,Source/maps/%,$(WADFILES))
MAPFILES = $(MAPDIRS:=/vertices) $(MAPDIRS:=/sectors) $(MAPDIRS:=/walls)

######
# IMPORTANT: You must add your source folders to VPATH for make to find them
# ex: VPATH += src1:src2
######

VPATH += src src/map

# List C source files here
SRC = src/core.c src/main.c src/map/load.c src/r_draw.c src/g_map.c src/b_math.c src/b_aabb.c

# List all user directories here
UINCDIR = 

# List user asm files
UASRC = 

# List all user C define here, like -D_DEBUG=1
UDEFS = 

# Define ASM defines here
UADEFS = 

# List the user directory to look for the libraries here
ULIBDIR =

# List all user libraries here
ULIBS =

include $(SDK)/C_API/buildsupport/common.mk

all: $(MAPFILES)

Source/maps/%/vertices Source/maps/%/sectors Source/maps/%/walls: maps/%.wad
	mkdir -p Source/maps/$*
	tools/map_converter.py $< Source/maps/$*

clean: mapclean

mapclean:
	-rm -rf Source/maps
