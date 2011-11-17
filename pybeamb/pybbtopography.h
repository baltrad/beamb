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
 * Python version of the beam blockage topography field
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-11-16
 */
#ifndef PYBBTOPOGRAPHY_H
#define PYBBTOPOGRAPHY_H
#include "Python.h"
#include "bbtopography.h"

/**
 * The beam blockage
 */
typedef struct {
   PyObject_HEAD /*Always have to be on top*/
   BBTopography_t* topo;  /**< the native object */
} PyBBTopography;

#define PyBBTopography_Type_NUM 0                              /**< index of type */

#define PyBBTopography_GetNative_NUM 1                         /**< index of GetNative*/
#define PyBBTopography_GetNative_RETURN BBTopography_t*         /**< return type for GetNative */
#define PyBBTopography_GetNative_PROTO (PyBBTopography*)        /**< arguments for GetNative */

#define PyBBTopography_New_NUM 2                               /**< index of New */
#define PyBBTopography_New_RETURN PyBBTopography*              /**< return type for New */
#define PyBBTopography_New_PROTO (BBTopography_t*)             /**< arguments for New */

#define PyBBTopography_API_pointers 3                          /**< number of type and function pointers */

#ifdef PYBBTOPOGRAPHY_MODULE
/** Forward declaration of type */
extern PyTypeObject PyBBTopography_Type;

/** Checks if the object is a PyBBTopography or not */
#define PyBBTopography_Check(op) ((op)->ob_type == &PyBBTopography_Type)

/** Forward declaration of PyBBTopography_GetNative */
static PyBBTopography_GetNative_RETURN PyBBTopography_GetNative PyBBTopography_GetNative_PROTO;

/** Forward declaration of PyRopoGenerator_New */
static PyBBTopography_New_RETURN PyBBTopography_New PyBBTopography_New_PROTO;

#else
/** Pointers to types and functions */
static void **PyBBTopography_API;

/**
 * Returns a pointer to the internal object, remember to release the reference
 * when done with the object. (RAVE_OBJECT_RELEASE).
 */
#define PyBBTopography_GetNative \
  (*(PyBBTopography_GetNative_RETURN (*)PyBBTopography_GetNative_PROTO) PyBBTopography_API[PyBBTopography_GetNative_NUM])

/**
 * Creates a new instance. Release this object with Py_DECREF. If a BBTopography_t instance is
 * provided and this instance already is bound to a python instance, this instance will be increfed and
 * returned.
 * @param[in] obj - the BBTopography_t instance.
 * @returns the PyBBTopography instance.
 */
#define PyBBTopography_New \
  (*(PyBBTopography_New_RETURN (*)PyBBTopography_New_PROTO) PyBBTopography_API[PyBBTopography_New_NUM])

/**
 * Checks if the object is a python topography instance
 */
#define PyBBTopography_Check(op) \
   ((op)->ob_type == (PyTypeObject *)PyBBTopography_API[PyBBTopography_Type_NUM])

/**
 * Imports the PyBBTopography module (like import _bbtopography in python).
 */
static int
import_bbtopography(void)
{
  PyObject *module;
  PyObject *c_api_object;

  module = PyImport_ImportModule("_bbtopography");
  if (module == NULL) {
    return -1;
  }

  c_api_object = PyObject_GetAttrString(module, "_C_API");
  if (c_api_object == NULL) {
    Py_DECREF(module);
    return -1;
  }
  if (PyCObject_Check(c_api_object)) {
    PyBBTopography_API = (void **)PyCObject_AsVoidPtr(c_api_object);
  }
  Py_DECREF(c_api_object);
  Py_DECREF(module);
  return 0;
}

#endif

#endif /* PYBBTOPOGRAPHY_H */
