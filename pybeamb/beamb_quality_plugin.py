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

import beamb_options

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
  # Default constructor
  def __init__(self):
    super(beamb_quality_plugin, self).__init__()
    self._beamboptions = beamb_options.beamb_options()
  
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
          options = self._beamboptions.get_options_for_object(obj)
          result = bb.getBlockage(obj, options.dblimit)
          if quality_control_mode != QUALITY_CONTROL_MODE_ANALYZE:
            _beamblockage.restore(obj, result, "DBZH", options.bblimit)
          obj.addOrReplaceQualityField(result)
          
        elif _polarvolume.isPolarVolume(obj):
          options = self._beamboptions.get_options_for_object(obj)
          for i in range(obj.getNumberOfScans()):
            scan = obj.getScan(i)
            if reprocess_quality_flag == False and scan.findQualityFieldByHowTask("se.smhi.detector.beamblockage") != None:
              continue
            bb = self._create_bb()
            result = bb.getBlockage(scan, options.dblimit)
            if quality_control_mode != QUALITY_CONTROL_MODE_ANALYZE:
              _beamblockage.restore(scan, result, "DBZH", options.bblimit)
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
