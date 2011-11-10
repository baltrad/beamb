/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI

This is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with HLHDF.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/
/**
 * Beam-blockage analysis
 * @file
 * @author Lars Norin (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-10-01
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef M_PI
/**
 * Value of PI. Can be replaced with that found in PROJ.4 ...
 */
#define M_PI 3.14159265358979323846
#endif

/**
 * Struct containing map parameters.
 * ulxmap - longitude of the center of the upper-left pixel (decimal degrees)
 * ulymap - latitude  of the center of the upper-left pixel (decimal degrees)
 * nbits - number of bits per pixel (16 for a DEM)
 * nrows - number of rows in the image
 * ncols - number of columns in the image
 * xdim - x dimension of a pixel in geographic units (decimal degrees)
 * ydim - y dimension of a pixel in geographic units (decimal degrees)
 */
typedef struct
{
    double ulxmap;
    double ulymap;
    int nbits;
    int nrows;
    int ncols;
    double xdim;
    double ydim;
} map_info;

#include "beamb_fn.h"
#include "beamb_map.h"

/**
 * Calculates percentage of beam power blocked.
 * Result is written to file.
 * @param [in] hdf5file - hdf5 file containing metadata on radar
 * @param [in] dBlim - limit of Gaussian approximation of main lobe
 */
