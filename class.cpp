#include "class.h"

#undef DEBUG
#define NHEAD 14

typedef struct {
    int origpos;
    const char *value;
} SORT;

SpectrumHeader::SpectrumHeader(): id(0), scanno(0), RA(0.0), Dec(0.0),
                                  fLO(0.0), f0(0.0), df(0.0), vs(0.0), dt(0.0), tsys(0.0), utc(0L)
{
    memset(target, 0, 13);
    memset(line,   0, 13);
    memset(instr,  0, 13);
}

void SpectrumHeader::print()
{
    static char fmt[] = "0000-00-00 00:00:00"; // %Y-%m-%d %H:%M:%S
    strftime(fmt, sizeof(fmt), "%Y-%m-%d %H:%M:%S", gmtime(&utc));
    printf("%4d %8d '%-12s' '%-12s' '%-12s' %8.4f %8.4f %10.3f %10.3f %7.3f %+7.1f %5.1f %6.1f '%s'\n",
           id, scanno, target, line, instr, RA, Dec, fLO, f0, df, vs, dt, tsys, fmt);
}

int fileType(const char *cfname)
{
    FILE *cfp = fopen(cfname, "r");
    if (!cfp) return -1;

    char code[4];
    size_t len = fread(code, sizeof(char), 4, cfp);
    if (len != 4) return -2;

    code[2] = '\0';
    code[3] = '\0';
    fclose(cfp);

    if (strncmp((const char *)code, "1A", 2) == 0) return 1;
    if (strncmp((const char *)code, "2A", 2) == 0) return 2;
    return 0;
}

ClassReader *openClassFile(const char *cfname)
{
    ClassReader *reader = 0;

    int type = fileType(cfname);
    if (type == -1) {
        fprintf(stderr, "failed to open file '%s'\n", cfname);
        return reader;
    }
    if (type == -2) {
        fprintf(stderr, "failed to determine file type of '%s'\n", cfname);
        return reader;
    }

    if ((type != 1) && (type != 2)) {
        fprintf(stderr, "unrecognized file type!\n");
        return reader;
    }

    if (type == 1) reader = new Type1Reader(cfname);
    if (type == 2) reader = new Type2Reader(cfname);

    return reader;
}

ClassReader::ClassReader(const char *filename)
{
    strncpy(cfname, filename, 255);
    m_reclen = 0;
    m_nspec = 0;
    open();
}

ClassReader::~ClassReader()
{
    if (cfp != NULL) {
        fclose(cfp);
        cfp = NULL;
    }
}

bool ClassReader::open()
{
    cfp = fopen(cfname, "r");
    return true;
}

void ClassReader::getRecord()
{
    if (m_reclen == 0) return;
    size_t len = fread(buffer, sizeof(int), m_reclen, cfp);
    if (feof(cfp)) return;

    if (len != m_reclen) {
        fprintf(stderr, "failed to read record of size %d words (%ld)\n", m_reclen, len);
    }
}

int ClassReader::getInt()
{
    int data = 0;
    memcpy(&data, m_ptr, sizeof(int));
    m_ptr += sizeof(int);

    return data;
}

long int ClassReader::getLong()
{
    long int data = 0;
    memcpy(&data, m_ptr, sizeof(long int));
    m_ptr += sizeof(long int);

    return data;
}

float ClassReader::getFloat()
{
    float data = 0;
    memcpy(&data, m_ptr, sizeof(float));
    m_ptr += sizeof(float);

    return data;
}

double ClassReader::getDouble()
{
    double data = 0;
    memcpy(&data, m_ptr, sizeof(double));
    m_ptr += sizeof(double);

    return data;
}

void ClassReader::dumpRecord()
{
    int word;
    const int wpl = 8;

    if (m_reclen == 0) return;
    m_ptr = buffer;
    for (unsigned int i = 0; i < m_reclen; i++) {
        word = getInt();
        printf("[%03d] %10d ", i, word);
        if ((i % wpl) == (wpl-1)) printf("\n");
    }
}

void ClassReader::trim(unsigned char *input)
{
    unsigned char *dst = input, *src = input;
    unsigned char *end;

    // Skip whitespace at front...
    while (isspace(*src)) {
        ++src;
    }
    // Trim at end...
    end = src + strlen((const char *)src) - 1;
    while (end > src && isspace(*end)) {
        *end-- = 0;
    }
    // Move if needed.
    if (src != dst) {
        while ((*dst++ = *src++));
    }
}

time_t ClassReader::obssecond(long mjdn, double utc)
{
    const long int Jan01of1970 = 40587L;
    time_t elapsed = static_cast<time_t>(mjdn - Jan01of1970)*86400L;
    /* utc is in radians, convert to seconds and add */
    elapsed += static_cast<time_t>(floor(utc*3600.0*12.0/M_PI));

    return elapsed;
}

double ClassReader::rta(float rad)
{
    const double RADTOSEC = (3600.0*180.0/M_PI);
    return (double)rad * RADTOSEC;
}

std::vector<double> ClassReader::dataVector(int nchan, float *s)
{
    std::vector<double> data(nchan);

    m_ptr = (char *)s;
    for (int k = 0; k < nchan; k++) data[k] = getFloat();

    return data;
}

