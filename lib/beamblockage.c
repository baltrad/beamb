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
#include "rave_debug.h"
#include "rave_alloc.h"
#include "math.h"
#include <string.h>
#include "rave_field.h"
#include "polarnav.h"

/**
 * Represents the beam blockage algorithm
 */
struct _BeamBlockage_t {
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
static int BeamBlockage_constructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
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
static void BeamBlockage_destructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
  RAVE_FREE(self->topodir);
  RAVE_OBJECT_RELEASE(self->navigator);
}

/**
 * Copy constructor
 */
static int BeamBlockage_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  BeamBlockage_t* this = (BeamBlockage_t*)obj;
  BeamBlockage_t* src = (BeamBlockage_t*)srcobj;
  this->topodir = NULL;
  this->navigator = RAVE_OBJECT_CLONE(src->navigator);
  if (!BeamBlockage_setTopo30Directory(this, BeamBlockage_getTopo30Directory(src)) ||
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
static int BeamBlockageInternal_readTopographyHeader(BeamBlockage_t* self, const char* filename, map_info* info)
{
  /* Declaration of variables */
  char fname[1024];
  char* s = NULL;
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

static RaveField_t* BeamBlockageInternal_readData(BeamBlockage_t* self, const char* filename, map_info* info)
{
  FILE *fp = NULL;
  char fname[1024];
  char* *tmp = NULL;
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
      RaveField_setValue(field, x, y, ntohs(tmp[y*info->ncols + x]));
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
static RaveField_t* BeamBlockageInternal_readTopography(BeamBlockage_t* self, double lat, double lon, double d)
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
    if (!BeamBlockageInternal_readTopographyHeader(self, "W020N90", &minfo)) {
      goto done;
    }
    if ((field = BeamBlockageInternal_readData(self, "W020N90", &minfo)) == NULL) {
      goto done;
    }
  } else if (RAD2DEG(lon_w) > 20.0) {
    // Read E020N90
    if (!BeamBlockageInternal_readTopographyHeader(self, "E020N90", &minfo)) {
      goto done;
    }
    if ((field = BeamBlockageInternal_readData(self, "E020N90", &minfo)) == NULL) {
      goto done;
    }
  } else {
    // Read both
    map_info minfo2;
    RaveField_t *field2 = NULL;

    if (!BeamBlockageInternal_readTopographyHeader(self, "W020N90", &minfo) ||
        !BeamBlockageInternal_readTopographyHeader(self, "E020N90", &minfo2)) {
      goto done;
    }

    field = BeamBlockageInternal_readData(self, "W020N90", &minfo);
    field2 = BeamBlockageInternal_readData(self, "E020N90", &minfo2);

    if (field == NULL || field2 == NULL) {
      RAVE_OBJECT_RELEASE(field);
      RAVE_OBJECT_RELEASE(field2);
      goto done;
    }

#ifdef KALLE
    /* Check that number of rows and spacing are the same */
    if(map1->nrows == map2->nrows && map1->xdim == map2-> xdim \
        && map1->ydim == map2-> ydim && map1->nbits == map2->nbits)
    {
#endif
  }

done:
  RAVE_OBJECT_RELEASE(field);
  return NULL;
}


/*@{ Interface functions */
int BeamBlockage_setTopo30Directory(BeamBlockage_t* self, const char* topodirectory)
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

const char* BeamBlockage_getTopo30Directory(BeamBlockage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return (const char*)self->topodir;
}

/*@} End of Interface functions */

RaveCoreObjectType BeamBlockage_TYPE = {
    "BeamBlockage",
    sizeof(BeamBlockage_t),
    BeamBlockage_constructor,
    BeamBlockage_destructor,
    BeamBlockage_copyconstructor
};
