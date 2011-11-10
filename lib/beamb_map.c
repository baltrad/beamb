/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI

This is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with HLHDF.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/
/**
 * Map reading-functions for beam-blockage analysis
 * @file
 * @author Lars Norin (Swedish Meteorological and Hydrological Institute, SMHI)
 * @date 2011-10-01
 */
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

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

#ifndef M_PI
/**
 * Value of PI. Can be replaced with that found in PROJ.4 ...
 */
#define M_PI 3.14159265358979323846
#endif

/**
 * Find out which maps are needed to cover given area
 * @param[in] lat - latitude of radar in degrees
 * @param[in] lon - longitude of radar in degrees
 * @param[in] d - maximum range of radar in meters
 * @returns flag corresponding to map to be read
 */
int getTopo(double lat, double lon, double d)
{
    double latRad, lonRad, RE, lat_e, lon_e, lat_w, lon_w, lat_n, lat_s;
    
    lonRad = deg2rad(lon);
    latRad = deg2rad(lat);
    RE = getEarthRadius(lat);
    
    /* Find out which map is needed
       see http://www.movable-type.co.uk/scripts/latlong.html */
    lat_e = asin(sin(latRad) * cos(d/RE) + cos(latRad) * sin(d/RE) * cos(M_PI/2.));
    lon_e = lonRad + atan2( sin(M_PI/2.) * sin(d/RE) * cos(latRad), \
            cos(d/RE) - sin(latRad) * sin(lat_e) );
    lat_w = asin( sin(latRad) * cos(d/RE) + cos(latRad) * sin(d/RE) * cos(3.*M_PI/2.) );
    lon_w = lonRad + atan2( sin(3.*M_PI/2.) * sin(d/RE) * cos(latRad), \
            cos(d/RE) - sin(latRad) * sin(lat_w) );
    lat_n = asin( sin(latRad) * cos(d/RE) + cos(latRad) * sin(d/RE) * cos(0.) );
    lat_s = asin( sin(latRad) * cos(d/RE) + cos(latRad) * sin(d/RE) * cos(M_PI) );
    
    /* Check if maps cover the area */
    if(rad2deg(lat_n)>80. || rad2deg(lat_s)<40. || rad2deg(lon_w)<-20. || \
       rad2deg(lon_e)>60.)
    {
        printf("Maps do not cover area. Exiting.\n");
        return -1;
    }
    
    if(rad2deg(lon_e)<=20.)
    {
        /* Read W020N90 */
        return 1;
    }
    else if(rad2deg(lon_w)>20.)
    {
        /* Read E020N90 */
        return 2;
    }
    else
    {
        /* Read both maps */
        return 3;
    }
    printf("...done!\n");
}

/**
 * Read header of gtopo30 files
 * @param[in] *map - pointer to struct containing map information
 * @param[in] *filename - name of file to be read
 */
void readHdr(map_info *map, char *filename)
{
    /* Declaration of variables */
    char filename_copy[100];
    char *s, line[100];
    int n = 15, f_int;
    double f;
    
    strcpy(filename_copy,filename);
    FILE *fp = fopen(strcat(filename_copy,".HDR"),"rt");
    if(fp != (FILE *)NULL)
    {
        /* Get number of rows and columns, coordinates, and lat/lon increments */
        s = (char *)malloc(n);
        while(fgets(line, sizeof(line), fp) > 0)
        {
            if(sscanf(line, "%s %lf", s, &f) == 2)
            {
                if(strcmp(s,"NROWS") == 0)
                {
                    f_int = (int) f;
                    map->nrows = f_int;
                }
                else if(strcmp(s,"NCOLS") == 0)
                {
                    f_int = (int) f;
                    map->ncols = f_int;
                }
                else if(strcmp(s,"NBITS") == 0)
                {
                    f_int = (int) f;
                    map->nbits = f_int;
                }
                else if(strcmp(s,"ULXMAP") == 0)
                    map->ulxmap = f;
                else if(strcmp(s,"ULYMAP") == 0)
                    map->ulymap = f;
                else if(strcmp(s,"XDIM") == 0)
                    map->xdim = f;
                else if(strcmp(s,"YDIM") == 0)
                    map->ydim = f;
            }
        }
        free(s);
        if(fclose(fp))
        {
            printf("Error closing file.");
            return;
        }
    }
    else
    {
        printf("Unable to open file %s\n",filename);
        return;
    }
    return;
}

/**
 * Read data from a single gtopo30 map
 * -- NOTE: Is there a better way to switch byte order? --
 * @param[in] *data - pointer to data array
 * @param[in] *map - pointer to struct containing map information
 * @param[in] *filename - name of file to be read
 */
void readMap(short int *data, map_info *map, char *filename)
{
    FILE *fp = fopen(strcat(filename,".DEM"),"rb");
    int nrows = map->nrows, ncols = map->ncols, i;
    short temp;
    if(fp != NULL)
    {
        fread(data,sizeof(short),nrows*ncols,fp);
        /* Change byte order */
        for(i=0; i<ncols*nrows; i++)
        {
            temp = ntohs(data[i]);
            data[i] = temp;
        }
    }
    else
    {
        printf("Unable to open file %s\n",filename);
        return;
    }
    fclose(fp);
}

