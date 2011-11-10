/* Various functions used by beamb.c */

double deg2rad(double deg);

double rad2deg(double rad);

double getEarthRadius(double lat);

void compute_ground_range(double *slant_range, double *range, double lat0, \
                         double alt0, double el, int range_len);

void polar2latlon(double *lat, double *lon, double latR, double lonR, \
                  double *range, double *azimuth, int r_len, int a_len);

double min_double(double *a, int n);

double max_double(double *a, int n);

void BilinearInterpolation(double *x1, double *y1, double *z1, int x1_max, \
                           int y1_max, double *x2, double *y2, double *z2, \
                           int x2_max, int y2_max);

void cummax(double *a, int a_max);

