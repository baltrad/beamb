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

BeamBlockage tests

@file
@author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
@date 2011-11-15
'''
import unittest

import _raveio
import _beamblockage
import _beamblockagemap
import os, string
import _rave
import math

class PyBeamBlockageMapTest(unittest.TestCase):
  SCAN_FILENAME = "fixtures/scan_sevil_20100702T113200Z.h5"
  
  def setUp(self):
    pass    

  def tearDown(self):
    pass
  
  def testNew(self):
    a = _beamblockagemap.new()
    self.assertNotEqual(-1, string.find(`type(a)`, "BeamBlockageMapCore"))

  def testTopo30(self):
    a = _beamblockagemap.new()
    self.assertTrue(None != a.topo30dir)
    a.topo30dir="/tmp"
    self.assertEquals("/tmp", a.topo30dir)
  
  def testReadTopo30(self):
    a = _beamblockagemap.new()
    a.topo30dir="../../data/gtopo30"
    result = a.readTopography(60*math.pi/180, 0*math.pi/180.0, 100000)
    self.assertTrue(result != None)
    self.assertEquals(6000, result.nrows)
    self.assertEquals(4800, result.ncols)
    
  def testReadTopo30_combined(self):
    a = _beamblockagemap.new()
    a.topo30dir="../../data/gtopo30"
    result = a.readTopography(60*math.pi/180, 20*math.pi/180.0, 200000)
    self.assertTrue(result != None)
    self.assertEquals(6000, result.nrows)
    self.assertEquals(9600, result.ncols)
  
  def testGetTopographyForScan(self):
    a = _beamblockagemap.new()
    a.topo30dir="../../data/gtopo30"
    scan = _raveio.open(self.SCAN_FILENAME).object
    topo = a.getTopographyForScan(scan)
    self.assertEqual(174, topo.getData()[0][0])
    
if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()