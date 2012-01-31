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
#include "Python.h"
#include <math.h>
#include <stdio.h>
#include <string.h>

#define PYBBTOPOGRAPHY_MODULE   /**< to get correct part in pybbtopography */
#include "pybbtopography.h"

#include "pyrave_debug.h"
#include "rave_alloc.h"
#include <arrayobject.h>
#include "raveutil.h"
#include "rave.h"
/**
 * Debug this module
 */
PYRAVE_DEBUG_MODULE("_bbtopography");

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
/// BB Topography
/// --------------------------------------------------------------------
/*@{ BB Topography */
/**
 * Returns the native BeamBlockage_t instance.
 * @param[in] beamb - the python beam blockage instance
 * @returns the native BeamBlockage_t instance.
 */
static BBTopography_t*
PyBBTopography_GetNative(PyBBTopography* topo)
{
  RAVE_ASSERT((topo != NULL), "topo == NULL");
  return RAVE_OBJECT_COPY(topo->topo);
}

/**
 * Creates a python object from a native object or will create an
 * initial native object if p is NULL.
 * @param[in] p - the native object (or NULL)
 * @returns the python object.
 */
static PyBBTopography* PyBBTopography_New(BBTopography_t* p)
{
  PyBBTopography* result = NULL;
  BBTopography_t* cp = NULL;

  if (p == NULL) {
    cp = RAVE_OBJECT_NEW(&BBTopography_TYPE);
    if (cp == NULL) {
      RAVE_CRITICAL0("Failed to allocate memory for topography.");
      raiseException_returnNULL(PyExc_MemoryError, "Failed to allocate memory for topography.");
    }
  } else {
    cp = RAVE_OBJECT_COPY(p);
    result = RAVE_OBJECT_GETBINDING(p); // If p already have a binding, then this should only be increfed.
    if (result != NULL) {
      Py_INCREF(result);
    }
  }

  if (result == NULL) {
    result = PyObject_NEW(PyBBTopography, &PyBBTopography_Type);
    if (result != NULL) {
      PYRAVE_DEBUG_OBJECT_CREATED;
      result->topo = RAVE_OBJECT_COPY(cp);
      RAVE_OBJECT_BIND(result->topo, result);
    } else {
      RAVE_CRITICAL0("Failed to create PyBBTopography instance");
      raiseException_gotoTag(done, PyExc_MemoryError, "Failed to allocate memory for topography.");
    }
  }
done:
  RAVE_OBJECT_RELEASE(cp);
  return result;
}

/**
 * Deallocates the object
 * @param[in] obj the object to deallocate.
 */
static void _pybbtopography_dealloc(PyBBTopography* obj)
{
  /*Nothing yet*/
  if (obj == NULL) {
    return;
  }
  PYRAVE_DEBUG_OBJECT_DESTROYED;
  RAVE_OBJECT_UNBIND(obj->topo, obj);
  RAVE_OBJECT_RELEASE(obj->topo);
  PyObject_Del(obj);
}

/**
 * Creates a new instance of the python object
 * @param[in] self this instance.
 * @param[in] args N/A
 * @return the object on success, otherwise NULL
 */
static PyObject* _pybbtopography_new(PyObject* self, PyObject* args)
{
  if (!PyArg_ParseTuple(args, "")) {
    return NULL;
  }
  return (PyObject*)PyBBTopography_New(NULL);
}

static PyObject* _pybbtopography_getValue(PyBBTopography* self, PyObject* args)
{
  double value = 0.0L;
  long col = 0, row = 0;
  int result = 0;

  if (!PyArg_ParseTuple(args, "ll", &col, &row)) {
    return NULL;
  }

  result = BBTopography_getValue(self->topo, col, row, &value);

  return Py_BuildValue("(id)", result, value);
}

