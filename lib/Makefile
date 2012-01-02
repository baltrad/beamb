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

# Ropo specific c flags
#
CFLAGS= $(SHARED_FLAG) $(CCOPTS) $(DEFS) -I. \
			$(RAVE_INCLUDE_FLAG) $(HLHDF_INCLUDE_FLAG) $(ZLIB_INCDIR) \
			$(HDF5_INCDIR) $(PROJ_INCLUDE_FLAG)

# Linker flags
#
LDFLAGS= -L. $(RAVE_LIB_FLAG) $(HLHDF_LIBRARY_FLAG) $(ZLIB_LIBDIR) \
			$(HDF5_LIBDIR) $(PROJ_LIB_FLAG) 

# We also need to ensure that raves expat requirements are managed
EXPAT_LIBRARY=
ifeq ($(EXPAT_SUPPRESSED), no)
CFLAGS+= $(EXPAT_INCLUDE_DIR) -DRAVE_XML_SUPPORTED
LDFLAGS+= $(EXPAT_LIB_DIR)
EXPAT_LIBRARY=-lexpat
endif

# --------------------------------------------------------------------
# Fixed definitions

SOURCES= beamblockage.c beamblockagemap.c bbtopography.c
				
OBJECTS= $(SOURCES:.c=.o)

TARGET= libbeamb.so

MAKECDEPEND=$(CC) -MM $(CFLAGS) -MT '$(@F)' -o $(DF).d $<

DEPDIR=.dep
DF=$(DEPDIR)/$(*F)

# Ensures that the .dep directory exists
.PHONY=$(DEPDIR)
$(DEPDIR):
	+@[ -d $@ ] || mkdir -p $@

# And the rest of the make file targets
#
.PHONY=all
all:		$(TARGET)

$(TARGET): $(DEPDIR) $(OBJECTS)
	$(LDSHARED) -o $@ $(OBJECTS)

.PHONY=install
install:
	@mkdir -p ${prefix}/include/
	@mkdir -p ${prefix}/lib/
	@mkdir -p ${BEAMB_CACHE_DIR}
	@cp -v -f *.h ${prefix}/include/
	@cp -v -f $(TARGET) ${prefix}/lib/

.PHONY=clean
clean:
	@\rm -f *.o core *~
	@\rm -fr $(DEPDIR)

.PHONY=distclean		 
distclean:	clean
	@\rm -f $(TARGET) config.h

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
