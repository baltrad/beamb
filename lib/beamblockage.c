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
  RaveData2D_t *lon = NULL, *lat = NULL;
  BBTopography_t* topo = NULL;
  double lon_min = 0.0, lon_max = 0.0, lat_min = 0.0, lat_max = 0.0;

  long ulx = 0, uly = 0, llx = 0, lly = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");
  if (scan == NULL) {
    return NULL;
  }

  topo = BeamBlockageMap_readTopography(self->mapper,
                                        PolarScan_getLatitude(scan),
                                        PolarScan_getLongitude(scan),
                                        PolarScan_getMaxDistance(scan));
  if (topo == NULL) {
    goto done;
  }

#ifdef KALLE
  if (!BeamBlockageInternal_getLonLatFields(scan, &lon, &lat)) {
    goto done;
  }

  if (!BeamBlockageInternal_min(lon, &lon_min) ||
      !BeamBlockageInternal_max(lon, &lon_max) ||
      !BeamBlockageInternal_min(lat, &lat_min) ||
      !BeamBlockageInternal_max(lon, &lat_max)) {
    goto done;
  }
  lon_min = floor((lon_min - BBTopography_getUlxmap(topo)) / BBTopography_getXDim(topo)) - 1;
  lon_max = ceil((lon_max - BBTopography_getUlxmap(topo)) / BBTopography_getXDim(topo)) + 1;
  lat_max = floor((BBTopography_getUlymap(topo) - lat_max) / BBTopography_getYDim(topo)) - 1;
  lat_min = ceil((BBTopography_getUlymap(topo) - lat_min) / BBTopography_getYDim(topo)) + 1;

#endif
  /*
  lon_min = floor( (min_double(lon,ri*ai)-ulxmap) / xdim ) - 1;
  lon_max = ceil( (max_double(lon,ri*ai)-ulxmap) / xdim ) + 1;
  lat_max = floor( (ulymap-max_double(lat,ri*ai)) / ydim ) - 1;
  lat_min = ceil( (ulymap-min_double(lat,ri*ai)) / ydim ) + 1;
  */
  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  RAVE_OBJECT_RELEASE(lon);
  RAVE_OBJECT_RELEASE(lat);
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
