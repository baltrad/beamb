/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI

This is free software: you can redistribute it and/or modify it under the terms of the GNU Lesser General Public License as published by the Free Software Foundation, either version 3 of the License, or (at your option) any later version.

This software is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License along with HLHDF.  If not, see <http://www.gnu.org/licenses/>.
------------------------------------------------------------------------*/
/**
 * Convenience functions for beam-blockage analysis
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
 * Converts degrees to radians
 * @param[in] deg - input value expressed in degrees
 * @returns value converted to radians
 */
double deg2rad(double deg)
{
    return deg*M_PI/180.;
}

/**
 * Convert radians to degrees
 * @param[in] rad - input value expressed in radians
 * @returns value converted to degrees
 */
double rad2deg(double rad)
{
    return rad*180./M_PI;
}

/**
 * Get simplified Earth radius for a certain latitude
 * @param[in] lat0 - input value expressed in degrees
 * @returns Earth radius at lat0 in meters
 */
double getEarthRadius(double lat0)
{
    double R_EQU, R_POL, a, b, radius;
    R_EQU = 6378160.; /* [m] */
    R_POL = 6356780.; /* [m] */
    a = sin(deg2rad(lat0))*R_POL;
    b = cos(deg2rad(lat0))*R_EQU;
    radius = sqrt(pow(a,2)+pow(b,2)); /* [m] */
    return radius;
}

/**
 * Get range from radar, projected on surface
 * @param[in] *ground_range - projected range in meters
 * @param[in] *range - radar range in meters
 * @param[in] lat0 - latitude in degrees
 * @param[in] alt0 - radar altitude in meters
 * @param[in] el - elevation of radar beam in degrees
 * @param[in] r_len - length of vector range
 */
void compute_ground_range(double *ground_range, double *range, double lat0, \
                         double alt0, double el, int r_len)
{
    double dndh = -3.9e-8, RE, R, A, H, gamma;
    int i;
    RE = getEarthRadius(lat0);
    R = 1./((1./RE) + dndh);
    A = R + alt0;
    for(i = 0; i < r_len; i++)
    {
        H = sqrt(pow(A,2) + pow(range[i],2) + 2*A*range[i]*sin(deg2rad(el))) \
            -A;
        gamma = asin(range[i]*sin(M_PI/2+deg2rad(el))/(A+H));
        ground_range[i] = A * gamma;
    }
    return;
}

/**
 * Given a start point, initial bearing, and distance, this will calculate the
 * destination point and final bearing travelling along a (shortest distance) great
 * circle arc. See http://www.movable-type.co.uk/scripts/latlong.html
 * @param[in] *lat - vector of latitudes in degrees
 * @param[in] *lon - vector of longitudes in degrees
 * @param[in] latR - radar latitude in degrees
 * @param[in] lonR - radar longitude in meters
 * @param[in] *range - elevation of radar beam in degrees
 * @param[in] *azimuth - length of vector range
 * @param[in] r_len - length of vector range
 * @param[in] a_len - length of vector range
 */
void polar2latlon(double *lat, double *lon, double latR, double lonR, \
                  double *range, double *azimuth, int r_len, int a_len)
{
    double RE;
    int i, j;
    RE=getEarthRadius(latR);
    
    for(i=0; i<a_len; i++)
    {
        for(j=0; j<r_len; j++)
        {
            lat[i*r_len+j] = rad2deg( asin( sin(deg2rad(latR)) * cos(range[j]/RE) + \
                            cos(deg2rad(latR)) * sin(range[j]/RE) * \
                            cos(deg2rad(azimuth[i])) ) );
            lon[i*r_len+j] = lonR + rad2deg( atan2( cos(deg2rad(latR)) * \
                            sin(range[j]/RE) * sin(deg2rad(azimuth[i])) , \
                            cos(range[j]/RE) - sin(deg2rad(latR)) * \
                            sin(deg2rad(lat[i*r_len+j])) ) );
        }
    }
}

/**
 * Find minimum value in vector
 * @param[in] *a - pointer to vector of values
 * @param[in] n - length of vector a
 * @returns minimum value of a
 */
