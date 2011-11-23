/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of beamb.

beamb is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

beamb is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with beamb.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/
/**
 * Python API to the beam blockage functions
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-11-14
 */
#include "Python.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PYBEAMBLOCKAGE_MODULE   /**< to get correct part in pybeamblockage */
#include "pybeamblockage.h"

#include "pypolarscan.h"
#include "pyravefield.h"
#include "pyrave_debug.h"
#include "rave_alloc.h"

/**
 * Debug this module
 */
PYRAVE_DEBUG_MODULE("_beamblockage");

/**
 * Sets a python exception and goto tag
 */
#define raiseException_gotoTag(tag, type, msg) \
{PyErr_SetString(type, msg); goto tag;}

/**
 * Sets python exception and returns NULL
 */
#define raiseException_returnNULL(type, msg) \
{PyErr_SetString(type, msg); return NULL;}

/**
 * Error object for reporting errors to the python interpreeter
 */
static PyObject *ErrorObject;

/// --------------------------------------------------------------------
/// BeamBlockage
/// --------------------------------------------------------------------
/*@{ BeamBlockage */
/**
 * Returns the native BeamBlockage_t instance.
 * @param[in] beamb - the python beam blockage instance
 * @returns the native BeamBlockage_t instance.
 */
static BeamBlockage_t*
PyBeamBlockage_GetNative(PyBeamBlockage* beamb)
{
  RAVE_ASSERT((beamb != NULL), "beamb == NULL");
  return RAVE_OBJECT_COPY(beamb->beamb);
}

/**
 * Creates a python beam blockage from a native beam blockage or will create an
 * initial native beam blockage if p is NULL.
 * @param[in] p - the native beam blockage (or NULL)
 * @param[in] beamb- the beam blockage (only used if p != NULL)
 * @returns the python beam blockage.
 */
static PyBeamBlockage* PyBeamBlockage_New(BeamBlockage_t* p)
{
  PyBeamBlockage* result = NULL;
  BeamBlockage_t* cp = NULL;

  if (p == NULL) {
    cp = RAVE_OBJECT_NEW(&BeamBlockage_TYPE);
    if (cp == NULL) {
      RAVE_CRITICAL0("Failed to allocate memory for beam blockage.");
      raiseException_returnNULL(PyExc_MemoryError, "Failed to allocate memory for beam blockage.");
    }
  } else {
    cp = RAVE_OBJECT_COPY(p);
    result = RAVE_OBJECT_GETBINDING(p); // If p already have a binding, then this should only be increfed.
    if (result != NULL) {
      Py_INCREF(result);
    }
  }

  if (result == NULL) {
    result = PyObject_NEW(PyBeamBlockage, &PyBeamBlockage_Type);
    if (result != NULL) {
      PYRAVE_DEBUG_OBJECT_CREATED;
      result->beamb = RAVE_OBJECT_COPY(cp);
      RAVE_OBJECT_BIND(result->beamb, result);
    } else {
      RAVE_CRITICAL0("Failed to create PyBeamBlockage instance");
      raiseException_gotoTag(done, PyExc_MemoryError, "Failed to allocate memory for beam blockage.");
    }
  }
done:
  RAVE_OBJECT_RELEASE(cp);
  return result;
}

/**
 * Deallocates the beam blockage
 * @param[in] obj the object to deallocate.
 */
static void _pybeamblockage_dealloc(PyBeamBlockage* obj)
{
  /*Nothing yet*/
  if (obj == NULL) {
    return;
  }
  PYRAVE_DEBUG_OBJECT_DESTROYED;
  RAVE_OBJECT_UNBIND(obj->beamb, obj);
  RAVE_OBJECT_RELEASE(obj->beamb);
  PyObject_Del(obj);
}

/**
 * Creates a new instance of the beam blockage
 * @param[in] self this instance.
 * @param[in] args arguments for creation or a beam blockage
 * @return the object on success, otherwise NULL
 */
static PyObject* _pybeamblockage_new(PyObject* self, PyObject* args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  return (PyObject*)PyBeamBlockage_New(NULL);
}

/**
 * Returns the blockage for the provided scan given gaussian limit.
 * @param[in] self - self
 * @param[in] args - the arguments (PyPolarScan, double (Limit of Gaussian approximation of main lobe))
 * @return the PyRaveField on success otherwise NULL
 */
