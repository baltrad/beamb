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
import _bbtopography
import os, string
import _rave
import numpy

class PyBBTopographyTest(unittest.TestCase):
  def setUp(self):
    pass    

  def tearDown(self):
    pass
  
  def test_new(self):
    a = _bbtopography.new()
    self.assertNotEqual(-1, string.find(`type(a)`, "BBTopographyCore"))

  def test_nodata(self):
    a = _bbtopography.new()
    self.assertAlmostEqual(-9999.0, a.nodata, 4)
    a.nodata = 10.0
    self.assertAlmostEqual(10.0, a.nodata, 4)

  def test_ulxmap(self):
    a = _bbtopography.new()
    self.assertAlmostEqual(0.0, a.ulxmap, 4)
    a.ulxmap = 2.5
    self.assertAlmostEqual(2.5, a.ulxmap, 4)

  def test_ulymap(self):
    a = _bbtopography.new()
    self.assertAlmostEqual(0.0, a.ulymap, 4)
    a.ulymap = 2.5
    self.assertAlmostEqual(2.5, a.ulymap, 4)

  def test_xdim(self):
    a = _bbtopography.new()
    self.assertAlmostEqual(0.0, a.xdim, 4)
    a.xdim = 2.5
    self.assertAlmostEqual(2.5, a.xdim, 4)

  def test_ydim(self):
    a = _bbtopography.new()
    self.assertAlmostEqual(0.0, a.ydim, 4)
    a.ydim = 2.5
    self.assertAlmostEqual(2.5, a.ydim, 4)
    
  def test_ncols(self):
    a = _bbtopography.new()
    self.assertAlmostEqual(0.0, a.ncols, 4)
    try:
      a.ncols = 10
      self.fail("Expected AttributeError")
    except AttributeError:
      pass
    self.assertAlmostEqual(0.0, a.ncols, 4)

  def test_nrows(self):
    a = _bbtopography.new()
    self.assertAlmostEqual(0.0, a.nrows, 4)
    try:
      a.nrows = 10
      self.fail("Expected AttributeError")
    except AttributeError:
      pass
    self.assertAlmostEqual(0.0, a.nrows, 4)

  def test_data(self):
    obj = _bbtopography.new()
    obj.setData(numpy.zeros((10,10), numpy.uint8))
    obj.setValue(0,1,10.0)
    obj.setValue(5,4,20.0)
  
    data = obj.getData()
    self.assertEqual(10, data[1][0])
    self.assertEqual(20, data[4][5])

  def test_concatx(self):
    obj = _bbtopography.new()
    obj.setData(numpy.zeros((10,10), numpy.uint8))
    obj.setValue(0,1,10.0)
    obj.setValue(5,4,20.0)

    obj2 = _bbtopography.new()
    obj2.setData(numpy.zeros((10,6), numpy.uint8))
    obj2.setValue(0,1,15.0)
    obj2.setValue(5,4,25.0)

    result = obj.concatx(obj2)
    self.assertEquals(16, result.ncols)
    self.assertEquals(10, result.nrows)
    self.assertAlmostEquals(10.0, result.getValue(0,1)[1], 4)
    self.assertAlmostEquals(20.0, result.getValue(5,4)[1], 4)
    self.assertAlmostEquals(15.0, result.getValue(10,1)[1], 4)
    self.assertAlmostEquals(25.0, result.getValue(15,4)[1], 4)    

if __name__ == "__main__":
  #import sys;sys.argv = ['', 'Test.testName']
  unittest.main()