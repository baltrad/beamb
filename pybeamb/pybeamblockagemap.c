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
 * Python API to the beam blockage map functions
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-11-15
 */
#include "Python.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PYBEAMBLOCKAGEMAP_MODULE   /**< to get correct part in pybeamblockagemap */
#include "pybeamblockagemap.h"

#include "pypolarscan.h"
#include "pyrave_debug.h"
#include "rave_alloc.h"
#include "pybbtopography.h"

/**
 * Debug this module
 */
PYRAVE_DEBUG_MODULE("_beamblockagemap");

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
 * Returns the native BeamBlockageMap_t instance.
 * @param[in] beamb - the python beam blockage instance
 * @returns the native BeamBlockageMap_t instance.
 */
static BeamBlockageMap_t*
PyBeamBlockageMap_GetNative(PyBeamBlockageMap* beamb)
{
  RAVE_ASSERT((beamb != NULL), "beamb == NULL");
  return RAVE_OBJECT_COPY(beamb->map);
}

/**
 * Creates a python beam blockage from a native beam blockage or will create an
 * initial native beam blockage if p is NULL.
 * @param[in] p - the native beam blockage (or NULL)
 * @param[in] beamb- the beam blockage (only used if p != NULL)
 * @returns the python beam blockage.
 */
static PyBeamBlockageMap* PyBeamBlockageMap_New(BeamBlockageMap_t* p)
{
  PyBeamBlockageMap* result = NULL;
  BeamBlockageMap_t* cp = NULL;

  if (p == NULL) {
    cp = RAVE_OBJECT_NEW(&BeamBlockageMap_TYPE);
    if (cp == NULL) {
      RAVE_CRITICAL0("Failed to allocate memory for beam blockage map.");
      raiseException_returnNULL(PyExc_MemoryError, "Failed to allocate memory for beam blockage map.");
    }
  } else {
    cp = RAVE_OBJECT_COPY(p);
    result = RAVE_OBJECT_GETBINDING(p); // If p already have a binding, then this should only be increfed.
    if (result != NULL) {
      Py_INCREF(result);
    }
  }

  if (result == NULL) {
    result = PyObject_NEW(PyBeamBlockageMap, &PyBeamBlockageMap_Type);
    if (result != NULL) {
      PYRAVE_DEBUG_OBJECT_CREATED;
      result->map = RAVE_OBJECT_COPY(cp);
      RAVE_OBJECT_BIND(result->map, result);
    } else {
      RAVE_CRITICAL0("Failed to create PyBeamBlockageMap instance");
      raiseException_gotoTag(done, PyExc_MemoryError, "Failed to allocate memory for beam blockage.");
    }
  }
done:
  RAVE_OBJECT_RELEASE(cp);
  return result;
}

/**
 * Deallocates the beam blockage map
 * @param[in] obj the object to deallocate.
 */
static void _pybeamblockagemap_dealloc(PyBeamBlockageMap* obj)
{
  /*Nothing yet*/
  if (obj == NULL) {
    return;
  }
  PYRAVE_DEBUG_OBJECT_DESTROYED;
  RAVE_OBJECT_UNBIND(obj->map, obj);
  RAVE_OBJECT_RELEASE(obj->map);
  PyObject_Del(obj);
}

/**
 * Creates a new instance of the beam blockage map
 * @param[in] self this instance.
 * @param[in] args arguments for creation or a beam blockage map
 * @return the object on success, otherwise NULL
 */
static PyObject* _pybeamblockagemap_new(PyObject* self, PyObject* args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  return (PyObject*)PyBeamBlockageMap_New(NULL);
}

/**
 * Reads the topography necessary for working with the provided
 * lat/lon with a radius of x-meters.
 * @param[in] self - self
 * @param[in] args - lat, lon (in radians) and radius (in meters)
 * @return the read topography as a ravefield or NULL on failure
 */
static PyObject* _pybeamblockagemap_readTopography(PyBeamBlockageMap* self, PyObject* args)
{
  double lat = 0.0, lon = 0.0, radius = 0.0;
  PyObject* result = NULL;
  BBTopography_t* field = NULL;

  if (!PyArg_ParseTuple(args, "ddd", &lat, &lon, &radius)) {
    return NULL;
  }
  field = BeamBlockageMap_readTopography(self->map, lat, lon, radius);
  if (field != NULL) {
    result = (PyObject*)PyBBTopography_New(field);
  } else {
    PyErr_SetString(PyExc_EnvironmentError, "Could not open topography");
  }
  RAVE_OBJECT_RELEASE(field);
  return result;
}

