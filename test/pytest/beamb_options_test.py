'''
Copyright (C) 2024- Swedish Meteorological and Hydrological Institute, SMHI,

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

beamb_options tests

@file
@author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
@date 2024-11-09
'''
import unittest
import beamb_options
import _polarscan

class beamb_options_test(unittest.TestCase):
  OPTIONS_FIXTURE = "fixtures/beamb_option_fixture.xml"
  def setUp(self):
    self.classUnderTest = beamb_options.beamb_options(self.OPTIONS_FIXTURE)
      
  def tearDown(self):
    self.classUnderTest = None

  def test_has_options(self):
    self.assertTrue(self.classUnderTest.has_options("default"))
    self.assertTrue(self.classUnderTest.has_options("seang"))
    self.assertTrue(self.classUnderTest.has_options("sekkr"))
    self.assertFalse(self.classUnderTest.has_options("noname"))

  def test_get_default(self):
    d = self.classUnderTest.get_options("default")
    self.assertAlmostEqual(-6.0, d.dblimit, 4)
    self.assertAlmostEqual(1.0, d.bblimit, 4)

    d = self.classUnderTest.get_options("noname")
    self.assertAlmostEqual(-6.0, d.dblimit, 4)
    self.assertAlmostEqual(1.0, d.bblimit, 4)

  def test_get_options_seang(self):
    d = self.classUnderTest.get_options("seang")
    self.assertAlmostEqual(-6.0, d.dblimit, 4)
    self.assertAlmostEqual(1.0, d.bblimit, 4)

  def test_get_options_sekkr(self):
    d = self.classUnderTest.get_options("sekkr")
    self.assertAlmostEqual(-7.0, d.dblimit, 4)
    self.assertAlmostEqual(2.0, d.bblimit, 4)

  def test_get_options_for_object_default(self):
    a=_polarscan.new()
    a.source="NOD:sella"

    d = self.classUnderTest.get_options_for_object(a)
    self.assertAlmostEqual(-6.0, d.dblimit, 4)
    self.assertAlmostEqual(1.0, d.bblimit, 4)

  def test_get_options_for_object_seang(self):
    a=_polarscan.new()
    a.source="NOD:seang"

    d = self.classUnderTest.get_options_for_object(a)
    self.assertAlmostEqual(-6.0, d.dblimit, 4)
    self.assertAlmostEqual(1.0, d.bblimit, 4)

  def test_get_options_for_object_sekkr(self):
    a=_polarscan.new()
    a.source="NOD:sekkr"

    d = self.classUnderTest.get_options_for_object(a)
    self.assertAlmostEqual(-7.0, d.dblimit, 4)
    self.assertAlmostEqual(2.0, d.bblimit, 4)
