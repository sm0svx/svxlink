/**
@file	 Logic.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

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

#include <dlfcn.h>

#include <iostream>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncSampleFifo.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Module.h"
#include "MsgHandler.h"
#include "LocalRx.h"
#include "LocalTx.h"
#include "Logic.h"



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

Logic::Logic(Config &cfg, const string& name)
  : m_cfg(cfg), m_name(name), m_rx(0), m_tx(0), msg_handler(0),
    write_msg_flush_timer(0), active_module(0), module_tx_fifo(0)
{

} /* Logic::Logic */


Logic::~Logic(void)
{
  delete module_tx_fifo;
  delete write_msg_flush_timer;
  delete msg_handler;
  delete m_tx;
  delete m_rx;
} /* Logic::~Logic */


bool Logic::initialize(void)
{
  string rx_name;
  string tx_name;
  string sounds;
  
  if (!cfg().getValue(name(), "RX", rx_name))
  {
    cerr << "*** Error: Config variable " << name() << "/RX not set\n";
    goto cfg_failed;
  }
  
  if (!cfg().getValue(name(), "TX", tx_name))
  {
    cerr << "*** Error: Config variable " << name() << "/TX not set\n";
    goto cfg_failed;
  }
  
  if (!cfg().getValue(name(), "SOUNDS", sounds))
  {
    cerr << "*** Error: Config variable " << name() << "/SOUNDS not set\n";
    goto cfg_failed;
  }
  
  loadModules();
  
  m_rx = new LocalRx(cfg(), rx_name);
  if (!rx().initialize())
  {
    cerr << "*** Error: Could not initialize RX \"" << rx_name << "\"\n";
    goto rx_init_failed;
  }
  rx().squelchOpen.connect(slot(this, &Logic::squelchOpen));
  //rx().audioReceived.connect(slot(this, &Logic::audioReceived));
  rx().dtmfDigitDetected.connect(slot(this, &Logic::dtmfDigitDetected));
  
  m_tx = new LocalTx(cfg(), tx_name);
  if (!tx().initialize())
  {
    cerr << "*** Error: Could not initialize TX \"" << tx_name << "\"\n";
    goto tx_init_failed;
  }
  tx().allSamplesFlushed.connect(slot(this, &Logic::allTxSamplesFlushed));
  
  msg_handler = new MsgHandler(sounds);
  msg_handler->writeAudio.connect(slot(this, &Logic::transmitAudio));
  msg_handler->allMsgsWritten.connect(slot(this, &Logic::allMsgsWritten));
  tx().transmitBufferFull.connect(
      	  slot(msg_handler, &MsgHandler::writeBufferFull));
  
  module_tx_fifo = new SampleFifo(10*8000); // 10 seconds
  module_tx_fifo->allSamplesWritten.connect(
      	  slot(this, &Logic::allModuleSamplesWritten));
  module_tx_fifo->writeSamples.connect(slot(this, &Logic::transmitAudio));
  tx().transmitBufferFull.connect(
      	  slot(module_tx_fifo, &SampleFifo::writeBufferFull));
  module_tx_fifo->stopOutput(true);
  
  return true;
  
  tx_init_failed:
    delete m_tx;
    m_tx = 0;
    
  rx_init_failed:
    delete m_rx;
    m_rx = 0;
    
  cfg_failed:
    return false;
    
} /* Logic::initialize */


void Logic::playMsg(const string& msg, const Module *module)
{
  //module_tx_fifo->stopOutput(true);
  transmit(true);
  if (module == 0)
  {
    msg_handler->playMsg("Core", msg);
  }
  else
  {
    msg_handler->playMsg(module->name(), msg);
  }
  
} /* Logic::playMsg */


void Logic::playNumber(int number)
{
  //module_tx_fifo->stopOutput(true);
  transmit(true);
  msg_handler->playNumber(number);
} /* Logic::playNumber */


void Logic::spellWord(const string& word)
{
  module_tx_fifo->stopOutput(true);
  transmit(true);
  msg_handler->spellWord(word);
} /* Logic::spellWord */


void Logic::audioFromModule(short *samples, int count)
{
  module_tx_fifo->addSamples(samples, count);
} /* Logic::audioFromModule */


void Logic::moduleTransmitRequest(bool do_transmit)
{
  printf("Logic::moduleTransmitRequest\n");
  if (!do_transmit && tx().isTransmitting())
  {
    tx().flushSamples();
  }
  transmitCheck();
} /* Logic::moduleTransmitRequest */


bool Logic::activateModule(Module *module)
{
  if (active_module == module)
  {
    return true;
  }
  
  if (active_module == 0)
  {
    active_module = module;
    module->activate();
    return true;
  }
  
  return false;
  
} /* Logic::activateModule */


void Logic::deactivateModule(Module *module)
{
  if (module == active_module)
  {
    active_module = 0;
    module->deactivate();
  }
} /* Logic::deactivateModule */


