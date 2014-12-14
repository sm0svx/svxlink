/**
@file	 ModuleFrn.cpp
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
#include <AsyncConfig.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioSelector.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include <version/MODULE_FRN.h>
#include "ModuleFrn.h"



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
    return new ModuleFrn(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleFrn::ModuleFrn(void *dl_handle, Logic *logic, const string& cfg_name)
  : Module(dl_handle, logic, cfg_name)
  , qso(0)
  , audio_valve(0)
  , audio_splitter(0)
  , audio_selector(0)
{
  cout << "\tModule Frn v" MODULE_FRN_VERSION " starting...\n";

} /* ModuleFrn */


ModuleFrn::~ModuleFrn(void)
{
  moduleCleanup();
} /* ~ModuleFrn */


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
bool ModuleFrn::initialize(void)
{
  if (!Module::initialize())
  {
    return false;
  }
  if (!cfg().getValue(cfgName(), "SERVER", opt_server))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/SERVER not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "EMAIL_ADDRESS", opt_email_address))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/EMAIL_ADDRESS not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "DYN_PASSWORD", opt_dyn_password))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/DYN_PASSWORD not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "CALLSIGN_AND_USER", opt_callsign_and_user))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/CALLSIGN_AND_USER not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "CLIENT_TYPE", opt_client_type))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/CLIENT_TYPE not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "BAND_AND_CHANNEL", opt_band_and_channel))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/BAND_AND_CHANNEL not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "DESCRIPTION", opt_description))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/DESCRIPTION not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "COUNTRY", opt_country))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/COUNTRY not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "CITY_CITY_PART", opt_city_city_part))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/CITY_CITY_PART not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "NET", opt_net))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/NET not set\n";
    return false;
  }
  if (!cfg().getValue(cfgName(), "VERSION", opt_version))
  {
    cerr << "*** ERROR: Config variable " << cfgName()
         << "/VERSION not set\n";
    return false;
  }
  
  audio_valve = new AudioValve;
  AudioSink::setHandler(audio_valve);

  audio_splitter = new AudioSplitter;
  audio_valve->registerSink(audio_splitter);

  audio_selector = new AudioSelector;
  AudioSource::setHandler(audio_selector);

  qso = new QsoFrn(this);
  audio_splitter->addSink(qso);
  audio_selector->addSource(qso);
  audio_selector->enableAutoSelect(qso, 0);

  return true;
  
} /* initialize */


void ModuleFrn::moduleCleanup()
{
  audio_splitter->removeSink(qso);
  audio_selector->removeSource(qso);
  delete qso;
  qso = 0;

  AudioSink::clearHandler();
  delete audio_splitter;
  audio_splitter = 0;
  delete audio_valve;
  audio_valve = 0;

  AudioSource::clearHandler();
  delete audio_selector;
  audio_selector = 0;
}

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
void ModuleFrn::activateInit(void)
{
    audio_valve->setOpen(true);
}


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
void ModuleFrn::deactivateCleanup(void)
{
  audio_valve->setOpen(true);
} 


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received. This function will only be called if the module
 *    	      is active.
 * Input:     digit   	- The DTMF digit received (0-9, A-D, *, #)
 *            duration	- The length in milliseconds of the received digit
 * Output:    Return true if the digit is handled or false if not
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
bool ModuleFrn::dtmfDigitReceived(char digit, int duration)
{
  cout << "DTMF digit received in module " << name() << ": " << digit << endl;
  
  return false;
  
} /* dtmfDigitReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfCmdReceived
 * Purpose:   Called by the core system when a DTMF command has been
 *    	      received. A DTMF command consists of a string of digits ended
 *    	      with a number sign (#). The number sign is not included in the
 *    	      command string. This function will only be called if the module
 *    	      is active.
 * Input:     cmd - The received command.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleFrn::dtmfCmdReceived(const string& cmd)
{
  cout << "DTMF command received in module " << name() << ": " << cmd << endl;
  
  if (cmd == "")
  {
    deactivateMe();
  }
  else
  {

  }
} /* dtmfCmdReceived */


#if 0
void ModuleFrn::dtmfCmdReceivedWhenIdle(const std::string &cmd)
{

} /* dtmfCmdReceivedWhenIdle */
#endif


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
void ModuleFrn::squelchOpen(bool is_open)
{
  cout << __PRETTY_FUNCTION__ << " " << is_open << endl;
}


/*
 *----------------------------------------------------------------------------
 * Method:    allMsgsWritten
 * Purpose:   Called by the core system when all announcement messages has
 *    	      been played. Note that this function also may be called even
 *    	      if it wasn't this module that initiated the message playing.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleFrn::allMsgsWritten(void)
{

} /* allMsgsWritten */


/*
 *----------------------------------------------------------------------------
 * Method:    reportState
 * Purpose:   This function is called by the logic core when it wishes the
 *    	      module to report its state on the radio channel. Typically this
 *    	      is done when a manual identification has been triggered by the
 *    	      user by sending a "*".
 *    	      This function will only be called if this module is active.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleFrn::reportState(void)
{

} /* reportState */



/*
 * This file has not been truncated
 */
