import matplotlib.pyplot as plt
import pandas as pd
import numpy as np

import classic
print(classic.version())

foo = classic.Reader("/home/olberg/R/O-097.F-9401A-2016/O-097.F-9401A-2016/O-097.F-9401A-2016-2016-06-08.apex")
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

freq = foo.getFreq(iscan)
data = foo.getData(iscan)
plt.plot(freq, data, color='b', label='CLASS')
plt.show()