static PyObject* _pybeamblockage_getBlockage(PyBeamBlockage* self, PyObject* args)
{
  PyObject* pyin = NULL;
  double dBlim = 0;
  RaveField_t* field = NULL;
  PyObject* result = NULL;

  if (!PyArg_ParseTuple(args, "Od", &pyin, &dBlim)) {
    return NULL;
  }

  if (!PyPolarScan_Check(pyin)) {
    raiseException_returnNULL(PyExc_ValueError, "First argument should be a Polar Scan");
  }

  field = BeamBlockage_getBlockage(self->beamb, ((PyPolarScan*)pyin)->scan, dBlim);
  if (field != NULL) {
    result = (PyObject*)PyRaveField_New(field);
  }
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/**
 * All methods a ropo generator can have
 */
static struct PyMethodDef _pybeamblockage_methods[] =
{
  {"topo30dir", NULL},
  {"getBlockage", (PyCFunction)_pybeamblockage_getBlockage, 1},
  {NULL, NULL} /* sentinel */
};

/**
 * Returns the specified attribute in the beam blockage
 */
static PyObject* _pybeamblockage_getattr(PyBeamBlockage* self, char* name)
{
  PyObject* res = NULL;

  if (strcmp("topo30dir", name) == 0) {
    const char* str = BeamBlockage_getTopo30Directory(self->beamb);
    if (str != NULL) {
      return PyString_FromString(str);
    } else {
      Py_RETURN_NONE;
    }
  }

  res = Py_FindMethod(_pybeamblockage_methods, (PyObject*) self, name);
  if (res)
    return res;

  PyErr_Clear();
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

/**
 * Returns the specified attribute in the polar volume
 */
static int _pybeamblockage_setattr(PyBeamBlockage* self, char* name, PyObject* val)
{
  int result = -1;
  if (name == NULL) {
    goto done;
  }

  if (strcmp("topo30dir", name) == 0) {
    if (PyString_Check(val)) {
      if (!BeamBlockage_setTopo30Directory(self->beamb, PyString_AsString(val))) {
        raiseException_gotoTag(done, PyExc_ValueError, "topo30dir must be a string or None");
      }
    } else if (val == Py_None) {
      BeamBlockage_setTopo30Directory(self->beamb, NULL);
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "topo30dir must be a string or None");
    }
  } else {
    raiseException_gotoTag(done, PyExc_AttributeError, name);
  }

  result = 0;
done:
  return result;
}

/*@} End of Fmi Image */

/// --------------------------------------------------------------------
/// Type definitions
/// --------------------------------------------------------------------
/*@{ Type definitions */
PyTypeObject PyBeamBlockage_Type =
{
  PyObject_HEAD_INIT(NULL)0, /*ob_size*/
  "BeamBlockageCore", /*tp_name*/
  sizeof(PyBeamBlockage), /*tp_size*/
  0, /*tp_itemsize*/
  /* methods */
  (destructor)_pybeamblockage_dealloc, /*tp_dealloc*/
  0, /*tp_print*/
  (getattrfunc)_pybeamblockage_getattr, /*tp_getattr*/
  (setattrfunc)_pybeamblockage_setattr, /*tp_setattr*/
  0, /*tp_compare*/
  0, /*tp_repr*/
  0, /*tp_as_number */
  0,
  0, /*tp_as_mapping */
  0 /*tp_hash*/
};
/*@} End of Type definitions */

/*@{ Functions */

/*@} End of Functions */

/*@{ Module setup */
static PyMethodDef functions[] = {
  {"new", (PyCFunction)_pybeamblockage_new, 1},
  {NULL,NULL} /*Sentinel*/
};

PyMODINIT_FUNC
init_beamblockage(void)
{
  PyObject *module=NULL,*dictionary=NULL;
  static void *PyBeamBlockage_API[PyBeamBlockage_API_pointers];
  PyObject *c_api_object = NULL;
  PyBeamBlockage_Type.ob_type = &PyType_Type;

  module = Py_InitModule("_beamblockage", functions);
  if (module == NULL) {
    return;
  }
  PyBeamBlockage_API[PyBeamBlockage_Type_NUM] = (void*)&PyBeamBlockage_Type;
  PyBeamBlockage_API[PyBeamBlockage_GetNative_NUM] = (void *)PyBeamBlockage_GetNative;
  PyBeamBlockage_API[PyBeamBlockage_New_NUM] = (void*)PyBeamBlockage_New;

  c_api_object = PyCObject_FromVoidPtr((void *)PyBeamBlockage_API, NULL);

  if (c_api_object != NULL) {
    PyModule_AddObject(module, "_C_API", c_api_object);
  }

  dictionary = PyModule_GetDict(module);
  ErrorObject = PyString_FromString("_beamblockage.error");

  if (ErrorObject == NULL || PyDict_SetItemString(dictionary, "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _beamblockage.error");
  }
  import_pyravefield();
  import_pypolarscan();
  PYRAVE_DEBUG_INITIALIZE;
}
/*@} End of Module setup */