std::vector<double> ClassReader::freqVector(int nchan, double f0, double ref, double df)
{
    std::vector<double> freq(nchan);
    if (df == 0.0) {
        for (int k = 0; k < nchan; k++) freq[k] = (double)(k+1);
    } else {
        for (int k = 0; k < nchan; k++) freq[k] = ((double)(k+1)-ref)*df + f0;
    }

    return freq;
}

void ClassReader::getChar(unsigned char *dst, int len)
{
    memcpy(dst, m_ptr, len);
    dst[len] = '\0';
    trim(dst);
    m_ptr += len;
}

void ClassReader::fillHeader(char *obsblock, int code, int addr, int len)
{
    int noff = (addr-1)*4;
    m_ptr = obsblock + noff;
    if (code == -2) { /* General */
        cdesc.ut = getDouble();
        cdesc.st = getDouble();
        cdesc.az = getFloat();
        cdesc.el = getFloat();
        cdesc.tau = getFloat();
        cdesc.tsys = getFloat();
        cdesc.time = getFloat();

        if (len > 9) cdesc.xunit = getInt();
        else         cdesc.xunit = 0;
#ifdef DEBUG
        printf("    -2  UT=%f %f   Az,El=%f,%f  time=%f\n", cdesc.ut, cdesc.st, cdesc.az, cdesc.el, cdesc.time);
#endif
    } else if (code == -3) { /* Position */
        if (len == 17) {
            getChar(cdesc.source, 12);
            cdesc.epoch   = getFloat();
            cdesc.lam     = getDouble();
            cdesc.bet     = getDouble();
            cdesc.lamof   = getFloat();
            cdesc.betof   = getFloat();
            cdesc.proj    = getInt();
            cdesc.sl0p    = getDouble();
            cdesc.sb0p    = getDouble();
            cdesc.sk0p    = getDouble();
        } else {
            getChar(cdesc.source, 12);
            cdesc.system  = getInt();
            cdesc.epoch   = getFloat();
            cdesc.proj    = getInt();
            cdesc.lam     = getDouble();
            cdesc.bet     = getDouble();
            cdesc.projang = getDouble();
            cdesc.lamof   = getFloat();
            cdesc.betof   = getFloat();
        }

#ifdef DEBUG
        printf("    -3  '%s' %f Coord:%f,%f (%f,%f) %4d\n", cdesc.source,
               cdesc.epoch, cdesc.lam, cdesc.bet, cdesc.lamof, cdesc.betof, cdesc.proj);
#endif
    } else if (code == -4) { /* Spectroscopic */
        getChar(cdesc.line, 12);
        cdesc.restf   = getDouble();
        cdesc.nchan   = getInt();
        cdesc.rchan   = getFloat();
        cdesc.fres    = getFloat();
        cdesc.foff    = getFloat();
        cdesc.vres    = getFloat();
        cdesc.voff    = getFloat();
        cdesc.badl    = getFloat();
        cdesc.image   = getDouble();
        cdesc.vtype   = getInt();
        cdesc.doppler = getDouble();

#ifdef DEBUG
        printf("    -4  '%s' %f i%f (%f %f) n=%d r=%f v=%f %f %f\n", cdesc.line,
               cdesc.restf, cdesc.image, cdesc.fres, cdesc.foff, cdesc.nchan, cdesc.rchan, cdesc.vres, cdesc.voff, cdesc.doppler);
#endif
    } else if (code == -5) { /* Baseline info section (for spectra or drifts) */
        /* Not supported, silently ignored */
    } else if (code == -6) { /* Scan numbers of initial observations */
        /* Not supported , silently ignored*/
    } else if (code == -7) { /* Default plotting limits */
        /* Not supported, silently ignored */
    } else if (code == -8) { /* Switching information */
        /* Not supported, silently ignored */
    } else if (code == -9) { /* Gauss fit results */
        /* Not supported, silently ignored */
    } else if (code == -10) { /* Continuum drifts */
        cdesc.freq     = getDouble();
        cdesc.width    = getFloat();
        cdesc.npoin    = getInt();
        cdesc.rpoin    = getFloat();
        cdesc.tref     = getFloat();
        cdesc.aref     = getFloat();
        cdesc.apos     = getFloat();
        cdesc.tres     = getFloat();
        cdesc.ares     = getFloat();
        cdesc.badc     = getFloat();
        cdesc.ctype    = getInt();
        cdesc.cimag    = getDouble();
        cdesc.colla    = getFloat();
        cdesc.colle    = getFloat();

#ifdef DEBUG
        printf("   -10  f=%f %f  n=%d r=%f\n", cdesc.freq, cdesc.width, cdesc.npoin, cdesc.rpoin);
        printf("   -10  t=%f %f  a=%f %f\n", cdesc.tref, cdesc.tres, cdesc.aref, cdesc.ares);
#endif
    } else if (code == -14) { /* Calibration */
        cdesc.beeff    = getFloat();
        cdesc.foeff    = getFloat();
        cdesc.gaini    = getFloat();
        cdesc.h2omm    = getFloat();
        cdesc.pamb     = getFloat();
        cdesc.tamb     = getFloat();
        cdesc.tatms    = getFloat();
        cdesc.tchop    = getFloat();
        cdesc.tcold    = getFloat();
        cdesc.taus     = getFloat();
        cdesc.taui     = getFloat();
        cdesc.tatmi    = getFloat();
        cdesc.trec     = getFloat();
        cdesc.cmode    = getInt();
        cdesc.atfac    = getFloat();
        cdesc.alti     = getFloat();
        cdesc.count[0] = getFloat();
        cdesc.count[1] = getFloat();
        cdesc.count[2] = getFloat();
        cdesc.lcalof   = getFloat();
        cdesc.bcalof   = getFloat();
        cdesc.geolong  = getDouble();
        cdesc.geolat   = getDouble();

#ifdef DEBUG
        printf("   -14 %f %f %f trec=%f  (%f,%f) Site:(%f,%f, %f)\n",
               cdesc.h2omm, cdesc.tamb, cdesc.pamb, cdesc.trec,
               cdesc.lcalof, cdesc.bcalof, cdesc.geolong, cdesc.geolat, cdesc.alti);
#endif
    } else {
        fprintf(stderr, "cannot handle CLASS section code %d yet.Sorry.", code);
    }
}

