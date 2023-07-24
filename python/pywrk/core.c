#include <Python.h>
#include "wrk.h"
#include "stats.h"


PyObject *pywrk_benchmark(PyObject* self, PyObject* args, PyObject* kwargs) {
    wrk_cfg.host = NULL;
    wrk_cfg.connections = 2;
    wrk_cfg.duration = 3;
    wrk_cfg.timeout = 3000;
    wrk_cfg.threads = 12;
    wrk_request = NULL;
    PyObject* py_stat_latency_percentile = NULL;

    static char *kwlist[] = {
        "host",
        "connections",
        "duration",
        "timeout",
        "threads",
        "http_message",
        "stat_latency_percentile",
        NULL // sentinel value
    };

    if (!PyArg_ParseTupleAndKeywords(
        args,
        kwargs,
        "s|iiiisO",
        kwlist, 
        &wrk_cfg.host,
        &wrk_cfg.connections, 
        &wrk_cfg.duration,
        &wrk_cfg.timeout,
        &wrk_cfg.threads,
        &wrk_request,
        &py_stat_latency_percentile
    )) {
        return PyLong_FromLong(1);
    }

    Py_BEGIN_ALLOW_THREADS
    benchmark(wrk_cfg.host);
    Py_END_ALLOW_THREADS

    PyObject* py_errors = Py_BuildValue("{s:i,s:i,s:i,s:i,s:i}", 
        "read", wrk_errors.read,
        "write", wrk_errors.write,
        "connect", wrk_errors.connect,
        "timeout", wrk_errors.timeout,
        "status", wrk_errors.status
    );
    Py_XINCREF(py_errors);

    PyObject* py_latency = Py_BuildValue("{s:i,s:i,s:i,s:i}",
        "count", wrk_statistics.latency->count,
        "limit", wrk_statistics.latency->limit,
        "min", wrk_statistics.latency->min,
        "max", wrk_statistics.latency->max
    );
    Py_XINCREF(py_latency);

    PyObject* py_requests = Py_BuildValue("{s:i,s:i,s:i,s:i}",
        "count", wrk_statistics.requests->count,
        "limit", wrk_statistics.requests->limit,
        "min", wrk_statistics.requests->min,
        "max", wrk_statistics.requests->max
    );
    Py_XINCREF(py_requests);

    PyObject* py_ttfb = Py_BuildValue("{s:i,s:i,s:i,s:i}",
        "count", wrk_statistics.ttfb->count,
        "limit", wrk_statistics.ttfb->limit,
        "min", wrk_statistics.ttfb->min,
        "max", wrk_statistics.ttfb->max
    );
    Py_XINCREF(py_ttfb);

    PyObject* latency_percentile = Py_None;
    if (py_stat_latency_percentile != NULL && py_stat_latency_percentile != Py_None) {
        latency_percentile = PyList_New(0);
        int n_percentile = PyList_GET_SIZE(py_stat_latency_percentile);
        for (int i = 0; i < n_percentile; ++i) {
            PyObject* item = PyList_GetItem(py_stat_latency_percentile, i);
            if (PyFloat_Check(item)) {
                PyList_Insert(latency_percentile, i, PyLong_FromLong(
                    stats_percentile(wrk_statistics.latency, PyFloat_AsDouble(item))
                ));
            }
            else {
                PyList_Insert(latency_percentile, i, PyLong_FromLong(0));
            }
        }
    }
    Py_INCREF(latency_percentile);


    PyObject* stats = Py_BuildValue("{s:O,s:O,s:O,s:O}",
        "latency", py_latency,
        "requests", py_requests,
        "ttfb", py_ttfb,
        "latency_percentile", latency_percentile
    );
    Py_XINCREF(stats);

    PyObject* result = Py_BuildValue("{s:i,s:i,s:i,s:O,s:O}",
        "completed_requests", wrk_complete,
        "completed_bytes", wrk_bytes,
        "runtime_us", wrk_runtime_us,
        "errors", py_errors,
        "stats", stats
    );
    Py_XINCREF(result);
    return result;
}

PyMethodDef methods[] = {
    {"benchmark", (PyCFunction) pywrk_benchmark, METH_VARARGS | METH_KEYWORDS, "Python interface for wrk"},
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
