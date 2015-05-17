/**
@file	 ModuleTrx.cpp
@brief   A module that provide access to a tranceiver
@author  Tobias Blomberg / SM0SVX
@date	 2015-05-16

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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
#include <iomanip>
#include <sstream>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <version/MODULE_TRX.h>
#include <Rx.h>
#include <Tx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleTrx.h"



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
 * Pure C-functions
 *
 ****************************************************************************/


extern "C" {
  Module *module_init(void *dl_handle, Logic *logic, const char *cfg_name)
  {
    return new ModuleTrx(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ModuleTrx::ModuleTrx(void *dl_handle, Logic *logic, const string& cfg_name)
  : Module(dl_handle, logic, cfg_name), rx(0), tx(0)
{
  cout << "\tModule Trx v" MODULE_TRX_VERSION " starting...\n";

} /* ModuleTrx */


ModuleTrx::~ModuleTrx(void)
{
  AudioSink::clearHandler();
  AudioSource::clearHandler();
  delete rx;
  delete tx;
} /* ~ModuleTrx */



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
 *----------------------------------------------------------------------------
 * Method:    initialize
 * Purpose:   Called by the core system right after the object has been
 *    	      constructed. As little of the initialization should be done in
 *    	      the constructor. It's easier to handle errors here.
 * Input:     None
 * Output:    Return \em true on success or else \em false should be returned
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2015-05-16
 * Remarks:   The base class initialize method must be called from here.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
bool ModuleTrx::initialize(void)
{
  if (!Module::initialize())
  {
    return false;
  }

  string rx_name;
  cfg().getValue(cfgName(), "RX", rx_name);
  rx = RxFactory::createNamedRx(cfg(), rx_name);
  if ((rx == 0) || !rx->initialize())
  {
    cerr << "*** ERROR: Could not initialize receiver \"" << rx_name
         << "\" in ModuleTrx\n";
    return false;
  }
  AudioSource::setHandler(rx);
  
  string tx_name;
  cfg().getValue(cfgName(), "TX", tx_name);
  tx = TxFactory::createNamedTx(cfg(), tx_name);
  if ((tx == 0) || !tx->initialize())
  {
    cerr << "*** ERROR: Could not initialize transmitter \"" << tx_name
         << "\" in ModuleTrx\n";
    return false;
  }
  AudioSink::setHandler(tx);

  return true;
  
} /* initialize */


/*
 *----------------------------------------------------------------------------
 * Method:    activateInit
 * Purpose:   Called by the core system when this module is activated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2015-05-16
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTrx::activateInit(void)
{
  rx->setMuteState(Rx::MUTE_NONE);
  tx->setTxCtrlMode(Tx::TX_AUTO);
} /* activateInit */


/*
 *----------------------------------------------------------------------------
 * Method:    deactivateCleanup
 * Purpose:   Called by the core system when this module is deactivated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2015-05-16
 * Remarks:   Do NOT call this function directly unless you really know what
 *    	      you are doing. Use Module::deactivate() instead.
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTrx::deactivateCleanup(void)
{
  rx->setMuteState(Rx::MUTE_ALL);
  tx->setTxCtrlMode(Tx::TX_OFF);
} /* deactivateCleanup */


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
 * Created:   2015-05-16
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
bool ModuleTrx::dtmfDigitReceived(char digit, int duration)
{
  //cout << "DTMF digit received in module " << name() << ": " << digit << endl;
  
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
 * Created:   2015-05-16
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTrx::dtmfCmdReceived(const string& cmd)
{
  cout << "DTMF command received in module " << name() << ": " << cmd << endl;
  
  if (cmd == "")
  {
    deactivateMe();
  }
  else
  {
    string fqstr(cmd);
    replace(fqstr.begin(), fqstr.end(), '*', '.');
    stringstream ss(fqstr);
    double fq;
    ss >> fq;
    ios_base::fmtflags orig_cout_flags(cout.flags());
    cout << name() << ": Setting receiver frequency "
         << setprecision(3) << fixed << fq << "kHz\n";
    cout.flags(orig_cout_flags);
    rx->setFq(1000 * fq);
  }
} /* dtmfCmdReceived */


#if 0
void ModuleTrx::dtmfCmdReceivedWhenIdle(const std::string &cmd)
{

} /* dtmfCmdReceivedWhenIdle */


/*
 *----------------------------------------------------------------------------
 * Method:    squelchOpen
 * Purpose:   Called by the core system when the squelch open or close.
 * Input:     is_open - Set to \em true if the squelch is open or \em false
 *    	      	      	if it's not.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2015-05-16
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTrx::squelchOpen(bool is_open)
{
  
} /* squelchOpen */


/*
 *----------------------------------------------------------------------------
 * Method:    allMsgsWritten
 * Purpose:   Called by the core system when all announcement messages has
 *    	      been played. Note that this function also may be called even
 *    	      if it wasn't this module that initiated the message playing.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2015-05-16
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTrx::allMsgsWritten(void)
{

} /* allMsgsWritten */
#endif



/*
 * This file has not been truncated
 */
