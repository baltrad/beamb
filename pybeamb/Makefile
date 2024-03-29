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
# along with beamb.  If not, see <http://www.gnu.org/licenses/>.
# ------------------------------------------------------------------------
# 
# beamb make file
# @file
# @author Anders Henja (Swedish Meteorological and Hydrological Institute, SMHI)
# @date 2011-11-14
###########################################################################
-include ../def.mk

# c flags, use rave suggested ones.
#
CFLAGS= -I../lib -I. $(RAVE_MODULE_PYCFLAGS)

# Linker flags
LDFLAGS= -L../lib -L. $(RAVE_MODULE_LDFLAGS) 

LIBRARIES= -lbeamb $(RAVE_MODULE_PYLIBRARIES)

# --------------------------------------------------------------------
# Fixed definitions

BEAMBLOCKAGE_SOURCE= pybeamblockage.c
BEAMBLOCKAGE_OBJECTS= $(BEAMBLOCKAGE_SOURCE:.c=.o)
BEAMBLOCKAGE_TARGET= _beamblockage.so

BEAMBLOCKAGEMAP_SOURCE= pybeamblockagemap.c
BEAMBLOCKAGEMAP_OBJECTS= $(BEAMBLOCKAGEMAP_SOURCE:.c=.o)
BEAMBLOCKAGEMAP_TARGET= _beamblockagemap.so

BBTOPOGRAPHY_SOURCE= pybbtopography.c
BBTOPOGRAPHY_OBJECTS= $(BBTOPOGRAPHY_SOURCE:.c=.o)
BBTOPOGRAPHY_TARGET= _bbtopography.so

MAKECDEPEND=$(CC) -MM $(CFLAGS) -MT '$(@D)/$(@F)' -o $(DF).d $<

DEPDIR=.dep
DF=$(DEPDIR)/$(*F)

# Ensures that the .dep directory exists
.PHONY=$(DEPDIR)
$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

# And the rest of the make file targets
#
.PHONY=all
all:		$(BEAMBLOCKAGE_TARGET) $(BEAMBLOCKAGEMAP_TARGET) $(BBTOPOGRAPHY_TARGET)

$(BEAMBLOCKAGE_TARGET): $(DEPDIR) $(BEAMBLOCKAGE_OBJECTS) ../lib/libbeamb.so
	$(LDSHARED) -o $@ $(BEAMBLOCKAGE_OBJECTS) $(LDFLAGS) $(LIBRARIES)

$(BEAMBLOCKAGEMAP_TARGET): $(DEPDIR) $(BEAMBLOCKAGEMAP_OBJECTS) ../lib/libbeamb.so
	$(LDSHARED) -o $@ $(BEAMBLOCKAGEMAP_OBJECTS) $(LDFLAGS) $(LIBRARIES)

$(BBTOPOGRAPHY_TARGET): $(DEPDIR) $(BBTOPOGRAPHY_OBJECTS) ../lib/libbeamb.so
	$(LDSHARED) -o $@ $(BBTOPOGRAPHY_OBJECTS) $(LDFLAGS) $(LIBRARIES)
	
.PHONY=install
install:
	@mkdir -p "${DESTDIR}${prefix}/share/beamb/pybeamb"
	@cp -v -f $(BEAMBLOCKAGE_TARGET) "${DESTDIR}${prefix}/share/beamb/pybeamb/"
	@cp -v -f $(BEAMBLOCKAGEMAP_TARGET) "${DESTDIR}${prefix}/share/beamb/pybeamb/"
	@cp -v -f $(BBTOPOGRAPHY_TARGET) "${DESTDIR}${prefix}/share/beamb/pybeamb/"
	@cp -v -f *.py "${DESTDIR}${prefix}/share/beamb/pybeamb/"
	@mkdir -p "${DESTDIR}${SITEPACK_PYTHON}"
	@-echo "$(prefix)/share/beamb/pybeamb" > "${DESTDIR}$(SITEPACK_PYTHON)/pybeamb.pth"

.PHONY=clean
clean:
	@\rm -f *.o core *~
	@\rm -fr $(DEPDIR)

.PHONY=distclean		 
distclean:	clean
	@\rm -f $(BEAMBLOCKAGE_TARGET) $(BEAMBLOCKAGEMAP_TARGET) $(BBTOPOGRAPHY_TARGET)

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
-include $(BEAMBLOCKAGE_SOURCE:%.c=$(DEPDIR)/%.P)
-include $(BEAMBLOCKAGEMAP_SOURCE:%.c=$(DEPDIR)/%.P)
-include $(BBTOPOGRAPHY_SOURCE:%.c=$(DEPDIR)/%.P)
