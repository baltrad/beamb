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
#include "config.h"

/**
 * Represents the beam blockage algorithm
 */
struct _BeamBlockage_t {
  RAVE_OBJECT_HEAD /** Always on top */
  BeamBlockageMap_t* mapper; /**< the topography reader */
  char* cachedir;            /**< the cache directory */
};

/*@{ Private functions */
/**
 * Constructor.
 */
static int BeamBlockage_constructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
  self->cachedir = NULL;
  self->mapper = RAVE_OBJECT_NEW(&BeamBlockageMap_TYPE);

  if (self->mapper == NULL || !BeamBlockage_setCacheDirectory(self, BEAMB_CACHE_DIR)) {
	  goto error;
  }

  return 1;
error:
  RAVE_OBJECT_RELEASE(self->mapper);
  RAVE_FREE(self->cachedir);
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
  RAVE_FREE(self->cachedir);
}

/**
 * Copy constructor
 */
static int BeamBlockage_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  BeamBlockage_t* this = (BeamBlockage_t*)obj;
  BeamBlockage_t* src = (BeamBlockage_t*)srcobj;
  this->mapper = RAVE_OBJECT_CLONE(src->mapper);
  this->cachedir = NULL;

  if (this->mapper == NULL || !BeamBlockage_setCacheDirectory(this, src->cachedir)) {
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

/**
 * Generate a running cumulative maximum.
 * @param[in] p - pointer to array of values
 * @param[in] nlen - length of vector p
 */
static void BeamBlockageInternal_cummax(double* p, long nlen)
{
  double t = 0.0;
  long i = 0;

  RAVE_ASSERT((p != NULL), "p == NULL");
  if (nlen <= 0) {
    RAVE_WARNING0("Trying to generate a cumulative max without any data");
    return;
  }

  t = p[0];
  for (i = 1; i < nlen; i++) {
    if (p[i] < t) {
      p[i] = t;
    } else {
      t = p[i];
    }
  }
}

/**
 * Creates a full filename from the information in the scan file and the cache dir name. If
 * cachedir is NULL, only the filename will be set.
 * We format the filename like this.
 *   lon_lat_height_elangle_nrays_nbins_rscale_rstart_beamwidth
 *   All floating point values except height are represented with 2 decimals
 *
 * @param[in] self - self
 * @param[in] scan - scan
 * @param[in] filename - the allocated array where the filename should be written
 * @param[in] len - the length of the allocated array
 * @return 1 on success otherwise 0
 */
static int BeamBlockageInternal_createCacheFilename(BeamBlockage_t* self, PolarScan_t* scan, char* filename, int len)
{
  int result = 0;
  double lat, lon, height, bw, elangle, rscale, rstart;
  long nrays, nbins;
  int elen = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((scan != NULL), "scan == NULL");

  lat = PolarScan_getLatitude(scan) * 180.0 / M_PI;
  lon = PolarScan_getLongitude(scan) * 180.0 / M_PI;
  height = PolarScan_getHeight(scan);
  bw = PolarScan_getBeamwidth(scan) * 180.0 / M_PI;
  nrays = PolarScan_getNrays(scan);
  nbins = PolarScan_getNbins(scan);
  elangle = PolarScan_getElangle(scan) * 180.0 / M_PI;
  rscale = PolarScan_getRscale(scan);
  rstart = PolarScan_getRstart(scan);

  if (self->cachedir == NULL) {
    elen = snprintf(filename, len,
                    "%.2f_%.2f_%.0f_%.2f_%ld_%ld_%.2f_%.2f_%.2f.h5\n",
                    lon, lat, height, elangle, nrays, nbins, rscale, rstart, bw);
  } else {
    elen = snprintf(filename, len,
                    "%s/%.2f_%.2f_%.0f_%.2f_%ld_%ld_%.2f_%.2f_%.2f.h5\n",
                    self->cachedir, lon, lat, height, elangle, nrays, nbins, rscale, rstart, bw);
  }

  if (elen >= len) {
    RAVE_ERROR0("Not enough room was created for filename");
    goto done;
  }

  result = 1;
done:
  return result;
}

/**
 * Returns a cached file matching the given scan if there is one.
 * @param[in] self - self
 * @param[in] scan - the scan
 */
static RaveField_t* BeamBlockageInternal_getCachedFile(BeamBlockage_t* self, PolarScan_t* scan)
{
  RaveField_t* result = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((scan != NULL), "scan == NULL");

  if (self->cachedir != NULL) {
    char filename[512];
    if (!BeamBlockageInternal_createCacheFilename(self, scan, filename, 512)) {
      goto done;
    }
    fprintf(stderr, "FILENAME: %s\n", filename);
  }

done:
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

int BeamBlockage_setCacheDirectory(BeamBlockage_t* self, const char* cachedir)
{
  char* tmp = NULL;
  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (cachedir != NULL) {
    tmp = RAVE_STRDUP(cachedir);
    if (tmp == NULL) {
      goto done;
    }
  }
  self->cachedir = tmp;
  tmp = NULL; // Release responsibility for memory
  result = 1;
done:
  RAVE_FREE(tmp);
  return result;
}

const char* BeamBlockage_getCacheDirectory(BeamBlockage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return (const char*)self->cachedir;
}

RaveField_t* BeamBlockage_getBlockage(BeamBlockage_t* self, PolarScan_t* scan, double dBlim)
{
  RaveField_t *field = NULL, *result = NULL;
  BBTopography_t *topo = NULL;
  double RE = 0.0, R = 0;
  PolarNavigator_t* navigator = NULL;
  long bi = 0, ri = 0;
  long nbins = 0, nrays = 0;
  double* phi = NULL;
  double* groundRange = NULL;
  double height = 0.0;
  double beamwidth = 0.0, elangle = 0.0;
  double c, elLim, bb_tot;
  double elBlock = 0.0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (scan == NULL) {
    return NULL;
  }

  BeamBlockageInternal_getCachedFile(self, scan);

  navigator = PolarScan_getNavigator(scan);
  if (navigator == NULL) {
    RAVE_ERROR0("Scan does not have a polar navigator instance attached");
    goto done;
  }

  groundRange = BeamBlockageInternal_computeGroundRange(self, scan);
  if (groundRange == NULL) {
    goto done;
  }

  topo = BeamBlockageMap_getTopographyForScan(self->mapper, scan);
  if (topo == NULL) {
    goto done;
  }

  nbins = PolarScan_getNbins(scan);
  nrays = PolarScan_getNrays(scan);

  field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (field == NULL || !RaveField_createData(field, nbins, nrays, RaveDataType_DOUBLE)) {
    goto done;
  }

  phi = RAVE_MALLOC(sizeof(double)*nbins);
  if (phi == NULL) {
    goto done;
  }

  RE = PolarNavigator_getEarthRadiusOrigin(navigator);
  R = 1.0/((1.0/RE) + PolarNavigator_getDndh(navigator));
  height = PolarNavigator_getAlt0(navigator);

  beamwidth = PolarScan_getBeamwidth(scan);
  elangle = PolarScan_getElangle(scan);

  /* Width of Gaussian */
  c = -((beamwidth/2.0)*(beamwidth/2.0))/log(0.5);

  /* Elevation limits */
  elLim = sqrt( -c*log(pow(10.0,(dBlim/10.0)) ) );

  /* Find total blockage within -elLim to +elLim */
  bb_tot = sqrt(M_PI*c) * erf(elLim/sqrt(c));

  for (ri = 0; ri < nrays; ri++) {
    for (bi = 0; bi < nbins; bi++) {
      double v = 0.0;
      BBTopography_getValue(topo, bi, ri, &v);
      phi[bi] = asin((((v+R)*(v+R)) - (groundRange[bi]*groundRange[bi]) - ((R+height)*(R+height))) / (2*groundRange[bi]*(R+height)));
      //fprintf(stderr, "ri=%d, bi=%d => phi(%d) = %f\n", ri, bi, bi, phi[bi]);
      //phi[j] = asin((pow(v + R, 2) - pow(groundRange[bi],2) - pow(R + height,2) ) / (2*groundRange[bi]*(R+height)));
    }
    BeamBlockageInternal_cummax(phi, nbins);

    for (bi = 0; bi < nbins; bi++) {
      double bbval = 0.0;
      elBlock = phi[bi];
      if (elBlock < elangle - elLim) {
        elBlock = -9999.0;
      }
      if (elBlock > elangle + elLim) {
        elBlock = elangle + elLim;
      }
      bbval = -1.0/2.0 * sqrt(M_PI * c) * (erf((elangle - elBlock)/sqrt(c)) - erf(elLim/sqrt(c)))/bb_tot;
      RaveField_setValue(field, bi, ri, bbval);
      /*bb[i*ri+j] = -1./2. * sqrt(M_PI*c) * ( erf( (el-elBlock[i*ri+j]) / \
                      sqrt(c) ) - erf( (el-(el-elLim))/sqrt(c) ) ) / bb_tot; */
    }
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(navigator);
  RAVE_OBJECT_RELEASE(field);
  RAVE_OBJECT_RELEASE(topo);
  RAVE_FREE(phi);
  RAVE_FREE(groundRange);
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
