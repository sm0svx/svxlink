/**
@file	 ModuleFrn.cpp
@brief   Free Radio Network (FRN) module
@author  sh123
@date	 2014-12-30

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
#include <AsyncAudioFifo.h>
#include <AsyncAudioJitterFifo.h>
#include <AsyncAudioDecimator.h>
#include <AsyncAudioInterpolator.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/
#include <version/MODULE_FRN.h>
#include "ModuleFrn.h"
#include "multirate_filter_coeff.h"


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
 
  qso = new QsoFrn(this);
  qso->error.connect(
      mem_fun(*this, &ModuleFrn::onQsoError));

  // rig/mic -> frn
  audio_valve = new AudioValve;
  audio_splitter = new AudioSplitter;

  AudioSink::setHandler(audio_valve);
  audio_valve->registerSink(audio_splitter);
#if INTERNAL_SAMPLE_RATE == 16000
  AudioDecimator *down_sampler = new AudioDecimator(
      2, coeff_16_8, coeff_16_8_taps);
  audio_splitter->addSink(down_sampler, true);
  down_sampler->registerSink(qso);
#else
  audio_splitter->addSink(qso);
#endif

  // frn -> rig/speaker
  audio_selector = new AudioSelector;
  audio_fifo = new Async::AudioFifo(100 * 320 * 5);

#if INTERNAL_SAMPLE_RATE == 16000
  AudioInterpolator *up_sampler = new AudioInterpolator(
      2, coeff_16_8, coeff_16_8_taps);
  qso->registerSink(up_sampler, true);
  audio_selector->addSource(up_sampler);
  audio_selector->enableAutoSelect(up_sampler, 0);
#else
  audio_selector->addSource(qso);
  audio_selector->enableAutoSelect(qso, 0);
#endif
  audio_fifo->registerSource(audio_selector);
  AudioSource::setHandler(audio_fifo);

  if (!qso->initOk())
  {
    delete qso;
    cerr << "*** ERROR: Creation of Qso object failed\n";
    return false;
  }

  return true;
  
} /* initialize */


void ModuleFrn::moduleCleanup()
{
  AudioSource::clearHandler();
  audio_fifo->unregisterSource();

  audio_splitter->removeSink(qso);
  audio_valve->unregisterSink();
  AudioSink::clearHandler();

  delete qso;
  qso = 0;

  delete audio_fifo;
  audio_fifo = 0;

  delete audio_splitter;
  audio_splitter = 0;

  delete audio_valve;
  audio_valve = 0;

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
    qso->connect();
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
  qso->disconnect();
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
    return;
  }

  stringstream ss;

  switch (cmd[0])
  {
    case CMD_HELP:
      playHelpMsg();
      break;

    case CMD_COUNT_CLIENTS:
    {
      if (!validateCommand(cmd, 1))
        return;
      ss << "count_clients ";
      ss << qso->clientsCount();
      break;
    }

    case CMD_RF_DISABLE:
    {
      if (!validateCommand(cmd, 2))
        return;

      bool disable = (cmd[1] != '0');
      qso->setRfDisabled(disable);
      cout << "rf disable: " << disable << endl;
      ss << "rf_disable " <<  (qso->isRfDisabled() ? "1 " : "0 ")
         << (disable ? "1" : "0");
      break;
    }

    default:
      ss << "unknown_command " << cmd;
      break;
  }

  processEvent(ss.str());

} /* dtmfCmdReceived */


bool ModuleFrn::validateCommand(const string& cmd, size_t argc)
{
  if (cmd.size() == argc)
  {
    return true;
  } 
  else
  {
    stringstream ss;
    ss << "command_failed " << cmd;
    processEvent(ss.str());
    return false;
  }
} /* ModulrFrn::commandFailed */


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
  qso->squelchOpen(is_open);
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
  stringstream ss;
  ss << "count_clients " << qso->clientsCount();
  processEvent(ss.str());
} /* reportState */


void ModuleFrn::onQsoError(void)
{
  cerr << "QSO errored, deactivating module" << endl;
  deactivateMe();
}

/*
 * This file has not been truncated
 */
