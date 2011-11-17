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
#include "beamblockagemap.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include "math.h"
#include <string.h>
#include "rave_field.h"
#include "polarnav.h"
#include <stdio.h>
#include <arpa/inet.h>

/**
 * Represents the beam blockage algorithm
 */
struct _BeamBlockageMap_t {
  RAVE_OBJECT_HEAD /** Always on top */
  char* topodir;   /**< the topo30 directory */
  PolarNavigator_t* navigator; /**< the navigator */
};

/**
 * Converts a radian to a degree
 */
#define RAD2DEG(rad) (rad*180.0/M_PI)

/**
 * Converts a degree into a radian
 */
#define DEG2RAD(deg) (deg*M_PI/180.0)

/*@{ Private functions */
/**
 * Constructor.
 */
static int BeamBlockageMap_constructor(RaveCoreObject* obj)
{
  BeamBlockageMap_t* self = (BeamBlockageMap_t*)obj;
  self->topodir = NULL;
  self->navigator = RAVE_OBJECT_NEW(&PolarNavigator_TYPE);

  if (self->navigator == NULL) {
    goto error;
  }

  return 1;
error:
  RAVE_OBJECT_RELEASE(self->navigator);
  return 0;
}

/**
 * Destroys the polar navigator
 * @param[in] polnav - the polar navigator to destroy
 */
static void BeamBlockageMap_destructor(RaveCoreObject* obj)
{
  BeamBlockageMap_t* self = (BeamBlockageMap_t*)obj;
  RAVE_FREE(self->topodir);
  RAVE_OBJECT_RELEASE(self->navigator);
}

/**
 * Copy constructor
 */
static int BeamBlockageMap_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  BeamBlockageMap_t* this = (BeamBlockageMap_t*)obj;
  BeamBlockageMap_t* src = (BeamBlockageMap_t*)srcobj;
  this->topodir = NULL;
  this->navigator = RAVE_OBJECT_CLONE(src->navigator);
  if (!BeamBlockageMap_setTopo30Directory(this, BeamBlockageMap_getTopo30Directory(src)) ||
      this->navigator == NULL) {
    goto error;
  }
  return 1;
error:
  RAVE_FREE(this->topodir);
  RAVE_OBJECT_RELEASE(this->navigator);
  return 0;
}

/**
 * Reads the gropo30 header file and populates the BBTopography instance with header information.
 * @param[in] self - self
 * @param[in] filename - the gtopo file
 * @param[in] info - the structure
 */
static BBTopography_t* BeamBlockageMapInternal_readHeader(BeamBlockageMap_t* self, const char* filename)
{
  /* Declaration of variables */
  char fname[1024];
  char line[100];
  char token[64];
  double f;
  FILE* fp = NULL;
  BBTopography_t *result = NULL, *field = NULL;
  long ncols = 0, nrows = 0;
  int nbits = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (filename == NULL) {
    goto done;
  }

  if (self->topodir != NULL) {
    sprintf(fname, "%s/%s.HDR", self->topodir, filename);
  } else {
    sprintf(fname, "%s.HDF", filename);
  }

  fp = fopen(fname, "r");
  if (fp == NULL) {
    RAVE_ERROR1("Failed to open %s for reading", fname);
    goto done;
  }

  field = RAVE_OBJECT_NEW(&BBTopography_TYPE);
  if (field == NULL) {
    goto done;
  }

  while (fgets(line, sizeof(line), fp) > 0) {
    if (sscanf(line, "%s %lf", token, &f) == 2) {
      if (strcmp(token, "NROWS") == 0) {
        nrows = (long)f;
      } else if (strcmp(token, "NCOLS") == 0) {
        ncols = (long)f;
      } else if (strcmp(token, "NBITS") == 0) {
        nbits = (int)f;
      } else if (strcmp(token, "ULXMAP") == 0) {
        BBTopography_setUlxmap(field, f);
      } else if (strcmp(token, "ULYMAP") == 0) {
        BBTopography_setUlymap(field, f);
      } else if (strcmp(token, "XDIM") == 0) {
        BBTopography_setXDim(field, f);
      } else if (strcmp(token, "YDIM") == 0) {
        BBTopography_setYDim(field, f);
      }
    }
  }

  if (nbits != 0 && nbits != 16) {
    RAVE_ERROR0("Only 16bit topography files supported");
    goto done;
  }

  if (nrows == 0 || ncols == 0) {
    RAVE_ERROR0("NROWS / NCOLS must not be 0");
    goto done;
  }

  if (!BBTopography_createData(field, ncols, nrows, RaveDataType_SHORT));

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  if (fp != NULL) {
    fclose(fp);
  }
  return result;
}

