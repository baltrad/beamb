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
struct _BBTopograhy_t {
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
static int BBTopograhy_constructor(RaveCoreObject* obj)
{
  BBTopograhy_t* self = (BBTopograhy_t*)obj;
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
static void BBTopograhy_destructor(RaveCoreObject* obj)
{
  BBTopograhy_t* self = (BBTopograhy_t*)obj;
  RAVE_OBJECT_RELEASE(self->data);
}

/**
 * Copy constructor
 * @param[in] obj - target object
 * @param[in] srcobj - source object
 * @return 1 on success otherwise 0
 */
static int BBTopograhy_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  BBTopograhy_t* this = (BBTopograhy_t*)obj;
  BBTopograhy_t* src = (BBTopograhy_t*)srcobj;
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
void BBTopography_setNodata(BBTopograhy_t* self, double nodata)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->nodata = nodata;
}

double BBTopography_getNodata(BBTopograhy_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->nodata;
}

void BBTopography_setXDim(BBTopograhy_t* self, double xdim)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->xdim = xdim;
}

double BBTopography_getXDim(BBTopograhy_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->xdim;
}


void BBTopography_setYDim(BBTopograhy_t* self, double ydim)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ydim = ydim;
}

double BBTopography_getYDim(BBTopograhy_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ydim;
}

void BBTopography_setUlxmap(BBTopograhy_t* self, double ulxmap)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ulxmap = ulxmap;
}

double BBTopography_getUlxmap(BBTopograhy_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ulxmap;
}

void BBTopography_setUlymap(BBTopograhy_t* self, double ulymap)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->ulymap = ulymap;
}

double BBTopography_getUlymap(BBTopograhy_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return self->ulymap;
}

int BBTopography_createData(BBTopograhy_t* self, long ncols, long nrows, RaveDataType type)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_createData(self->data, ncols, nrows, type);
}

int BBTopography_setDatafield(BBTopograhy_t* self, RaveData2D_t* datafield)
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

RaveData2D_t* BBTopography_getDatafield(BBTopograhy_t* self)
{
  RaveData2D_t* result = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");

  result = RAVE_OBJECT_CLONE(self->data);
  if (result == NULL) {
    RAVE_ERROR0("Failed to clone data field");
  }

  return result;
}

long BBTopography_getNcols(BBTopograhy_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getXsize(self->data);
}

long BBTopography_getNrows(BBTopograhy_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getYsize(self->data);
}

int BBTopography_getValue(BBTopograhy_t* self, long col, long row, double* v)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_getValue(self->data, col, row, v);
}

int BBTopography_setValue(BBTopograhy_t* self, long col, long row, double value)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return RaveData2D_setValue(self->data, col, row, value);
}

/*@} End of Interface functions */

RaveCoreObjectType BBTopograhy_TYPE = {
    "BBTopograhy",
    sizeof(BBTopograhy_t),
    BBTopograhy_constructor,
    BBTopograhy_destructor,
    BBTopograhy_copyconstructor
};
