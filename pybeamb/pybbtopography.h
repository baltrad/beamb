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
   BBTopograhy_t* topo;  /**< the native object */
} PyBBTopograhy;

#define PyBBTopograhy_Type_NUM 0                              /**< index of type */

#define PyBBTopograhy_GetNative_NUM 1                         /**< index of GetNative*/
#define PyBBTopograhy_GetNative_RETURN BBTopograhy_t*         /**< return type for GetNative */
#define PyBBTopograhy_GetNative_PROTO (PyBBTopograhy*)        /**< arguments for GetNative */

#define PyBBTopograhy_New_NUM 2                               /**< index of New */
#define PyBBTopograhy_New_RETURN PyBBTopograhy*              /**< return type for New */
#define PyBBTopograhy_New_PROTO (BBTopograhy_t*)             /**< arguments for New */

#define PyBBTopograhy_API_pointers 3                          /**< number of type and function pointers */

#ifdef PYBBTOPOGRAPHY_MODULE
/** Forward declaration of type */
extern PyTypeObject PyBBTopograhy_Type;

/** Checks if the object is a PyBBTopograhy or not */
#define PyBBTopograhy_Check(op) ((op)->ob_type == &PyBBTopograhy_Type)

/** Forward declaration of PyBBTopograhy_GetNative */
static PyBBTopograhy_GetNative_RETURN PyBBTopograhy_GetNative PyBBTopograhy_GetNative_PROTO;

/** Forward declaration of PyRopoGenerator_New */
static PyBBTopograhy_New_RETURN PyBBTopograhy_New PyBBTopograhy_New_PROTO;

#else
/** Pointers to types and functions */
static void **PyBBTopograhy_API;

/**
 * Returns a pointer to the internal object, remember to release the reference
 * when done with the object. (RAVE_OBJECT_RELEASE).
 */
#define PyBBTopograhy_GetNative \
  (*(PyBBTopograhy_GetNative_RETURN (*)PyBBTopograhy_GetNative_PROTO) PyBBTopograhy_API[PyBBTopograhy_GetNative_NUM])

/**
 * Creates a new instance. Release this object with Py_DECREF. If a BBTopograhy_t instance is
 * provided and this instance already is bound to a python instance, this instance will be increfed and
 * returned.
 * @param[in] obj - the BBTopograhy_t instance.
 * @returns the PyBBTopograhy instance.
 */
#define PyBBTopograhy_New \
  (*(PyBBTopograhy_New_RETURN (*)PyBBTopograhy_New_PROTO) PyBBTopograhy_API[PyBBTopograhy_New_NUM])

/**
 * Checks if the object is a python topography instance
 */
#define PyBBTopograhy_Check(op) \
   ((op)->ob_type == (PyTypeObject *)PyBBTopograhy_API[PyBBTopograhy_Type_NUM])

/**
 * Imports the PyBBTopograhy module (like import _bbtopography in python).
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
    PyBBTopograhy_API = (void **)PyCObject_AsVoidPtr(c_api_object);
  }
  Py_DECREF(c_api_object);
  Py_DECREF(module);
  return 0;
}

#endif

#endif /* PYBBTOPOGRAPHY_H */
