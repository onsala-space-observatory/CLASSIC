/* Copyright 2017 Michael Olberg <michael.olberg@chalmers.se> */

#define NPY_NO_DEPRECATED_API NPY_1_7_API_VERSION

#include <Python.h>
#include <structmember.h>
#include <numpy/arrayobject.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fstream>

#include "class.h"

typedef struct {
    PyObject_HEAD
    ClassReader *reader;
    PyObject *filename;
    int count;
} Reader;

static PyObject* getDirectory(Reader* self)
{
    int nscans = 0;
    if (self->reader) {
        ClassReader *reader = self->reader;
        nscans = reader->getDirectory();
        self->count = nscans;
    }
    return Py_BuildValue("i", nscans);
}

static PyObject* getHead(Reader* self, PyObject *args)
{
    int iscan = 1;
    if (!PyArg_ParseTuple(args, "i:getHead", &iscan)) return NULL;

    if (iscan < 1 || iscan > self->count) {
        PyErr_SetString(PyExc_IOError, "scan number out of range");
        return NULL;
    }
    if (self->reader) {
        ClassReader *reader = self->reader;
        SpectrumHeader S = reader->getHead(iscan);
        // S.print();
        static char fmt[] = "0000-00-00 00:00:00"; // %Y-%m-%d %H:%M:%S
        strftime(fmt, sizeof(fmt), "%Y-%m-%d %H:%M:%S", gmtime(&S.utc));

        return Py_BuildValue("{s:i,s:i,s:s,s:s,s:s,s:d,s:d,s:d,s:d,s:d,s:d,s:d,s:d,s:s}",
                             "id", S.id,
                             "scanno", S.scanno,
                             "target", S.target,
                             "line", S.line,
                             "instr", S.instr,
                             "RA", S.RA,
                             "Dec", S.Dec,
                             "fLO", S.fLO,
                             "f0", S.f0,
                             "df", S.df,
                             "vs", S.vs,
                             "dt", S.dt,
                             "tsys", S.tsys,
                             "utc", fmt);
    }
    Py_RETURN_NONE;
}

static PyObject* getFreq(Reader* self, PyObject *args)
{
    int iscan = 1;
    if (!PyArg_ParseTuple(args, "i:getHead", &iscan)) return NULL;

    if (iscan < 1 || iscan > self->count) {
        PyErr_SetString(PyExc_IOError, "scan number out of range");
        return NULL;
    }
    if (self->reader) {
        ClassReader *reader = self->reader;
        std::vector<double> f = reader->getFreq(iscan);
        PyObject *array;

        long int dims[] = { 0 };
        dims[0] = f.size();
        array = PyArray_SimpleNew(1, dims, NPY_DOUBLE);
        double *data = (double *)PyArray_GETPTR1((PyArrayObject *)array, 0);
        for (int i = 0; i < dims[0]; i++) {
            data[i] = f[i];
        }
        return array;
    }

    Py_RETURN_NONE;
}

static PyObject* getData(Reader* self, PyObject *args)
{
    int iscan = 1;
    if (!PyArg_ParseTuple(args, "i:getHead", &iscan)) return NULL;

    if (iscan < 1 || iscan > self->count) {
        PyErr_SetString(PyExc_IOError, "scan number out of range");
        return NULL;
    }
    if (self->reader) {
        ClassReader *reader = self->reader;
        std::vector<double> d = reader->getData(iscan);
        PyObject *array;

        long int dims[] = { 0 };
        dims[0] = d.size();
        array = PyArray_SimpleNew(1, dims, NPY_DOUBLE);
        double *data = (double *)PyArray_GETPTR1((PyArrayObject *)array, 0);
        for (int i = 0; i < dims[0]; i++) {
            data[i] = d[i];
        }
        return array;
    }

    Py_RETURN_NONE;
}

static void class_dealloc(Reader* self)
{
    if (self->reader) delete self->reader;
    Py_XDECREF(self->filename);
    Py_TYPE(self)->tp_free((PyObject*)self);
}

static PyObject *Reader_new(PyTypeObject *type, PyObject *args, PyObject *kwds)
{
    Reader *self;

    self = (Reader *)type->tp_alloc(type, 0);
    printf("new Reader ... %p\n", self);
    if (self != NULL) {
        self->filename = PyUnicode_FromString("");
        if (self->filename == NULL) {
            Py_DECREF(self);
            return NULL;
        }
        self->count = 0;
    }
    return (PyObject *)self;
}

static int Reader_init(Reader *self, PyObject *args, PyObject *kwds)
{
    PyObject *filename = NULL, *tmp;

    static char *kwlist[] = {"filename", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwds, "O", kwlist, &filename)) return -1;

    if (self != NULL) {
        if (filename) {
            tmp = self->filename;
            Py_INCREF(filename);
            self->filename = filename;
            Py_XDECREF(tmp);
        }
        self->count = 0;
    }
    const char *name = PyUnicode_AsUTF8(self->filename);
    // printf("got filename: '%s'\n", name);
    struct stat buffer;
    if (stat(name, &buffer) == 0) self->reader = openClassFile(name);
    else                          self->reader = 0;

    return 0;
}

static PyMemberDef Reader_members[] = {
    {"filename", T_OBJECT_EX, offsetof(Reader, filename), 0,
     "filename"},
    {"count",    T_INT,       offsetof(Reader, count),    0,
     "count"},
    {NULL}  /* Sentinel */
};

