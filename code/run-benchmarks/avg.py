import numpy as np
from sys import argv

f = argv[1]

arr = []

with open(f, 'r') as f:
    for l in f:
        l = l.strip()
        arr.append(float(l))

np_arr = np.array(arr)

print("%s avg : %.2f (%.2f)" % (argv[1], round(np_arr.mean(), 3), round(np_arr.std(), 3)))