int main(void)
{
    /* Declaration of variables */
    char *radar_name, filedir[100], filedir1[100], filedir2[100];
    int filenr;
    double radar[3];
    double el, beamwidth, phi0, az_step;
    double latRadar, lonRadar, hRadar;
    double r_step, r_min, r_max, dBlim;
    double range_len, a_len;
    int ri, ai;

    /* Directory where map files are located */
    strcpy(filedir,"/data/proj/radar/lnorin/baltrad/beam_blockage/gtopo30/");
    strcpy(filedir1,"/data/proj/radar/lnorin/baltrad/beam_blockage/gtopo30/");
    strcpy(filedir2,"/data/proj/radar/lnorin/baltrad/beam_blockage/gtopo30/");

    /* Limit of Gaussian approximation of main lobe (given as parameter to main?) */
    dBlim=-20.;
    
    /* Name of the radar (should be read from hdf5 file) */
/*    radar_name = "arl";*/
    radar_name = "var";
    
    /* Properties of the radar (should be read from hdf5 file) */
    if (strcmp(radar_name,"arl") == 0)
    {
        radar[0] = 59.654437083; /* Latitude [degrees] */
        radar[1] = 17.946310106; /* Longitude [degrees] */
        radar[2] = 73.51;        /* Altitude [m] */
    }
    else if (strcmp(radar_name,"var") == 0)
    {
        radar[0] = 58.255645047; /* Latitude [degrees] */
        radar[1] = 12.826024108; /* Longitude [degrees] */
        radar[2] = 163.61;       /* Altitude [m] */
    }
    else
    {
        printf("Unknown radar. Exiting.");
        return 0;
    }

    /* Properties of the radar (should be read from hdf5 file) */
    if (strcmp(radar_name,"hur") == 0)
    {
            /*** Norwegian radars ***/
            el = .5;          /* Elevation angles [degrees] */
            beamwidth = 1.;   /* Beamwidth [degrees] */
            phi0 = 90.;       /* Start position for sweep [degrees] */
            az_step = 1.;
    }
    else
    {
            /*** Swedish radars ***/
            el = .5;          /* Elevation angles [degrees] */
            beamwidth = .9;   /* Beamwidth [degrees] */
            phi0 = 90.;       /* Start position for sweep [degrees] */
            az_step = 360./420.;
    }

    /* Coordinates of the radar */
    latRadar = radar[0];
    lonRadar = radar[1];
    hRadar = radar[2];
    
    /* Range resolution [m] (should be read from hdf5 file) */
    r_step = 2.e3;
    r_min = 0.;
    r_max = 240.e3;

    /* Number of range bins and azimuth gates */
    range_len = floor((r_max-r_min)/r_step);
    ri = (int) range_len;
    a_len = 360./az_step;
    ai = (int) a_len;
    
    /* Allocating variables */
    double *range = malloc(sizeof(double)*ri);
    double *azimuth = malloc(sizeof(double)*ai);
    
    /* Polar grid of the radar, centered in each pixel */
    int i;
    for (i=0; i<ai; i++)
        azimuth[i] = az_step/2 + i*az_step;
    
    for (i=0; i<ri; i++)
        range[i] = r_min + r_step/2 + i*r_step;
    
    /* Find out which map to read */
    filenr = getTopo(latRadar, lonRadar, range[ri-1]);
    
    /* Allocating variables */
    map_info *map = malloc(sizeof(*map));
    map_info *map1 = malloc(sizeof(*map1));
    map_info *map2 = malloc(sizeof(*map2));
    
    int nrows, ncols, nbytes;
    double ulxmap, ulymap, xdim, ydim;
    
    switch (filenr)
    {
        case 1:
            /* One map covers radar grid */
            strcat(filedir,"W020N90"); 
            /* Read map header */
            readHdr(map, filedir);
            nrows = map->nrows;
            ncols = map->ncols;
            nbytes = (map->nbits)/8;
            xdim = map->xdim;
            ydim = map->ydim;
            ulxmap = map->ulxmap; 
            ulymap = map->ulymap;
            break;
        case 2:
            /* One map covers radar grid */
            strcat(filedir,"E020N90"); 
            /* Read map header */
            readHdr(map, filedir);
            nrows = map->nrows;
            ncols = map->ncols;
            nbytes = (map->nbits)/8;
            xdim = map->xdim;
            ydim = map->ydim;
            ulxmap = map->ulxmap; 
            ulymap = map->ulymap;
            break;
        case 3:
            /* Two maps must be used to cover radar grid */
            strcat(filedir1,"W020N90"); 
            strcat(filedir2,"E020N90"); 
            /* Read map headers */
            readHdr(map1, filedir1);
            readHdr(map2, filedir2);
            /* Check that number of rows and spacing are the same */
            if(map1->nrows == map2->nrows && map1->xdim == map2-> xdim \
                && map1->ydim == map2-> ydim && map1->nbits == map2->nbits)
            {
                nrows = map1->nrows;
                ncols = map1->ncols + map2->ncols;
                nbytes = (map1->nbits)/8;
                xdim = map1->xdim;
                ydim = map1->ydim;
                ulxmap = map1->ulxmap; 
                ulymap = map1->ulymap;
            }
            break;
    }
    
    /* Should maybe be extended to cover other format than int16 */
    if(nbytes != 2)
        printf("Warning! Unexpected file format.");
    
    /* Allocating variables */
    short int *data = malloc(nrows*ncols*nbytes);
    
    /* Read map(s) */
    switch (filenr)
    {
        case 1:
            readMap(data, map, filedir);
            break;
        case 2:
            readMap(data, map, filedir);
            break;
        case 3:
            readMap2(data, map1, map2, filedir1, filedir2);
            break;
    }
    
    /* Allocating variables */
    double *ground_range = malloc(sizeof(double)*ri);
    double *lat = malloc(sizeof(double)*ri*ai);
    double *lon = malloc(sizeof(double)*ri*ai);
    double *zi = malloc(sizeof(double)*ri*ai);
    
    /* Find the projected range on the ground [m] */
    compute_ground_range(ground_range, range, latRadar, hRadar, el, ri);
    
    /* Convert radar polar grid to lat/long */
    polar2latlon(lat, lon, latRadar, lonRadar, ground_range, azimuth, ri, ai);

    /* Find indices of lat and lon limits of radar grid */
    int lon_min, lon_max, lat_min, lat_max, lon1, lon2, lat1, lat2;
    
    lon_min = floor( (min_double(lon,ri*ai)-ulxmap) / xdim ) - 1;
    lon_max = ceil( (max_double(lon,ri*ai)-ulxmap) / xdim ) + 1;
    lat_max = floor( (ulymap-max_double(lat,ri*ai)) / ydim ) - 1;
    lat_min = ceil( (ulymap-min_double(lat,ri*ai)) / ydim ) + 1;

    /* Check that we are not outside map */
    lon1 = lon_min > 1 ? lon_min : 1;
    lon2 = lon_max < ncols ? lon_max : ncols;
    lat1 = lat_max > 1 ? lat_max : 1;
    lat2 = lat_min < nrows ? lat_min : nrows;
    
    /* Declaration of variables */
    int nlon = lon2 - lon1 +1;
    int nlat = lat2 - lat1 +1;

    /* Allocating variables */
    double *data_small = malloc(sizeof(double)*nlon*nlat);
    
    /* Cut out smaller part of map */
    int j, k = 0;
    for(i = lat1-1; i < lat2; i++)
    {
        for(j = lon1-1; j < lon2; j++)
        {
            if(data[i*ncols+j] < 0)
            {
                data_small[k] = 0;
                k++;
            }
            else
            {
                data_small[k] = (double) data[i*ncols+j];
                k++;
            }
        }
    }
    
   /* Allocating variables */
    double *lon_map = malloc(sizeof(double)*nlon);
    double *lat_map = malloc(sizeof(double)*nlat);
    
   /* Define the grid for the small map */
    for(i=0; i<nlon; i++)
    {
        lon_map[i] = ulxmap+(lon1-1)*xdim + i*xdim;
    }
    for(i=0; i<nlat; i++)
    {
        lat_map[i] = ulymap-(lat1-1)*ydim - i*ydim;
    }

    /* Interpolate map from original grid to radar grid. */
    BilinearInterpolation(lon_map, lat_map, data_small, nlon, nlat, \
                         lon, lat, zi, ri, ai);
   
    /* Allocating variables */
    double *bb = malloc(sizeof(double)*ri*ai);
    
    /* Compute blockage */
    getBlockage(bb, zi, latRadar, hRadar, ground_range, el, beamwidth, \
                dBlim, ri, ai);
    
    /* Write to file */
    FILE *fp1;
    int n;
    if((fp1 = fopen("/data/proj/radar/lnorin/baltrad/beam_blockage/bb.dat", "w")))
    {
        n = fwrite(bb, sizeof(double), ri*ai, fp1);
        fclose(fp1);
    }
    else
    {
        printf("Error writing file.");
    }
    
    /* Free memory */
    free(range);
    free(ground_range);
    free(azimuth);
    free(lat);
    free(lon);
    free(map);
    free(map1);
    free(map2);
    free(data);
    free(data_small);
    free(lon_map);
    free(lat_map);
    free(zi);
    free(bb);
    
    return 0;
}


