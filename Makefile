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
WADFILE = maps/brute.wad

######
# IMPORTANT: You must add your source folders to VPATH for make to find them
# ex: VPATH += src1:src2
######

VPATH += src

# List C source files here
SRC = \
	src/a_actor.c \
	src/a_player.c \
	src/b_core.c \
	src/b_main.c \
	src/m_iter.c \
	src/m_load.c \
	src/m_map.c \
	src/r_draw.c \
	src/u_aabb.c \
	src/u_error.c \
	src/u_file.c \
	src/u_format.c \
	src/u_math.c \
	src/u_vec.c

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

all: wadextract

wadextract: $(WADFILE)
	tools/map_converter.py $< Source/

clean: mapclean

mapclean:
	-rm -rf Source/maps