Type1Reader::Type1Reader(const char *filename) : ClassReader(filename)
{
    getFileDescriptor();
}

Type1Reader::~Type1Reader()
{
}

void Type1Reader::getFileDescriptor()
{
    m_type = 1;

    m_reclen = 128;

    fseek(cfp, 0L, SEEK_SET);
    getRecord();
    m_ptr = buffer;

    getChar(fdesc.code, 4);

#ifdef DEBUG
    printf("type=%d, reclen=%d\n", m_type, m_reclen);
#endif

    fdesc.next  = getInt();
    fdesc.lex   = getInt();
    fdesc.nex   = getInt();
    fdesc.xnext = getInt();

    int nex = fdesc.nex;
    if (nex > MAXEXT) {
        fprintf(stderr, "number of extensions too large!");
    }
    for (int i = 0; i < nex; i++) {
        ext[i] = getInt();
#ifdef DEBUG
        printf("ext[%d] = %d\n", i, ext[i]);
#endif
    }
    return;
}

void  Type1Reader::getEntry(int k)
{
    m_ptr = buffer + k*m_reclen;
    centry.xblock = getInt();
    centry.xnum = getInt();
    centry.xver = getInt();
    getChar(centry.xsourc, 12);
    getChar(centry.xline,  12);
    getChar(centry.xtel,   12);

    centry.xdobs = getInt();
    centry.xdred = getInt();
    centry.xoff1 = getFloat();
    centry.xoff2 = getFloat();
    getChar(centry.xtype, 4);
    centry.xkind = getInt();
    centry.xqual = getInt();
    centry.xscan = getInt();
    centry.xposa = getInt();
}

int Type1Reader::getDirectory()
{
    int nst = fdesc.xnext;
    long pos = (ext[0]-1)*m_reclen;

    fseek(cfp, 4*pos, SEEK_SET);

    int nrec = 2;
    int nspec = 0;
    while (nrec < nst) {
#ifdef DEBUG
        printf("in %s extension 0 spectrum %d at position %ld\n", __FUNCTION__, nspec+1, ftell(cfp));
#endif
        getRecord();
        for (int k = 0; k < 4; k++) {
            getEntry(k);
            if (centry.xver == 1 && centry.xnum > 0 && centry.xnum < nst) {
                nspec++;
#ifdef DEBUG
                printf("%5d: xblock=%6d xnum=%4d xver=%d: xsource='%s'  xline='%s' xtel='%s'\n",
                        nspec, centry.xblock, centry.xnum, centry.xver,
                        centry.xsourc, centry.xline, centry.xtel);
#endif
            } else {
                nrec = nst;
                break;
            }
        }
        nrec++;
    }
    m_nspec = nspec;
    return nspec;
}

