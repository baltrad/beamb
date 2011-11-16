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
 * Beam-blockage analysis
 * @file
 * @author Lars Norin (Swedish Meteorological and Hydrological Institute, SMHI)
 *
 * @author Anders Henja (SMHI, refactored to work together with rave)
 * @date 2011-11-10
 */
#ifndef BEAMBLOCKAGE_H
#define BEAMBLOCKAGE_H
#include "rave_object.h"
#include "rave_field.h"
#include "polarscan.h"

/**
 * Defines a beam blockage object
 */
typedef struct _BeamBlockage_t BeamBlockage_t;

/**
 * Type definition to use when creating a rave object.
 */
extern RaveCoreObjectType BeamBlockage_TYPE;

/**
 * Sets the topo30 directory.
 * @param[in] self - self
 * @param[in] topodirectory - the topo directory
 * @return 1 on success otherwise 0
 */
int BeamBlockage_setTopo30Directory(BeamBlockage_t* self, const char* topodirectory);

/**
 * Returns the topo30 directory
 * @param[in] self - self
 * @return the topo30 directory
 */
const char* BeamBlockage_getTopo30Directory(BeamBlockage_t* self);

/**
 * Gets the blockage for the provided scan.
 * @param[in] self - self
 * @param[in] scan - the scan to check blockage
 * @return the beam blockage field
 */
RaveField_t* BeamBlockage_getBlockage(BeamBlockage_t* self, PolarScan_t* scan);

#endif /* BEAMBLOCKAGE_H */
