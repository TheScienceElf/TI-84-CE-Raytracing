import numpy as np

# Generates the LUT for our gamma and tone-mapping curve

# Generate a set of values from 0 to 1
x = np.arange(255) / 256

# Convert from sRGB to linear space, and then apply
# tone mapping which asymptotically approaches 1
y = -np.log(1 - (x ** 2.1))
y_int = (y * 4096).astype(np.int32)

print(y_int)

line = ''
for num in y_int:
  line = line +  str(num) + ',\n'

print(line)