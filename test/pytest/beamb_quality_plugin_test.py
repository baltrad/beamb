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
@date 2012-01-03
'''
import unittest

import _raveio
import _beamblockage
import beamb_quality_plugin
import rave_quality_plugin
import os, string
import _rave
import numpy
from rave_quality_plugin import QUALITY_CONTROL_MODE_ANALYZE

class beamb_quality_plugin_test(unittest.TestCase):
  SCAN_FIXTURE = "fixtures/sevil_0.5_20111223T0000Z.h5"
  VOLUME_FIXTURE = "fixtures/pvol_seosu_20090501T120000Z.h5"
  
  CACHEFILES = ["/tmp/15.94_58.11_223_0.50_420_120_2000.00_0.00_0.90_-20.00.h5",
                "/tmp/15.94_58.11_223_1.00_420_120_2000.00_0.00_0.90_-20.00.h5",
                "/tmp/14.76_63.30_465_0.50_420_120_2000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_1.00_420_120_2000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_14.00_420_120_1000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_1.50_420_120_2000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_2.00_420_120_2000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_24.00_420_120_1000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_2.50_420_120_1000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_40.00_420_120_1000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_4.00_420_120_1000.00_0.00_1.00_-20.00.h5",
                "/tmp/14.76_63.30_465_8.00_420_120_1000.00_0.00_1.00_-20.00.h5"]

  def setUp(self):
    for file in self.CACHEFILES:
      if os.path.isfile(file):
        try:
          os.unlink(file)
        except:
          pass
      
  def tearDown(self):
    for file in self.CACHEFILES:
      if os.path.isfile(file):
        try:
          os.unlink(file)
        except:
          pass

  def testNew(self):
    a = beamb_quality_plugin.beamb_quality_plugin()
    self.assertTrue(isinstance(a, rave_quality_plugin.rave_quality_plugin))
    self.assertEquals(None, a._cachedir)
    self.assertEquals(None, a._topodir)
    self.assertEquals(beamb_quality_plugin.BEAMBLOCKAGE_DBLIMIT, a._dblimit)
    
  def test_getQualityFields(self):
    a = beamb_quality_plugin.beamb_quality_plugin()
    result = a.getQualityFields()
    self.assertEquals(1, len(result))
    self.assertEquals("se.smhi.detector.beamblockage", result[0])

  def test_create_bb_default(self):
    a = beamb_quality_plugin.beamb_quality_plugin()
    result = a._create_bb()
    self.assertTrue(None != result.cachedir)
    self.assertTrue(None != result.topo30dir)
    
  def test_create_bb_modified_cache(self):
    a = beamb_quality_plugin.beamb_quality_plugin()
    a._cachedir="/tmp"
    result = a._create_bb()
    self.assertEquals("/tmp", result.cachedir)

  def test_create_bb_modified_topo(self):
    a = beamb_quality_plugin.beamb_quality_plugin()
    a._topodir="../../data/gtopo30"
    result = a._create_bb()
    self.assertEquals("../../data/gtopo30", result.topo30dir)

  def test_create_bb_modified_dirs(self):
    a = beamb_quality_plugin.beamb_quality_plugin()
    a._cachedir="/tmp"
    a._topodir="../../data/gtopo30"
    result = a._create_bb()
    self.assertEquals("/tmp", result.cachedir)
    self.assertEquals("../../data/gtopo30", result.topo30dir)


  def test_process_scan(self):
    classUnderTest = beamb_quality_plugin.beamb_quality_plugin()
    classUnderTest._cachedir="/tmp"
    classUnderTest._topodir="../../data/gtopo30"
    
    scan = _raveio.open(self.SCAN_FIXTURE).object
    
    self.assertTrue(scan.findQualityFieldByHowTask("se.smhi.detector.beamblockage") == None)
    
    oldparam = scan.getParameter("DBZH").clone()

    result = classUnderTest.process(scan)
    
    self.assertTrue(result == scan) # Return original scan with added field
    
    self.assertFalse(numpy.array_equal(oldparam.getData(), result.getParameter("DBZH").getData())) # Default behaviour is analyze&apply
    
    self.assertTrue(result.findQualityFieldByHowTask("se.smhi.detector.beamblockage") != None)

  def test_process_scan_reprocess_true(self):
    classUnderTest = beamb_quality_plugin.beamb_quality_plugin()
    classUnderTest._cachedir="/tmp"
    classUnderTest._topodir="../../data/gtopo30"
    
    scan = _raveio.open(self.SCAN_FIXTURE).object
    
    result = classUnderTest.process(scan)
    field1 = result.getQualityFieldByHowTask("se.smhi.detector.beamblockage")

    result = classUnderTest.process(scan, reprocess_quality_flag=True)
    field2 = result.getQualityFieldByHowTask("se.smhi.detector.beamblockage")
    
    self.assertTrue(field1 != field2)

  def test_process_scan_reprocess_false(self):
    classUnderTest = beamb_quality_plugin.beamb_quality_plugin()
    classUnderTest._cachedir="/tmp"
    classUnderTest._topodir="../../data/gtopo30"
    
    scan = _raveio.open(self.SCAN_FIXTURE).object
    
    result = classUnderTest.process(scan)
    field1 = result.getQualityFieldByHowTask("se.smhi.detector.beamblockage")

    result = classUnderTest.process(scan, reprocess_quality_flag=False)
    field2 = result.getQualityFieldByHowTask("se.smhi.detector.beamblockage")
    
    self.assertTrue(field1 == field2)

  def test_process_scan_only_analyse(self):
    classUnderTest = beamb_quality_plugin.beamb_quality_plugin()
    classUnderTest._cachedir="/tmp"
    classUnderTest._topodir="../../data/gtopo30"
    
    scan = _raveio.open(self.SCAN_FIXTURE).object
    
    self.assertTrue(scan.findQualityFieldByHowTask("se.smhi.detector.beamblockage") == None)

    oldparam = scan.getParameter("DBZH").clone()

    result = classUnderTest.process(scan, True, QUALITY_CONTROL_MODE_ANALYZE)
    
    self.assertTrue(numpy.array_equal(oldparam.getData(), result.getParameter("DBZH").getData())) # Return new scan with added field
    self.assertTrue(result.findQualityFieldByHowTask("se.smhi.detector.beamblockage") != None)
    
  def test_process_volume(self):
    classUnderTest = beamb_quality_plugin.beamb_quality_plugin()
    classUnderTest._cachedir="/tmp"
    classUnderTest._topodir="../../data/gtopo30"
    
    volume = _raveio.open(self.VOLUME_FIXTURE).object
    for i in range(volume.getNumberOfScans()):
      scan = volume.getScan(i)
      self.assertTrue(scan.findQualityFieldByHowTask("se.smhi.detector.beamblockage") == None)
    
    result = classUnderTest.process(volume)
    
    self.assertTrue(result == volume)
    for i in range(volume.getNumberOfScans()):
      scan = volume.getScan(i)
      self.assertTrue(scan.findQualityFieldByHowTask("se.smhi.detector.beamblockage") != None)

  def test_process_volume_reprocess_true(self):
    classUnderTest = beamb_quality_plugin.beamb_quality_plugin()
    classUnderTest._cachedir="/tmp"
    classUnderTest._topodir="../../data/gtopo30"
    
    volume = _raveio.open(self.VOLUME_FIXTURE).object
    
    result = classUnderTest.process(volume)
    
    fields=[]
    for i in range(volume.getNumberOfScans()):
      scan = volume.getScan(i)
      fields.append(scan.getQualityFieldByHowTask("se.smhi.detector.beamblockage"))
    
    result = classUnderTest.process(volume, reprocess_quality_flag=True)
    fields2=[]
    for i in range(volume.getNumberOfScans()):
      scan = volume.getScan(i)
      fields2.append(scan.getQualityFieldByHowTask("se.smhi.detector.beamblockage"))
    
    self.assertEquals(len(fields), len(fields2))
    for i in range(len(fields)):
      self.assertTrue(fields[i] != fields2[i])

  def test_process_volume_reprocess_false(self):
    classUnderTest = beamb_quality_plugin.beamb_quality_plugin()
    classUnderTest._cachedir="/tmp"
    classUnderTest._topodir="../../data/gtopo30"
    
    volume = _raveio.open(self.VOLUME_FIXTURE).object
    
    result = classUnderTest.process(volume)
    
    fields=[]
    for i in range(volume.getNumberOfScans()):
      scan = volume.getScan(i)
      fields.append(scan.getQualityFieldByHowTask("se.smhi.detector.beamblockage"))
    
    result = classUnderTest.process(volume, reprocess_quality_flag=False)
    fields2=[]
    for i in range(volume.getNumberOfScans()):
      scan = volume.getScan(i)
      fields2.append(scan.getQualityFieldByHowTask("se.smhi.detector.beamblockage"))
    
    self.assertEquals(len(fields), len(fields2))
    for i in range(len(fields)):
      self.assertTrue(fields[i] == fields2[i])
    
if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()