Module *Logic::findModule(int id)
{
  list<Module *>::iterator it;
  for (it=modules.begin(); it!=modules.end(); ++it)
  {
    if ((*it)->id() == id)
    {
      return (*it);
    }
  }
  
  return 0;
  
} /* Logic::findModule */


void Logic::dtmfDigitDetected(char digit)
{
  printf("digit=%c\n", digit);
  
  if (active_module != 0)
  {
    active_module->dtmfDigitReceived(digit);
    if (digit == '#')
    {
      active_module->dtmfCmdReceived(received_digits);
      received_digits = "";
    }
    else
    {
      received_digits += digit;
    }
  }
  else
  {
    if (digit == '#')
    {
      if (received_digits.empty())
      {

      }
      else
      {
	int module_id = atoi(received_digits.c_str());
	received_digits = "";
	Module *module = findModule(module_id);
	if (module != 0)
	{
	  activateModule(module);
	}
	else
	{
	  playNumber(module_id);
	  playMsg("no_such_module");
	}
      }
    }
    else if (isdigit(digit))
    {
      received_digits += digit;
    }
  }  
} /* Logic::dtmfDigitDetected */





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
void Logic::transmit(bool do_transmit)
{
  printf("Logic::transmit: do_transmit=%s\n", do_transmit ? "true" : "false");
  
  tx().transmit(do_transmit);
  if (do_transmit)
  {
    if (!msg_handler->isWritingMessage())
    {
      module_tx_fifo->stopOutput(false);
    }
  }
  else
  {
    module_tx_fifo->stopOutput(true);
  }
} /* Logic::transmit */


int Logic::transmitAudio(short *samples, int count)
{
  return tx().transmitAudio(samples, count);
} /* Logic::transmitAudio */


void Logic::clearPendingSamples(void)
{
  msg_handler->clear();
  module_tx_fifo->clear();
} /* Logic::clearPendingSamples */




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
void Logic::allMsgsWritten(void)
{
  printf("Logic::allMsgsWritten\n");

  tx().flushSamples();
  transmitCheck();  
} /* Logic::allMsgsWritten */


void Logic::allModuleSamplesWritten(void)
{
  printf("Logic::allModuleSamplesWritten\n");
  
  tx().flushSamples();
  transmitCheck();  
} /* Logic::allModuleSamplesWritten */


void Logic::transmitCheck(void)
{
  printf("Logic::transmitCheck\n");
  
  if (((active_module != 0) && active_module->isTransmitting()) ||
      tx().isFlushing())
  {
    transmit(true);
  }
  else
  {
    transmit(false);
  }
} /* Logic::transmitCheck */


void Logic::allTxSamplesFlushed(void)
{
  printf("Logic::allTxSamplesFlushed\n");
  transmitCheck();
} /* Logic::allTxSamplesFlushed */


void Logic::loadModules(void)
{
  string modules;
  cfg().getValue(name(), "MODULES", modules);
  if (modules.empty())
  {
    return;
  }
  
  string::iterator comma;
  string::iterator begin = modules.begin();
  do
  {
    comma = find(begin, modules.end(), ',');
    if (comma == modules.end())
    {
      string module_name(begin, modules.end());
      loadModule(module_name);
    }
    else
    {
      string module_name(begin, comma);
      loadModule(module_name);
      begin = comma + 1;
    }
  } while (comma != modules.end());
} /* Logic::loadModules */


void Logic::loadModule(const string& module_cfg_name)
{
  printf("Loading module \"%s\"\n", module_cfg_name.c_str());

  string module_name;  
  if (!cfg().getValue(module_cfg_name, "NAME", module_name))
  {
    module_name = module_cfg_name;
  }
  
  string module_id_str;
  if (!cfg().getValue(module_cfg_name, "ID", module_id_str))
  {
    cerr << "*** Error: Config variable " << module_cfg_name
      	 << "/ID not set\n";
    return;
  }
  int id = atoi(module_id_str.c_str());
  
  string module_filename("Module" + module_name + ".so");
  void *handle = dlopen(module_filename.c_str(), RTLD_NOW);
  if (handle == NULL)
  {
    cerr << "*** Error: Failed to load module "
      	 << module_cfg_name.c_str() << ": " << dlerror() << endl;
    return;
  }
  Module::InitFunc init = (Module::InitFunc)dlsym(handle, "module_init");
  if (init == NULL)
  {
    cerr << "*** Error: Could not find init func for module "
      	 << module_cfg_name.c_str() << ": " << dlerror() << endl;
    dlclose(handle);
    return;
  }
  Module *module = init(handle, this, id, module_cfg_name.c_str());
  if (module == 0)
  {
    cerr << "*** Error: Initialization failed for module "
      	 << module_cfg_name.c_str() << endl;
    dlclose(handle);
    return;
  }
  modules.push_back(module);
  
} /* Logic::loadModule */


/*
 * This file has not been truncated
 */
