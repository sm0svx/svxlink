/**
@file	 SigLevDet.cpp
@brief   The base class for a signal level detector
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-17

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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
#include <cassert>


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
#include "SigLevDetConst.h"



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
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "NONE";

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

SigLevDet* createSigLevDet(Async::Config& cfg, const std::string& name)
{
  static SigLevDetSpecificFactory<SigLevDetNone> none_siglev_factory;
  static SigLevDetSpecificFactory<SigLevDetNoise> noise_siglev_factory;
  static SigLevDetSpecificFactory<SigLevDetTone> tone_siglev_factory;
  static SigLevDetSpecificFactory<SigLevDetDdr> ddr_siglev_factory;
  static SigLevDetSpecificFactory<SigLevDetSim> sim_siglev_factory;
  static SigLevDetSpecificFactory<SigLevDetAfsk> afsk_siglev_factory;
  static SigLevDetSpecificFactory<SigLevDetConst> const_siglev_factory;

  string det_name;
  if (!cfg.getValue(name, "SIGLEV_DET", det_name) || det_name.empty())
  {
    det_name = "NOISE";
  }

  auto& det_map = SigLevDet::detMap();
  auto it = det_map.find(name);
  if (it != det_map.end())
  {
    return it->second;
  }

  SigLevDet *det = SigLevDetFactory::createNamedObject(det_name);
  if (det == 0)
  {
    cerr << "*** ERROR: Unknown SIGLEV_DET \"" << det_name
         << "\" specified for receiver " << name << ". Legal values are: "
         << SigLevDetFactory::validFactories() << endl;
    return 0;
  }

  if (!det->initialize(cfg, name, INTERNAL_SAMPLE_RATE))
  {
    cout << "*** ERROR: Could not initialize the signal level detector for "
         << "receiver " << name << std::endl;
    delete det;
    det = 0;
    return 0;
  }

  det_map[name] = det;

  return det;
} /* createSigLevDet */


SigLevDet::SigLevDet(void)
  : force_rx_id(false), last_rx_id(Rx::ID_UNKNOWN)
{
} /* SigLevDet::SigLevDet */


SigLevDet::~SigLevDet(void)
{
  auto it = detMap().find(name);
  if (it != detMap().end())
  {
    detMap().erase(it);
  }
} /* SigLevDet::~SigLevDet */


bool SigLevDet::initialize(Async::Config &cfg, const std::string& name,
                           int sample_rate)
{
  this->name = name;
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