static int BeamBlockageMapInternal_fillData(BeamBlockageMap_t* self, const char* filename, BBTopography_t* field)
{
  int result = 0;
  FILE *fp = NULL;
  char fname[1024];
  short *tmp = NULL;
  long col, row;
  long nrows = 0, ncols = 0;
  long nitems = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((field != NULL), "field == NULL");

  if (filename == NULL) {
    goto done;
  }

  if (BBTopography_getDataType(field) != RaveDataType_SHORT) {
    RAVE_ERROR0("Only supports reading of short data");
    goto done;
  }

  if (self->topodir != NULL) {
    sprintf(fname, "%s/%s.DEM", self->topodir, filename);
  } else {
    sprintf(fname, "%s.DEM", filename);
  }

  fp = fopen(fname, "rb");
  if (fp == NULL) {
    goto done;
  }

  nrows = BBTopography_getNrows(field);
  ncols = BBTopography_getNcols(field);
  nitems = nrows * ncols;
  tmp = RAVE_MALLOC(sizeof(short)*nitems);
  if (tmp == NULL) {
    RAVE_ERROR0("Failed to allocate memory for data");
    goto done;
  }

  if (fread(tmp, sizeof(short), nitems, fp) != nitems) {
    RAVE_WARNING0("Could not read correct number of items");
  }

  for (col = 0; col < ncols; col++) {
    for (row = 0; row < nrows; row++) {
      BBTopography_setValue(field, col, row, (double)ntohs((short)(tmp[row*ncols + col])));
    }
  }

  result = 1;
done:
  if (fp != NULL) {
    fclose(fp);
  }
  RAVE_FREE(tmp);
  return result;
}

static BBTopography_t* BeamBlockageMapInternal_readTopography(BeamBlockageMap_t* self, const char* filename)
{
  BBTopography_t *result = NULL, *field = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (filename != NULL) {
    field = BeamBlockageMapInternal_readHeader(self, filename);
    if (field != NULL) {
      if (!BeamBlockageMapInternal_fillData(self, filename, field)) {
        goto done;
      }
    }
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/**
 * Find out which maps are needed to cover given area
 * @param[in] lat - latitude of radar in degrees
 * @param[in] lon - longitude of radar in degrees
 * @param[in] d - maximum range of radar in meters
 * @returns flag corresponding to map to be read
 */
BBTopography_t* BeamBlockageMap_readTopography(BeamBlockageMap_t* self, double lat, double lon, double d)
{
  double lat_e, lon_e, lat_w, lon_w, lat_n, lat_s;
  double earthRadius = 0.0;
  BBTopography_t *field = NULL, *result = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  earthRadius = PolarNavigator_getEarthRadius(self->navigator, lat);

  /* Find out which map is needed
     see http://www.movable-type.co.uk/scripts/latlong.html */
  lat_e = asin(sin(lat) * cos(d/earthRadius) + cos(lat) * sin(d/earthRadius) * cos(M_PI/2.));
  lon_e = lon + atan2( sin(M_PI/2.) * sin(d/earthRadius) * cos(lat), \
          cos(d/earthRadius) - sin(lat) * sin(lat_e) );
  lat_w = asin( sin(lat) * cos(d/earthRadius) + cos(lat) * sin(d/earthRadius) * cos(3.*M_PI/2.) );
  lon_w = lon + atan2( sin(3.*M_PI/2.) * sin(d/earthRadius) * cos(lat), \
          cos(d/earthRadius) - sin(lat) * sin(lat_w) );
  lat_n = asin( sin(lat) * cos(d/earthRadius) + cos(lat) * sin(d/earthRadius) * cos(0.) );
  lat_s = asin( sin(lat) * cos(d/earthRadius) + cos(lat) * sin(d/earthRadius) * cos(M_PI) );

  if (RAD2DEG(lat_n) > 80.0 || RAD2DEG(lat_s) < 40.0 || RAD2DEG(lon_w) < -20.0 || RAD2DEG(lon_e) > 60.0) {
    RAVE_ERROR0("Topography maps does not cover requested area");
    goto done;
  } else if (RAD2DEG(lon_e) <= 20.0) {
    // Read W020N90
    if ((field = BeamBlockageMapInternal_readTopography(self, "W020N90")) == NULL) {
      goto done;
    }
  } else if (RAD2DEG(lon_w) > 20.0) {
    // Read E020N90
    if ((field = BeamBlockageMapInternal_readTopography(self, "E020N90")) == NULL) {
      goto done;
    }
  } else {
    // Read both
    BBTopography_t *field2 = BeamBlockageMapInternal_readTopography(self, "W020N90");
    BBTopography_t *field3 = BeamBlockageMapInternal_readTopography(self, "E020N90");
    if (field2 != NULL && field3 != NULL) {
      field = BBTopography_concatX(field2, field3);
    }
    RAVE_OBJECT_RELEASE(field2);
    RAVE_OBJECT_RELEASE(field3);
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}

/*@{ Interface functions */
int BeamBlockageMap_setTopo30Directory(BeamBlockageMap_t* self, const char* topodirectory)
{
  char* tmp = NULL;
  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (topodirectory != NULL) {
    tmp = RAVE_STRDUP(topodirectory);
    if (tmp == NULL) {
      RAVE_ERROR0("Failed to duplicate string");
      goto done;
    }
  }

  RAVE_FREE(self->topodir);
  self->topodir = tmp;
  tmp = NULL; // Not responsible for memory any longer
  result = 1;
done:
  RAVE_FREE(tmp);
  return result;
}

const char* BeamBlockageMap_getTopo30Directory(BeamBlockageMap_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return (const char*)self->topodir;
}

/*@} End of Interface functions */

RaveCoreObjectType BeamBlockageMap_TYPE = {
    "BeamBlockageMap",
    sizeof(BeamBlockageMap_t),
    BeamBlockageMap_constructor,
    BeamBlockageMap_destructor,
    BeamBlockageMap_copyconstructor
};
