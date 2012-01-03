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
import os, string
import _rave
import numpy

class PyBeamBlockageTest(unittest.TestCase):
  SCAN_FILENAME = "fixtures/scan_sevil_20100702T113200Z.h5"
  FIXTURE_2 = "fixtures/sevil_0.5_20111223T0000Z.h5"
  
  CACHEFILE_1 = "/tmp/15.94_58.11_222_40.00_420_120_1000.00_0.00_0.90_-20.00.h5"
  CACHEFILE_2 = "/tmp/15.94_58.11_223_0.50_420_120_2000.00_0.00_0.90_-20.00.h5"
  CACHEFILE_3 = "/tmp/15.94_58.11_223_0.50_420_120_2000.00_0.00_0.90_-25.00.h5"
  
  def setUp(self):
    if os.path.isfile(self.CACHEFILE_1):
      os.unlink(self.CACHEFILE_1)
    if os.path.isfile(self.CACHEFILE_2):
      os.unlink(self.CACHEFILE_2)
    if os.path.isfile(self.CACHEFILE_3):
      os.unlink(self.CACHEFILE_3)
      
  def tearDown(self):
    if os.path.isfile(self.CACHEFILE_1):
      os.unlink(self.CACHEFILE_1)
    if os.path.isfile(self.CACHEFILE_2):
      os.unlink(self.CACHEFILE_2)
    if os.path.isfile(self.CACHEFILE_3):
      os.unlink(self.CACHEFILE_3)
        
  def testNew(self):
    a = _beamblockage.new()
    self.assertNotEqual(-1, string.find(`type(a)`, "BeamBlockageCore"))

  def testTopo30(self):
    a = _beamblockage.new()
    self.assertTrue(None != a.topo30dir)
    a.topo30dir="/tmp"
    self.assertEquals("/tmp", a.topo30dir)

  def testCachedir(self):
    a = _beamblockage.new()
    self.assertTrue(a.cachedir != None)
    a.cachedir="/tmp"
    self.assertEquals("/tmp", a.cachedir)
  
  def testReadTopo30(self):
    a = _beamblockage.new()
    a.topo30dir="../../data/gtopo30"
    
  def test_getBlockage(self):
    a = _beamblockage.new()
    a.topo30dir="../../data/gtopo30"
    a.cachedir=None # No caching
    scan = _raveio.open(self.SCAN_FILENAME).object
    result = a.getBlockage(scan, -20.0);
    self.assertFalse(os.path.isfile(self.CACHEFILE_1))

    self.assertEquals(_rave.RaveDataType_UCHAR, result.datatype)
    self.assertAlmostEquals(0.0, result.getAttribute("what/offset"), 4)
    self.assertAlmostEquals(1/255.0, result.getAttribute("what/gain"), 4)
    self.assertEquals(scan.nbins, result.xsize)
    self.assertEquals(scan.nrays, result.ysize)

  def test_getBlockage_caching(self):
    a = _beamblockage.new()
    a.topo30dir="../../data/gtopo30"
    a.cachedir="/tmp"
    scan = _raveio.open(self.SCAN_FILENAME).object

    self.assertFalse(os.path.isfile(self.CACHEFILE_1))
    
    result = a.getBlockage(scan, -20.0);
    
    self.assertTrue(os.path.isfile(self.CACHEFILE_1))

    self.assertEquals(_rave.RaveDataType_UCHAR, result.datatype)
    self.assertAlmostEquals(0.0, result.getAttribute("what/offset"), 4)
    self.assertAlmostEquals(1/255.0, result.getAttribute("what/gain"), 4)
    self.assertEquals(scan.nbins, result.xsize)
    self.assertEquals(scan.nrays, result.ysize)
    
  def test_getBlockage2(self):
    a = _beamblockage.new()
    a.topo30dir="../../data/gtopo30"
    a.cachedir="/tmp"
    scan = _raveio.open(self.FIXTURE_2).object
    result = a.getBlockage(scan, -20.0);
    self.assertEquals(_rave.RaveDataType_UCHAR, result.datatype)
    self.assertAlmostEquals(0.0, result.getAttribute("what/offset"), 4)
    self.assertAlmostEquals(1/255.0, result.getAttribute("what/gain"), 4)
    self.assertEquals(scan.nbins, result.xsize)
    self.assertEquals(scan.nrays, result.ysize)

  def test_getBlockage2_keep_cache(self):
    a = _beamblockage.new()
    a.topo30dir="../../data/gtopo30"
    a.cachedir="/tmp"
    scan = _raveio.open(self.FIXTURE_2).object
    result = a.getBlockage(scan, -20.0);

    self.assertTrue(os.path.isfile(self.CACHEFILE_2))
    c2stat = os.stat(self.CACHEFILE_2)

    result = a.getBlockage(scan, -20.0);
    c2stat_n = os.stat(self.CACHEFILE_2)

    self.assertEquals(c2stat.st_ctime, c2stat_n.st_ctime)
    self.assertEquals(c2stat.st_mtime, c2stat_n.st_mtime)
    
  def test_getBlockage2_recreate_cache(self):
    a = _beamblockage.new()
    a.topo30dir="../../data/gtopo30"
    a.cachedir="/tmp"
    a.rewritecache=True
    scan = _raveio.open(self.FIXTURE_2).object
    result = a.getBlockage(scan, -20.0);

    self.assertTrue(os.path.isfile(self.CACHEFILE_2))
    c2stat = os.stat(self.CACHEFILE_2)

    result = a.getBlockage(scan, -20.0);
    c2stat_n = os.stat(self.CACHEFILE_2)

    self.assertNotEquals(c2stat.st_ctime, c2stat_n.st_ctime)
    self.assertNotEquals(c2stat.st_mtime, c2stat_n.st_mtime)    

  def test_getBlockage2_different_dblim(self):
    a = _beamblockage.new()
    a.topo30dir="../../data/gtopo30"
    a.cachedir="/tmp"
    a.rewritecache=True
    scan = _raveio.open(self.FIXTURE_2).object
    result1 = a.getBlockage(scan, -20.0);
    result2 = a.getBlockage(scan, -25.0);

    self.assertTrue(os.path.isfile(self.CACHEFILE_2))
    self.assertTrue(os.path.isfile(self.CACHEFILE_3))
    c2stat = os.stat(self.CACHEFILE_2)

if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()