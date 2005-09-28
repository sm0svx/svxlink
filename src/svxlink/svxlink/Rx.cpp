/**
@file	 Rx.cpp
@brief   The base class for a receiver
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

A_detailed_description_for_this_file

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

#include "Rx.h"
#include "LocalRx.h"
#include "Voter.h"



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

Rx *Rx::create(Config& cfg, const string& name)
{
  Rx *rx = 0;
  string rx_type;
  if (!cfg.getValue(name, "TYPE", rx_type))
  {
    cerr << "*** ERROR: Config variable " << name << "/TYPE not set\n";
    return 0;
  }
  
  if (rx_type == "Local")
  {
    rx = new LocalRx(cfg, name);
  }
  else if (rx_type == "Voter")
  {
    rx = new Voter(cfg, name);
  }
  else
  {
    cerr << "*** ERROR: Unknown RX type \"" << rx_type << "\". Legal values "
      	 << "are: \"Local\" or \"Voter\"\n";
    return 0;
  }
  
  return rx;
  
} /* Rx::create */



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
void Rx::setSquelchState(bool is_open)
{
  if (m_verbose)
  {
    cout << m_name << ": The squelch is " << (is_open ? "OPEN" : "CLOSED")
         << endl;
  }
  squelchOpen(is_open);
} /* Rx::setSquelchState */





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

