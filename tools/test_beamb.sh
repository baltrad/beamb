#!/bin/sh
############################################################
# Description: Script that performs the actual unit test of
#   bRopo.
#
# Author(s):   Anders Henja
#
# Copyright:   Swedish Meteorological and Hydrological Institute, 2009
#
# History:  2009-06-15 Created by Anders Henja
############################################################
SCRFILE=`python -c "import os;print os.path.abspath(\"$0\")"`
SCRIPTPATH=`dirname "$SCRFILE"`

RES=255

if [ $# -gt 0 -a "$1" = "alltest" ]; then
  "$SCRIPTPATH/run_python_script.sh" "${SCRIPTPATH}/../test/pytest/RopoFullTestSuite.py" "${SCRIPTPATH}/../test/pytest"
  RES=$?
else
  "$SCRIPTPATH/run_python_script.sh" "${SCRIPTPATH}/../test/pytest/RopoTestSuite.py" "${SCRIPTPATH}/../test/pytest"
  RES=$?
fi

exit $RES
