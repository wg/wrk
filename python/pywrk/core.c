#include <Python.h>
#include "methodobject.h"
#include <pytypedefs.h>
#include <stdio.h>
#include "wrk.h"


PyObject *method_run(PyObject* self, PyObject* args) {
    printf("%x\n", &benchmark);
    return PyLong_FromLong(0);
}

PyMethodDef methods[] = {
    {"run", method_run, METH_VARARGS, "Python interface for fputs C library function"},
    {NULL, NULL, 0, NULL}
};

struct PyModuleDef module_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "core",
    .m_doc = "wrk binding for Python",
    .m_methods = methods,
};

PyMODINIT_FUNC PyInit_core(void) {
    return PyModule_Create(&module_def);
}
