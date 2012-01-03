'''
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of beamb.

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

Test suite for beamb

@file
@author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
@date 2011-11-15
'''
import unittest

from PyBeamBlockageTest import *
from PyBeamBlockageMapTest import *
from PyBBTopographyTest import *
from beamb_quality_plugin_test import *


if __name__ == "__main__":
  unittest.main()