double min_double(double *a, int n)
{
    double b = a[0];
    int i;
    for(i=0; i<n; i++)
    {
        if (a[i] < b)
        {
            b = a[i];
        }
    }
    return b;
}

/**
 * Find maximum value in vector
 * @param[in] *a - pointer to vector of values
 * @param[in] n - length of vector a
 * @returns maximum value of a
 */
double max_double(double *a, int n)
{
    double b = a[0];
    int i;
    for(i=0; i<n; i++)
    {
        if (a[i] > b)
        {
            b = a[i];
        }
    }
    return b;
}

/**
 * Bilinear interpolation, see e.g. 
 * http://en.wikipedia.org/wiki/Bilinear_interpolation
 * Given x1, y1, z1 and x2, y2 function finds z2
 * @param[in] *x1 - pointer to vector of abcissae
 * @param[in] *y1 - pointer to vector of ordinates
 * @param[in] *z1 - pointer to vector of values
 * @param[in] x1_max - length of vector x1
 * @param[in] y1_max - length of vector y1
 * @param[in] *x2 - pointer to vector of abcissae
 * @param[in] *y2 - pointer to vector of ordinates
 * @param[in] *z2 - pointer to vector of values
 * @param[in] x2_max - length of vector x2
 * @param[in] y2_max - length of vector y2
 */
void BilinearInterpolation(double *x1, double *y1, double *z1, int x1_max, \
                           int y1_max, double *x2, double *y2, double *z2, \
                           int x2_max, int y2_max)
{
    int i, j, m1, m2, n1, n2;
    double q1, q2, q3, q4, dx, dy, dxdy;
    double x1min = min_double(x1,x1_max), x1max = max_double(x1,x1_max), \
           y1min = min_double(y1,y1_max), y1max = max_double(y1,y1_max);
    dx = x1[1]-x1[0];
    dy = y1[1]-y1[0];
    dxdy = dx*dy;
    
    for (j=0; j<y2_max; j++)
    {
        for (i=0; i<x2_max; i++)
        {
            /* If (x2,y2) is outside ([x_min:x_max],[y_min:y_max]),
               let value be zero (no extrapolation) */
            if (x2[j*x2_max+i]<x1min || x2[j*x2_max+i]>x1max || \
                y2[j*x2_max+i]<y1min || y2[j*x2_max+i]>y1max)
            {
                z2[j*x2_max+i] = 0.0;
                printf("Warning! Function BilinearInterpolation does not \
                        perform extrapolation.");
                continue;
            }
            
            /* Find coordinates of surrounding grid points */
            m1 = (int) floor( (x2[j*x2_max+i] - x1[0]) / dx );
            n1 = (int) floor( (y2[j*x2_max+i] - y1[0]) / dy );
            m2 = m1 + 1;
            n2 = n1 + 1;
            
            q1 = (x1[m2]-x2[j*x2_max+i]) * (y1[n2]-y2[j*x2_max+i]) / dxdy;
            q2 = (x2[j*x2_max+i]-x1[m1]) * (y1[n2]-y2[j*x2_max+i]) / dxdy;
            q3 = (x1[m2]-x2[j*x2_max+i]) * (y2[j*x2_max+i]-y1[n1]) / dxdy;
            q4 = (x2[j*x2_max+i]-x1[m1]) * (y2[j*x2_max+i]-y1[n1]) / dxdy;
            
            z2[j*x2_max+i] = z1[n1*x1_max+m1]*q1 + \
                             z1[n2*x1_max+m1]*q3 + \
                             z1[n1*x1_max+m2]*q2 + \
                             z1[n2*x1_max+m2]*q4;
        }
    }
    return;
}

/**
 * Running cumulative maximum
 * @param[in] *a - pointer to vector of values
 * @param[in] a_max - length of vector a
 */
void cummax(double *a, int a_max)
{
    double t;
    int i;
    t = a[0];
    for(i=1; i<a_max; i++)
    {
        if(a[i]<t)
            a[i] = t;
        else
            t = a[i];
    }
    return;
}

