###########################################################################
# Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,
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
# @date 2011-11-10
###########################################################################
-include ../def.mk

# And the rest of the make file targets
#
.PHONY=all
all: gtopo30

gtopo30: gtopo30.tgz
	@tar -xvzf gtopo30.tgz
	@touch gtopo30
 
.PHONY=install
install: gtopo30
	@mkdir -p ${DESTDIR}${prefix}/share/beamb/data/gtopo30
	@cp -f gtopo30/*.* ${DESTDIR}${prefix}/share/beamb/data/gtopo30/
		
.PHONY=clean
clean:
	@\rm -f *.o core *~

.PHONY=distclean		 
distclean:	clean
	@\rm -fr gtopo30

.PHONY=clean_cache
clean_cache:
	@\rm -f ${prefix}/var/cache/beamb/*

# --------------------------------------------------------------------
# Rules

# Contains dependency generation as well, so if you are not using
# gcc, comment out everything until the $(CC) statement.

%.o : %.c
	@$(MAKECDEPEND); \
	cp $(DF).d $(DF).P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
		-e '/^$$/ d' -e 's/$$/ :/' < $(DF).d >> $(DF).P; \
	\rm -f $(DF).d
	$(CC) -c $(CFLAGS) $< -o $@

# NOTE! This ensures that the dependencies are setup at the right time so this should not be moved
-include $(SOURCES:%.c=$(DEPDIR)/%.P)
