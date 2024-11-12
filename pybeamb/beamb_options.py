'''
Copyright (C) 2024- Swedish Meteorological and Hydrological Institute (SMHI)

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
# Provides support for managing the beamb_options. 

## 
# @file
# @author Anders Henja, SMHI
# @date 2024-11-11
import xml.etree.ElementTree as ET
import os
import odim_source
import copy
import rave_pgf_logger

logger = rave_pgf_logger.create_logger()

try:
  from beamb_defines import BEAMB_OPTIONS_FILE
except:
  # If the beamb_defines for some reason is missing the options file path
  beambroot=os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
  BEAMB_OPTIONS_FILE=f"{beambroot}/config/beamb_options.xml"

## 
# These are the default values to be used for beamb_options unless it has
# been configured in the beamb_options.xml file. BEAMBLOCKAGE_DBLIMIT = -6.0 and
# BEAMBLOCKAGE_BBLIMIT = 1.0, the latter means that no masking with nodata will be done.
from beamb_defines import BEAMBLOCKAGE_DBLIMIT, BEAMBLOCKAGE_BBLIMIT 

class beamb_option:
  """ Options that can be used by beamb
  """
  def __init__(self):
    """Constructor
    """
    self.dblimit = BEAMBLOCKAGE_DBLIMIT
    self.bblimit = BEAMBLOCKAGE_BBLIMIT

  def __str__(self):
    return "dblimit=%f, bblimit=%f"%(self.dblimit, self.bblimit)

class beamb_options:
  """ Keeps tracks of all site options
  """
  def __init__(self, configfile=BEAMB_OPTIONS_FILE):
    """ Constructor. Will trigger an exception if configuration file can't be parsed.
    :param configfile: The xml configuration file
    """
    self._configfile=configfile
    self._siteoptions={}
    self.initialize()
  
  def initialize(self):
    """ Initializes the site options.
    """
    C = ET.parse(self._configfile)
    options = C.getroot()
    for site in list(options):
      opts = beamb_option()
      for k in site.attrib.keys():
        if k == "dblimit": opts.dblimit = float(site.attrib[k])
        elif k == "bblimit": opts.bblimit = float(site.attrib[k])
      self._siteoptions[site.tag] = opts

    if "default" not in self._siteoptions:
      self._siteoptions["default"] = beamb_option()

  def get_options(self, nod):
    """ Returns options for specified nod. If nod not found, then default will be returned.
    :param nod: The nod
    :return: The options for specified nod or default options
    """
    if nod in self._siteoptions:
      return copy.deepcopy(self._siteoptions[nod])
    return copy.deepcopy(self._siteoptions["default"])

  def get_options_for_object(self, inobj):
    """ Returns options for specified object. Will atempt to extract nod from object. See \ref get_options.
    :param inobj: A rave object
    :return: the options
    """
    try:
      odim_source.CheckSource(inobj)
      S = odim_source.ODIM_Source(inobj.source)
      return self.get_options(S.nod)
    except:
      logger.exception(f"Failed to handle source for object source {inobj.source}")
      return self.get_options("default")

  def has_options(self, nod):
    """ Returns if the nod exists in the options
    """
    return nod in self._siteoptions
