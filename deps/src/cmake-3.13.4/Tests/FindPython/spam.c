
#include <Python.h>

static PyObject* spam_system(PyObject* self, PyObject* args)
{
  const char* command;
  int sts;

  if (!PyArg_ParseTuple(args, "s", &command))
    return NULL;
  sts = system(command);
  /* return PyLong_FromLong(sts); */
  return Py_BuildValue("i", sts);
}

static PyMethodDef SpamMethods[] = {
  { "system", spam_system, METH_VARARGS, "Execute a shell command." },
  { NULL, NULL, 0, NULL } /* Sentinel */
};

#if defined(PYTHON2)
PyMODINIT_FUNC initspam2(void)
{
  (void)Py_InitModule("spam2", SpamMethods);
}
#endif

#if defined(PYTHON3)
static struct PyModuleDef spammodule = {
  PyModuleDef_HEAD_INIT, "spam3", /* name of module */
  NULL,                           /* module documentation, may be NULL */
  -1, /* size of per-interpreter state of the module,
         or -1 if the module keeps state in global variables. */
  SpamMethods
};

PyMODINIT_FUNC PyInit_spam3(void)
{
  return PyModule_Create(&spammodule);
}
#endif
