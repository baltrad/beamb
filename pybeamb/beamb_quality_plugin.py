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
from rave_quality_plugin import QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY
from rave_quality_plugin import QUALITY_CONTROL_MODE_ANALYZE

import rave_pgf_logger

import _polarscan
import _polarvolume
import _beamblockage

# The plugin needs to have access to the two parameters below, stated in beamb_defines.py. Earlier
# these parameters were given directly in the plugin but for convenience reasons e.g. when using 
# other than default values and e.g. making re-installations it is better to have the parameters
# specified in a separate file. If the data in beamb_defines.py is not changed, the plugin will
# run with the default parameters which are BEAMBLOCKAGE_DBLIMIT = -6.0 and
# BEAMBLOCKAGE_BBLIMIT = 1.0, the latter means that no masking with nodata will be done.
from beamb_defines import BEAMBLOCKAGE_DBLIMIT, BEAMBLOCKAGE_BBLIMIT 

logger = rave_pgf_logger.create_logger()

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
  # The default beam blockage Gaussian approximation main lobe. Defaults to -6.0 if not changed in beamb_defines.py.
  #
  _dblimit = BEAMBLOCKAGE_DBLIMIT

  ##
  # The default percent beam blockage (divided by 100). Defaults 1.0 if not changed in beamb_defines.py.
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
  # @param reprocess_quality_flag: If the quality fields should be reprocessed or not.
  # @param arguments: Not used
  # @return: The modified object if this quality plugin has performed changes 
  # to the object.
  def process(self, obj, reprocess_quality_flag=True, quality_control_mode=QUALITY_CONTROL_MODE_ANALYZE_AND_APPLY, arguments=None):
    if obj != None:
      try:
        if _polarscan.isPolarScan(obj):
          if reprocess_quality_flag == False and obj.findQualityFieldByHowTask("se.smhi.detector.beamblockage") != None:
            return obj
          bb = self._create_bb()
          result = bb.getBlockage(obj, self._dblimit)
          if quality_control_mode != QUALITY_CONTROL_MODE_ANALYZE:
            _beamblockage.restore(obj, result, "DBZH", self._bblimit)
          obj.addOrReplaceQualityField(result)
          
        elif _polarvolume.isPolarVolume(obj):
          for i in range(obj.getNumberOfScans()):
            scan = obj.getScan(i)
            if reprocess_quality_flag == False and scan.findQualityFieldByHowTask("se.smhi.detector.beamblockage") != None:
              continue
            bb = self._create_bb()
            result = bb.getBlockage(scan, self._dblimit)
            if quality_control_mode != QUALITY_CONTROL_MODE_ANALYZE:
              _beamblockage.restore(scan, result, "DBZH", self._bblimit)
            scan.addOrReplaceQualityField(result)
      except:
        logger.exception("Failed to generate beam blockage field")

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
