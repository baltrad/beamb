'''
Copyright (C) 2011- Swedish Meteorological and Hydrological Institute (SMHI)

This file is part of the BEAMB extension to RAVE.

BEAMB is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

BEAMB is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with BEAMB.  If not, see <http://www.gnu.org/licenses/>.
'''
##
# A quality plugin for enabling the beamb support 

## 
# @file
# @author Anders Henja, SMHI
# @date 2012-01-03

from rave_quality_plugin import rave_quality_plugin
import _polarscan
import _polarvolume
import _beamblockage

##
# The limit of the Gaussian approximation of main lobe
#
BEAMBLOCKAGE_DBLIMIT=-6.0
BEAMBLOCKAGE_BBLIMIT= 1.1

##
# The beam blockage quality plugin
#
class beamb_quality_plugin(rave_quality_plugin):
  ##
  # The gtopo30 directory. If not set, then default values
  # will be used.
  #
  _topodir = None
  
  ##
  # The cachedir. If not set, then default values
  # will be used.
  #
  _cachedir = None
  
  ##
  # The default beam blockage Gaussian approximation main lobe. Defaults to BEAMBLOCKAGE_DBLIMIT
  #
  _dblimit = BEAMBLOCKAGE_DBLIMIT

  ##
  # The default percent beam blockage (divided by 100). Defaults to BEAMBLOCKAGE_BBLIMIT which 
  # is set to 110% so that no radar data will be masked to NODATA.
  #
  _bblimit = BEAMBLOCKAGE_BBLIMIT
    
  ##
  # Default constructor
  def __init__(self):
    super(beamb_quality_plugin, self).__init__()
  
  ##
  # @return a list containing the string se.smhi.detector.beamblockage
  def getQualityFields(self):
    return ["se.smhi.detector.beamblockage"]
  
  ##
  # @param obj: A rave object that should be processed.
  # @return: The modified object if this quality plugin has performed changes 
  # to the object.
  def process(self, obj):
    if obj != None:
      try:
        if _polarscan.isPolarScan(obj):
          bb = self._create_bb()
          result = bb.getBlockage(obj, self._dblimit)
          obj.addQualityField(result)
        elif _polarvolume.isPolarVolume(obj):
          for i in range(obj.getNumberOfScans()):
            bb = self._create_bb()
            scan = obj.getScan(i)
            result = bb.getBlockage(scan, self._dblimit)
            restored = _beamblockage.restore(scan, result, "DBZH", self._bblimit)
            scan.addQualityField(result)
      except Exception,e:
        pass

    return obj

  ##
  # Creates a beam blockage instance
  #
  def _create_bb(self):
    bb = _beamblockage.new()
    if self._topodir != None:
      bb.topo30dir = self._topodir
    if self._cachedir != None:
      bb.cachedir = self._cachedir
    return bb
