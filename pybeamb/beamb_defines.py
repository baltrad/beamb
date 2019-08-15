'''
Copyright (C) 2005 - Swedish Meteorological and Hydrological Institute (SMHI)

This file is part of RAVE.

RAVE is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RAVE is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with RAVE.  If not, see <http://www.gnu.org/licenses/>.

'''
## Contains some parameters used by beamb_quality_plugin.py

## @file
## @author Ulf E. Nordh, SMHI
## @date 2019-06-10
#
# This file will be imported by beamb_quality_plugin.py.


##
# The limit of the Gaussian approximation of main lobe
#
BEAMBLOCKAGE_DBLIMIT = -6.0

##
# The threshold for correcting beamblockage, must be between 0.0 and 1.0, if set to 1.0,
# no masking with nodata will be done i.e correction will be done for all blockage up to 1.0
# with 1.0 excluded due to that the underlying C-code cannot deal with 1.0 (division by zero). An attempt to set
# the parameter smaller than 0.0 or larger than 1.0 will cause the restore function to abort.
#
BEAMBLOCKAGE_BBLIMIT = 0.7


if __name__ == "__main__":
    print(__doc__)