static PyMethodDef Reader_methods[] = {
    {"getDirectory", (PyCFunction)getDirectory, METH_NOARGS, "get number of spectra in file" },
    {"getHead", (PyCFunction)getHead, METH_VARARGS, "get header of spectrum" },
    {"getFreq", (PyCFunction)getFreq, METH_VARARGS, "get frequency vector of spectrum" },
    {"getData", (PyCFunction)getData, METH_VARARGS, "get data vector of spectrum" },
    {NULL}  /* Sentinel */
};

static PyTypeObject ClassicReaderType = {
    PyObject_HEAD_INIT(NULL)
    "classic.Reader",          /* tp_name */
    sizeof(Reader),            /* tp_basicsize */
    0,                         /* tp_itemsize */
    (destructor)class_dealloc, /* tp_dealloc */
    0,                         /* tp_print */
    0,                         /* tp_getattr */
    0,                         /* tp_setattr */
    0,                         /* tp_reserved */
    0,                         /* tp_repr */
    0,                         /* tp_as_number */
    0,                         /* tp_as_sequence */
    0,                         /* tp_as_mapping */
    0,                         /* tp_hash  */
    0,                         /* tp_call */
    0,                         /* tp_str */
    0,                         /* tp_getattro */
    0,                         /* tp_setattro */
    0,                         /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT,        /* tp_flags */
    "CLASSIC file reader object", /* tp_doc */
    0,		               /* tp_traverse */
    0,		               /* tp_clear */
    0,		               /* tp_richcompare */
    0,		               /* tp_weaklistoffset */
    0,		               /* tp_iter */
    0,		               /* tp_iternext */
    Reader_methods,            /* tp_methods */
    Reader_members,            /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Reader_init,     /* tp_init */
    0,                         /* tp_alloc */
    // Reader_new,                /* tp_new */
};

static PyObject* py_iarray(PyObject* self)
{
    int *data;
    long int dims[] = { 10 };
    PyObject *array;

    array = PyArray_SimpleNew(1, dims, NPY_INT32);
    data = (int *)PyArray_GETPTR1((PyArrayObject *)array, 0);
    for (int i = 0; i < 10; i++) {
        data[i] = i;
    }

    return array;
}

static PyObject* py_barray(PyObject* self)
{
    bool *data;
    long int dims[] = { 10 };
    PyObject *array;

    array = PyArray_SimpleNew(1, dims, NPY_BOOL);
    data = (bool *)PyArray_GETPTR1((PyArrayObject *)array, 0);
    for (int i = 0; i < 10; i++) {
        data[i] = (i%2) == 0;
    }

    return array;
}

static PyObject* py_darray(PyObject* self)
{
    double *data;
    long int dims[] = { 10 };
    PyObject *array;

    array = PyArray_SimpleNew(1, dims, NPY_DOUBLE);
    data = (double *)PyArray_GETPTR1((PyArrayObject *)array, 0);
    for (int i = 0; i < 10; i++) {
        data[i] = (double)i;
    }

    return array;
}

static PyObject* py_sarray(PyObject* self)
{
    char *data;
    long int dims[] = { 10 };
    PyObject *op, *array;
    PyArray_Descr *descr;

    op = Py_BuildValue("s", "S10");
    PyArray_DescrConverter(op, &descr);
    Py_DECREF(op);
    array = PyArray_SimpleNewFromDescr(1, dims, descr);
    data = (char *)PyArray_GETPTR1((PyArrayObject *)array, 0);
    memset(data, 0, 100);
    strncpy(data, "hello", 5);
    data = (char *)PyArray_GETPTR1((PyArrayObject *)array, 1);
    strncpy(data, "world", 5);
    data = (char *)PyArray_GETPTR1((PyArrayObject *)array, 2);
    strncpy(data, "helloworld", 10);

    return array;
}

static PyObject* py_version(PyObject* self)
{
    PyObject *version = Py_BuildValue("s", "Version 0.2");
    return version;
}

static PyMethodDef classicMethods[] = {
    {"newReader", (PyCFunction)Reader_new, METH_VARARGS, "open CLASSIC file" },
    {"intarray", (PyCFunction)py_iarray, METH_NOARGS, "Build integer array from scratch."},
    {"logarray", (PyCFunction)py_barray, METH_NOARGS, "Build boolean array from scratch."},
    {"dblarray", (PyCFunction)py_darray, METH_NOARGS, "Build double array from scratch."},
    {"strarray", (PyCFunction)py_sarray, METH_NOARGS, "Build string array from scratch."},
    {"version",  (PyCFunction)py_version, METH_NOARGS, "Returns the version."},
    {NULL, NULL, 0, NULL}
};

static struct PyModuleDef classic = {
    PyModuleDef_HEAD_INIT,
    "classic",                                  // name of module.
    "Interface to CLASSIC data container",      // doc string
    -1,
    classicMethods
};

PyMODINIT_FUNC PyInit_classic(void)
{
    import_array();
    ClassicReaderType.tp_new = PyType_GenericNew;
    if (PyType_Ready(&ClassicReaderType) < 0) return NULL;

    PyObject *m = PyModule_Create(&classic);
    if (m == NULL) return NULL;

    Py_INCREF(&ClassicReaderType);
    PyModule_AddObject(m, "Reader", (PyObject *)&ClassicReaderType);
    return m;
}
