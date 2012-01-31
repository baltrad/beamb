/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of beam blockage (beamb).

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
 * Beam-blockage topography layer
 * @file
 * @author Anders Henja (SMHI)
 * @date 2011-11-10
 */
#include "bbtopography.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include "math.h"
#include <string.h>

/**
 * Represents the beam blockage topography
 */
struct _BBTopography_t {
  RAVE_OBJECT_HEAD /** Always on top */
  RaveData2D_t* data; /**< the data */
  double nodata; /**< the nodata */
  double ulxmap; /**< the upper left x-coordinate (longitude / radians)*/
  double ulymap; /**< the upper left x-coordinate(latitude / radians) */
  double xdim; /**< the x step size (radians) */
  double ydim; /**< the y step size (radians) */
};

/*@{ Private functions */
/**
 * Constructor.
 */
static int BBTopography_constructor(RaveCoreObject* obj)
{
  BBTopography_t* self = (BBTopography_t*)obj;
  self->data = RAVE_OBJECT_NEW(&RaveData2D_TYPE);;
  self->nodata = -9999.0;
  self->ulxmap = 0.0;
  self->ulymap = 0.0;
  self->xdim = 0.0;
  self->ydim = 0.0;

  if (self->data == NULL) {
    goto error;
  }

  return 1;
error:
  RAVE_OBJECT_RELEASE(self->data);
  return 0;
}

/**
 * Destructor
 * @param[in] obj - object
 */
static void BBTopography_destructor(RaveCoreObject* obj)
{
  BBTopography_t* self = (BBTopography_t*)obj;
  RAVE_OBJECT_RELEASE(self->data);
}

/**
 * Copy constructor
 * @param[in] obj - target object
 * @param[in] srcobj - source object
 * @return 1 on success otherwise 0
 */
static int BBTopography_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  BBTopography_t* this = (BBTopography_t*)obj;
  BBTopography_t* src = (BBTopography_t*)srcobj;
  this->data = RAVE_OBJECT_CLONE(src->data);
  this->nodata = src->nodata;
  this->ulxmap = src->ulxmap;
  this->ulymap = src->ulymap;
  this->xdim = src->xdim;
  this->ydim = src->ydim;

  if (this->data == NULL) {
    RAVE_ERROR0("Failed to clone data2d field");
    goto error;
  }

  return 1;
error:
  RAVE_OBJECT_RELEASE(this->data);
  return 0;
}

/*@} End of Private functions */

/*@{ Interface functions */
void BBTopography_setNodata(BBTopography_t* self, double nodata)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->nodata = nodata;
}

double BBTopography_getNodata(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->nodata;
}

void BBTopography_setXDim(BBTopography_t* self, double xdim)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->xdim = xdim;
}

double BBTopography_getXDim(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->xdim;
}


void BBTopography_setYDim(BBTopography_t* self, double ydim)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ydim = ydim;
}

double BBTopography_getYDim(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ydim;
}

void BBTopography_setUlxmap(BBTopography_t* self, double ulxmap)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ulxmap = ulxmap;
}

double BBTopography_getUlxmap(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ulxmap;
}

void BBTopography_setUlymap(BBTopography_t* self, double ulymap)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ulymap = ulymap;
}

double BBTopography_getUlymap(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ulymap;
}

int BBTopography_createData(BBTopography_t* self, long ncols, long nrows, RaveDataType type)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_createData(self->data, ncols, nrows, type);
}

int BBTopography_setData(BBTopography_t* self, long ncols, long nrows, void* data, RaveDataType type)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_setData(self->data, ncols, nrows, data, type);
}

void* BBTopography_getData(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getData(self->data);
}

int BBTopography_setDatafield(BBTopography_t* self, RaveData2D_t* datafield)
{
  int result = 0;
  RAVE_ASSERT((self != NULL), "self == NULL");
  if (datafield != NULL) {
    RaveData2D_t* d = RAVE_OBJECT_CLONE(datafield);
    if (d != NULL) {
      RAVE_OBJECT_RELEASE(self->data);
      self->data = d;
      result = 1;
    } else {
      RAVE_ERROR0("Failed to clone 2d field");
    }
  }
  return result;
}

RaveData2D_t* BBTopography_getDatafield(BBTopography_t* self)
{
  RaveData2D_t* result = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");

  result = RAVE_OBJECT_CLONE(self->data);
  if (result == NULL) {
    RAVE_ERROR0("Failed to clone data field");
  }

  return result;
}

