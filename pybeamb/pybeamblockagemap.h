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
 * Python version of the beam blockage map
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-11-15
 */
#ifndef PYBEAMBLOCKAGEMAP_H
#define PYBEAMBLOCKAGEMAP_H
#include "beamblockagemap.h"

/**
 * The beam blockage
 */
typedef struct {
   PyObject_HEAD /*Always have to be on top*/
   BeamBlockageMap_t* map;  /**< the native object */
} PyBeamBlockageMap;

#define PyBeamBlockageMap_Type_NUM 0                              /**< index of type */

#define PyBeamBlockageMap_GetNative_NUM 1                         /**< index of GetNative*/
#define PyBeamBlockageMap_GetNative_RETURN BeamBlockageMap_t*         /**< return type for GetNative */
#define PyBeamBlockageMap_GetNative_PROTO (PyBeamBlockageMap*)        /**< arguments for GetNative */

#define PyBeamBlockageMap_New_NUM 2                               /**< index of New */
#define PyBeamBlockageMap_New_RETURN PyBeamBlockageMap*              /**< return type for New */
#define PyBeamBlockageMap_New_PROTO (BeamBlockageMap_t*)             /**< arguments for New */

#define PyBeamBlockageMap_API_pointers 3                          /**< number of type and function pointers */

#ifdef PYBEAMBLOCKAGEMAP_MODULE
/** Forward declaration of type */
extern PyTypeObject PyBeamBlockageMap_Type;

/** Checks if the object is a PyBeamBlockageMap or not */
#define PyBeamBlockageMap_Check(op) ((op)->ob_type == &PyBeamBlockageMap_Type)

/** Forward declaration of PyBeamBlockageMap_GetNative */
static PyBeamBlockageMap_GetNative_RETURN PyBeamBlockageMap_GetNative PyBeamBlockageMap_GetNative_PROTO;

/** Forward declaration of PyRopoGenerator_New */
static PyBeamBlockageMap_New_RETURN PyBeamBlockageMap_New PyBeamBlockageMap_New_PROTO;

#else
/** Pointers to types and functions */
static void **PyBeamBlockageMap_API;

/**
 * Returns a pointer to the internal beam blockage, remember to release the reference
 * when done with the object. (RAVE_OBJECT_RELEASE).
 */
#define PyBeamBlockageMap_GetNative \
  (*(PyBeamBlockageMap_GetNative_RETURN (*)PyBeamBlockageMap_GetNative_PROTO) PyBeamBlockageMap_API[PyBeamBlockageMap_GetNative_NUM])

/**
 * Creates a new beam blockage instance. Release this object with Py_DECREF. If a BeamBlockage_t instance is
 * provided and this instance already is bound to a python instance, this instance will be increfed and
 * returned.
 * @param[in] beamb - the BeamBlockage_t instance.
 * @returns the PyBeamBlockageMap instance.
 */
#define PyBeamBlockageMap_New \
  (*(PyBeamBlockageMap_New_RETURN (*)PyBeamBlockageMap_New_PROTO) PyBeamBlockageMap_API[PyBeamBlockageMap_New_NUM])

/**
 * Checks if the object is a python beam blockage instance
 */
#define PyBeamBlockageMap_Check(op) \
   ((op)->ob_type == (PyTypeObject *)PyBeamBlockageMap_API[PyBeamBlockageMap_Type_NUM])

/**
 * Imports the PyBeamBlockageMap module (like import _beamblockage in python).
 */
static int
import_beamblockagemap(void)
{
  PyObject *module;
  PyObject *c_api_object;

  module = PyImport_ImportModule("_beamblockagemap");
  if (module == NULL) {
    return -1;
  }

  c_api_object = PyObject_GetAttrString(module, "_C_API");
  if (c_api_object == NULL) {
    Py_DECREF(module);
    return -1;
  }
  if (PyCObject_Check(c_api_object)) {
    PyBeamBlockageMap_API = (void **)PyCObject_AsVoidPtr(c_api_object);
  }
  Py_DECREF(c_api_object);
  Py_DECREF(module);
  return 0;
}

#endif

#endif /* PYBEAMBLOCKAGEMAP_H */
