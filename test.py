import pandas as pd
import numpy as np
import classic

print(classic.version())

foo = classic.Reader("/home/olberg/R/O-097.F-9401A-2016/O-097.F-9401A-2016/O-097.F-9401A-2016-2016-06-08.apex")
print(foo.__doc__)

# get number of spectra in file
nscans = foo.getDirectory()
print("number of spectra = %d" % (nscans))
iscan = 1
header = foo.getHead(iscan)
print(header.__class__)
print(header)

freq = foo.getFreq(iscan)
print(freq)

data = foo.getData(iscan)
print(data)

# attributes
print(foo.filename)
print(foo.count)

headers = []
for i in range(nscans):
    headers.append(foo.getHead(i+1))

df = pd.DataFrame(headers)
print(df)
