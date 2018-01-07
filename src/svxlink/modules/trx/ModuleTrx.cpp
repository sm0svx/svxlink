/**
@file	 ModuleTrx.cpp
@brief   A module that provide access to a tranceiver
@author  Tobias Blomberg / SM0SVX
@date	 2015-05-16

\verbatim
A module (plugin) for the multi purpose tranceiver frontend system.
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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
#include <AsyncTimer.h>


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
    return new ModuleTrx(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

ModuleTrx::ModuleTrx(void *dl_handle, Logic *logic, const string& cfg_name)
  : Module(dl_handle, logic, cfg_name), rx(0), tx(0), auto_mod_select(false),
    rx_timeout_timer(0)
{
  cout << "\tModule Trx v" MODULE_TRX_VERSION " starting...\n";

} /* ModuleTrx */


ModuleTrx::~ModuleTrx(void)
{
  delete rx_timeout_timer;
  rx_timeout_timer = 0;
  AudioSink::clearHandler();
  AudioSource::clearHandler();
  delete rx;
  rx = 0;
  delete tx;
  tx = 0;
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
         << "\" in module \"" << name() << "\"\n";
    return false;
  }
  rx->squelchOpen.connect(mem_fun(*this, &ModuleTrx::rxSquelchOpen));
  AudioSource::setHandler(rx);
  
  string tx_name;
  cfg().getValue(cfgName(), "TX", tx_name);
  tx = TxFactory::createNamedTx(cfg(), tx_name);
  if ((tx == 0) || !tx->initialize())
  {
    cerr << "*** ERROR: Could not initialize transmitter \"" << tx_name
         << "\" in module \"" << name() << "\"\n";
    return false;
  }
  AudioSink::setHandler(tx);

  string modstr;
  if (cfg().getValue(cfgName(), "MODULATION", modstr))
  {
    if (modstr == "AUTO")
    {
      auto_mod_select = true;
    }
    else
    {
      Modulation::Type mod = Modulation::fromString(modstr);
      if (mod == Modulation::MOD_UNKNOWN)
      {
        cerr << "*** ERROR: Unsupported modulation \"" << modstr
             << "\" configured in module \"" << name() << "\"\n";
        return false;
      }
      rx->setModulation(mod);
      tx->setModulation(mod);
    }
  }

  int rx_timeout = 0;
  cfg().getValue(cfgName(), "RX_TIMEOUT", rx_timeout);
  if (rx_timeout > 0)
  {
    rx_timeout_timer = new Timer(1000 * rx_timeout);
    rx_timeout_timer->setEnable(false);
    rx_timeout_timer->expired.connect(mem_fun(*this, &ModuleTrx::rxTimeout));
  }

  stringstream ss;
  ss << cfgName() << ":Bands";
  std::list<std::string> bandlist = cfg().listSection(ss.str());
  for (std::list<std::string>::iterator it=bandlist.begin();
       it!=bandlist.end();
       ++it)
  {
    istringstream iss(*it);
    double fqstart=0.0, fqend=0.0;
    char minus=0;
    iss >> fqstart >> minus >> fqend;
    if (iss.fail() || (minus != '-') || (fqstart <= 0.0) || (fqend <= 0.0))
    {
      cerr << "*** WARNING[" << cfgName() << "]: Illegal fq band: "
           << *it << endl;
      continue;
    }
    std::string modstr;
    cfg().getValue(ss.str(), *it, modstr);
    Modulation::Type mod = Modulation::fromString(modstr);
    if (mod == Modulation::MOD_UNKNOWN)
    {
      cerr << "*** WARNING[" << cfgName() << "]: Illegal modulation specified: "
           << *it << "=" << modstr << endl;
      continue;
    }
    cout << "\tBand: " << *it << ": fqstart=" << fqstart
         << " fqend=" << fqend
         << " modstr=" << Modulation::toString(mod)
         << endl;
    Band band;
    band.fqstart = static_cast<unsigned>(fqstart * 1000);
    band.fqend = static_cast<unsigned>(fqend * 1000);
    band.mod = mod;
    bands.push_back(band);
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
  if (rx_timeout_timer != 0)
  {
    rx_timeout_timer->setEnable(false);
  }
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
    double fqin;
    ss >> fqin;
    unsigned fq = static_cast<unsigned>(1000 * fqin);
    for (Bands::const_iterator it=bands.begin(); it!=bands.end(); ++it)
    {
      //cout << "### fqstart=" << (*it).fqstart << " fqend=" << (*it).fqend
      //     << " mod=" << Modulation::toString((*it).mod) << endl;
      if ((fq >= (*it).fqstart) && (fq <= (*it).fqend))
      {
        ios_base::fmtflags orig_cout_flags(cout.flags());
        cout << name() << ": Setting transciver to "
             << setprecision(3) << fixed << fqin << "kHz "
             << Modulation::toString((*it).mod) << endl;
        cout.flags(orig_cout_flags);
        rx->setFq(fq);
        tx->setFq(fq);
        if (auto_mod_select)
        {
          rx->setModulation((*it).mod);
          tx->setModulation((*it).mod);
        }
        return;
      }
    }
    cerr << "*** WARNING[" << cfgName()
         << "]: Out of band frequency specified: "
         << fqin << "kHz" << endl;
  }
} /* dtmfCmdReceived */


#if 0
void ModuleTrx::dtmfCmdReceivedWhenIdle(const std::string &cmd)
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
 * Created:   2015-05-16
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleTrx::squelchOpen(bool is_open)
{
  //cout << "### ModuleTrx::squelchOpen: is_open=" << is_open << endl;
  if (isActive())
  {
    rx->setMuteState(Rx::MUTE_NONE);
    if (rx_timeout_timer != 0)
    {
      rx_timeout_timer->reset();
    }
  }
} /* squelchOpen */


#if 0
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


void ModuleTrx::rxTimeout(Async::Timer *t)
{
  cout << cfgName() << ": RX Timeout" << endl;
  rx->setMuteState(Rx::MUTE_ALL);
} /* ModuleTrx::rxTimeout */


void ModuleTrx::rxSquelchOpen(bool is_open)
{
  //cout << "### ModuleTrx::rxSquelchOpen: is_open=" << is_open << endl;
  if (isActive())
  {
    setIdle(!is_open);

    if (rx_timeout_timer != 0)
    {
      rx_timeout_timer->setEnable(is_open);
    }
  }
} /* ModuleTrx::rxSquelchOpen */


/*
 * This file has not been truncated
 */
