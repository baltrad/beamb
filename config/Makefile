###########################################################################
# Copyright (C) 2024 Swedish Meteorological and Hydrological Institute, SMHI,
#
# This file is part of beamb.
#
# beamb is free software: you can redistribute it and/or modify
# it under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
# 
# beamb is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU Lesser General Public License for more details.
# 
# You should have received a copy of the GNU Lesser General Public License
# along with RAVE.  If not, see <http://www.gnu.org/licenses/>.
# ------------------------------------------------------------------------
# 
# beamb make file
# @file
# @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
# @date 2024-11-13
###########################################################################
-include ../def.mk

# And the rest of the make file targets
#
.PHONY=all
all: ;

.PHONY=install
install: 
	@mkdir -p ${DESTDIR}${prefix}/share/beamb/config
	@cp -f beamb_options.xml ${DESTDIR}${prefix}/share/beamb/config/
		
.PHONY=clean
clean:
	@\rm -f *.o core *~

.PHONY=distclean		 
distclean:	clean
