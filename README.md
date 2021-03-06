# A Python interface to the CLASSIC data container

The CLASSIC data container is a a digital format for single-dish or
interferometric radio-astronomy data used by the GILDAS software. The
format is described in [IRAM Memo 2013-2](https://www.researchgate.net/publication/262378314_IRAM_memo_2013-2_CLASSIC_Data_Container), which is also distributed with this software.

The software consists of two parts:

 * a C++ interface to the CLASSIC data container format. A small test
   program using it may be built by running `make classictest` and run
   via `./classictest <filename>`, where `filename` is a data file in
   CLASS(IC) format.
 * a python interface which can be built by `make install` and tested
   via `python3 test.py <filename>`, where `filename` again is a data
   file in CLASS(IC) format.

## Example:

``` shell
make classictest
./classictest O-097.F-9401A-2016-2016-06-08.apex
make install
python3 test.py O-097.F-9401A-2016-2016-06-08.apex
```
