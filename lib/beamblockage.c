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
 *
 * @author Ulf E. Nordh (SMHI, removal of bugs in fkns computeGroundRange and restore)
 * @date 2019-05-22 
 */
#include "beamblockage.h"
#include "beamblockagemap.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include "math.h"
#include <string.h>
#include "config.h"
#include "hlhdf.h"
#include "odim_io_utilities.h"
#include "lazy_nodelist_reader.h"
#include "rave_field.h"
/**
 * Represents the beam blockage algorithm
 */
struct _BeamBlockage_t {
  RAVE_OBJECT_HEAD /** Always on top */
  BeamBlockageMap_t* mapper; /**< the topography reader */
  char* cachedir;            /**< the cache directory */
  int rewritecache;         /**< if cache should be recreated */
};

/**
 * Converts a radian to a degree
 * @param[in] rad - input value expressed in radians
 */
#define RAD2DEG(rad) (rad*180.0/M_PI)

/*@{ Private functions */
/**
 * Constructor.
 */
static int BeamBlockage_constructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
  self->cachedir = NULL;
  self->mapper = RAVE_OBJECT_NEW(&BeamBlockageMap_TYPE);
  self->rewritecache = 0;

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
  this->rewritecache = src->rewritecache;

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
  elangle = PolarScan_getElangle(scan);

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
 * @param[in] dblim - Limit of Gaussian approximation of main lobe
 * @param[in] filename - the allocated array where the filename should be written
 * @param[in] len - the length of the allocated array
 * @return 1 on success otherwise 0
 */
static int BeamBlockageInternal_createCacheFilename(BeamBlockage_t* self, PolarScan_t* scan, double dblim, char* filename, int len)
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
                    "%.2f_%.2f_%.0f_%.2f_%ld_%ld_%.2f_%.2f_%.2f_%.2f.h5",
                    lon, lat, height, elangle, nrays, nbins, rscale, rstart, bw, dblim);
  } else {
    elen = snprintf(filename, len,
                    "%s/%.2f_%.2f_%.0f_%.2f_%ld_%ld_%.2f_%.2f_%.2f_%.2f.h5",
                    self->cachedir, lon, lat, height, elangle, nrays, nbins, rscale, rstart, bw, dblim);
  }

  if (elen >= len) {
    RAVE_ERROR0("Not enough room was created for filename");
    goto done;
  }

  result = 1;
done:
  return result;
}

static int BeamBlockageInternal_addMetaInformation(RaveField_t* field, double gain, double offset, double dbLimit)
{
  RaveAttribute_t* attribute = NULL;
  int result = 0;

  attribute = RaveAttributeHelp_createString("how/task", "se.smhi.detector.beamblockage");
  if (attribute == NULL || !RaveField_addAttribute(field, attribute)) {
    RAVE_ERROR0("Failed to add how/task");
    goto done;
  }
  RAVE_OBJECT_RELEASE(attribute);

  attribute = RaveAttributeHelp_createDouble("what/gain", gain);
  if (attribute == NULL || !RaveField_addAttribute(field, attribute)) {
    RAVE_ERROR0("Failed to add what/gain");
    goto done;
  }
  RAVE_OBJECT_RELEASE(attribute);

  attribute = RaveAttributeHelp_createDouble("what/offset", offset);
  if (attribute == NULL || !RaveField_addAttribute(field, attribute)) {
    RAVE_ERROR0("Failed to add what/offset");
    goto done;
  }
  RAVE_OBJECT_RELEASE(attribute);

  attribute = RaveAttributeHelp_createStringFmt("how/task_args", "DBLIMIT:%g", dbLimit);
  if (attribute == NULL || !RaveField_addAttribute(field, attribute)) {
    RAVE_ERROR0("Failed to add how/task_args");
    goto done;
  }
  RAVE_OBJECT_RELEASE(attribute);

  result = 1;
done:
  RAVE_OBJECT_RELEASE(attribute);
  return result;
}