static PyObject* _pybeamblockagemap_getTopographyForScan(PyBeamBlockageMap* self, PyObject* args)
{
  PyObject* pyin = NULL;
  BBTopography_t* field = NULL;
  PyObject* result = NULL;
  if (!PyArg_ParseTuple(args, "O", &pyin)) {
    return NULL;
  }
  if (!PyPolarScan_Check(pyin)) {
    raiseException_returnNULL(PyExc_ValueError, "In object must be a polar scan");
  }
  field = BeamBlockageMap_getTopographyForScan(self->map, ((PyPolarScan*)pyin)->scan);
  if (field != NULL) {
    result = (PyObject*)PyBBTopography_New(field);
  }
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/**
 * All methods a beam blockage map can have
 */
static struct PyMethodDef _pybeamblockagemap_methods[] =
{
  {"topo30dir", NULL},
  {"readTopography", (PyCFunction)_pybeamblockagemap_readTopography, 1},
  {"getTopographyForScan", (PyCFunction)_pybeamblockagemap_getTopographyForScan, 1},
  {NULL, NULL} /* sentinel */
};

/**
 * Returns the specified attribute in the beam blockage map
 */
static PyObject* _pybeamblockagemap_getattr(PyBeamBlockageMap* self, char* name)
{
  PyObject* res = NULL;

  if (strcmp("topo30dir", name) == 0) {
    const char* str = BeamBlockageMap_getTopo30Directory(self->map);
    if (str != NULL) {
      return PyString_FromString(str);
    } else {
      Py_RETURN_NONE;
    }
  }

  res = Py_FindMethod(_pybeamblockagemap_methods, (PyObject*) self, name);
  if (res)
    return res;

  PyErr_Clear();
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

/**
 * Returns the specified attribute in the beam blockage instance
 */
static int _pybeamblockagemap_setattr(PyBeamBlockageMap* self, char* name, PyObject* val)
{
  int result = -1;
  if (name == NULL) {
    goto done;
  }

  if (strcmp("topo30dir", name) == 0) {
    if (PyString_Check(val)) {
      if (!BeamBlockageMap_setTopo30Directory(self->map, PyString_AsString(val))) {
        raiseException_gotoTag(done, PyExc_ValueError, "topo30dir must be a string or None");
      }
    } else if (val == Py_None) {
      BeamBlockageMap_setTopo30Directory(self->map, NULL);
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
PyTypeObject PyBeamBlockageMap_Type =
{
  PyObject_HEAD_INIT(NULL)0, /*ob_size*/
  "BeamBlockageMapCore", /*tp_name*/
  sizeof(PyBeamBlockageMap), /*tp_size*/
  0, /*tp_itemsize*/
  /* methods */
  (destructor)_pybeamblockagemap_dealloc, /*tp_dealloc*/
  0, /*tp_print*/
  (getattrfunc)_pybeamblockagemap_getattr, /*tp_getattr*/
  (setattrfunc)_pybeamblockagemap_setattr, /*tp_setattr*/
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
  {"new", (PyCFunction)_pybeamblockagemap_new, 1},
  {NULL,NULL} /*Sentinel*/
};

PyMODINIT_FUNC
init_beamblockagemap(void)
{
  PyObject *module=NULL,*dictionary=NULL;
  static void *PyBeamBlockageMap_API[PyBeamBlockageMap_API_pointers];
  PyObject *c_api_object = NULL;
  PyBeamBlockageMap_Type.ob_type = &PyType_Type;

  module = Py_InitModule("_beamblockagemap", functions);
  if (module == NULL) {
    return;
  }
  PyBeamBlockageMap_API[PyBeamBlockageMap_Type_NUM] = (void*)&PyBeamBlockageMap_Type;
  PyBeamBlockageMap_API[PyBeamBlockageMap_GetNative_NUM] = (void *)PyBeamBlockageMap_GetNative;
  PyBeamBlockageMap_API[PyBeamBlockageMap_New_NUM] = (void*)PyBeamBlockageMap_New;

  c_api_object = PyCObject_FromVoidPtr((void *)PyBeamBlockageMap_API, NULL);

  if (c_api_object != NULL) {
    PyModule_AddObject(module, "_C_API", c_api_object);
  }

  dictionary = PyModule_GetDict(module);
  ErrorObject = PyString_FromString("_beamblockagemap.error");

  if (ErrorObject == NULL || PyDict_SetItemString(dictionary, "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _beamblockagemap.error");
  }
  import_bbtopography();
  import_pypolarscan();
  PYRAVE_DEBUG_INITIALIZE;
}
/*@} End of Module setup */
