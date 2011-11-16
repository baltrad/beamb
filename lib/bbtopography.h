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
 * Beam-blockage topography field
 * @file
 * @author Anders Henja (SMHI)
 * @date 2011-11-16
 */
#ifndef BBTOPOGRAPHY_H
#define BBTOPOGRAPHY_H
#include "rave_object.h"
#include "rave_data2d.h"

/**
 * Defines a beam blockage object
 */
typedef struct _BBTopograhy_t BBTopograhy_t;

/**
 * Type definition to use when creating a rave object.
 */
extern RaveCoreObjectType BBTopograhy_TYPE;

/**
 * Sets the nodata value
 * @param[in] self - self
 * @param[in] nodata - the nodata value
 */
void BBTopography_setNodata(BBTopograhy_t* self, double nodata);

/**
 * Return the nodata value
 * @param[in] self - self
 * @return the nodata value
 */
double BBTopography_getNodata(BBTopograhy_t* self);

/**
 * Sets the x-scale in radians.
 * @param[in] self - self
 * @param[in] xdim - the x-scale (step-size) in radians
 */
void BBTopography_setXDim(BBTopograhy_t* self, double xdim);

/**
 * Return the x-scale in radians.
 * @param[in] self - self
 * @return the x-scale (step size) in radians
 */
double BBTopography_getXDim(BBTopograhy_t* self);

/**
 * Sets the y-scale in radians.
 * @param[in] self - self
 * @param[in] ydim - the y-scale (step-size) in radians
 */
void BBTopography_setYDim(BBTopograhy_t* self, double ydim);

/**
 * Return the y-scale in radians.
 * @param[in] self - self
 * @return the y-scale (step size) in radians
 */
double BBTopography_getYDim(BBTopograhy_t* self);

/**
 * Sets the upper left x-coordinate (longitude) in the topography map (in radians).
 * @param[in] self - self
 * @param[in] ulxmap - the upper left x-coordinate (longitude in radians))
 */
void BBTopography_setUlxmap(BBTopograhy_t* self, double ulxmap);

/**
 * Return the upper left x-coordinate (longitude) in the topography map (in radians).
 * @param[in] self - self
 * @return the upper left x-coordinate (longitude in radians))
 */
double BBTopography_getUlxmap(BBTopograhy_t* self);

/**
 * Sets the upper left y-coordinate (latitude) in the topography map (in radians).
 * @param[in] self - self
 * @param[in] ulxmap - the upper left y-coordinate (latitude in radians))
 */
void BBTopography_setUlymap(BBTopograhy_t* self, double ulymap);

/**
 * Return the upper left y-coordinate (latitude) in the topography map (in radians).
 * @param[in] self - self
 * @return the upper left y-coordinate (latitude in radians))
 */
double BBTopography_getUlymap(BBTopograhy_t* self);

/**
 * Creates a empty data field
 * @param[in] self - self
 * @param[in] ncols - the ncols
 * @param[in] nrows - the nrows
 * @param[in] type - the data type
 * @returns 1 on success otherwise 0
 */
int BBTopography_createData(BBTopograhy_t* self, long ncols, long nrows, RaveDataType type);

/**
 * Sets the data in the topography field.
 * @param[in] self - self
 * @param[in] ncols - the column count
 * @param[in] nrows - the row count
 * @param[in] data - the data
 * @param[in] type - the data type
 * @returns 1 on success otherwise 0
 */
int BBTopography_setData(BBTopograhy_t* self, long ncols, long nrows, void* data, RaveDataType type);
/**
 * Returns a pointer to the internal data storage.
 * @param[in] self - self
 * @return the internal data pointer (NOTE! Do not release this pointer)
 */
void* BBTopography_getData(BBTopograhy_t* self);

/**
 * Sets the rave data 2d field. This will create a clone from the provided data field.
 * @param[in] self - self
 * @param[in] datafield - the data field to use (MAY NOT BE NULL)
 * @return 1 on success otherwise 0
 */
int BBTopography_setDatafield(BBTopograhy_t* self, RaveData2D_t* datafield);

/**
 * Returns the 2d field associated with this topography field. Note, it is a
 * clone so don't expect that any modifications will modify the rave fields
 * data array.
 * @param[in] field - self
 * @returns a clone of the internal data array on success otherwise NULL
 */
RaveData2D_t* BBTopography_getDatafield(BBTopograhy_t* self);

/**
 * Returns the number of columns.
 * @param[in] self - self
 * @return the number of columns
 */
long BBTopography_getNcols(BBTopograhy_t* self);

/**
 * Returns the number of rows.
 * @param[in] self - self
 * @return the number of rows
 */
long BBTopography_getNrows(BBTopograhy_t* self);

/**
 * Returns the data type
 * @param[in] self - self
 * @return the data type
 */
RaveDataType BBTopography_getDataType(BBTopograhy_t* self);

/**
 * Returns the value at the specified index.
 * @param[in] self - self
 * @param[in] col - the column
 * @param[in] row - the row
 * @param[out] v - the data at the specified index
 * @return 1 on success, 0 otherwise
 */
int BBTopography_getValue(BBTopograhy_t* self, long col, long row, double* v);

/**
 * Sets the value at specified position
 * @param[in] self - self
 * @param[in] col - the column
 * @param[in] row - the row
 * @param[in] value - the value to be set at specified coordinate
 */
int BBTopography_setValue(BBTopograhy_t* self, long col, long row, double value);

#endif /* BEAMBLOCKAGE_H */