static PyObject* _pybbtopography_setValue(PyBBTopography* self, PyObject* args)
{
  long col = 0, row = 0;
  double value = 0.0;
  if (!PyArg_ParseTuple(args, "lld", &col, &row, &value)) {
    return NULL;
  }

  if (!BBTopography_setValue(self->topo, col, row, value)) {
    return NULL;
  }

  Py_RETURN_NONE;
}

static PyObject* _pybbtopography_setData(PyBBTopography* self, PyObject* args)
{
  PyObject* inarray = NULL;
  PyArrayObject* arraydata = NULL;
  RaveDataType datatype = RaveDataType_UNDEFINED;
  long ncols = 0;
  long nrows = 0;
  unsigned char* data = NULL;

  if (!PyArg_ParseTuple(args, "O", &inarray)) {
    return NULL;
  }

  if (!PyArray_Check(inarray)) {
    raiseException_returnNULL(PyExc_TypeError, "Data must be of arrayobject type")
  }

  arraydata = (PyArrayObject*)inarray;

  if (PyArray_NDIM(arraydata) != 2) {
    raiseException_returnNULL(PyExc_ValueError, "A cartesian product must be of rank 2");
  }

  datatype = translate_pyarraytype_to_ravetype(PyArray_TYPE(arraydata));

  if (PyArray_ITEMSIZE(arraydata) != get_ravetype_size(datatype)) {
    raiseException_returnNULL(PyExc_TypeError, "numpy and rave does not have same data sizes");
  }

  ncols  = PyArray_DIM(arraydata, 1);
  nrows  = PyArray_DIM(arraydata, 0);
  data   = PyArray_DATA(arraydata);

  if (!BBTopography_setData(self->topo, ncols, nrows, data, datatype)) {
    raiseException_returnNULL(PyExc_MemoryError, "Could not allocate memory");
  }

  Py_RETURN_NONE;
}

static PyObject* _pybbtopography_getData(PyBBTopography* self, PyObject* args)
{
  long ncols = 0, nrows = 0;
  RaveDataType type = RaveDataType_UNDEFINED;
  PyObject* result = NULL;
  npy_intp dims[2] = {0,0};
  int arrtype = 0;
  void* data = NULL;

  ncols = BBTopography_getNcols(self->topo);
  nrows = BBTopography_getNrows(self->topo);
  type = BBTopography_getDataType(self->topo);
  data = BBTopography_getData(self->topo);

  dims[1] = (npy_intp)ncols;
  dims[0] = (npy_intp)nrows;
  arrtype = translate_ravetype_to_pyarraytype(type);

  if (data == NULL) {
    raiseException_returnNULL(PyExc_IOError, "topography does not have any data");
  }

  if (arrtype == PyArray_NOTYPE) {
    raiseException_returnNULL(PyExc_IOError, "Could not translate data type");
  }
  result = PyArray_SimpleNew(2, dims, arrtype);
  if (result == NULL) {
    raiseException_returnNULL(PyExc_MemoryError, "Could not create resulting array");
  }
  if (result != NULL) {
    int nbytes = ncols*nrows*PyArray_ITEMSIZE(result);
    memcpy(((PyArrayObject*)result)->data, (unsigned char*)BBTopography_getData(self->topo), nbytes);
  }
  return result;
}

/**
 * Concatenates two fields x-wise.
 * @param[in] self - self
 * @param[in] args - the other rave field object
 * @return a topography object on success otherwise NULL
 */
