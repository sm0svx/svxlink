/**
@file	 ModuleParrot.cpp
@brief   A module that implements a "parrot" function.
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This module implements a "parrot" function. It plays back everything you say
to it. This can be used as a simplex repeater or just so you can hear how
you sound.

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

#include <version/SVXLINK.h>
#include <AsyncConfig.h>
#include <AsyncSampleFifo.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleParrot.h"



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
    return new ModuleParrot(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleParrot::ModuleParrot(void *dl_handle, Logic *logic,
      	      	      	   const string& cfg_name)
  : Module(dl_handle, logic, cfg_name), fifo(0), squelch_is_open(false),
    pacer(8000, 800, 1000), repeat_delay(0), repeat_delay_timer(0)
{
  cout << "\tModule " << name()
       << " v" SVXLINK_VERSION " starting...\n";
  
} /* ModuleParrot */


ModuleParrot::~ModuleParrot(void)
{
  delete fifo;
} /* ~ModuleParrot */


bool ModuleParrot::initialize(void)
{
  if (!Module::initialize())
  {
    return false;
  }
  
  string fifo_len;
  if (!cfg().getValue(cfgName(), "FIFO_LEN", fifo_len))
  {
    cerr << "*** Error: Config variable " << cfgName() << "/FIFO_LEN not set\n";
    return false;
  }
  string value;
  if (cfg().getValue(cfgName(), "REPEAT_DELAY", value))
  {
    repeat_delay = atoi(value.c_str());
  }
  
  fifo = new SampleFifo(atoi(fifo_len.c_str())*8000);
  fifo->setDebugName("parrot_fifo");
  fifo->stopOutput(true);
  fifo->setOverwrite(true);
  fifo->writeSamples.connect(slot(&pacer, &AudioPacer::audioInput));
  fifo->allSamplesWritten.connect(slot(&pacer, &AudioPacer::flushAllAudio));
  
  pacer.audioInputBufFull.connect(slot(fifo, &SampleFifo::writeBufferFull));
  pacer.audioOutput.connect(slot(this, &ModuleParrot::audioFromFifo));
  pacer.allAudioFlushed.connect(slot(this, &ModuleParrot::allSamplesWritten));
  
  return true;
  
} /* ModuleParrot::initialize */






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
void ModuleParrot::activateInit(void)
{
  fifo->clear();
  cmd_queue.clear();
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
void ModuleParrot::deactivateCleanup(void)
{
  fifo->clear();
  delete repeat_delay_timer;
  repeat_delay_timer = 0;
} /* deactivateCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received.
 * Input:     digit - The DTMF digit received (0-9, A-D, *, #)
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleParrot::dtmfDigitReceived(char digit)
{
  //printf("DTMF digit received in module %s: %c\n", name(), digit);
  
} /* dtmfDigitReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfCmdReceived
 * Purpose:   Called by the core system when a DTMF command has been
 *    	      received. A DTMF command consists of a string of digits ended
 *    	      with a number sign (#). The number sign is not included in the
 *    	      command string.
 * Input:     cmd - The received command.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void ModuleParrot::dtmfCmdReceived(const string& cmd)
{
  printf("DTMF command received in module %s: %s\n", name(), cmd.c_str());
  
  cmd_queue.push_back(cmd);
  if (fifo->empty() && !squelch_is_open)
  {
    execCmdQueue();
  }
} /* dtmfCmdReceived */


void ModuleParrot::squelchOpen(bool is_open)
{
  squelch_is_open = is_open;
  
  if (is_open)
  {
    setIdle(false);
    fifo->stopOutput(true);
    delete repeat_delay_timer;
    repeat_delay_timer = 0;
  }
  else
  {
    if (!fifo->empty())
    {
      if (repeat_delay > 0)
      {
      	repeat_delay_timer = new Timer(repeat_delay);
	repeat_delay_timer->expired.connect(
	    slot(this, &ModuleParrot::onRepeatDelayExpired));
      }
      else
      {
      	onRepeatDelayExpired(0);
      }
    }
    else if (!cmd_queue.empty())
    {
      execCmdQueue();
    }
    else
    {
      setIdle(true);
    }
  }

} /* ModuleParrot::squelchOpen */


int ModuleParrot::audioFromRx(short *samples, int count)
{
  if (squelch_is_open)
  {
    //printf("Adding samples to FIFO...\n");
    fifo->addSamples(samples, count);
  }
  
  return count;
  
} /* ModuleParrot::audioFromRx */


void ModuleParrot::allMsgsWritten(void)
{
  //printf("ModuleParrot::allMsgsWritten\n");
  if (fifo->empty())
  {
    setIdle(true);
  }
} /* ModuleParrot::allMsgsWritten */




int ModuleParrot::audioFromFifo(short *samples, int count)
{
  //printf("Writing %d samples from FIFO...\n", count);
  audioFromModule(const_cast<short *>(samples), count);
  return count;
} /* ModuleParrot::audioFromFifo */


void ModuleParrot::allSamplesWritten(void)
{
  //cout << "ModuleParrot::allSamplesWritten\n";
  
  if (!cmd_queue.empty())
  {
    execCmdQueue();
  }
  else
  {
    setIdle(true);
  }
  transmit(false);
  fifo->stopOutput(true);
} /* ModuleParrot::allSamplesWritten */


void ModuleParrot::onRepeatDelayExpired(Timer *t)
{
  delete repeat_delay_timer;
  repeat_delay_timer = 0;
  
  transmit(true);
  fifo->flushSamples();
  fifo->stopOutput(false);
  
} /* ModuleParrot::onRepeatDelayExpired */


void ModuleParrot::execCmdQueue(void)
{
  //printf("ModuleParrot::execCmdQueue\n");
  
  list<string> cq = cmd_queue;
  cmd_queue.clear();
  
  list<string>::iterator it;
  for (it=cq.begin(); it!=cq.end(); ++it)
  {
    string cmd(*it);
    
    if (cmd == "")
    {
      deactivateMe();
    }
    else
    {
      if (cmd == "0")
      {
	playHelpMsg();
      }
      else
      {
      	stringstream ss;
	ss << "spell_digits " << cmd;
      	processEvent(ss.str());
      }
    }
  }
} /* ModuleParrot::execCmdQueue */




/*
 * This file has not been truncated
 */
