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

#include "Rx.h"
#include "SigLevDet.h"
#include "SigLevDetNoise.h"
#include "SigLevDetTone.h"
#include "SigLevDetDdr.h"
#include "SigLevDetSim.h"
#include "SigLevDetAfsk.h"



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
  SigLevDetAfsk::Factory afsk_siglev_factory;
  
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


SigLevDet::SigLevDet(void)
  : force_rx_id(false), last_rx_id(Rx::ID_UNKNOWN)
{
} /* SigLevDet::SigLevDet */


#if 0
SigLevDet::~SigLevDet(void)
{
} /* SigLevDet::~SigLevDet */
#endif


bool SigLevDet::initialize(Async::Config &cfg, const std::string& name,
                           int sample_rate)
{
  if (cfg.getValue(name, "RX_ID", last_rx_id))
  {
    force_rx_id = true;
  }
  return true;
}



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void SigLevDet::updateRxId(char rx_id)
{
  if (!force_rx_id)
  {
    last_rx_id = rx_id;
  }
} /* SigLevDet::updateRxId */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

