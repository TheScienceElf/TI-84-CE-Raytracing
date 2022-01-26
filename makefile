# ----------------------------
# Makefile Options
# ----------------------------

NAME ?= RAYTRACE
ICON ?= icon.png
DESCRIPTION ?= "Raytracer for Ti84 CE"
COMPRESSED ?= NO
ARCHIVED ?= NO

CFLAGS ?= -Wall -Wextra -Oz
CXXFLAGS ?= -Wall -Wextra -Oz

# ----------------------------

ifndef CEDEV
$(error CEDEV environment path variable is not set)
endif

include $(CEDEV)/meta/makefile.mk