SpectrumHeader Type1Reader::getHead(int scan)
{
    SpectrumHeader head;
    // head.print();

    long pos = (ext[0]-1)*m_reclen;
    int nrec = (scan-1)/4;
    int k = (scan-1)%4;

    fseek(cfp, 4*(pos+nrec*m_reclen), SEEK_SET);
    getRecord();
    getEntry(k);

    pos = (centry.xblock-1)*m_reclen;
    fseek(cfp, 4*pos, SEEK_SET);
    getRecord();
    m_ptr = buffer;

    getChar(csect.ident, 4);
    csect.nbl = getInt();
    csect.bytes = getInt();
    csect.adr = getInt();
    csect.nhead = getInt();
    csect.len = getInt();
    csect.ientry = getInt();
    csect.nsec = getInt();
    csect.obsnum = getInt();
    int nsec = csect.nsec;
    if (nsec > 4) nsec = 4;
    for (int i = 0; i < nsec; i++) csect.sec_cod[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_len[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_adr[i] = getInt();
#ifdef DEBUG
    printf("%7d %12s %12s %d %d %d %d %d %d %d %d %d (%d %d %d %d | %d %d %d %d | %d %d %d %d)\n",
           scan, centry.xsourc, centry.xline, centry.xkind,
           csect.nbl, csect.bytes, csect.adr, csect.nhead, csect.len, csect.ientry, csect.nsec, csect.obsnum,
           csect.sec_cod[0], csect.sec_cod[1], csect.sec_cod[2], csect.sec_cod[3],
           csect.sec_adr[0], csect.sec_adr[1], csect.sec_adr[2], csect.sec_adr[3],
           csect.sec_len[0], csect.sec_len[1], csect.sec_len[2], csect.sec_len[3]);
#endif
    unsigned int size = csect.nbl*m_reclen*4;
    if (size > sizeof(buffer)) {
        fprintf(stderr, "buffer too small!");
    }
    if (csect.nbl > 1) {
        size -= 4*m_reclen;
        size_t len = fread(buffer+4*m_reclen, sizeof(char), size, cfp);
        if (len != size) {
            fprintf(stderr, "failed to read obsblock (%ld != %ld)!", len, static_cast<size_t>(size));
        }
    }

    for (int i = 0; i < nsec; i++) {
        fillHeader(buffer, csect.sec_cod[i], csect.sec_adr[i], csect.sec_len[i]);
    }

    bool spectrum = (centry.xkind == 0);

    double restf, LO, fres;
    if (spectrum) {
        restf = cdesc.restf;
        LO = (cdesc.restf + cdesc.image)/2.0;
        fres = cdesc.fres;
    } else {
        restf = cdesc.tref;
        LO = (cdesc.freq + cdesc.cimag)/2.0;
        fres = cdesc.tres;
    }
    double lam = cdesc.lam;
    double bet = cdesc.bet;
    lam += cdesc.lamof/cos(bet);
    bet += cdesc.betof;
    time_t datetime = obssecond(centry.xdobs + 60549L, cdesc.ut);
    // const char *datetime = obstime(centry.xdobs + 60549, cdesc.ut);
    head.id = scan;
    head.scanno = centry.xscan;
    strncpy(head.target, (const char *)centry.xsourc, 12);
    strncpy(head.line, (const char *)centry.xline, 12);
    strncpy(head.instr, (const char *)centry.xtel, 12);
    head.RA = lam*180.0/M_PI;
    head.Dec = bet*180.0/M_PI;
    head.fLO = LO;
    head.f0 = restf;
    head.df = fres;
    head.vs = cdesc.voff;
    head.dt = cdesc.time;
    head.tsys = cdesc.tsys;
    head.utc = datetime;
    return head;
}

std::vector<double> Type1Reader::getFreq(int scan)
{
    std::vector<double> f;

    long pos = (ext[0]-1)*m_reclen;
    int nrec = (scan-1)/4;
    int k = (scan-1)%4;

    fseek(cfp, 4*(pos+nrec*m_reclen), SEEK_SET);
    getRecord();
    getEntry(k);

    pos = (centry.xblock-1)*m_reclen;
    fseek(cfp, 4*pos, SEEK_SET);
    getRecord();
    m_ptr = buffer;

    getChar(csect.ident, 4);
    csect.nbl = getInt();
    csect.bytes = getInt();
    csect.adr = getInt();
    csect.nhead = getInt();
    csect.len = getInt();
    csect.ientry = getInt();
    csect.nsec = getInt();
    csect.obsnum = getInt();
    int nsec = csect.nsec;
    if (nsec > 4) nsec = 4;
    for (int i = 0; i < nsec; i++) csect.sec_cod[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_len[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_adr[i] = getInt();

    unsigned int size = csect.nbl*m_reclen*4;
    if (size > sizeof(buffer)) {
        fprintf(stderr, "buffer too small!");
    }
    if (csect.nbl > 1) {
        size -= 4*m_reclen;
        size_t len = fread(buffer+4*m_reclen, sizeof(char), size, cfp);
        if (len != size) {
            fprintf(stderr, "failed to read obsblock (%ld != %ld)!", len, static_cast<size_t>(size));
        }
    }

    for (int i = 0; i < nsec; i++) {
        fillHeader(buffer, csect.sec_cod[i], csect.sec_adr[i], csect.sec_len[i]);
    }

    bool spectrum = (centry.xkind == 0);
    int rchan = 0;
    double restf = 0.0;
    double fres = 0.0;
    int ndata = 0;
    if (spectrum) {
        rchan = cdesc.rchan;
        restf = cdesc.restf;
        fres = cdesc.fres;
        ndata = cdesc.nchan;
    } else {
        rchan = cdesc.rpoin;
        restf = cdesc.tref;
        fres = cdesc.tres;
        ndata = cdesc.npoin;
    }

    if (ndata > (int)MAXCHANNELS) {
        fprintf(stderr, "maximum number of channels exceeded: %d %ld\n", ndata, MAXCHANNELS);
        return f;
    }

    f = freqVector(ndata, restf, rchan, fres);
    return f;
}

std::vector<double> Type1Reader::getData(int scan)
{
    std::vector<double> d;

    long pos = (ext[0]-1)*m_reclen;
    int nrec = (scan-1)/4;
    int k = (scan-1)%4;

    fseek(cfp, 4*(pos+nrec*m_reclen), SEEK_SET);
    getRecord();
    getEntry(k);

    pos = (centry.xblock-1)*m_reclen;
    fseek(cfp, 4*pos, SEEK_SET);
    getRecord();
    m_ptr = buffer;

    getChar(csect.ident, 4);
    csect.nbl = getInt();
    csect.bytes = getInt();
    csect.adr = getInt();
    csect.nhead = getInt();
    csect.len = getInt();
    csect.ientry = getInt();
    csect.nsec = getInt();
    csect.obsnum = getInt();
    int nsec = csect.nsec;
    if (nsec > 4) nsec = 4;
    for (int i = 0; i < nsec; i++) csect.sec_cod[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_len[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_adr[i] = getInt();
#ifdef DEBUG
    printf("%7d %12s %12s %d %d %d %d %d %d %d %d %d (%d %d %d %d | %d %d %d %d | %d %d %d %d)\n",
           scan, centry.xsourc, centry.xline, centry.xkind,
           csect.nbl, csect.bytes, csect.adr, csect.nhead, csect.len, csect.ientry, csect.nsec, csect.obsnum,
           csect.sec_cod[0], csect.sec_cod[1], csect.sec_cod[2], csect.sec_cod[3],
           csect.sec_adr[0], csect.sec_adr[1], csect.sec_adr[2], csect.sec_adr[3],
           csect.sec_len[0], csect.sec_len[1], csect.sec_len[2], csect.sec_len[3]);
#endif
    unsigned int size = csect.nbl*m_reclen*4;
    if (size > sizeof(buffer)) {
        fprintf(stderr, "buffer too small!");
    }
    if (csect.nbl > 1) {
        size -= 4*m_reclen;
        size_t len = fread(buffer+4*m_reclen, sizeof(char), size, cfp);
        if (len != size) {
            fprintf(stderr, "failed to read obsblock (%ld != %ld)!", len, static_cast<size_t>(size));
        }
    }

    for (int i = 0; i < nsec; i++) {
        fillHeader(buffer, csect.sec_cod[i], csect.sec_adr[i], csect.sec_len[i]);
    }

    bool spectrum = (centry.xkind == 0);

    int ndata = 0;
    if (spectrum) ndata = cdesc.nchan;
    else          ndata = cdesc.npoin;

    if (ndata > (int)MAXCHANNELS) {
        printf("maximum number of channels exceeded: %d %ld\n", ndata, MAXCHANNELS);
    }

    float *s = (float *)(buffer+4*(csect.nhead-1));
    d = dataVector(ndata, s);
    return d;
}

Type2Reader::Type2Reader(const char *filename) : ClassReader(filename)
{
    getFileDescriptor();
}

Type2Reader::~Type2Reader()
{
}

void Type2Reader::getFileDescriptor()
{
    size_t len = 0;
    m_type = 2;

    fseek(cfp, 0L, SEEK_SET);
    len = fread((char *)&fdesc, sizeof(char), 4, cfp);
    len = fread((char *)&fdesc.reclen, sizeof(int), 1, cfp);
    if (len != 1) {
        fprintf(stderr, "failed to read record length\n");
    }
    m_reclen = fdesc.reclen;

#ifdef DEBUG
    printf("type=%d, reclen=%d\n", m_type, m_reclen);
#endif

    fseek(cfp, 0L, SEEK_SET);
    getRecord();
    m_ptr = buffer+4*sizeof(char)+sizeof(int);

    fdesc.kind  = getInt();
    fdesc.vind  = getInt();
    fdesc.lind  = getInt();
    fdesc.flags = getInt();

    fdesc.xnext    = getLong();
    fdesc.nextrec  = getLong();
    fdesc.nextword = getInt();

    fdesc.lex1 = getInt();
    fdesc.nex  = getInt();
    fdesc.gex  = getInt();

    if (fdesc.kind != 1) {
        fprintf(stderr, "not a file written by CLASS!");
        return;
    }
    if ((fdesc.gex != 10) && (fdesc.gex != 20)) {
        fprintf(stderr, "problem with extension growth!");
        return;
    }

    int nex = fdesc.nex;
    if (nex > MAXEXT) {
        fprintf(stderr, "number of extensions too large!");
    }
    unsigned int size = m_reclen;
    for (int i = 0; i < nex; i++) {
        ext[i] = getLong();
        if (fdesc.gex == 20) {
            for (int j = 0; j < i; j++) size *= 2;
        }
#ifdef DEBUG
        printf("ext[%d] = %ld %d\n", i, ext[i], size);
#endif
    }
    return;
}

void Type2Reader::getEntry(int k)
{
    m_ptr = buffer + k*4*fdesc.lind;
    centry.xblock = getLong();
    centry.xword = getInt();
    centry.xnum = getLong();
    centry.xver = getInt();
    getChar(centry.xsourc, 12);
    getChar(centry.xline,  12);
    getChar(centry.xtel,   12);

    centry.xdobs = getInt();
    centry.xdred = getInt();
    centry.xoff1 = getFloat();
    centry.xoff2 = getFloat();
    getChar(centry.xtype, 4);
    centry.xkind = getInt();
    centry.xqual = getInt();
    centry.xposa = getInt();
    centry.xscan = getLong();
    centry.xsubs = getInt();
}

int Type2Reader::getDirectory()
{
    int growth = 1;

    int nspec = 0;
    for (int iext = 0; iext < fdesc.nex; iext++) {
        int nst = fdesc.lex1*growth;
        unsigned int isize = nst*fdesc.lind;
        long pos = (ext[iext]-1)*1024;

        fseek(cfp, 4*pos, SEEK_SET);
#ifdef DEBUG
        printf("in %s extension %d spectrum %d at position %ld\n", __FUNCTION__, iext, nspec+1, ftell(cfp));
#endif
        size_t len = fread(buffer, sizeof(int), isize, cfp);
        if (len != isize) {
            fprintf(stderr, "failed to read directory information\n");
            return 0;
        }
        m_ptr = buffer;

        for (int k = 0; k < nst; k++) {
            getEntry(k);
            if (centry.xnum >= 1) {
                nspec++;
#ifdef DEBUG
                printf("%5d: xblock=%3ld xword=%4d xnum=%3ld xver=%d: xsource='%s'  xline='%s' xtel='%s'\n",
                       nspec, centry.xblock, centry.xword, centry.xnum, centry.xver,
                       centry.xsourc, centry.xline, centry.xtel);
#endif
            }
        }
        if (fdesc.gex == 20) growth *= 2;
    }
    m_nspec = nspec;
    return nspec;
}

SpectrumHeader Type2Reader::getHead(int scan)
{
    SpectrumHeader head;
    // head.print();

    size_t len = 0;
    int iext = 0;
    unsigned int isize = 0;
    int jent = 0;

    int growth = 1;
    int nspec = 0;
    for (int i = 0; i < fdesc.nex; i++) {
        int nst = fdesc.lex1*growth;
        isize = nst*fdesc.lind;
        for (int k = 0; k < nst; k++) {
            nspec++;
            if (nspec == scan) {
                jent = k;
                iext = i;
                break;
            }
        }
        if (nspec == scan) break;
        if (fdesc.gex == 20) growth *= 2;
    }

    long pos = (ext[iext]-1)*m_reclen;
#ifdef DEBUG
    // printf("in %s spectrum %d at position %ld in extension %d entry %d at word %d\n",
    //         __FUNCTION__, scan, pos, iext, jent, startword);
#endif
    fseek(cfp, 4*pos, SEEK_SET);
    len = fread(buffer, sizeof(int), isize, cfp);
    if (len != isize) {
        fprintf(stderr, "failed to read entry descriptor\n");
        return head;
    }

    getEntry(jent);
    pos = (centry.xblock-1)*m_reclen+centry.xword-1;

    fseek(cfp, 4*pos, SEEK_SET);
#ifdef DEBUG
    printf("in %s reading spectrum %d at block %ld, word %d position %ld (%ld)\n",
            __FUNCTION__, scan, centry.xblock, centry.xword, ftell(cfp), 4*pos);
#endif
    getRecord();
    m_ptr = buffer;

    getChar(csect.ident, 4);
    csect.version = getInt();
    csect.nsec = getInt();
    csect.nword = getLong();
    csect.adata = getLong();
    csect.ldata = getLong();
    csect.xnum = getLong();
    int nsec = csect.nsec;
    if (nsec > 10) nsec = 10;
    for (int i = 0; i < nsec; i++) csect.sec_cod[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_len[i] = getLong();
    for (int i = 0; i < nsec; i++) csect.sec_adr[i] = getLong();
#ifdef DEBUG
    printf("%7d %12s %12s %d %d (%d %d %d %d | %d %d %d %d | %d %d %d %d)\n",
           scan,
           centry.xsourc, centry.xline, centry.xkind,
           csect.nsec,
           csect.sec_cod[0], csect.sec_cod[1], csect.sec_cod[2], csect.sec_cod[3],
           csect.sec_adr[0], csect.sec_adr[1], csect.sec_adr[2], csect.sec_adr[3],
           csect.sec_len[0], csect.sec_len[1], csect.sec_len[2], csect.sec_len[3]);
#endif

    for (int i = 0; i < nsec; i++) {
        unsigned int isize = csect.sec_len[i];
        unsigned int size = 4*isize;
        long secpos = 4*(pos + csect.sec_adr[i]-1);
#ifdef DEBUG
        printf("in %s reading section %d at position %ld (%ld,%ld)\n",
                __FUNCTION__, i, ftell(cfp), pos, csect.sec_adr[i]);
#endif
        int ierr = fseek(cfp, secpos, SEEK_SET);
        if (ierr == -1) {
            fprintf(stderr, "failure to position section %d (%ld)\n", i, secpos);
            break;
        }
        len = fread(buffer, sizeof(char), size, cfp);
        if (len != size) {
            fprintf(stderr, "failed to read section %d (%ld != %ld)\n", i, len, static_cast<size_t>(size));
            break;
        }
        m_ptr = buffer;
#ifdef DEBUG
        printf("Sect[%d]=%3d at adr %d of length %d.\n", i, csect.sec_cod[i], secpos, size);
#endif
        fillHeader(buffer, csect.sec_cod[i], 1, csect.sec_len[i]);
    }

    bool spectrum = (centry.xkind == 0);

    double restf, LO, fres;
    if (spectrum) {
        restf = cdesc.restf;
        LO = (cdesc.restf + cdesc.image)/2.0;
        fres = cdesc.fres;
    } else {
        restf = cdesc.tref;
        LO = (cdesc.freq + cdesc.cimag)/2.0;
        fres = cdesc.tres;
    }
    double lam = cdesc.lam;
    double bet = cdesc.bet;
    lam += cdesc.lamof/cos(bet);
    bet += cdesc.betof;
    time_t datetime = obssecond(centry.xdobs + 60549L, cdesc.ut);
    // const char *datetime = obstime(centry.xdobs + 60549, cdesc.ut);
    head.id = scan;
    head.scanno = centry.xscan;
    strncpy(head.target, (const char *)centry.xsourc, 12);
    strncpy(head.line, (const char *)centry.xline, 12);
    strncpy(head.instr, (const char *)centry.xtel, 12);
    head.RA = lam*180.0/M_PI;
    head.Dec = bet*180.0/M_PI;
    head.fLO = LO;
    head.f0 = restf;
    head.df = fres;
    head.vs = cdesc.voff;
    head.dt = cdesc.time;
    head.tsys = cdesc.tsys;
    head.utc = datetime;
    return head;
}

std::vector<double> Type2Reader::getFreq(int scan)
{
    std::vector<double> f;
    size_t len = 0;
    int iext = 0;
    unsigned int isize = 0;
    int jent = 0;

    int growth = 1;
    int nspec = 0;
    for (int i = 0; i < fdesc.nex; i++) {
        int nst = fdesc.lex1*growth;
        isize = nst*fdesc.lind;
        for (int k = 0; k < nst; k++) {
            nspec++;
            if (nspec == scan) {
                jent = k;
                iext = i;
                break;
            }
        }
        if (nspec == scan) break;
        if (fdesc.gex == 20) growth *= 2;
    }

    long pos = (ext[iext]-1)*m_reclen;
#ifdef DEBUG
    // printf("in %s spectrum %d at position %ld in extension %d entry %d at word %d\n",
    //         __FUNCTION__, scan, pos, iext, jent, startword);
#endif
    fseek(cfp, 4*pos, SEEK_SET);
    len = fread(buffer, sizeof(int), isize, cfp);
    if (len != isize) {
        fprintf(stderr, "failed to read entry descriptor\n");
        return f;
    }

    getEntry(jent);
    pos = (centry.xblock-1)*m_reclen+centry.xword-1;

    fseek(cfp, 4*pos, SEEK_SET);
#ifdef DEBUG
    printf("in %s reading spectrum %d at block %ld, word %d position %ld (%ld)\n",
            __FUNCTION__, scan, centry.xblock, centry.xword, ftell(cfp), 4*pos);
#endif
    getRecord();
    m_ptr = buffer;

    getChar(csect.ident, 4);
    csect.version = getInt();
    csect.nsec = getInt();
    csect.nword = getLong();
    csect.adata = getLong();
    csect.ldata = getLong();
    csect.xnum = getLong();
    int nsec = csect.nsec;
    if (nsec > 10) nsec = 10;
    for (int i = 0; i < nsec; i++) csect.sec_cod[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_len[i] = getLong();
    for (int i = 0; i < nsec; i++) csect.sec_adr[i] = getLong();
#ifdef DEBUG
    printf("%7d %12s %12s %d %d (%d %d %d %d | %d %d %d %d | %d %d %d %d)\n",
           scan,
           centry.xsourc, centry.xline, centry.xkind,
           csect.nsec,
           csect.sec_cod[0], csect.sec_cod[1], csect.sec_cod[2], csect.sec_cod[3],
           csect.sec_adr[0], csect.sec_adr[1], csect.sec_adr[2], csect.sec_adr[3],
           csect.sec_len[0], csect.sec_len[1], csect.sec_len[2], csect.sec_len[3]);
#endif

    for (int i = 0; i < nsec; i++) {
        unsigned int isize = csect.sec_len[i];
        unsigned int size = 4*isize;
        long secpos = 4*(pos + csect.sec_adr[i]-1);
#ifdef DEBUG
        printf("in %s reading section %d at position %ld (%ld,%ld)\n",
                __FUNCTION__, i, ftell(cfp), pos, csect.sec_adr[i]);
#endif
        int ierr = fseek(cfp, secpos, SEEK_SET);
        if (ierr == -1) {
            fprintf(stderr, "failure to position section %d (%ld)\n", i, secpos);
            break;
        }
        len = fread(buffer, sizeof(char), size, cfp);
        if (len != size) {
            fprintf(stderr, "failed to read section %d (%ld != %ld)\n", i, len, static_cast<size_t>(size));
            break;
        }
        m_ptr = buffer;
#ifdef DEBUG
        printf("Sect[%d]=%3d at adr %d of length %d.\n", i, csect.sec_cod[i], secpos, size);
#endif
        fillHeader(buffer, csect.sec_cod[i], 1, csect.sec_len[i]);
    }

    bool spectrum = (centry.xkind == 0);

    int ndata = 0;
    double restf, rchan, fres;
    if (spectrum) {
        rchan = cdesc.rchan;
        restf = cdesc.restf;
        fres = cdesc.fres;
        ndata = cdesc.nchan;
    } else {
        rchan = cdesc.rpoin;
        restf = cdesc.tref;
        fres = cdesc.tres;
        ndata = cdesc.npoin;
    }
    if (ndata > (int)MAXCHANNELS) {
        fprintf(stderr, "maximum number of channels exceeded: %d %ld\n", ndata, MAXCHANNELS);
    }

    f = freqVector(ndata, restf, rchan, fres);
    return f;
}

std::vector<double> Type2Reader::getData(int scan)
{
    std::vector<double> d;
    size_t len = 0;
    int iext = 0;
    unsigned int isize = 0;
    int jent = 0;

    int growth = 1;
    int nspec = 0;
    for (int i = 0; i < fdesc.nex; i++) {
        int nst = fdesc.lex1*growth;
        isize = nst*fdesc.lind;
        for (int k = 0; k < nst; k++) {
            nspec++;
            if (nspec == scan) {
                jent = k;
                iext = i;
                break;
            }
        }
        if (nspec == scan) break;
        if (fdesc.gex == 20) growth *= 2;
    }

    long pos = (ext[iext]-1)*m_reclen;
#ifdef DEBUG
    // printf("in %s spectrum %d at position %ld in extension %d entry %d at word %d\n",
    //         __FUNCTION__, scan, pos, iext, jent, startword);
#endif
    fseek(cfp, 4*pos, SEEK_SET);
    len = fread(buffer, sizeof(int), isize, cfp);
    if (len != isize) {
        fprintf(stderr, "failed to read entry descriptor\n");
        return d;
    }

    getEntry(jent);
    pos = (centry.xblock-1)*m_reclen+centry.xword-1;

    fseek(cfp, 4*pos, SEEK_SET);
#ifdef DEBUG
    printf("in %s reading spectrum %d at block %ld, word %d position %ld (%ld)\n",
            __FUNCTION__, scan, centry.xblock, centry.xword, ftell(cfp), 4*pos);
#endif
    getRecord();
    m_ptr = buffer;

    getChar(csect.ident, 4);
    csect.version = getInt();
    csect.nsec = getInt();
    csect.nword = getLong();
    csect.adata = getLong();
    csect.ldata = getLong();
    csect.xnum = getLong();
    int nsec = csect.nsec;
    if (nsec > 10) nsec = 10;
    for (int i = 0; i < nsec; i++) csect.sec_cod[i] = getInt();
    for (int i = 0; i < nsec; i++) csect.sec_len[i] = getLong();
    for (int i = 0; i < nsec; i++) csect.sec_adr[i] = getLong();
#ifdef DEBUG
    printf("%7d %12s %12s %d %d (%d %d %d %d | %d %d %d %d | %d %d %d %d)\n",
           scan,
           centry.xsourc, centry.xline, centry.xkind,
           csect.nsec,
           csect.sec_cod[0], csect.sec_cod[1], csect.sec_cod[2], csect.sec_cod[3],
           csect.sec_adr[0], csect.sec_adr[1], csect.sec_adr[2], csect.sec_adr[3],
           csect.sec_len[0], csect.sec_len[1], csect.sec_len[2], csect.sec_len[3]);
#endif

    for (int i = 0; i < nsec; i++) {
        unsigned int isize = csect.sec_len[i];
        unsigned int size = 4*isize;
        long secpos = 4*(pos + csect.sec_adr[i]-1);
#ifdef DEBUG
        printf("in %s reading section %d at position %ld (%ld,%ld)\n",
                __FUNCTION__, i, ftell(cfp), pos, csect.sec_adr[i]);
#endif
        int ierr = fseek(cfp, secpos, SEEK_SET);
        if (ierr == -1) {
            fprintf(stderr, "failure to position section %d (%ld)\n", i, secpos);
            break;
        }
        len = fread(buffer, sizeof(char), size, cfp);
        if (len != size) {
            fprintf(stderr, "failed to read section %d (%ld != %ld)\n", i, len, static_cast<size_t>(size));
            break;
        }
        m_ptr = buffer;
#ifdef DEBUG
        printf("Sect[%d]=%3d at adr %d of length %d.\n", i, csect.sec_cod[i], secpos, size);
#endif
        fillHeader(buffer, csect.sec_cod[i], 1, csect.sec_len[i]);
    }

    bool spectrum = (centry.xkind == 0);

    isize = csect.ldata;
    char *datatbl = buffer;
    if (4*isize > sizeof(buffer)) {
        fprintf(stderr, "buffer too small!");
    }
    long datapos = 4*(pos + csect.adata-1);
    fseek(cfp, datapos, SEEK_SET);
#ifdef DEBUG
    printf("in %s reading data for spectrum %d at position %ld (%ld,%ld)\n",
           __FUNCTION__, scan, ftell(cfp), pos, csect.adata);
#endif
    len = fread(datatbl, sizeof(int), isize, cfp);
    if (len != isize) {
        fprintf(stderr, "failed to read data block\n");
    }

    int ndata = 0;
    if (spectrum) ndata = cdesc.nchan;
    else          ndata = cdesc.npoin;

    if (ndata > (int)MAXCHANNELS) {
        fprintf(stderr, "maximum number of channels exceeded: %d %ld\n", ndata, MAXCHANNELS);
    }

    float *s = (float *)(datatbl);
    d = dataVector(ndata, s);
    return d;
}
