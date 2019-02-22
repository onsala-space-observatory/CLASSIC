import numpy as np
import classic

print(classic.version())

foo = classic.Reader("/home/olberg/R/O-097.F-9401A-2016/O-097.F-9401A-2016/O-097.F-9401A-2016-2016-06-08.apex")
print(foo.__doc__)

# get number of spectra in file
nscans = foo.getDirectory()
print("number of spectra = %d" % (nscans))
iscan = 1
print(foo.getHead(iscan))
print(foo.getFreq(iscan))
print(foo.getData(iscan))

# attributes
print(foo.filename)
print(foo.count)
