/**
@file	 SigLevDet.cpp
@brief   The base class for a signal level detector
@author  Tobias Blomberg / SM0SVX
@date	 2013-07-30

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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
#include "Rx.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



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

SigLevDet::SigLevDet(void)
  : last_rx_id(Rx::ID_UNKNOWN)
{
  
} /* SigLevDet::SigLevDet */


#if 0
SigLevDet::~SigLevDet(void)
{
  
} /* SigLevDet::~SigLevDet */
#endif


bool SigLevDet::initialize(Async::Config &cfg, const std::string& name)
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

