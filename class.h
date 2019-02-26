/* Copyright 2019 Michael Olberg <michael.olberg@chalmers.se> */

#ifndef CLASSREADER_H
#define CLASSREADER_H

#include <vector>

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <math.h>
#include <time.h>

/**
 * @file class.h
 */

/**
 * @brief The structure representing a spectrum header.
 * @class SpectrumHeader
 *
 * Right now, this holds jsut the bare minimum. This will need to be
 * filled with additional structure members, if we want more of the
 * CLASSIC header data to be retrieved.
 */
struct SpectrumHeader {
    SpectrumHeader();     ///< constructor

    int id;               ///< the index of the spectrum in the file, 1..nscans
    int scanno;           ///< the scan number
    char target[13];      ///< source name
    char line[13];        ///< spectral line name, i.e. molecule and transition
    char instr[13];       ///< frontend/backend designator
    double RA;            ///< source x-coordinate in decimal degrees
    double Dec;           ///< source y-coordinate in decimal degrees
    double fLO;           ///< LO frequency in MHz
    double f0;            ///< signal frequency in MHz
    double df;            ///< frequency resolution in MHz
    double vs;            ///< source LSR velocity
    double dt;            ///< integration time in seconds
    double tsys;          ///< system temperature in K
    time_t utc;           ///< time of observation, seconds since 1970-01-01 00:00 (UTC).

    void print();         ///< print string representation to stdout
};

/**
 * @brief Get type of CLASSIC file
 * 
 * This will check the first four bytes in a file, in order to determine
 * the type of CLASSIC file.
 *
 * @param filename a string holding the name of a file to check
 * @return 1 or 2 for file type, or error code (< 1) if unsuccessful
 */
int fileType(const char *filename);

struct ClassDescriptor {
    int xbloc;
    int xnum;
    int xver;
    unsigned char xsourc[13];
    char xline[13];
    char xtel[13];
    int xdobs;
    int xdred;
    float xoff1;
    float xoff2;
    char xtype[5];
    int xkind;
    int xqual;
    int xscan;
    int xposa;
    /* Version 2 additions */
    long long int xlbloc, xlnum, xlscan;
    int xword, xsubs;
    /* End */
    char xfront[9];
    char xback[9];
    char xproc[9];
    char xproj[9];
    char unused[25];

    double ut, st;
    float az, el;
    float tau, tsys, time;
    int xunit;

    unsigned char source[13];
    float epoch;
    double lam, bet, projang;
    float lamof, betof;
    int system, proj;
    double sl0p, sb0p, sk0p;

    unsigned char line[13];
    double restf;
    int nchan;
    float rchan, fres, foff, vres, voff, badl;
    double image;
    int vtype;
    double doppler;

    float beeff, foeff, gaini, h2omm, pamb, tamb, tatms, tchop, tcold, taus, taui, tatmi, trec;
    int cmode;
    float atfac, alti, count[3], lcalof, bcalof;
    double geolong, geolat;

    double freq;
    float width;
    int npoin;
    float rpoin, tref, aref, apos, tres, ares, badc;
    int ctype;
    double cimag;
    float colla, colle;

    int ndata;
    float *data;
};

struct FileDescriptor1 {
    unsigned char code[4];
    int next;
    int lex;
    int nex;
    int lind;
    int xnext;
};

struct FileDescriptor2 {
    unsigned char code[4];
    int reclen;
    int kind;
    int vind;
    int lind;
    int flags;
    long int xnext;
    long int nextrec;
    int nextword;
    int lex1;
    int nex;
    int gex;
};

struct Type1Entry {
    int xblock;
    int xnum;
    int xver;
    unsigned char xsourc[13];
    unsigned char xline[13];
    unsigned char xtel[13];
    unsigned char xtype[5];
    int xdobs;
    int xdred;
    float xoff1;
    float xoff2;
    int xkind;
    int xqual;
    int xposa;
    int xscan;
    unsigned char xfront[9];
    unsigned char xback[9];
    unsigned char xproc[9];
    unsigned char xproj[9];
};

