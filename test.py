import sys

# import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

import classic
print(classic.version())

if len(sys.argv) == 1:
    print("usage: %s <filename>" % (sys.argv[0]))
    sys.exit(1)

classfile = sys.argv[1]
foo = classic.Reader(classfile)
# attributes
print(foo.__doc__)
print(foo.filename)
print(foo.count)

# get number of spectra in file
nscans = foo.getDirectory()
print("number of spectra = %d (%d)" % (nscans, foo.count))

headers = []
for i in range(nscans):
    headers.append(foo.getHead(i+1))

df = pd.DataFrame(headers)
print(df)


iscan = 100
header = foo.getHead(iscan)
print(header)

# freq = foo.getFreq(iscan)
# data = foo.getData(iscan)
# plt.plot(freq, data, color='b', label='CLASS')
# plt.show()