/**
 * Read data from two gtopo30 maps
 * -- NOTE: Is there a better way to switch byte order? --
 * @param[in] *data - pointer to data array
 * @param[in] *map1 - pointer to struct containing map information
 * @param[in] *map2 - pointer to struct containing map information
 * @param[in] *filename1 - name of file to be read
 * @param[in] *filename2 - name of file to be read
 */
void readMap2(short int *data, map_info *map1, map_info *map2, \
              char *filename1, char *filename2)
{
    FILE *fp1 = fopen(strcat(filename1,".DEM"),"rb");
    FILE *fp2 = fopen(strcat(filename2,".DEM"),"rb");
    int i, j;
    int nrows1 = map1->nrows, ncols1 = map1->ncols, nbytes1 = (map1->nbits)/8;
    int nrows2 = map2->nrows, ncols2 = map2->ncols, nbytes2 = (map2->nbits)/8;
    short int temp;
    short int *data1 = malloc(nrows1*ncols2*nbytes1);
    short int *data2 = malloc(nrows1*ncols2*nbytes2);
    if(fp1 != NULL)
    {
        fread(data1,sizeof(short),nrows1*ncols1,fp1);
        /* Change byte order */
        for(j=0; j<nrows1; j++)
        {
            for(i=0; i<ncols1; i++)
            {
                temp = ntohs(data1[j*ncols1+i]);
                data[j*(ncols1+ncols2)+i] = temp;
            }
        }
    }
    else
    {
        printf("Unable to open file %s\n",filename1);
        return;
    }
    free(data1);
    fclose(fp1);
    if(fp2 != NULL)
    {
        fread(data2,sizeof(short),nrows2*ncols2,fp2);
        /* Change byte order */
        for(j=0; j<nrows2; j++)
        {
            for(i=0; i<ncols1; i++)
            {
                temp = ntohs(data2[j*ncols1+i]);
                data[j*(ncols1+ncols2)+ncols1+i] = temp;
            }
        }
    }
    else
    {
        printf("Unable to open file %s\n",filename2);
        return;
    }
    free(data2);
    fclose(fp2);
}

/**
 * Compute amount of blockage, assuming Gaussian main lobe
 * Blockage is defined by \int_{-elLim}^elMax exp(-(x-b)^2/c) dx / bb_tot
 * where bb_tot is defined through \int_{-elLim}^elLim exp(-x^2/c) dx=1,
 * elLim = sqrt(-c*log(10^(dBlim/10)))), and c=-(beamwidth/2)^2/log(.5);
 * @param[in] *bb - pointer to beam blockage array
 * @param[in] *zi - pointer to interpolated topography array
 * @param[in] lat - latitude of radar in degrees
 * @param[in] height - altitude of radar above sea level in meters
 * @param[in] *range - pointer to radar range in meters
 * @param[in] el - lowest elevation angle of radar in degrees
 * @param[in] beamwidth - beamwidth of radar beam
 * @param[in] dBlim - limit of Gaussian approximation of main lobe
 * @param[in] ri - number of elements in array range
 * @param[in] ai - number of elements in azimuth
 */
void getBlockage(double *bb, double *zi, double lat, double height, \
                   double *range, double el, double beamwidth, \
                   double dBlim, int ri, int ai)
{
    /* Declaration of variables */
    double dndh = -3.9e-8, RE, R, c, elLim, bb_tot;
    double *elBlock = malloc(sizeof(double)*ri*ai);
    double *phi = malloc(sizeof(double)*ri);
    int i, j;
    
    RE = getEarthRadius(lat);
    R = 1./((1./RE) + dndh);
    for(i=0; i<ai ; i++)
    {
        /* Get angle phi from height equation */
        for(j=0; j<ri ; j++)
        {
            phi[j] = rad2deg( asin( ( pow(zi[i*ri+j]+R,2) - pow(range[j],2) - \
                           pow(R+height,2) ) / (2*range[j]*(R+height)) ) );
        }
        /* Find cumulative maximum */
        cummax(phi, ri);
        /* Write angles for all area */
        for(j=0; j<ri ; j++)
        {
            elBlock[i*ri+j] = phi[j];
        }
    }
    
    /* Width of Gaussian */
    c = -pow(beamwidth/2,2) / log(.5);
    
    /* Elevation limits */
    elLim = sqrt( -c*log( pow(10.,(dBlim/10.)) ) );
    
    /* Find total blockage within -elLim to +elLim */
    bb_tot = sqrt(M_PI*c) * erf(elLim/sqrt(c));
    
    for(i=0; i<ai; i++)
    {
        for(j=0; j<ri; j++)
        {
            /* Check if blockage is seen, otherwise -9999 */
            if(elBlock[i*ri+j]<el-elLim)
                elBlock[i*ri+j] = -9999;
            
            /* Set maximum for blockage elevation */
            if(elBlock[i*ri+j]>el+elLim)
                elBlock[i*ri+j] = el+elLim;
            
            bb[i*ri+j] = -1./2. * sqrt(M_PI*c) * ( erf( (el-elBlock[i*ri+j]) / \
                        sqrt(c) ) - erf( (el-(el-elLim))/sqrt(c) ) ) / bb_tot;
        }
    }
    free(phi);
    free(elBlock);
    return;
}