struct Type2Entry {
    long int xblock;
    int xword;
    long int xnum;
    int xver;
    unsigned char xsourc[13];
    unsigned char xline[13];
    unsigned char xtel[13];
    unsigned char xtype[5];
    int xdobs;
    int xdred;
    float xoff1;
    float xoff2;
    int xkind;
    int xqual;
    int xposa;
    long int xscan;
    int xsubs;
};

struct ClassSection1 {
    unsigned char ident[5];
    int nbl;
    int bytes;
    int adr;
    int nhead;
    int len;
    int ientry;
    int nsec;
    int obsnum;
    int sec_cod[4];
    int sec_adr[4];
    int sec_len[4];
};

struct ClassSection2 {
    unsigned char ident[5];
    int version;
    int nsec;
    long int nword;
    long int adata;
    long int ldata;
    long int xnum;
    int sec_cod[10];
    long int sec_len[10];
    long int sec_adr[10];
};

#define BUFSIZE (1024*1024)                  // 1 Mb
#define MAXCHANNELS (BUFSIZE/sizeof(float))  // 262144

/**
 * A class to read a CLASSIC file.
 *
 * This is the virtual base class, from which the Type1Reader and Type2Reader,
 * for file types 1 and 2, respectively, will inherit.
 */ 
class ClassReader {
 public:
    ClassReader(const char *);
    virtual ~ClassReader();

    /**
     * Scan the file contents and return number of spectra found.
     */
    virtual int getDirectory() = 0;
    /**
     * Return header for given spectrum number.
     *
     * @param scan number of spectrum (1..nscans)
     * @return object of type SpectrumHeader
     */
    virtual SpectrumHeader getHead(int scan) = 0;
    /**
     * Return frequency vector for given spectrum number.
     *
     * @param scan number of spectrum (1..nscans)
     * @return vector of doubles
     */
    virtual std::vector<double> getFreq(int scan) = 0;
    /**
     * Return data vector for given spectrum number.
     *
     * @param scan number of spectrum (1..nscans)
     * @return vector of doubles
     */
    virtual std::vector<double> getData(int scan) = 0;

    void dumpRecord();

 protected:
    bool open();
    virtual void getFileDescriptor() = 0;
    virtual void  getEntry(int k) = 0;
    void getRecord();
    int getInt();
    long int getLong();
    void getChar(unsigned char *dst, int len);
    float getFloat();
    double getDouble();
    void fillHeader(char *obsblock, int code, int addr, int len);
    void trim(unsigned char *ptr);
    time_t obssecond(long mjdn, double utc);
    double rta(float rad);
    std::vector<double> dataVector(int nchan, float *data);
    std::vector<double> freqVector(int nchan, double f0, double ref, double df);

    struct ClassDescriptor cdesc;
    int m_type;
    char cfname[256];
    FILE *cfp;
    char *m_ptr;
    unsigned int m_reclen;
    int m_nspec;
    char buffer[BUFSIZE];
};

#define MAXEXT 10

/**
 * A class to read a CLASSIC file of type 1.
 *
 */ 
class Type1Reader : public ClassReader {

 public:
    Type1Reader(const char *);
    ~Type1Reader();

    int getDirectory();
    SpectrumHeader getHead(int scan);
    std::vector<double> getFreq(int scan);
    std::vector<double> getData(int scan);

 private:
    void getFileDescriptor();
    void getEntry(int k);

    FileDescriptor1 fdesc;
    Type1Entry centry;
    ClassSection1 csect;
    int ext[MAXEXT];
};

/**
 * A class to read a CLASSIC file of type 2.
 *
 */ 
class Type2Reader : public ClassReader {

 public:
    Type2Reader(const char *);
    ~Type2Reader();

    int getDirectory();
    SpectrumHeader getHead(int scan);
    std::vector<double> getFreq(int scan);
    std::vector<double> getData(int scan);

 private:
    void getFileDescriptor();
    void getEntry(int k);

    FileDescriptor2 fdesc;
    Type2Entry centry;
    ClassSection2 csect;
    long int ext[MAXEXT];
};

/**
 * Open a CLASSIC file and return a suitable reader.
 *
 * @param filename a string holding the name of a file to check
 * @return a pointer to either a Type1Reader or Type2Reader instance.
 */
ClassReader *openClassFile(const char *filename);

#endif
