#include <Python.h>
#include <moduleobject.h>
#include <modsupport.h>
#include <pyport.h>

struct PyModuleDef module_def = {
    .m_base = PyModuleDef_HEAD_INIT,
    .m_name = "core",
    .m_doc = "wrk binding for Python",
    .m_methods = NULL,
};

PyMODINIT_FUNC PyInit_core(void) {
    return PyModule_Create(&module_def);
}
