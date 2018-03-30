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
#include "pybeamb_compat.h"
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
 * Restores the provided scan with the beam blockage field.
 * @param[in] self - this instance
 * @param[in] args - (OOsf), (scan, field, quantity, threshold value)
 * @returns None
 */
static PyObject* _pybeamblockage_restore(PyObject* self, PyObject* args)
{
  PyObject *o1 = NULL, *o2 = NULL;
  char* quantity = NULL;
  double threshold = 0.0;

  if (!PyArg_ParseTuple(args, "OOsd", &o1, &o2, &quantity, &threshold)) {
    return NULL;
  }

  if (!PyPolarScan_Check(o1)) {
    raiseException_returnNULL(PyExc_TypeError, "First argument should be a PolarScan");
  }
  if (!PyRaveField_Check(o2)) {
    raiseException_returnNULL(PyExc_TypeError, "Second argument should be a PolarScan");
  }

  if (!BeamBlockage_restore(((PyPolarScan*)o1)->scan, ((PyRaveField*)o2)->field, quantity, threshold)) {
    raiseException_returnNULL(PyExc_RuntimeError, "Failed to restore scan");
  }
  Py_RETURN_NONE;
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
  {"cachedir", NULL},
  {"rewritecache", NULL},
  {"getBlockage", (PyCFunction)_pybeamblockage_getBlockage, 1},
  {NULL, NULL} /* sentinel */
};

/**
 * Returns the specified attribute in the beam blockage
 */
static PyObject* _pybeamblockage_getattro(PyBeamBlockage* self, PyObject* name)
{
  if (PY_COMPARE_STRING_WITH_ATTRO_NAME("topo30dir", name) == 0) {
    const char* str = BeamBlockage_getTopo30Directory(self->beamb);
    if (str != NULL) {
      return PyString_FromString(str);
    } else {
      Py_RETURN_NONE;
    }
  } else if (PY_COMPARE_STRING_WITH_ATTRO_NAME("cachedir", name) == 0) {
    const char* str = BeamBlockage_getCacheDirectory(self->beamb);
    if (str != NULL) {
      return PyString_FromString(str);
    } else {
      Py_RETURN_NONE;
    }
  } else if (PY_COMPARE_STRING_WITH_ATTRO_NAME("rewritecache", name) == 0) {
    int val = BeamBlockage_getRewriteCache(self->beamb);
    return PyBool_FromLong(val);
  }
  return PyObject_GenericGetAttr((PyObject*)self, name);
}

/**
 * Returns the specified attribute in the polar volume
 */
