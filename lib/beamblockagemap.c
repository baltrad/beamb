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

/**
 * Internally used struct when reading header info from a TOPO30-file
 */
typedef struct _map_info {
  double ulxmap; /**< ulxmap */
  double ulymap; /**< ulymap */
  int nbits;     /**< nbits */
  int nrows;     /**< nrows */
  int ncols;     /**< ncols */
  double xdim;   /**< xdim */
  double ydim;   /**< ydim */
} map_info;

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
 * Reads the gropo30 header file and populates the info struct
 * @param[in] self - self
 * @param[in] filename - the gtopo file
 * @param[in] info - the structure
 */
static int BeamBlockageMapInternal_readTopographyHeader(BeamBlockageMap_t* self, const char* filename, map_info* info)
{
  /* Declaration of variables */
  char fname[1024];
  char line[100];
  char token[64];
  double f;
  FILE* fp = NULL;
  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((info != NULL), "info == NULL");

  memset(info, 0, sizeof(map_info));

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

  while (fgets(line, sizeof(line), fp) > 0) {
    if (sscanf(line, "%s %lf", token, &f) == 2) {
      if (strcmp(token, "NROWS") == 0) {
        info->nrows = (int)f;
      } else if (strcmp(token, "NCOLS") == 0) {
        info->ncols = (int)f;
      } else if (strcmp(token, "NBITS") == 0) {
        info->nbits = (int)f;
      } else if (strcmp(token, "ULXMAP") == 0) {
        info->ulxmap = f;
      } else if (strcmp(token, "ULYMAP") == 0) {
        info->ulymap = f;
      } else if (strcmp(token, "XDIM") == 0) {
        info->xdim = f;
      } else if (strcmp(token, "YDIM") == 0) {
        info->ydim = f;
      }
    }
  }

  result = 1;
done:
  if (fp != NULL) {
    fclose(fp);
  }
  return result;
}

static RaveField_t* BeamBlockageMapInternal_readData(BeamBlockageMap_t* self, const char* filename, map_info* info)
{
  FILE *fp = NULL;
  char fname[1024];
  short *tmp = NULL;
  RaveField_t *result=NULL, *field = NULL;
  long x, y;
  long nitems = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((info != NULL), "info == NULL");

  if (filename == NULL) {
    goto done;
  }

  if (self->topodir != NULL) {
    sprintf(fname, "%s/%s.DEM", self->topodir, filename);
  } else {
    sprintf(fname, "%s.DEM", filename);
  }

  nitems = info->ncols * info->nrows;

  tmp = RAVE_MALLOC(sizeof(short)*nitems);
  if (tmp == NULL) {
    goto done;
  }

  fp = fopen(fname, "rb");
  if (fp == NULL) {
    goto done;
  }

  if (fread(tmp, sizeof(short), nitems, fp) != nitems) {
    RAVE_WARNING0("Could not read correct number of items");
  }

  field = RAVE_OBJECT_NEW(&RaveField_TYPE);
  if (field == NULL || !RaveField_createData(field, info->ncols, info->nrows, RaveDataType_SHORT)) {
    goto done;
  }

  for (x = 0; x < info->ncols; x++) {
    for (y = 0; y < info->nrows; y++) {
      RaveField_setValue(field, x, y, (double)ntohs((short)(tmp[y*info->ncols + x])));
    }
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_FREE(tmp);
  RAVE_OBJECT_RELEASE(field);
  if (fp != NULL) {
    fclose(fp);
  }
  return result;
}

/**
 * Find out which maps are needed to cover given area
 * @param[in] lat - latitude of radar in degrees
 * @param[in] lon - longitude of radar in degrees
 * @param[in] d - maximum range of radar in meters
 * @returns flag corresponding to map to be read
 */
RaveField_t* BeamBlockageMap_readTopography(BeamBlockageMap_t* self, double lat, double lon, double d)
{
  double lat_e, lon_e, lat_w, lon_w, lat_n, lat_s;
  double earthRadius = 0.0;
  RaveField_t *field = NULL, *result = NULL;
  map_info minfo;

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
    if (!BeamBlockageMapInternal_readTopographyHeader(self, "W020N90", &minfo)) {
      goto done;
    }
    if ((field = BeamBlockageMapInternal_readData(self, "W020N90", &minfo)) == NULL) {
      goto done;
    }
  } else if (RAD2DEG(lon_w) > 20.0) {
    // Read E020N90
    if (!BeamBlockageMapInternal_readTopographyHeader(self, "E020N90", &minfo)) {
      goto done;
    }
    if ((field = BeamBlockageMapInternal_readData(self, "E020N90", &minfo)) == NULL) {
      goto done;
    }
  } else {
    // Read both
    map_info minfo2;
    map_info minfo3;
    RaveField_t *field2 = NULL;
    RaveField_t *field3 = NULL;

    if (!BeamBlockageMapInternal_readTopographyHeader(self, "W020N90", &minfo2) ||
        !BeamBlockageMapInternal_readTopographyHeader(self, "E020N90", &minfo3)) {
      goto done;
    }

    field2 = BeamBlockageMapInternal_readData(self, "W020N90", &minfo2);
    field3 = BeamBlockageMapInternal_readData(self, "E020N90", &minfo3);

    if (field2 != NULL && field3 != NULL) {
      field = RaveField_concatX(field2, field3);
    }

    memcpy(&minfo, &minfo2, sizeof(map_info));
    minfo.ncols += minfo3.ncols;

    RAVE_OBJECT_RELEASE(field2);
    RAVE_OBJECT_RELEASE(field3);
  }

  if (field != NULL) {
    // TODO: Add attributes to field
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
