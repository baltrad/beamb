/* --------------------------------------------------------------------
Copyright (C) 2011 Swedish Meteorological and Hydrological Institute, SMHI,

This file is part of beam blockage (beamb).

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
/**
 * Beam-blockage analysis
 * @file
 * @author Lars Norin (Swedish Meteorological and Hydrological Institute, SMHI)
 *
 * @author Anders Henja (SMHI, refactored to work together with rave)
 * @date 2011-11-10
 */
#include "beamblockage.h"
#include "rave_debug.h"
#include "rave_alloc.h"
#include "math.h"
#include <string.h>

/**
 * Represents the beam blockage algorithm
 */
struct _BeamBlockage_t {
  RAVE_OBJECT_HEAD /** Always on top */
  char* topodir;   /**< the topo30 directory */
};


/*@{ Private functions */
/**
 * Constructor.
 */
static int BeamBlockage_constructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
  self->topodir = NULL;
  return 1;
}

/**
 * Destroys the polar navigator
 * @param[in] polnav - the polar navigator to destroy
 */
static void BeamBlockage_destructor(RaveCoreObject* obj)
{
  BeamBlockage_t* self = (BeamBlockage_t*)obj;
  RAVE_FREE(self->topodir);
}

/**
 * Copy constructor
 */
static int BeamBlockage_copyconstructor(RaveCoreObject* obj, RaveCoreObject* srcobj)
{
  BeamBlockage_t* this = (BeamBlockage_t*)obj;
  BeamBlockage_t* src = (BeamBlockage_t*)srcobj;
  this->topodir = NULL;
  return 1;
}

/*@{ Interface functions */
int BeamBlockage_setTopo30Directory(BeamBlockage_t* self, const char* topodirectory)
{
  char* tmp = NULL;
  int result = 0;

  RAVE_ASSERT((self != NULL), "self == NULL");

  if (topodirectory != NULL) {
    tmp = RAVE_STRDUP(topodirectory);
    if (tmp == NULL) {
      RAVE_ERROR0("Failed to duplicate string");
      goto done;
    }
  }

  RAVE_FREE(self->topodir);
  self->topodir = tmp;
  tmp = NULL; // Not responsible for memory any longer
  result = 1;
done:
  RAVE_FREE(tmp);
  return result;
}

const char* BeamBlockage_getTopo30Directory(BeamBlockage_t* self)
{
  RAVE_ASSERT((self != NULL), "self == NULL");
  return (const char*)self->topodir;
}

/*@} End of Interface functions */

RaveCoreObjectType BeamBlockage_TYPE = {
    "BeamBlockage",
    sizeof(BeamBlockage_t),
    BeamBlockage_constructor,
    BeamBlockage_destructor,
    BeamBlockage_copyconstructor
};