static int _pybeamblockage_setattro(PyBeamBlockage* self, PyObject* name, PyObject* val)
{
  int result = -1;
  if (name == NULL) {
    goto done;
  }

  if (PY_COMPARE_STRING_WITH_ATTRO_NAME("topo30dir", name) == 0) {
    if (PyString_Check(val)) {
      if (!BeamBlockage_setTopo30Directory(self->beamb, PyString_AsString(val))) {
        raiseException_gotoTag(done, PyExc_ValueError, "topo30dir must be a string or None");
      }
    } else if (val == Py_None) {
      BeamBlockage_setTopo30Directory(self->beamb, NULL);
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "topo30dir must be a string or None");
    }
  } else if (PY_COMPARE_STRING_WITH_ATTRO_NAME("cachedir", name) == 0) {
    if (PyString_Check(val)) {
      if (!BeamBlockage_setCacheDirectory(self->beamb, PyString_AsString(val))) {
        raiseException_gotoTag(done, PyExc_ValueError, "cachedir must be a string or None");
      }
    } else if (val == Py_None) {
      BeamBlockage_setCacheDirectory(self->beamb, NULL);
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "cachedir must be a string or None");
    }
  } else if (PY_COMPARE_STRING_WITH_ATTRO_NAME("rewritecache", name) == 0) {
    if (PyBool_Check(val)) {
      BeamBlockage_setRewriteCache(self->beamb, val == Py_True?1:0);
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "rewritecache must be a boolean");
    }
  } else {
    raiseException_gotoTag(done, PyExc_AttributeError, PY_RAVE_ATTRO_NAME_TO_STRING(name));
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
  PyVarObject_HEAD_INIT(NULL, 0) /*ob_size*/
  "BeamBlockageCore", /*tp_name*/
  sizeof(PyBeamBlockage), /*tp_size*/
  0, /*tp_itemsize*/
  /* methods */
  (destructor)_pybeamblockage_dealloc, /*tp_dealloc*/
  0, /*tp_print*/
  (getattrfunc)0,               /*tp_getattr*/
  (setattrfunc)0,               /*tp_setattr*/
  0,                            /*tp_compare*/
  0,                            /*tp_repr*/
  0,                            /*tp_as_number */
  0,
  0,                            /*tp_as_mapping */
  0,                            /*tp_hash*/
  (ternaryfunc)0,               /*tp_call*/
  (reprfunc)0,                  /*tp_str*/
  (getattrofunc)_pybeamblockage_getattro, /*tp_getattro*/
  (setattrofunc)_pybeamblockage_setattro, /*tp_setattro*/
  0,                            /*tp_as_buffer*/
  Py_TPFLAGS_DEFAULT, /*tp_flags*/
  0,                            /*tp_doc*/
  (traverseproc)0,              /*tp_traverse*/
  (inquiry)0,                   /*tp_clear*/
  0,                            /*tp_richcompare*/
  0,                            /*tp_weaklistoffset*/
  0,                            /*tp_iter*/
  0,                            /*tp_iternext*/
  _pybeamblockage_methods,              /*tp_methods*/
  0,                            /*tp_members*/
  0,                            /*tp_getset*/
  0,                            /*tp_base*/
  0,                            /*tp_dict*/
  0,                            /*tp_descr_get*/
  0,                            /*tp_descr_set*/
  0,                            /*tp_dictoffset*/
  0,                            /*tp_init*/
  0,                            /*tp_alloc*/
  0,                            /*tp_new*/
  0,                            /*tp_free*/
  0,                            /*tp_is_gc*/
};
/*@} End of Type definitions */

/*@{ Functions */

/*@} End of Functions */

/*@{ Module setup */
static PyMethodDef functions[] = {
  {"new", (PyCFunction)_pybeamblockage_new, 1},
  {"restore", (PyCFunction)_pybeamblockage_restore, 1},
  {NULL,NULL} /*Sentinel*/
};

MOD_INIT(_beamblockage)
{
  PyObject *module=NULL,*dictionary=NULL;
  static void *PyBeamBlockage_API[PyBeamBlockage_API_pointers];
  PyObject *c_api_object = NULL;

  MOD_INIT_SETUP_TYPE(PyBeamBlockage_Type, &PyType_Type);

  MOD_INIT_VERIFY_TYPE_READY(&PyBeamBlockage_Type);

  MOD_INIT_DEF(module, "_beamblockage", NULL/*doc*/, functions);
  if (module == NULL) {
    return MOD_INIT_ERROR;
  }

  PyBeamBlockage_API[PyBeamBlockage_Type_NUM] = (void*)&PyBeamBlockage_Type;
  PyBeamBlockage_API[PyBeamBlockage_GetNative_NUM] = (void *)PyBeamBlockage_GetNative;
  PyBeamBlockage_API[PyBeamBlockage_New_NUM] = (void*)PyBeamBlockage_New;

  c_api_object = PyCapsule_New(PyBeamBlockage_API, PyBeamBlockage_CAPSULE_NAME, NULL);
  dictionary = PyModule_GetDict(module);
  PyDict_SetItemString(dictionary, "_C_API", c_api_object);

  ErrorObject = PyErr_NewException("_beamblockage.error", NULL, NULL);
  if (ErrorObject == NULL || PyDict_SetItemString(dictionary, "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _beamblockage.error");
    return MOD_INIT_ERROR;
  }

  import_pyravefield();
  import_pypolarscan();
  PYRAVE_DEBUG_INITIALIZE;

  return MOD_INIT_SUCCESS(module);
}
/*@} End of Module setup */
