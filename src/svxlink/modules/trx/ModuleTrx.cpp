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
  : Module(dl_handle, logic, cfg_name), rx(0), tx(0), current_band(0)
{
  cout << "\tModule Trx v" MODULE_TRX_VERSION " starting...\n";

} /* ModuleTrx */


ModuleTrx::~ModuleTrx(void)
{
  setTrx("NONE", "NONE");
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

  Band defband;
  cfg().getValue(cfgName(), "RX", defband.rx_name);
  cfg().getValue(cfgName(), "TX", defband.tx_name);

  string modstr;
  cfg().getValue(cfgName(), "MODULATION", modstr);
  defband.mod = Modulation::fromString(modstr);

  cfg().getValue(cfgName(), "RX_TIMEOUT", defband.rx_timeout);
  rx_timeout_timer.setEnable(false);
  rx_timeout_timer.expired.connect(mem_fun(*this, &ModuleTrx::rxTimeout));

  double def_fqtxshift = 0.0;
  cfg().getValue(cfgName(), "SHIFT", def_fqtxshift);
  defband.fqtxshift = static_cast<FrequencyDiff>(def_fqtxshift * 1000.0);

  cout << "\t"
       << " " << setw(10) << "Name"
       << " " << setw(9) << "FQ Start"
       << " " << setw(9) << "FQ End"
       << " " << setw(5) << "Shift"
       << " " << setw(4) << "Mod"
       << " " << setw(5) << "Short"
       << " " << setw(10) << "RX"
       << " " << setw(10) << "TX"
       //<< " " << setw(3) << "TMO"
       << endl;

  std::list<std::string> sectionlist = cfg().listSections();
  for (std::list<std::string>::iterator it=sectionlist.begin();
       it!=sectionlist.end();
       ++it)
  {
    string& section = *it;
    if (section.find(cfgName() + ":Band:") != 0)
    {
      continue;
    }
    string::size_type pos = section.rfind(':');
    assert(pos != string::npos);

    Band band(defband);
    band.name = section.substr(pos+1, string::npos);
    if (band.name.empty())
    {
      cerr << "*** WARNING[" << cfgName()
           << "]: Illegal band configuration section: "
           << section << endl;
      continue;
    }

    string fq;
    cfg().getValue(section, "FQ", fq);
    istringstream fqss(fq);
    double fqstart=0.0, fqend=0.0;
    char minus=0;
    fqss >> fqstart;
    if (!(fqss >> minus).eof())
    {
      if ((minus != '-') || !(fqss >> fqend))
      {
        fqstart = 0.0;
      }
    }
    else
    {
      fqend = fqstart;
    }
    if ((fqstart <= 0.0) || (fqend <= 0.0))
    {
      cerr << "*** WARNING[" << cfgName() << "]: Illegal fq band: "
           << fq << endl;
      continue;
    }
    band.fqstart = static_cast<Frequency>(fqstart * 1000);
    band.fqend = static_cast<Frequency>(fqend * 1000);

    double fqtxshift = 0.0;
    if (cfg().getValue(section, "SHIFT", fqtxshift))
    {
      band.fqtxshift = static_cast<FrequencyDiff>(fqtxshift * 1000);
    }

    band.fqdefault = band.fqstart;
    cfg().getValue(section, "FQ_DEFAULT", band.fqdefault);
    if ((band.fqdefault < band.fqstart) || (band.fqdefault > band.fqend))
    {
      cerr << "*** WARNING[]: Default frequency is outside of band: "
           << band.fqdefault << endl;
      continue;
    }

    string modstr;
    if (cfg().getValue(section, "MODULATION", modstr))
    {
      band.mod = Modulation::fromString(modstr);
    }
    if (band.mod == Modulation::MOD_UNKNOWN)
    {
      cerr << "*** WARNING[" << cfgName()
           << "]: Illegal modulation specified: "
           << section << "/MODULATION=" << modstr << endl;
      continue;
    }

    cfg().getValue(section, "SHORTCUT", band.shortcut);
    cfg().getValue(section, "RX", band.rx_name);
    cfg().getValue(section, "TX", band.tx_name);
    cfg().getValue(section, "RX_TIMEOUT", band.rx_timeout);

    bands.push_back(band);

    string rx_name(band.rx_name);
    if (rx_name.find(cfgName()) == 0)
    {
      rx_name.erase(0, cfgName().size());
    }
    string tx_name(band.tx_name);
    if (tx_name.find(cfgName()) == 0)
    {
      tx_name.erase(0, cfgName().size());
    }
    cout << "\t"
         << " " << setw(10) << band.name
         << " " << setw(9) << (band.fqstart / 1000.0)
         << " " << setw(9) << (band.fqend / 1000.0)
         << " " << setw(5) << (band.fqtxshift / 1000.0)
         << " " << setw(4) << Modulation::toString(band.mod)
         << " " << setw(5) << band.shortcut
         << " " << setw(10) << rx_name
         << " " << setw(10) << tx_name
         //<< " " << setw(3) << band.rx_timeout
         << endl;
  }

  setTrx("NONE", "NONE");

  return true;

} /* initialize */


