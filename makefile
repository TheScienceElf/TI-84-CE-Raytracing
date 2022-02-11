# ----------------------------
# Makefile Options
# ----------------------------

NAME = RAYTRACE
ICON = icon.png
DESCRIPTION = "Raytracer for Ti84 CE"
COMPRESSED = NO
ARCHIVED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
