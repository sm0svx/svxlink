/**
@file	 Uplink.cpp
@brief   Contains the base class for implementing a remote trx uplink
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2004-2010 Tobias Blomberg / SM0SVX

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
#include <cstring>


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

#include "Uplink.h"
#include "NetUplink.h"
#include "RfUplink.h"



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

Uplink *Uplink::create(Config &cfg, const string &name, Rx *rx, Tx *tx)
{
  Uplink *uplink = 0;
  string uplink_type;
  if (!cfg.getValue(name, "TYPE", uplink_type))
  {
    cerr << "*** ERROR: Config variable " << name << "/TYPE not set\n";
    return 0;
  }
  
  if (uplink_type == "Net")
  {
    uplink = new NetUplink(cfg, name, rx, tx);
  }
  else if (uplink_type == "RF")
  {
    uplink = new RfUplink(cfg, name, rx, tx);
  }
  else
  {
    cerr << "*** ERROR: Unknown uplink type \"" << uplink_type
      	 << "\" for uplink " << name
         << ". Legal values are: \"Net\" and \"RF\"\n";
    return 0;
  }
  
  return uplink;
  
} /* Uplink::create */




Uplink::Uplink(void)
{
  
} /* Uplink::Uplink */


Uplink::~Uplink(void)
{
  
} /* Uplink::~Uplink */




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

