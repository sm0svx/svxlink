/**
@file	 RxHandler.cpp
@brief   This file contains the receiver handler class
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "RxHandler.h"
#include "Rx.h"
#include "Uplink.h"


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

RxHandler::RxHandler(Async::Config &cfg)
  : cfg(cfg), uplink(0), rx(0)
{
  
} /* RxHandler::RxHandler */


RxHandler::~RxHandler(void)
{
  delete uplink;
  delete rx;
} /* RxHandler::~RxHandler */


bool RxHandler::initialize(void)
{
  string rx_name;
  if (!cfg.getValue("GLOBAL", "RX", rx_name))
  {
    cerr << "*** ERROR: Configuration variable GLOBAL/RX not set.\n";
    return false;
  }

  string uplink_name;
  if (!cfg.getValue("GLOBAL", "UPLINK", uplink_name))
  {
    cerr << "*** ERROR: Configuration variable GLOBAL/UPLINK not set.\n";
    return false;
  }
  
  rx = Rx::create(cfg, rx_name);
  if (!rx->initialize())
  {
    cerr << "*** ERROR: Could not initialize Rx object \""
      	 << rx_name << "\". Skipping...\n";
    delete rx;
  }
  /*
  rx->squelchOpen.connect(slot(this, &RxHandler::squelchOpen));
  rx->audioReceived.connect(slot(this, &RxHandler::audioReceived));
  rx->dtmfDigitDetected.connect(slot(this, &RxHandler::dtmfDigitDetected));
  rx->toneDetected.connect(slot(this, &RxHandler::toneDetected));
  rx->mute(false);
  */
  
  uplink = Uplink::create(cfg, uplink_name, rx);
  if ((uplink == 0) || !uplink->initialize())
  {
    return false;
  }
  
  return true;
  
} /* RxHandler::initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */






/*
 * This file has not been truncated
 */

