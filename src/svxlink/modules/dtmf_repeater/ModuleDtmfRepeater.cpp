/**
@file	 ModuleDtmfRepeater.cpp
@brief   A_brief_description_of_this_module
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-28

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004  Tobias Blomberg

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

#include <stdio.h>

#include <iostream>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <version/MODULE_DTMF_REPEATER.h>
#include <AsyncConfig.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleDtmfRepeater.h"



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
 * Pure C-functions
 *
 ****************************************************************************/


extern "C" {
  Module *module_init(void *dl_handle, Logic *logic, const char *cfg_name)
  {
    return new ModuleDtmfRepeater(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleDtmfRepeater::ModuleDtmfRepeater(void *dl_handle, Logic *logic,
      	      	      	      	       const string& cfg_name)
  : Module(dl_handle, logic, cfg_name), repeat_delay(0), repeat_delay_timer(0)
{
  cout << "\tModule DTMF Repeater v" MODULE_DTMF_REPEATER_VERSION
      	  " starting...\n";

} /* ModuleDtmfRepeater */


ModuleDtmfRepeater::~ModuleDtmfRepeater(void)
{
  delete repeat_delay_timer;
} /* ~ModuleDtmfRepeater */





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
 * Method:    initialize
 * Purpose:   Called by the core system right after the object has been
 *    	      constructed. As little of the initialization should be done in
 *    	      the constructor. It's easier to handle errors here.
 * Input:     None
 * Output:    Return \em true on success or else \em false should be returned
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:   The base class initialize method must be called from here.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
bool ModuleDtmfRepeater::initialize(void)
{
  if (!Module::initialize())
  {
    return false;
  }
  
  string value;
  if (cfg().getValue(cfgName(), "REPEAT_DELAY", value))
  {
    repeat_delay = atoi(value.c_str());
  }
  
  return true;
  
} /* initialize */


/*
 *----------------------------------------------------------------------------
 * Method:    activateInit
 * Purpose:   Called by the core system when this module is activated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleDtmfRepeater::activateInit(void)
{
  received_digits.clear();
} /* activateInit */


/*
 *----------------------------------------------------------------------------
 * Method:    deactivateCleanup
 * Purpose:   Called by the core system when this module is deactivated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   Do NOT call this function directly unless you really know what
 *    	      you are doing. Use Module::deactivate() instead.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleDtmfRepeater::deactivateCleanup(void)
{
  delete repeat_delay_timer;
  repeat_delay_timer = 0;
} /* deactivateCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received. This function will only be called if the module
 *    	      is active.
 * Input:     digit   	- The DTMF digit received (0-9, A-D, *, #)
 *            duration	- The length in milliseconds of the received digit
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
bool ModuleDtmfRepeater::dtmfDigitReceived(char digit, int duration)
{
  cout << "DTMF digit " << digit << " (" << duration
       << "ms) received in module " << name() << endl;
  
  if (digit == '#' && (duration > 3000))
  {
    deactivateMe();
    return true;
  }
  
  received_digits += digit;
  
  if (repeat_delay == 0)
  {
    onRepeatDelayExpired(0);
  }
  
  return true;
  
} /* dtmfDigitReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    squelchOpen
 * Purpose:   Called by the core system when the squelch open or close.
 * Input:     is_open - Set to \em true if the squelch is open or \em false
 *    	      	      	if it's not.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleDtmfRepeater::squelchOpen(bool is_open)
{
  setIdle(!is_open);
  
  if (is_open)
  {
    delete repeat_delay_timer;
    repeat_delay_timer = 0;    
  }
  else
  {
    if ((repeat_delay > 0) && !received_digits.empty())
    {
      repeat_delay_timer = new Timer(repeat_delay);
      repeat_delay_timer->expired.connect(
	  slot(this, &ModuleDtmfRepeater::onRepeatDelayExpired));
    }
  }
} /* squelchOpen */


void ModuleDtmfRepeater::onRepeatDelayExpired(Timer *t)
{
  cout << name() << ": Sending DTMF digits " << received_digits << endl;
  sendDtmf(received_digits);
  received_digits.clear();
} /* ModuleDtmfRepeater::onRepeatDelayExpired */



/*
 * This file has not been truncated
 */
