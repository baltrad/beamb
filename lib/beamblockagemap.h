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
 * Beam-blockage topography map reading functionallity
 * @file
 * @author Anders Henja (SMHI)
 * @date 2011-11-14
 */
#ifndef BEAMBLOCKAGEMAP_H
#define BEAMBLOCKAGEMAP_H
#include "rave_object.h"
#include "rave_field.h"

/**
 * Defines a beam blockage object
 */
typedef struct _BeamBlockageMap_t BeamBlockageMap_t;

/**
 * Type definition to use when creating a rave object.
 */
extern RaveCoreObjectType BeamBlockageMap_TYPE;

/**
 * Sets the topo30 directory.
 * @param[in] self - self
 * @param[in] topodirectory - the topo directory
 * @return 1 on success otherwise 0
 */
int BeamBlockageMap_setTopo30Directory(BeamBlockageMap_t* self, const char* topodirectory);

/**
 * Returns the topo30 directory
 * @param[in] self - self
 * @return the topo30 directory
 */
const char* BeamBlockageMap_getTopo30Directory(BeamBlockageMap_t* self);

/**
 * Find out which maps are needed to cover given area
 * @param[in] lat - latitude of radar in radians
 * @param[in] lon - longitude of radar in radians
 * @param[in] d - maximum range of radar in meters
 * @returns flag corresponding to map to be read
 */
RaveField_t* BeamBlockageMap_readTopography(BeamBlockageMap_t* self, double lat, double lon, double d);

#endif /* BEAMBLOCKAGEMAP_H */
