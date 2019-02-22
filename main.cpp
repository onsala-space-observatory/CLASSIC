/* Copyright 2017 Michael Olberg <michael.olberg@chalmers.se> */
#include "class.h"

int main(int argc, char *argv[])
{
    static char filename[] = "/home/olberg/R/O-097.F-9401A-2016/O-097.F-9401A-2016/O-097.F-9401A-2016-2016-06-08.apex";
    ClassReader *reader = openClassFile(filename);
    std::vector<SpectrumHeader> shv;

    int nscans = reader->getDirectory();
    printf("number of spectra = %d\n", nscans);

    shv.reserve(nscans);
    for (int iscan = 0; iscan < nscans; iscan++) {
        SpectrumHeader S = reader->getHead(iscan+1);
        shv.push_back(S);
    }

    std::vector<SpectrumHeader>::const_iterator i;
    for(i = shv.begin(); i != shv.end(); ++i) {
        SpectrumHeader head = *i;
        head.print();
    }

    int n = 120;
    SpectrumHeader S = reader->getHead(n);
    S.print();
    std::vector<double> f = reader->getFreq(n);
    std::vector<double> d = reader->getData(n);

    int nchan = f.size();
    printf("number of channels = %d\n", nchan);
    for (int k = 0; k < nchan; k++) {
        printf("%10.5f %10.5f\n", f.at(k), d.at(k));
    }
    delete reader;
    exit(0);
}
