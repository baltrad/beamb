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
 * Python version of the beam blockage
 * @file
 * @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-11-14
 */
#ifndef PYBEAMBLOCKAGE_H
#define PYBEAMBLOCKAGE_H
#include "beamblockage.h"

/**
 * The beam blockage
 */
typedef struct {
   PyObject_HEAD /*Always have to be on top*/
   BeamBlockage_t* beamb;  /**< the native object */
} PyBeamBlockage;

#define PyBeamBlockage_Type_NUM 0                              /**< index of type */

#define PyBeamBlockage_GetNative_NUM 1                         /**< index of GetNative*/
#define PyBeamBlockage_GetNative_RETURN BeamBlockage_t*         /**< return type for GetNative */
#define PyBeamBlockage_GetNative_PROTO (PyBeamBlockage*)        /**< arguments for GetNative */

#define PyBeamBlockage_New_NUM 2                               /**< index of New */
#define PyBeamBlockage_New_RETURN PyBeamBlockage*              /**< return type for New */
#define PyBeamBlockage_New_PROTO (BeamBlockage_t*)             /**< arguments for New */

#define PyBeamBlockage_API_pointers 3                          /**< number of type and function pointers */

#define PyBeamBlockage_CAPSULE_NAME "_beamblockage._C_API"

#ifdef PYBEAMBLOCKAGE_MODULE
/** Forward declaration of type */
extern PyTypeObject PyBeamBlockage_Type;

/** Checks if the object is a PyBeamBlockage or not */
#define PyBeamBlockage_Check(op) ((op)->ob_type == &PyBeamBlockage_Type)

/** Forward declaration of PyBeamBlockage_GetNative */
static PyBeamBlockage_GetNative_RETURN PyBeamBlockage_GetNative PyBeamBlockage_GetNative_PROTO;

/** Forward declaration of PyRopoGenerator_New */
static PyBeamBlockage_New_RETURN PyBeamBlockage_New PyBeamBlockage_New_PROTO;

#else
/** Pointers to types and functions */
static void **PyBeamBlockage_API;

/**
 * Returns a pointer to the internal beam blockage, remember to release the reference
 * when done with the object. (RAVE_OBJECT_RELEASE).
 */
#define PyBeamBlockage_GetNative \
  (*(PyBeamBlockage_GetNative_RETURN (*)PyBeamBlockage_GetNative_PROTO) PyBeamBlockage_API[PyBeamBlockage_GetNative_NUM])

/**
 * Creates a new beam blockage instance. Release this object with Py_DECREF. If a BeamBlockage_t instance is
 * provided and this instance already is bound to a python instance, this instance will be increfed and
 * returned.
 * @param[in] beamb - the BeamBlockage_t instance.
 * @returns the PyBeamBlockage instance.
 */
#define PyBeamBlockage_New \
  (*(PyBeamBlockage_New_RETURN (*)PyBeamBlockage_New_PROTO) PyBeamBlockage_API[PyBeamBlockage_New_NUM])

/**
 * Checks if the object is a python beam blockage instance
 */
#define PyBeamBlockage_Check(op) \
	(Py_TYPE(op) == &PyBeamBlockage_Type)


#define PyBeamBlockage_Type (*(PyTypeObject*)PyBeamBlockage_API[PyBeamBlockage_Type_NUM])


/**
 * Imports the PyBeamBlockage module (like import _beamblockage in python).
 */
#define import_beamblockage() \
		PyBeamBlockage_API = (void **)PyCapsule_Import(PyBeamBlockage_CAPSULE_NAME, 1);

#endif

#endif /* PYBEAMBLOCKAGE_H */
