/* Copyright 2017 Michael Olberg <michael.olberg@chalmers.se> */
#include "class.h"

int main(int argc, char *argv[])
{
    if (argc < 2) {
        fprintf(stderr, "usage: %s <filename>\n", argv[0]);
        exit(1);
    }

    ClassReader *reader = openClassFile(argv[1]);
    std::vector<SpectrumHeader> shv;

    if (reader) {
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

        int n = 1;
        if (argc > 2) n = atoi(argv[2]);
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
    } else exit(1);
    exit(0);
}
