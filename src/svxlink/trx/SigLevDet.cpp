/**
@file	 SigLevDet.cpp
@brief   The base class for a signal level detector
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-17

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDet.h"
#include "SigLevDetNoise.h"
#include "SigLevDetTone.h"
#include "SigLevDetDdr.h"
#include "SigLevDetSim.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

namespace {
  /**
   * @brief A dummy signal level detector which always return 0 siglev
   */
  class SigLevDetNone : public SigLevDet
  {
    public:
      struct Factory : public SigLevDetFactory<SigLevDetNone>
      {
        Factory(void) : SigLevDetFactory<SigLevDetNone>("NONE") {}
      };

    virtual void setContinuousUpdateInterval(int interval_ms) {}
    virtual void setIntegrationTime(int time_ms) {}
    virtual float lastSiglev(void) const { return 0; }
    virtual float siglevIntegrated(void) const { return 0; }
    virtual void reset(void) {}
    virtual int writeSamples(const float *samples, int count) { return count; }
    virtual void flushSamples(void) {}
  };
};


/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SigLevDet *SigLevDetFactoryBase::createNamedSigLevDet(Config& cfg,
                                                      const string& name)
{
  SigLevDetNone::Factory none_siglev_factory;
  SigLevDetNoise::Factory noise_siglev_factory;
  SigLevDetTone::Factory tone_siglev_factory;
  SigLevDetDdr::Factory ddr_siglev_factory;
  SigLevDetSim::Factory sim_siglev_factory;
  
  string det_name;
  if (!cfg.getValue(name, "SIGLEV_DET", det_name) || det_name.empty())
  {
    det_name = "NOISE";
  }

  SigLevDet *det = createNamedObject(det_name);
  if (det == 0)
  {
    cerr << "*** ERROR: Unknown SIGLEV_DET \"" << det_name
         << "\" specified for receiver " << name << ". Legal values are: "
         << validFactories() << endl;
  }
  
  return det;
} /* SigLevDetFactoryBase::createNamedSigLevDet */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

