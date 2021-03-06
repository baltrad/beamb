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
#include "config.h"

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

  if (self->navigator == NULL || !BeamBlockageMap_setTopo30Directory(self, BEAMB_GTOPO30_DIR)) {
    goto error;
  }

  return 1;
error:
  RAVE_OBJECT_RELEASE(self->navigator);
  RAVE_FREE(self->topodir);
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
        BBTopography_setUlxmap(field, f * M_PI/180.0);
      } else if (strcmp(token, "ULYMAP") == 0) {
        BBTopography_setUlymap(field, f * M_PI/180.0);
      } else if (strcmp(token, "XDIM") == 0) {
        BBTopography_setXDim(field, f * M_PI/180.0);
      } else if (strcmp(token, "YDIM") == 0) {
        BBTopography_setYDim(field, f * M_PI/180.0);
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

  if (!BBTopography_createData(field, ncols, nrows, RaveDataType_SHORT)) {
    goto done;
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  if (fp != NULL) {
    fclose(fp);
  }
  return result;
}

/**
 * Fills the data in the topography field. The topography field should first have been created
 * with a call to \ref BeamBlockageMapInternal_readHeader in order to get correct dimensions
 * etc.
 * @param[in] self - self
 * @param[in] filename - the filename (exluding suffix and directory name)
 * @param[in] field - the gtopo30 topography skeleton.
 * @returns 1 on success otherwise 0
 */
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
      short temp = ntohs(tmp[row*ncols + col]);
      BBTopography_setValue(field, col, row, temp);
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

/**
 * Reads the topography with the specified filename. The directory path should not be used, instead
 * the topo30dir variable will be used.
 * @param[in] self - self
 * @param[in] filename - the gtopo30 filename (excluding suffix)
 * @returns the topography on success otherwise NULL
 */
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
 * Creates a topography that is mapped against a specific scan.
 * @param[in] self - self
 * @param[in] topo - the overall topography that hopefully covers the scan
 * @param[in] scan - the scan that should get the topography mapped
 * @return the mapped topography on success otherwise NULL
 */
static BBTopography_t* BeamBlockageMapInternal_createMappedTopography(BeamBlockageMap_t* self, BBTopography_t* topo, PolarScan_t* scan)
{
  BBTopography_t *field = NULL, *result = NULL;
  long nrays = 0, nbins = 0;
  long ri = 0, bi = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((topo != NULL), "topo == NULL");
  RAVE_ASSERT((scan != NULL), "scan == NULL");

  nrays = PolarScan_getNrays(scan);
  nbins = PolarScan_getNbins(scan);

  field = RAVE_OBJECT_NEW(&BBTopography_TYPE);
  if (field == NULL) {
    goto done;
  }
  if (!BBTopography_createData(field, nbins, nrays, BBTopography_getDataType(topo))) {
    RAVE_ERROR0("Failed to create data field");
    goto done;
  }

  for (ri = 0; ri < nrays; ri++) {
    for (bi = 0; bi < nbins; bi++) {
      double lonval = 0.0, latval = 0.0;
      if (PolarScan_getLonLatFromIndex(scan, bi, ri, &lonval, &latval)) {
        double v = 0.0;
        BBTopography_getValueAtLonLat(topo, lonval, latval, &v);
        /* According to original code, no values < 0 are allowed */
        if (v < 0.0) {
          BBTopography_setValue(field, bi, ri, 0.0);
        } else {
          BBTopography_setValue(field, bi, ri, v);
        }
      }
    }
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}


/**
 * Read the actual tiles and concatenate them if required.
 * @param[in] tnames - comma-separated string (no spaces) containing the names of GTOPO30 tiles.
 * If there are two aligned vertically, then the order is: "north,south".
 * If there are two aligned horizontally, then the order is: "west,east".
 * If there are four, then the order is: "nw,ne,sw,se".
 * @param[in] orient - concatenation orientation, when relevant: "v" or "h", otherwise NULL
 * @returns flag corresponding to topography field made
 */
BBTopography_t* BeamBlockageMapInternal_makeTopographyField(BeamBlockageMap_t* self, const char* tnames, const char* orient)
{
	BBTopography_t *field = NULL, *result = NULL;
	const char* delim = ",";
	char* saveptr;
	int i, n;

	/* Determine number of substrings */
	for (i=0, n=1; tnames[i]; i++)
		n += (tnames[i] == ',');

	/* Single tile */
	if (n == 1) {
		if ((field = BeamBlockageMapInternal_readTopography(self, tnames)) == NULL) {
			goto done;
		}
	}

	/* Two tiles */
	else if (n == 2) {
		char* s = RAVE_STRDUP(tnames);
		char* one = strtok_r(s, delim, &saveptr);
		char* two = strtok_r(NULL, delim, &saveptr);
	    BBTopography_t *field1 = BeamBlockageMapInternal_readTopography(self, (const char*)one);
	    BBTopography_t *field2 = BeamBlockageMapInternal_readTopography(self, (const char*)two);
	    if (field1 != NULL && field2 != NULL) {
			if (strcmp(orient, "h")==0) {
				field = BBTopography_concatX(field1, field2);
			} else if (strcmp(orient,"v")==0) {
				field = BBTopography_concatY(field1, field2);
			}
	    }
	    RAVE_FREE(s);
	    RAVE_OBJECT_RELEASE(field1);
	    RAVE_OBJECT_RELEASE(field2);
	}

	/* Four tiles */
	else if (n == 4) {
		char* s = RAVE_STRDUP(tnames);
		char* one = strtok_r(s, delim, &saveptr);
		char* two = strtok_r(NULL, delim, &saveptr);
		char* tre = strtok_r(NULL, delim, &saveptr);
		char* fyr = strtok_r(NULL, delim, &saveptr);
	    BBTopography_t *field1 = BeamBlockageMapInternal_readTopography(self, (const char*)one);
	    BBTopography_t *field2 = BeamBlockageMapInternal_readTopography(self, (const char*)two);
	    BBTopography_t *field3 = BeamBlockageMapInternal_readTopography(self, (const char*)tre);
	    BBTopography_t *field4 = BeamBlockageMapInternal_readTopography(self, (const char*)fyr);
	    if (field1 != NULL && field2 != NULL && field3 != NULL && field4 != NULL) {
	        BBTopography_t *field5 = BBTopography_concatX(field1, field2);
	        BBTopography_t *field6 = BBTopography_concatX(field3, field4);
	        field = BBTopography_concatY(field5, field6);
	        RAVE_OBJECT_RELEASE(field5);
	        RAVE_OBJECT_RELEASE(field6);
	    }
	    RAVE_FREE(s);
	    RAVE_OBJECT_RELEASE(field1);
	    RAVE_OBJECT_RELEASE(field2);
	    RAVE_OBJECT_RELEASE(field3);
	    RAVE_OBJECT_RELEASE(field4);
	}

	result = RAVE_OBJECT_COPY(field);
done:
	RAVE_OBJECT_RELEASE(field);
	return result;
}

/*@} End of Private functions */

/*@{ Interface functions */

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

  lat_e = RAD2DEG(lat_e);  /* For more legible code below */
  lon_e = RAD2DEG(lon_e);
  lat_w = RAD2DEG(lat_w);
  lon_w = RAD2DEG(lon_w);
  lat_n = RAD2DEG(lat_n);
  lat_s = RAD2DEG(lat_s);

  /* Some of the following combination are clearly unrealistic from a radar perspective, but ... */
  /* West to East, North to South */
  /* Row 1 */
  if      ( (lat_s >= 40.0) && (lon_w >= -180.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= -140.0) && (lon_e <= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= -100.0) && (lon_e <= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= -60.0) && (lon_e <= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= -20.0) && (lon_e <= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= 20.0) && (lon_e <= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= 60.0) && (lon_e <= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= 100.0) && (lon_e <= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N90", NULL);
  }
  else if ( (lat_s >= 40.0) && (lon_w >= 140.0) && (lon_e <= 180.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N90", NULL);
  }
  /* Row 2 */
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= -180.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= -140.0) && (lon_e <= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= -100.0) && (lon_e <= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= -60.0) && (lon_e <= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= -20.0) && (lon_e <= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= 20.0) && (lon_e <= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= 60.0) && (lon_e <= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= 100.0) && (lon_e <= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N40", NULL);
  }
  else if ( (lat_s >= -10.0) && (lat_n <= 40.0) && (lon_w >= 140.0) && (lon_e <= 180.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N40", NULL);
  }
  /* Row 3 */
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= -180.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= -140.0) && (lon_e <= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= -100.0) && (lon_e <= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= -60.0) && (lon_e <= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= -20.0) && (lon_e <= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= 20.0) && (lon_e <= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= 60.0) && (lon_e <= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= 100.0) && (lon_e <= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100S10", NULL);
  }
  else if ( (lat_s >= -60.0) && (lat_n <= -10.0) && (lon_w >= 140.0) && (lon_e <= 180.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140S10", NULL);
  }
  /* Horizontally-aligned pairs */
  /* Row 1 */
  else if ( (lat_s >= 40.0) && (lon_w >= 140.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N90,W180N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= -140.0) && (lon_e >= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N90,W140N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= -100.0) && (lon_e >= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N90,W100N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= -60.0) && (lon_e >= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N90,W060N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= -20.0) && (lon_e >= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N90,W020N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= 20.0) && (lon_e >= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N90,E020N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= 60.0) && (lon_e >= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N90,E060N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= 100.0) && (lon_e >= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N90,E100N90", "h");
  }
  else if ( (lat_s >= 40.0) && (lon_w <= 140.0) && (lon_e >= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N90,E140N90", "h");
  }
  /* Row 2 */
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w >= 140.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N40,W180N40", "h");
  }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= -140.0) && (lon_e >= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N40,W140N40", "h");
  }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= -100.0) && (lon_e >= -100.0) ) {
  	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N40,W100N40", "h");
    }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= -60.0) && (lon_e >= -60.0) ) {
  	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N40,W060N40", "h");
    }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= -20.0) && (lon_e >= -20.0) ) {
  	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N40,W020N40", "h");
    }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= 20.0) && (lon_e >= 20.0) ) {
  	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N40,E020N40", "h");
    }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= 60.0) && (lon_e >= 60.0) ) {
  	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N40,E060N40", "h");
    }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= 100.0) && (lon_e >= 100.0) ) {
  	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N40,E100N40", "h");
    }
  else if ( (lat_n <= 40.0) && (lat_s >= -10.0) && (lon_w <= 140.0) && (lon_e >= 140.0) ) {
  	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N40,E140N40", "h");
    }
  /* Row 3 */
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w >= 140.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140S10,W180S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= -140.0) && (lon_e >= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180S10,W140S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= -100.0) && (lon_e >= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140S10,W100S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= -60.0) && (lon_e >= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100S10,W060S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= -20.0) && (lon_e >= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060S10,W020S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= 20.0) && (lon_e >= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020S10,E020S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= 60.0) && (lon_e >= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020S10,E060S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= 100.0) && (lon_e >= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060S10,E100S10", "h");
  }
  else if ( (lat_n <= -10.0) && (lat_s >= -60.0) && (lon_w <= 140.0) && (lon_e >= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100S10,E140S10", "h");
  }
  /* Vertically-aligned pairs, North to south, West to East */
  /* Column 1 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N90,W180N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N40,W180S10", "v");
  }
  /* Column 2 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= -140.0) && (lon_e <= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N90,W140N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= -140.0) && (lon_e <= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N40,W140S10", "v");
  }
  /* Column 3 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= -100.0) && (lon_e <= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N90,W100N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= -100.0) && (lon_e <= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N40,W100S10", "v");
  }
  /* Column 4 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= -60.0) && (lon_e <= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N90,W060N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= -60.0) && (lon_e <= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N40,W060S10", "v");
  }
  /* Column 5 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= -20.0) && (lon_e <= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N90,W020N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= -20.0) && (lon_e <= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N40,W020S10", "v");
  }
  /* Column 6 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= 20.0) && (lon_e <= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N90,E020N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= 20.0) && (lon_e <= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N40,E020S10", "v");
  }
  /* Column 7 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= 60.0) && (lon_e <= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N90,E060N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= 60.0) && (lon_e <= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N40,E060S10", "v");
  }
  /* Column 8 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= 100.0) && (lon_e <= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N90,E100N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= 100.0) && (lon_e <= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N40,E100S10", "v");
  }
  /* Column 9 */
  else if ( (lat_n >= 40.0) && (lat_s <= 40.0) && (lon_w >= 140.0) && (lon_e <= 180.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N90,E140N40", "v");
  }
  else if ( (lat_n >= -10.0) && (lat_s <= -10.0) && (lon_w >= 140.0) && (lon_e <= 180.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N40,E140S10", "v");
  }
  /* Groups of four, again West to East, North to South */
  /* Row 1 */
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w >= 140.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N90,W180N90,E140N40,W180N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= -140.0) && (lon_e >= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N90,W140N90,W180N40,W140N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= -100.0) && (lon_e >= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N90,W100N90,W140N40,W100N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= -60.0) && (lon_e >= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N90,W060N90,W100N40,W060N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= -20.0) && (lon_e >= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N90,W020N90,W060N40,W020N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= 20.0) && (lon_e >= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N90,E020N90,W020N40,E020N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= 60.0) && (lon_e >= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N90,E060N90,E020N40,E060N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= 100.0) && (lon_e >= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N90,E100N90,E060N40,E100N40", NULL);
  }
  else if ( (lat_n > 40.0) && (lat_s < 40.0) && (lon_w <= 140.0) && (lon_e >= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N90,E140N90,E100N40,E140N40", NULL);
  }
  /* Row 2 */
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w >= 140.0) && (lon_e <= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E140N40,W180S10,E140N40,W180S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= -140.0) && (lon_e >= -140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W180N40,W140S10,W180N40,W140S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= -100.0) && (lon_e >= -100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W140N40,W100S10,W140N40,W100S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= -60.0) && (lon_e >= -60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W100N40,W060S10,W100N40,W060S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= -20.0) && (lon_e >= -20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W060N40,W020S10,W060N40,W020S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= 20.0) && (lon_e >= 20.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "W020N40,E020S10,W020N40,E020S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= 60.0) && (lon_e >= 60.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E020N40,E060S10,E020N40,E060S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= 100.0) && (lon_e >= 100.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E060N40,E100S10,E060N40,E100S10", NULL);
  }
  else if ( (lat_n > -10.0) && (lat_s < -10.0) && (lon_w <= 140.0) && (lon_e >= 140.0) ) {
	  field = BeamBlockageMapInternal_makeTopographyField(self, "E100N40,E140S10,E100N40,E140S10", NULL);
  }

  else {
    RAVE_ERROR0("Topography maps do not cover requested area");
    goto done;
  }

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  return result;
}

BBTopography_t* BeamBlockageMap_getTopographyForScan(BeamBlockageMap_t* self, PolarScan_t* scan)
{
  BBTopography_t *field = NULL, *result = NULL;
  BBTopography_t* topo = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  if (scan == NULL) {
    RAVE_ERROR0("Trying to get topography for NULL scan");
    return NULL;
  }

  if ((topo = BeamBlockageMap_readTopography(self,
                                             PolarScan_getLatitude(scan),
                                             PolarScan_getLongitude(scan),
                                             PolarScan_getMaxDistance(scan))) == NULL) {
    goto done;
  }

  field = BeamBlockageMapInternal_createMappedTopography(self, topo, scan);

  result = RAVE_OBJECT_COPY(field);
done:
  RAVE_OBJECT_RELEASE(field);
  RAVE_OBJECT_RELEASE(topo);
  return result;
}

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
