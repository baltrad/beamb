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
#include "beamblockage.h"
#include "beamblockagemap.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include "math.h"
#include <string.h>

/**
 * Represents the beam blockage algorithm
 */
struct _BeamBlockage_t {
  RAVE_OBJECT_HEAD /** Always on top */
  BeamBlockageMap_t* mapper; /**< the topography reader */
};

/*@{ Private functions */
/**
 * Constructor.
 */
static int BeamBlockage_constructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
  self->mapper = RAVE_OBJECT_NEW(&BeamBlockageMap_TYPE);

  if (self->mapper == NULL) {
	  goto error;
  }

  return 1;
error:
  RAVE_OBJECT_RELEASE(self->mapper);
  return 0;
}

/**
 * Destroys the polar navigator
 * @param[in] polnav - the polar navigator to destroy
 */
static void BeamBlockage_destructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
  RAVE_OBJECT_RELEASE(self->mapper);
}

/**
 * Copy constructor
 */
static int BeamBlockage_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  BeamBlockage_t* this = (BeamBlockage_t*)obj;
  BeamBlockage_t* src = (BeamBlockage_t*)srcobj;
  this->mapper = RAVE_OBJECT_CLONE(src->mapper);

  if (this->mapper == NULL) {
    goto error;
  }
  return 1;
error:
  RAVE_OBJECT_RELEASE(this->mapper);
  return 0;
}

/**
 * Get range from radar, projected on surface
 * @param[in] lat0 - latitude in radians
 * @param[in] alt0 - radar altitude in meters
 * @param[in] el - elevation of radar beam in radians
 * @param[in] r_len - length of vector range
 */
static double* BeamBlockageInternal_computeGroundRange(BeamBlockage_t* self, PolarScan_t* scan)
{
  double *result = NULL, *ranges = NULL;
  double elangle = 0.0;
  long nbins = 0;
  long i = 0;
  double rscale = 0.0;

  PolarNavigator_t* navigator = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((scan != NULL), "scan == NULL");

  navigator = PolarScan_getNavigator(scan);
  nbins = PolarScan_getNbins(scan);
  rscale = PolarScan_getRscale(scan);

  ranges = RAVE_MALLOC(sizeof(double) * nbins);
  if (ranges == NULL) {
    RAVE_CRITICAL0("Failed to allocate memory");
    goto done;
  }

  for (i = 0; i < nbins; i++) {
    double d = 0.0, h = 0.0;
    PolarNavigator_reToDh(navigator, (rscale * ((double)i + 0.5)), elangle, &d, &h);
    ranges[i] = d;
  }

  result = ranges;
  ranges = NULL; // Drop responsibility
done:
  RAVE_OBJECT_RELEASE(navigator);
  RAVE_FREE(ranges);
  return result;
}

/*@} End of Private functions */

/*@{ Interface functions */
int BeamBlockage_setTopo30Directory(BeamBlockage_t* self, const char* topodirectory)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return BeamBlockageMap_setTopo30Directory(self->mapper, topodirectory);
}

const char* BeamBlockage_getTopo30Directory(BeamBlockage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return (const char*)BeamBlockageMap_getTopo30Directory(self->mapper);
}

RaveField_t* BeamBlockage_getBlockage(BeamBlockage_t* self, PolarScan_t* scan)
{
  RaveField_t *field = NULL, *result = NULL;
  double* ground_range = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  if (scan == NULL) {
    return NULL;
  }
  ground_range = BeamBlockageInternal_computeGroundRange(self, scan);
  if (ground_range == NULL) {
    goto done;
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/*@} End of Interface functions */

RaveCoreObjectType BeamBlockage_TYPE = {
    "BeamBlockage",
    sizeof(BeamBlockage_t),
    BeamBlockage_constructor,
    BeamBlockage_destructor,
    BeamBlockage_copyconstructor
};