bool ModuleTrx::setTrx(const ModuleTrx::TxName& tx_name,
                       const ModuleTrx::RxName& rx_name)
{
  if ((rx == 0) || (rx_name != rx->name()))
  {
    if (rx != 0)
    {
      rx->reset();
    }
    AudioSource::clearHandler();
    delete rx;
    rx = RxFactory::createNamedRx(cfg(), rx_name);
    if ((rx == 0) || !rx->initialize())
    {
      cerr << "*** ERROR: Could not initialize receiver \"" << rx_name
           << "\" in module \"" << name() << "\"\n";
      return false;
    }
    rx->squelchOpen.connect(mem_fun(*this, &ModuleTrx::rxSquelchOpen));
    AudioSource::setHandler(rx);
  }

  if ((tx == 0) || (tx_name != tx->name()))
  {
    AudioSink::clearHandler();
    delete tx;
    tx = TxFactory::createNamedTx(cfg(), tx_name);
    if ((tx == 0) || !tx->initialize())
    {
      cerr << "*** ERROR: Could not initialize transmitter \"" << tx_name
           << "\" in module \"" << name() << "\"\n";
      return false;
    }
    AudioSink::setHandler(tx);
  }

  rx->setMuteState(Rx::MUTE_NONE);
  tx->setTxCtrlMode(Tx::TX_AUTO);

  return true;
} /* ModuleTrx::setTrx */


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
  //setTrx("NONE", "NONE");
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
  processEvent("set_frequency 0");
  current_band = 0;
  rx_timeout_timer.setEnable(false);
  setTrx("NONE", "NONE");
  //rx->reset();
  //AudioSource::clearHandler();
  //delete rx;
  //rx = 0;
  //tx->setTxCtrlMode(Tx::TX_OFF);
  //AudioSink::clearHandler();
  //delete tx;
  //tx = 0;
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
  else if (cmd == "0")
  {
    playHelpMsg();
  }
  else if (cmd == "1")
  {
    processEvent("play_current_fq");
  }
  else
  {
    string fqstr(cmd);
    replace(fqstr.begin(), fqstr.end(), '*', '.');
    stringstream ss(fqstr);
    double fqin;
    ss >> fqin;
    Frequency fq = static_cast<Frequency>(1000 * fqin);
    Shortcut shortcut = static_cast<Shortcut>(fqin);
    const Band *band = 0;
    for (Bands::const_iterator it=bands.begin(); it!=bands.end(); ++it)
    {
      //cout << "### fqstart=" << (*it).fqstart << " fqend=" << (*it).fqend
      //     << " mod=" << Modulation::toString((*it).mod) << endl;
      if (shortcut == (*it).shortcut)
      {
        fq = (*it).fqdefault;
        fqin = static_cast<double>(fq / 1000.0);
        band = &(*it);
        current_band = 0;
        break;
      }
      else if ((fq >= (*it).fqstart) && (fq <= (*it).fqend))
      {
        if ((band == 0) ||
            band->isSuperBandOf(*it) ||
            ((*it).shortcut < band->shortcut) ||
            ((*it).fqstart < band->fqstart))
        {
          band = &(*it);
        }
      }
    }
    if (band == 0)
    {
      cerr << "*** WARNING[" << cfgName()
           << "]: Could not find matching band for command: "
           << fqstr << endl;
      ostringstream ss;
      ss << "no_matching_band " << fqstr;
      processEvent(ss.str());
      return;
    }
    if ((current_band != 0) && !current_band->isSuperBandOf(*band) &&
        ((fq >= current_band->fqstart) && (fq <= current_band->fqend)))
    {
      band = current_band;
    }
    else
    {
      current_band = band;
    }

    ios_base::fmtflags orig_cout_flags(cout.flags());
    cout << cfgName() << ": Setting transceiver (RX=" << band->rx_name
         << " TX=" << band->tx_name << ") to "
         << setprecision(3) << fixed << fqin << "kHz "
         << Modulation::toString(band->mod) << endl;
    cout.flags(orig_cout_flags);
    if (setTrx(band->tx_name, band->rx_name))
    {
      rx->setFq(fq);
      tx->setFq(fq + band->fqtxshift);
      rx->setModulation(band->mod);
      tx->setModulation(band->mod);
      ostringstream ss;
      ss << "set_frequency " << fq;
      processEvent(ss.str());
    }
    else
    {
      setTrx("NONE", "NONE");
      processEvent("set_frequency 0");
      cerr << "*** WARNING[" << cfgName() << "]: Could not set up "
           << "transceiver (TX=" << band->tx_name
           << " RX=" << band->rx_name << ")" << endl;
      ostringstream ss;
      ss << "failed_to_set_trx";
      ss << " " << fqstr;
      ss << " " << band->rx_name;
      ss << " " << band->tx_name;
      processEvent(ss.str());
      return;
    }
  }
} /* dtmfCmdReceived */


void ModuleTrx::dtmfCmdReceivedWhenIdle(const std::string &cmd)
{
  activateMe();
  dtmfCmdReceived(cmd);
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
  //cout << "### ModuleTrx::squelchOpen: is_open=" << is_open << endl;
  if (isActive())
  {
    rx->setMuteState(is_open ? Rx::MUTE_ALL : Rx::MUTE_NONE);
    rx_timeout_timer.reset();
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
  assert(rx != 0);
  rx->setMuteState(Rx::MUTE_ALL);
} /* ModuleTrx::rxTimeout */


void ModuleTrx::rxSquelchOpen(bool is_open)
{
  //cout << "### ModuleTrx::rxSquelchOpen: is_open=" << is_open << endl;
  if (isActive())
  {
    setIdle(!is_open);

    if (rx_timeout_timer.timeout() > 0)
    {
      rx_timeout_timer.setEnable(is_open);
    }
  }
} /* ModuleTrx::rxSquelchOpen */


/*
 * This file has not been truncated
 */
