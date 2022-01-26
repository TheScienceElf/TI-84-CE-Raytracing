import numpy as np
import cv2

# Converts an image file to the Ti84's color format, and prints the output
# to be included in a header file.

# The file to convert
path = "../Floor128.bmp"

img = cv2.imread(path)

# Convert all channels to 5-bit color
lowRGB = (img // (256 / 32)).astype(np.uint16)

tiRGB = lowRGB[:,:,0] + (lowRGB[:,:,1] * (2 ** 6)) + (lowRGB[:,:,2] * (2 ** 11))

# Print output into a format which can be dropped into a header file
for row in tiRGB:
  line = ''
  for pixel in row:
    line += '0x%x, ' % pixel

  print('{%s},' % line[:-2])