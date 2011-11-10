/* Various functions to read map. Used by beamb.c */

int getTopo(double lat, double lon, double max_range);

void readHdr(map_info *map, char *filename);

void readMap(short int *data, map_info *map, char *filename);

void readMap2(short int *data, map_info *map1, map_info *map2, \
              char *filename1, char *filename2);

void getBlockage(double *bb, double *zi, double lat, double height, \
                 double *range, double el, double beamwidth, double dBlim, \
                 int ri, int ai);

