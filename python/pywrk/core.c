#include <Python.h>
#include "ceval.h"
#include "longobject.h"
#include "methodobject.h"
#include <pytypedefs.h>
#include <stdio.h>
#include <inttypes.h>
#include "wrk.h"


PyObject *method_benchmark(PyObject* self, PyObject* args, PyObject* kwargs) {
    Py_BEGIN_ALLOW_THREADS

    wrk_cfg.host = NULL;
    wrk_cfg.connections = 2;
    wrk_cfg.duration = 10;
    wrk_cfg.timeout = 3000;
    wrk_cfg.threads = 12;
    wrk_request = "GET / HTTP/1.1\nHost: google.com\r\n\r\n";

    static char *kwlist[] = {"host", "connections", "duration", "timeout", "timeout", "threads", "http_message", NULL};

    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s|iiiis", kwlist, 
                                     &wrk_cfg.host, &wrk_cfg.connections, &wrk_cfg.duration, &wrk_cfg.timeout, &wrk_cfg.threads, &wrk_request)) 
    {
        return PyLong_FromLong(1);
    }

    printf("Host %s\n", wrk_cfg.host);
    printf("connections %d\n", wrk_cfg.connections);
    printf("duration %d\n", wrk_cfg.duration);
    printf("timeout %d\n", wrk_cfg.timeout);
    printf("threads %d\n", wrk_cfg.threads);
    printf("msg %s\n", wrk_request);

    benchmark(wrk_cfg.host);

    printf("Complete: %" PRIu64 "\n", wrk_complete);
    printf("Error read: %" PRIu32 "\n", wrk_errors.read);
    printf("Error write: %" PRIu32 "\n", wrk_errors.write);
    printf("Error timeout: %" PRIu32 "\n", wrk_errors.timeout);
    printf("Error connect: %" PRIu32 "\n", wrk_errors.connect);
    printf("Error status: %" PRIu32 "\n", wrk_errors.status);
    
    Py_END_ALLOW_THREADS

    return PyLong_FromLong(0);
}

PyMethodDef methods[] = {
    {"benchmark", (PyCFunction) method_benchmark, METH_VARARGS | METH_KEYWORDS, "Python interface for fputs C library function"},
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