static int BeamBlockageInternal_getMetaInformation(RaveField_t* field, double* gain, double* offset)
{
  int result = 0;
  RaveAttribute_t* attribute = NULL;
  char* svalue = NULL;

  RAVE_ASSERT((field != NULL), "field == NULL");
  RAVE_ASSERT((gain != NULL), "gain == NULL");
  RAVE_ASSERT((offset != NULL), "offset == NULL");

  attribute = RaveField_getAttribute(field, "how/task");
  if (attribute == NULL || !RaveAttribute_getString(attribute, &svalue)) {
    RAVE_ERROR0("how/task missing");
    goto done;
  }
  if (svalue == NULL || strcmp("se.smhi.detector.beamblockage", svalue) != 0) {
    if (svalue == NULL) {
      RAVE_ERROR0("how/task == NULL");
    } else {
      RAVE_ERROR1("how/task = %s", svalue);
    }
    goto done;
  }
  RAVE_OBJECT_RELEASE(attribute);

  attribute = RaveField_getAttribute(field, "what/gain");
  if (attribute == NULL || !RaveAttribute_getDouble(attribute, gain)) {
    RAVE_ERROR0("Missing what/gain");
    goto done;
  }
  RAVE_OBJECT_RELEASE(attribute);

  attribute = RaveField_getAttribute(field, "what/offset");
  if (attribute == NULL || !RaveAttribute_getDouble(attribute, offset)) {
    RAVE_ERROR0("Missing what/offset");
    goto done;
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(attribute);
  return result;
}

/**
 * Returns a cached file matching the given scan if there is one.
 * @param[in] self - self
 * @param[in] scan - the scan
 * @param[in] dblim - Limit of Gaussian approximation of main lobe
 */
static RaveField_t* BeamBlockageInternal_getCachedFile(BeamBlockage_t* self, PolarScan_t* scan, double dblim)
{
  RaveField_t* result = NULL;
  LazyNodeListReader_t* nodelist = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((scan != NULL), "scan == NULL");

  if (self->cachedir != NULL) {
    char filename[512];
    if (!BeamBlockageInternal_createCacheFilename(self, scan, dblim, filename, 512)) {
      goto done;
    }

    if(HL_isHDF5File(filename)) {
      nodelist =  LazyNodeListReader_readPreloaded(filename);
      if (nodelist == NULL) {
        RAVE_ERROR1("Failed to read hdf5 file %s", filename);
        goto done;
      }
      result = OdimIoUtilities_loadField(nodelist, RaveIO_ODIM_Version_2_4, "/beamb_field");
    }
  }

done:
  RAVE_OBJECT_RELEASE(nodelist);
  return result;
}

/**
 * Writes a rave field to the cache. There is no particular file properties or
 * compressions used.
 * @param[in] self - self
 * @param[in] scan - the scan
 * @param[in] field - the rave field
 * @param[in] dblim - Limit of Gaussian approximation of main lobe
 * @return 1 on success otherwise 0
 */
static int BeamBlockageInternal_writeCachedFile(BeamBlockage_t* self, PolarScan_t* scan, RaveField_t* field, double dblim)
{
  int result = 0;
  HL_NodeList* nodelist = NULL;
  HL_Compression* compression = NULL;
  HL_FileCreationProperty* property = NULL;

  RAVE_ASSERT((self != NULL), "self == NULL");
  RAVE_ASSERT((scan != NULL), "scan == NULL");
  RAVE_ASSERT((field != NULL), "field == NULL");

  if (self->cachedir != NULL) {
    char filename[512];
    if (!BeamBlockageInternal_createCacheFilename(self, scan, dblim, filename, 512)) {
      goto done;
    }
    compression = HLCompression_new(CT_ZLIB);
    property = HLFileCreationProperty_new();
    nodelist = HLNodeList_new();

    if (nodelist == NULL || compression == NULL || property == NULL) {
      RAVE_ERROR0("Failed to create necessary hlhdf objects");
      goto done;
    }
    compression->level = (int)6;
    property->userblock = (hsize_t)0;
    property->sizes.sizeof_size = (size_t)4;
    property->sizes.sizeof_addr = (size_t)4;
    property->sym_k.ik = (int)1;
    property->sym_k.lk = (int)1;
    property->istore_k = (long)1;
    property->meta_block_size = (long)0;

    result = OdimIoUtilities_addRaveField(field, nodelist, RaveIO_ODIM_Version_2_4, "/beamb_field");
    if (result == 1) {
      result = HLNodeList_setFileName(nodelist, filename);
    }
    if (result == 1) {
      result = HLNodeList_write(nodelist, property, compression);
    }
  } else {
    result = 1; /* We always succeed when there is no cache file to be written */
  }

done:
  HLCompression_free(compression);
  HLFileCreationProperty_free(property);
  HLNodeList_free(nodelist);

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
  RAVE_FREE(self->cachedir);
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

void BeamBlockage_setRewriteCache(BeamBlockage_t* self, int recreateCache)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  self->rewritecache = recreateCache;
}

int BeamBlockage_getRewriteCache(BeamBlockage_t* self)
{
  return self->rewritecache;
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
  double gtopo_alt0 = 0.0, gtmp = 0.0;

  /* We want range to be between 0 - 255 as unsigned char */
  double gain = 1 / 255.0;
  double offset = 0.0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (scan == NULL) {
    return NULL;
  }

  if (self->rewritecache == 0) {
    /* If we want to recreate cache, there is no meaning to read the cached file */
    field = BeamBlockageInternal_getCachedFile(self, scan, dBlim);
    if (field != NULL) {
      return field; /* We already have what we want so return before we do anything else */
    }
  }

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
  if (field == NULL || !RaveField_createData(field, nbins, nrays, RaveDataType_UCHAR)) {
    goto done;
  }

  phi = RAVE_MALLOC(sizeof(double)*nbins);
  if (phi == NULL) {
    goto done;
  }

  RE = PolarNavigator_getEarthRadiusOrigin(navigator);
  R = 1.0/((1.0/RE) + PolarNavigator_getDndh(navigator));
  height = PolarNavigator_getAlt0(navigator);

  /* Determine topography's height at the radar's position
   * and use it if it is higher. Even add a short "tower"
   * to get the feed-horn's height above the ground.
   * Remember: this is a guess for dealing with cases where
   * the radar's height may be unknown or inconsistent with the DEM. */
  for (ri = 0; ri < nrays; ri++) {
    BBTopography_getValue(topo, 0, ri, &gtmp);
    if (gtmp > gtopo_alt0) {
      gtopo_alt0 = gtmp;
    }
  }  /* Assume a 5 m antenna radius (S-band) */
  if ((gtopo_alt0+5.0) > height) {
    height = gtopo_alt0 + 5.0;
  }

  beamwidth = PolarScan_getBeamwidth(scan) * 180.0 / M_PI;
  elangle = PolarScan_getElangle(scan) * 180.0 / M_PI;

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
      phi[bi] = RAD2DEG(asin((((v+R)*(v+R)) - (groundRange[bi]*groundRange[bi]) - ((R+height)*(R+height))) / (2*groundRange[bi]*(R+height))));
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

      /* Discard non-physical values for blockage */
      if (bbval < 0.0) {
        bbval = 0.0;
      } else if (bbval > 1.0) {
        bbval = 1.0;
      }

      /* ODIM's rule for representing quality is that 0=lowest, 1=highest quality. Therefore invert. */
      bbval = ((1.0-bbval) - offset) / gain;
      RaveField_setValue(field, bi, ri, bbval);
    }
  }

  if (!BeamBlockageInternal_addMetaInformation(field, gain, offset, dBlim)) {
    goto done;
  }

  if (!BeamBlockageInternal_writeCachedFile(self, scan, field, dBlim)) {
    RAVE_ERROR0("Failed to generate cache file");
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

int BeamBlockage_restore(PolarScan_t* scan, RaveField_t* blockage, const char* quantity, double threshold)
{
  int result = 0;
  PolarScanParam_t* parameter = NULL;
  double gain, offset, nodata;
  long ri, bi, nrays, nbins;
  double bbgain, bboffset, bbraw;
  double dbz_uncorr, dbz_corr, dbz_corr_raw, bbpercent, bb_corr_db, bb_corr_lin;
  RaveValueType rvt;
  RaveAttribute_t* attr = NULL;

  if (scan == NULL || blockage == NULL) {
    RAVE_ERROR0("Need to provide both scan and field containing blockage.");
    goto done;
  }

  if (!BeamBlockageInternal_getMetaInformation(blockage, &bbgain, &bboffset)) {
    RAVE_ERROR0("Could not get meta information from blockage field.");
    goto done;
  }

  if (quantity == NULL) {
    parameter = PolarScan_getParameter(scan, "DBZH");
  } else {
    parameter = PolarScan_getParameter(scan, quantity);
  }
  if (parameter == NULL) {
    RAVE_ERROR1("No parameter with quantity %s in scan", quantity);
    goto done;
  }

  gain = PolarScanParam_getGain(parameter);
  offset = PolarScanParam_getOffset(parameter);
  nodata = PolarScanParam_getNodata(parameter);
  nrays = PolarScanParam_getNrays(parameter);
  nbins = PolarScanParam_getNbins(parameter);

  if (nrays != RaveField_getYsize(blockage) || nbins != RaveField_getXsize(blockage)) {
    RAVE_ERROR0("field and scan dimensions must be the same");
    goto done;
  }

  /* Exits if the user failed to set the threshold between 0.0 and 1.0 */
  if (threshold < 0.0 || threshold > 1.0) {
    RAVE_ERROR0("A blockage threshold smaller than 0.0 or larger than 1.0 is set in the calling script, correct and retry");
    goto done;
  }

  attr = RaveField_getAttribute(blockage, "how/task_args");
  if (attr == NULL) {
    attr = RaveAttributeHelp_createStringFmt("how/task_args", "BBLIMIT:%g", threshold);
    if (attr == NULL) {
      RAVE_ERROR0("Failed to create how/task_args for BBLIMIT");
      goto done;
    }
    if (!RaveField_addAttribute(blockage, attr)) {
      RAVE_ERROR0("Failed to add attribute how/task_args to field");
      goto done;
    }
  } else {
    char buff[4096];
    char* value = NULL;
    RaveAttribute_getString(attr, &value);
    if (value != NULL && strlen(value) > 0) {
      snprintf(buff, 4096, "%s,BBLIMIT:%g",value, threshold);
    } else {
      snprintf(buff, 4096, "BBLIMIT:%g", threshold);
    }
    if (!RaveAttribute_setString(attr, buff)) {
      RAVE_ERROR0("Failed to set attribute string");
      goto done;
    }
  }

  for (ri = 0; ri < nrays; ri++) {
    for (bi = 0; bi < nbins; bi++) {

      /* The function below does two things, 1) checks the data type of the matrix element, and 2) converts from raw to dbz */
      /* Returns in variable rvt and the variable dbz_uncorr */
      rvt = PolarScanParam_getConvertedValue(parameter, bi, ri, &dbz_uncorr);

      if ((rvt == RaveValueType_DATA) || (rvt == RaveValueType_UNDETECT)) { /* Ignore nodata matrix elements */
        RaveField_getValue(blockage, bi, ri, &bbraw);

        /* ODIM's rule for representing quality is that 0=lowest, 1=highest quality. Therefore revert. */
        bbpercent = 1.0 - (bbgain * bbraw + bboffset);

        /* Discard non-physical values */
        if (bbpercent < 0.0 || bbpercent > 1.0) {
          RAVE_ERROR0("beamb values are out of bounds, check scaling");
          goto done;
        }

        /* Adjust the reflectivity IF we have blockage and IF it is smaller than the selected threshold AND we are dealing with data */
        if ((bbpercent > 0.0) && (bbpercent <= threshold) && (bbpercent != 1.0) && (rvt == RaveValueType_DATA)) {

          bb_corr_lin = 1.0 / (pow(1.0-bbpercent,2)); /* Two-way blockage multiplicative correction in linear representation */
          bb_corr_db = 10.0 * log10(bb_corr_lin); /* Two-way blockage multiplicative correction in dB */

          dbz_corr = dbz_uncorr + bb_corr_db;  /* In the "dB-regime" we use addition for the multiplicative correction */
          dbz_corr_raw = round((dbz_corr - offset) / gain); /* Converting to raw format */

          /* if (dbz_corr_raw > nodata - 1)  Brute force used to in worst case keep the corrected data within 8-bit,perhaps risky to use 
          {
             dbz_corr_raw = nodata - 1;
          } */

          PolarScanParam_setValue(parameter, bi, ri, dbz_corr_raw);

        /* If the blockage is larger than the selected threshold, mask with nodata, this is valid for both data and undetect */
        /* By doing like this we give adjacent radars the opportunity to fill in these pixels */
        } else if (bbpercent > threshold) {
          PolarScanParam_setValue(parameter, bi, ri, nodata);  /* Uncorrectable */
        }
      }
    }
  }

  result = 1;
done:
  RAVE_OBJECT_RELEASE(attr);
  RAVE_OBJECT_RELEASE(parameter);
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