static PyObject* _pybbtopography_concatx(PyBBTopography* self, PyObject* args)
{
  PyObject* result = NULL;
  PyObject* pyin = NULL;
  BBTopography_t *field = NULL;
  if (!PyArg_ParseTuple(args, "O", &pyin)) {
    return NULL;
  }
  if (!PyBBTopography_Check(pyin)) {
    raiseException_returnNULL(PyExc_ValueError, "Argument must be another topography field");
  }
  field = BBTopography_concatX(self->topo, ((PyBBTopography*)pyin)->topo);
  if (field == NULL) {
    raiseException_gotoTag(done, PyExc_ValueError, "Failed to concatenate fields");
  }

  result = (PyObject*)PyBBTopography_New(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/**
 * Concatenates two fields y-wise.
 * @param[in] self - self
 * @param[in] args - the other rave field object
 * @return a topography object on success otherwise NULL
 */
static PyObject* _pybbtopography_concaty(PyBBTopography* self, PyObject* args)
{
  PyObject* result = NULL;
  PyObject* pyin = NULL;
  BBTopography_t *field = NULL;
  if (!PyArg_ParseTuple(args, "O", &pyin)) {
    return NULL;
  }
  if (!PyBBTopography_Check(pyin)) {
    raiseException_returnNULL(PyExc_ValueError, "Argument must be another topography field");
  }
  field = BBTopography_concatY(self->topo, ((PyBBTopography*)pyin)->topo);
  if (field == NULL) {
    raiseException_gotoTag(done, PyExc_ValueError, "Failed to concatenate fields");
  }

  result = (PyObject*)PyBBTopography_New(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/**
 * All methods a topography instance can have
 */
static struct PyMethodDef _pybbtopography_methods[] =
{
  {"nodata", NULL},
  {"ulxmap", NULL},
  {"ulymap", NULL},
  {"xdim", NULL},
  {"ydim", NULL},
  {"ncols", NULL, 1},
  {"nrows", NULL, 1},
  {"getValue", (PyCFunction)_pybbtopography_getValue, 1},
  {"setValue", (PyCFunction)_pybbtopography_setValue, 1},
  {"setData", (PyCFunction)_pybbtopography_setData, 1},
  {"getData", (PyCFunction)_pybbtopography_getData, 1},
  {"concatx", (PyCFunction)_pybbtopography_concatx, 1},
  {"concaty", (PyCFunction)_pybbtopography_concaty, 1},
  {NULL, NULL} /* sentinel */
};

/**
 * Returns the specified attribute in the beam blockage
 */
static PyObject* _pybbtopography_getattr(PyBBTopography* self, char* name)
{
  PyObject* res = NULL;
  if (strcmp("nodata", name) == 0) {
    return PyFloat_FromDouble(BBTopography_getNodata(self->topo));
  } else if (strcmp("ulxmap", name) == 0) {
    return PyFloat_FromDouble(BBTopography_getUlxmap(self->topo));
  } else if (strcmp("ulymap", name) == 0) {
    return PyFloat_FromDouble(BBTopography_getUlymap(self->topo));
  } else if (strcmp("xdim", name) == 0) {
    return PyFloat_FromDouble(BBTopography_getXDim(self->topo));
  } else if (strcmp("ydim", name) == 0) {
    return PyFloat_FromDouble(BBTopography_getYDim(self->topo));
  } else if (strcmp("ncols", name) == 0) {
    return PyLong_FromLong(BBTopography_getNcols(self->topo));
  } else if (strcmp("nrows", name) == 0) {
    return PyLong_FromLong(BBTopography_getNrows(self->topo));
  }

  res = Py_FindMethod(_pybbtopography_methods, (PyObject*) self, name);
  if (res)
    return res;

  PyErr_Clear();
  PyErr_SetString(PyExc_AttributeError, name);
  return NULL;
}

/**
 * Returns the specified attribute in the polar volume
 */
static int _pybbtopography_setattr(PyBBTopography* self, char* name, PyObject* val)
{
  int result = -1;
  if (name == NULL) {
    goto done;
  }

  if (strcmp("nodata", name) == 0) {
    if (PyFloat_Check(val)) {
      BBTopography_setNodata(self->topo, PyFloat_AsDouble(val));
    } else if (PyLong_Check(val) || PyInt_Check(val)) {
      BBTopography_setNodata(self->topo, PyLong_AsDouble(val));
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "nodata must be a floating number");
    }
  } else if (strcmp("ulxmap", name) == 0) {
    if (PyFloat_Check(val)) {
      BBTopography_setUlxmap(self->topo, PyFloat_AsDouble(val));
    } else if (PyLong_Check(val) || PyInt_Check(val)) {
      BBTopography_setUlxmap(self->topo, PyLong_AsDouble(val));
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "ulxmap must be a floating number");
    }
  } else if (strcmp("ulymap", name) == 0) {
    if (PyFloat_Check(val)) {
      BBTopography_setUlymap(self->topo, PyFloat_AsDouble(val));
    } else if (PyLong_Check(val) || PyInt_Check(val)) {
      BBTopography_setUlymap(self->topo, PyLong_AsDouble(val));
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "ulymap must be a floating number");
    }
  } else if (strcmp("xdim", name) == 0) {
    if (PyFloat_Check(val)) {
      BBTopography_setXDim(self->topo, PyFloat_AsDouble(val));
    } else if (PyLong_Check(val) || PyInt_Check(val)) {
      BBTopography_setXDim(self->topo, PyLong_AsDouble(val));
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "xdim must be a floating number");
    }
  } else if (strcmp("ydim", name) == 0) {
    if (PyFloat_Check(val)) {
      BBTopography_setYDim(self->topo, PyFloat_AsDouble(val));
    } else if (PyLong_Check(val) || PyInt_Check(val)) {
      BBTopography_setYDim(self->topo, PyLong_AsDouble(val));
    } else {
      raiseException_gotoTag(done, PyExc_ValueError, "ydim must be a floating number");
    }
  } else {
    raiseException_gotoTag(done, PyExc_AttributeError, name);
  }

  result = 0;
done:
  return result;
}