long BBTopography_getNcols(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getXsize(self->data);
}

long BBTopography_getNrows(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getYsize(self->data);
}

RaveDataType BBTopography_getDataType(BBTopography_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getType(self->data);
}

int BBTopography_getValue(BBTopography_t* self, long col, long row, double* v)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getValue(self->data, col, row, v);
}

int BBTopography_setValue(BBTopography_t* self, long col, long row, double value)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_setValue(self->data, col, row, value);
}

int BBTopography_getValueAtLonLat(BBTopography_t* self, double lon, double lat, double* v)
{
  int result = 0;
  long ci = 0, ri = 0;
  double nv = 0.0;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((v != NULL), "v == NULL");
  *v = self->nodata;

  if (self->xdim == 0.0 || self->ydim == 0.0) {
    RAVE_CRITICAL0("xdim or ydim == 0.0 in topography field");
    goto done;
  }

  ci = (lon - self->ulxmap)/self->xdim;
  ri = (self->ulymap - lat)/self->ydim;

  if (RaveData2D_getValue(self->data, ci, ri, &nv)) {
    *v = nv;
    result = 1;
  }

done:
  return result;
}

BBTopography_t* BBTopography_concatX(BBTopography_t* self, BBTopography_t* other)
{
  BBTopography_t *result = NULL;
  RaveData2D_t* dfield = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  if (other == NULL) {
    RAVE_ERROR0("Trying to concatenate self with NULL");
    return NULL;
  }

  if (BBTopography_getNrows(self) != BBTopography_getNrows(other) ||
      BBTopography_getXDim(self) != BBTopography_getXDim(other) ||
      BBTopography_getYDim(self) != BBTopography_getYDim(other)) {
    RAVE_ERROR0("Cannot concatenate two topography fields that don't have same nrows/xdim and ydim values");
    return NULL;
  }

  dfield = RaveData2D_concatX(self->data, other->data);
  if (dfield != NULL) {
    result = RAVE_OBJECT_NEW(&BBTopography_TYPE);
    if (result == NULL) {
      RAVE_ERROR0("Failed to create topography field");
    } else {
      RAVE_OBJECT_RELEASE(result->data);
      result->data = RAVE_OBJECT_COPY(dfield);
      BBTopography_setNodata(result, BBTopography_getNodata(self));
      BBTopography_setUlxmap(result, BBTopography_getUlxmap(self));
      BBTopography_setUlymap(result, BBTopography_getUlymap(self));
      BBTopography_setXDim(result, BBTopography_getXDim(self));
      BBTopography_setYDim(result, BBTopography_getYDim(self));
    }
  }

  RAVE_OBJECT_RELEASE(dfield);
  return result;
}

BBTopography_t* BBTopography_concatY(BBTopography_t* self, BBTopography_t* other)
{
  BBTopography_t *result = NULL;
  RaveData2D_t* dfield = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  if (other == NULL) {
    RAVE_ERROR0("Trying to concatenate self with NULL");
    return NULL;
  }

  if (BBTopography_getNcols(self) != BBTopography_getNcols(other) ||
      BBTopography_getXDim(self) != BBTopography_getXDim(other) ||
      BBTopography_getYDim(self) != BBTopography_getYDim(other)) {
    RAVE_ERROR0("Cannot concatenate two topography fields that don't have same nrows/xdim and ydim values");
    return NULL;
  }

  dfield = RaveData2D_concatY(self->data, other->data);
  if (dfield != NULL) {
    result = RAVE_OBJECT_NEW(&BBTopography_TYPE);
    if (result == NULL) {
      RAVE_ERROR0("Failed to create topography field");
    } else {
      RAVE_OBJECT_RELEASE(result->data);
      result->data = RAVE_OBJECT_COPY(dfield);
      BBTopography_setNodata(result, BBTopography_getNodata(self));
      BBTopography_setUlxmap(result, BBTopography_getUlxmap(self));
      BBTopography_setUlymap(result, BBTopography_getUlymap(self));
      BBTopography_setXDim(result, BBTopography_getXDim(self));
      BBTopography_setYDim(result, BBTopography_getYDim(self));
    }
  }

  RAVE_OBJECT_RELEASE(dfield);
  return result;
}

/*@} End of Interface functions */

RaveCoreObjectType BBTopography_TYPE = {
    "BBTopography",
    sizeof(BBTopography_t),
    BBTopography_constructor,
    BBTopography_destructor,
    BBTopography_copyconstructor
};