/*@} End of BBTopography */

/// --------------------------------------------------------------------
/// Type definitions
/// --------------------------------------------------------------------
/*@{ Type definitions */
PyTypeObject PyBBTopography_Type =
{
  PyObject_HEAD_INIT(NULL)0, /*ob_size*/
  "BBTopographyCore", /*tp_name*/
  sizeof(PyBBTopography), /*tp_size*/
  0, /*tp_itemsize*/
  /* methods */
  (destructor)_pybbtopography_dealloc, /*tp_dealloc*/
  0, /*tp_print*/
  (getattrfunc)_pybbtopography_getattr, /*tp_getattr*/
  (setattrfunc)_pybbtopography_setattr, /*tp_setattr*/
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
  {"new", (PyCFunction)_pybbtopography_new, 1},
  {NULL,NULL} /*Sentinel*/
};

PyMODINIT_FUNC
init_bbtopography(void)
{
  PyObject *module=NULL,*dictionary=NULL;
  static void *PyBBTopography_API[PyBBTopography_API_pointers];
  PyObject *c_api_object = NULL;
  PyBBTopography_Type.ob_type = &PyType_Type;

  module = Py_InitModule("_bbtopography", functions);
  if (module == NULL) {
    return;
  }
  PyBBTopography_API[PyBBTopography_Type_NUM] = (void*)&PyBBTopography_Type;
  PyBBTopography_API[PyBBTopography_GetNative_NUM] = (void *)PyBBTopography_GetNative;
  PyBBTopography_API[PyBBTopography_New_NUM] = (void*)PyBBTopography_New;

  c_api_object = PyCObject_FromVoidPtr((void *)PyBBTopography_API, NULL);

  if (c_api_object != NULL) {
    PyModule_AddObject(module, "_C_API", c_api_object);
  }

  dictionary = PyModule_GetDict(module);
  ErrorObject = PyString_FromString("_bbtopography.error");

  if (ErrorObject == NULL || PyDict_SetItemString(dictionary, "error", ErrorObject) != 0) {
    Py_FatalError("Can't define _bbtopography.error");
  }
  import_array();

  PYRAVE_DEBUG_INITIALIZE;
}
/*@} End of Module setup */
