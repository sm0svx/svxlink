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
#include <cctype>
#include <cassert>
#include <sstream>


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

#include "EventHandler.h"
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
    write_msg_flush_timer(0), active_module(0), module_tx_fifo(0),
    cmd_tmo_timer(0), logic_transmit(false), anti_flutter(false),
    prev_digit('?'), exec_cmd_on_sql_close(0), exec_cmd_on_sql_close_timer(0),
    rgr_sound_timer(0), rgr_sound_delay(-1), report_ctcss(0), event_handler(0)
{

} /* Logic::Logic */


Logic::~Logic(void)
{
  unloadModules();

  delete cmd_tmo_timer;
  delete exec_cmd_on_sql_close_timer;
  delete module_tx_fifo;
  delete write_msg_flush_timer;
  delete msg_handler;
  delete m_tx;
  delete m_rx;
  delete rgr_sound_timer;
  delete event_handler;
} /* Logic::~Logic */


bool Logic::initialize(void)
{
  string rx_name;
  string tx_name;
  string sounds;
  string value;
  string macro_section;
  string event_handler_str;
  list<string> macro_list;
  list<string>::iterator mlit;
  
  if (!cfg().getValue(name(), "EVENT_HANDLER", event_handler_str))
  {
    cerr << "*** ERROR: Config variable " << name()
      	 << "/EVENT_HANDLER not set\n";
    goto cfg_failed;
  }

  if (!cfg().getValue(name(), "RX", rx_name))
  {
    cerr << "*** ERROR: Config variable " << name() << "/RX not set\n";
    goto cfg_failed;
  }
  
  if (!cfg().getValue(name(), "TX", tx_name))
  {
    cerr << "*** ERROR: Config variable " << name() << "/TX not set\n";
    goto cfg_failed;
  }
  
  if (!cfg().getValue(name(), "SOUNDS", sounds))
  {
    cerr << "*** ERROR: Config variable " << name() << "/SOUNDS not set\n";
    goto cfg_failed;
  }
  
  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: Config variable " << name() << "/CALLSIGN not set\n";
    goto cfg_failed;
  }
  
  if (cfg().getValue(name(), "EXEC_CMD_ON_SQL_CLOSE", value))
  {
    exec_cmd_on_sql_close = atoi(value.c_str());
  }
  
  if (cfg().getValue(name(), "RGR_SOUND_DELAY", value))
  {
    rgr_sound_delay = atoi(value.c_str());
  }
  
  if (cfg().getValue(name(), "REPORT_CTCSS", value))
  {
    report_ctcss = atof(value.c_str());
  }
  
  if (cfg().getValue(name(), "MACROS", macro_section))
  {
    macro_list = cfg().listSection(macro_section);
    for (mlit=macro_list.begin(); mlit!=macro_list.end(); ++mlit)
    {
      cfg().getValue(macro_section, *mlit, value);
      macros[atoi(mlit->c_str())] = value;
    }
  }
  
  loadModules();
  
  m_rx = new LocalRx(cfg(), rx_name);
  if (!rx().initialize())
  {
    cerr << "*** ERROR: Could not initialize RX \"" << rx_name << "\"\n";
    goto rx_init_failed;
  }
  rx().squelchOpen.connect(slot(this, &Logic::squelchOpen));
  //rx().audioReceived.connect(slot(this, &Logic::audioReceived));
  rx().dtmfDigitDetected.connect(slot(this, &Logic::dtmfDigitDetected));
  
  m_tx = new LocalTx(cfg(), tx_name);
  if (!tx().initialize())
  {
    cerr << "*** ERROR: Could not initialize TX \"" << tx_name << "\"\n";
    goto tx_init_failed;
  }
  tx().allSamplesFlushed.connect(slot(this, &Logic::allTxSamplesFlushed));
  
  module_tx_fifo = new SampleFifo(15*8000); // 15 seconds
  module_tx_fifo->setDebugName("module_tx_fifo");
  module_tx_fifo->allSamplesWritten.connect(
      	  slot(this, &Logic::allModuleSamplesWritten));
  module_tx_fifo->writeSamples.connect(slot(this, &Logic::transmitAudio));
  tx().transmitBufferFull.connect(
      	  slot(module_tx_fifo, &SampleFifo::writeBufferFull));
  module_tx_fifo->stopOutput(true);
  module_tx_fifo->setOverwrite(true);
  
  cmd_tmo_timer = new Timer(10000);
  cmd_tmo_timer->expired.connect(slot(this, &Logic::cmdTimeout));
  cmd_tmo_timer->setEnable(false);
  
  msg_handler = new MsgHandler(sounds, 8000);
  msg_handler->writeAudio.connect(slot(this, &Logic::transmitAudio));
  msg_handler->allMsgsWritten.connect(slot(this, &Logic::allMsgsWritten));
  tx().transmitBufferFull.connect(
      	  slot(msg_handler, &MsgHandler::writeBufferFull));
  
  event_handler = new EventHandler(event_handler_str, this);
  event_handler->setVariable("mycall", m_callsign);
  char str[256];
  sprintf(str, "%.1f", report_ctcss);
  event_handler->setVariable("report_ctcss", str);
  event_handler->setVariable("active_module", "");
  event_handler->setVariable("script_path", event_handler_str);
  event_handler->initialize();
  
  processEvent("startup");
  
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


void Logic::processEvent(const string& event, const Module *module)
{
  msg_handler->begin();
  if (module == 0)
  {
    event_handler->processEvent(name() + "_" + event);
  }
  else
  {
    event_handler->processEvent(string(module->name()) + "_" + event);
  }
  msg_handler->end();
}


void Logic::playFile(const string& path)
{
  module_tx_fifo->stopOutput(true);
  msg_handler->playFile(path);
  transmit(true);  
} /* Logic::playFile */


void Logic::playMsg(const string& msg, const Module *module)
{
  module_tx_fifo->stopOutput(true);
  if (module == 0)
  {
    msg_handler->playMsg("Core", msg);
  }
  else
  {
    msg_handler->playMsg(module->name(), msg);
  }
  transmit(true);  
} /* Logic::playMsg */


void Logic::playNumber(float number)
{
  module_tx_fifo->stopOutput(true);
  msg_handler->playNumber(number);
  transmit(true);
} /* Logic::playNumber */


void Logic::spellWord(const string& word)
{
  module_tx_fifo->stopOutput(true);
  msg_handler->spellWord(word);
  transmit(true);
} /* Logic::spellWord */


void Logic::playSilence(int length)
{
  module_tx_fifo->stopOutput(true);
  msg_handler->playSilence(length);
  transmit(true);
} /* Logic::playSilence */


void Logic::audioFromModule(short *samples, int count)
{
  module_tx_fifo->addSamples(samples, count);
} /* Logic::audioFromModule */


void Logic::moduleTransmitRequest(bool do_transmit)
{
  //printf("Logic::moduleTransmitRequest: do_transmit=%s\n",
  //    	  do_transmit ? "TRUE" : "FALSE");
  /*
  if (!do_transmit && tx().isTransmitting())
  {
    tx().flushSamples();
  }
  */
  //tx().flushSamples();
  if (!do_transmit)
  {
    if (!module_tx_fifo->empty())
    {
      module_tx_fifo->flushSamples();
    }
    else
    {
      tx().flushSamples();
    }
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
    event_handler->setVariable("active_module", module->name());
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
    event_handler->setVariable("active_module", "");
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


Module *Logic::findModule(const string& name)
{
  list<Module *>::iterator it;
  for (it=modules.begin(); it!=modules.end(); ++it)
  {
    if ((*it)->name() == name)
    {
      return (*it);
    }
  }
  
  return 0;
  
} /* Logic::findModule */


void Logic::dtmfDigitDetected(char digit)
{
  printf("digit=%c\n", digit);
  
  if (digit == '*')
  {
      // Ignore multiple "identify" commands
    if (find(cmd_queue.begin(), cmd_queue.end(), "*") == cmd_queue.end())
    {
      cmd_queue.push_back("*");
    }
  }
  else
  {
    if (active_module != 0)
    {
      active_module->dtmfDigitReceived(digit);
    }
    if ((digit == '#') || (anti_flutter && (digit == 'C')))
    {
      putCmdOnQueue();
      anti_flutter = false;
    }
    else if (digit == 'A')
    {
      anti_flutter = true;
      prev_digit = '?';
    }
    else if (digit == 'D')
    {
      received_digits = "D";
      prev_digit = '?';
    }
    else if (received_digits.size() < 10)
    {
      cmd_tmo_timer->reset();
      cmd_tmo_timer->setEnable(true);
      if (digit == 'B')
      {
      	if (anti_flutter && (prev_digit != '?'))
	{
	  received_digits += prev_digit;
	  prev_digit = '?';
	}
      }
      else if (isdigit(digit))
      {
      	if (anti_flutter)
	{
      	  if (digit != prev_digit)
	  {
      	    received_digits += digit;
	    prev_digit = digit;
	  }
	}
	else
	{
      	  received_digits += digit;
	}
      }
    }
  }
  
  if (!cmd_queue.empty() && !rx().squelchIsOpen())
  {
    processCommandQueue();
  }
    
} /* Logic::dtmfDigitDetected */





/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void Logic::squelchOpen(bool is_open)
{
  if (!is_open)
  {
    if ((exec_cmd_on_sql_close > 0) && !anti_flutter &&
      	!received_digits.empty())
    {
      exec_cmd_on_sql_close_timer = new Timer(exec_cmd_on_sql_close);
      exec_cmd_on_sql_close_timer->expired.connect(
	  slot(this, &Logic::putCmdOnQueue));
    }
    processCommandQueue();
  }
  else
  {
    delete exec_cmd_on_sql_close_timer;
    exec_cmd_on_sql_close_timer = 0;
  }
  
} /* Logic::squelchOpen */


void Logic::transmit(bool do_transmit)
{
  //printf("Logic::transmit: do_transmit=%s\n", do_transmit ? "true" : "false");
  
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


void Logic::logicTransmitRequest(bool do_transmit)
{
  logic_transmit = do_transmit;
  transmitCheck();
} /* Logic::logicTransmitRequest */


void Logic::enableRgrSoundTimer(bool enable)
{
  if (rgr_sound_delay == -1)
  {
    return;
  }
  
  delete rgr_sound_timer;
  rgr_sound_timer = 0;

  if (enable)
  {
    if (rgr_sound_delay > 0)
    {
      rgr_sound_timer = new Timer(rgr_sound_delay);
      rgr_sound_timer->expired.connect(slot(this, &Logic::sendRgrSound));
    }
    else
    {
      sendRgrSound();
    }
  }  
} /* Logic::enableRgrSoundTimer */





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
  //printf("Logic::allMsgsWritten\n");

  tx().flushSamples();
  transmitCheck();
  //module_tx_fifo->stopOutput(false);
  
  if (active_module != 0)
  {
     active_module->allMsgsWritten();
  }
  
} /* Logic::allMsgsWritten */


void Logic::allModuleSamplesWritten(void)
{
  //printf("Logic::allModuleSamplesWritten\n");
  
  tx().flushSamples();
  transmitCheck();  
} /* Logic::allModuleSamplesWritten */


void Logic::transmitCheck(void)
{
  //printf("Logic::transmitCheck:\n");
  /*
  if (active_module != 0)
  {
    printf("\tactive_module->isTransmitting   = %s\n",
      active_module->isTransmitting() ? "TRUE" : "FALSE");
  }
  printf("\tmsg_handler->isWritingMessage() = %s\n",
      msg_handler->isWritingMessage() ? "TRUE" : "FALSE");
  printf("\tmodule_tx_fifo->empty()         = %s\n",
      module_tx_fifo->empty() ? "TRUE" : "FALSE");
  printf("\tlogic_transmit                  = %s\n",
      logic_transmit ? "TRUE" : "FALSE");
  printf("\ttx().isFlushing()               = %s\n",
      tx().isFlushing() ? "TRUE" : "FALSE");
  */
    
  if (((active_module != 0) && active_module->isTransmitting()) ||
      msg_handler->isWritingMessage() ||
      !module_tx_fifo->empty() ||
      logic_transmit ||
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
  //printf("Logic::allTxSamplesFlushed\n");
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
  cout << "Loading module \"" << module_cfg_name << "\"\n";

  string module_path;
  if (!cfg().getValue("GLOBAL", "MODULE_PATH", module_path))
  {
    module_path = "";
  }
  
  string module_name;
  if (!cfg().getValue(module_cfg_name, "NAME", module_name))
  {
    module_name = module_cfg_name;
  }
  
  string module_filename;
  if (!module_path.empty())
  {
    module_filename = module_path + "/"; 
  }
  module_filename +=  "Module" + module_name + ".so";
  
  void *handle = dlopen(module_filename.c_str(), RTLD_NOW);
  if (handle == NULL)
  {
    cerr << "*** ERROR: Failed to load module "
      	 << module_cfg_name.c_str() << ": " << dlerror() << endl;
    return;
  }
  Module::InitFunc init = (Module::InitFunc)dlsym(handle, "module_init");
  if (init == NULL)
  {
    cerr << "*** ERROR: Could not find init func for module "
      	 << module_cfg_name.c_str() << ": " << dlerror() << endl;
    dlclose(handle);
    return;
  }
  
  Module *module = init(handle, this, module_cfg_name.c_str());
  if (module == 0)
  {
    cerr << "*** ERROR: Creation failed for module "
      	 << module_cfg_name.c_str() << endl;
    dlclose(handle);
    return;
  }
  
  if (!module->initialize())
  {
    cerr << "*** ERROR: Initialization failed for module "
      	 << module_cfg_name.c_str() << endl;
    delete module;
    dlclose(handle);
    return;
  }
  
  modules.push_back(module);
  
} /* Logic::loadModule */


void Logic::unloadModules(void)
{
  list<Module *>::iterator it;
  for (it=modules.begin(); it!=modules.end(); ++it)
  {
    void *plugin_handle = (*it)->pluginHandle();
    delete *it;
    dlclose(plugin_handle);
  }
  modules.clear();
} /* logic::unloadModules */


void Logic::cmdTimeout(Timer *t)
{
  received_digits = "";
  anti_flutter = false;
  prev_digit = '?';
} /* Logic::cmdTimeout */


void Logic::processCommandQueue(void)
{
  if (rx().squelchIsOpen() || cmd_queue.empty())
  {
    return;
  }
  
  list<string>::iterator it;
  for (it=cmd_queue.begin(); it!=cmd_queue.end(); ++it)
  {
    if (*it == "*")
    {
      processEvent("manual_identification");
    }
    else if ((*it)[0] == 'D')
    {
      processMacroCmd(*it);
    }
    else if (active_module != 0)
    {
      active_module->dtmfCmdReceived(*it);
    }
    else
    {
      if (!(*it).empty())
      {
	int module_id = atoi((*it).c_str());
	Module *module = findModule(module_id);
	if (module != 0)
	{
	  activateModule(module);
	}
	else
	{
	  stringstream ss;
	  ss << "no_such_module " << module_id;
	  processEvent(ss.str());
	}
      }
    }
  }
  
  cmd_queue.clear();
  
} /* Logic::processCommandQueue */


void Logic::processMacroCmd(string& cmd)
{
  printf("Processing macro command: %s...\n", cmd.c_str());
  assert(!cmd.empty() && (cmd[0] == 'D'));
  cmd.erase(0, 1);
  if (cmd.empty())
  {
    cerr << "*** Macro error: Empty command.\n";
    processEvent("macro_empty");
    return;
  }
  
  map<int, string>::iterator it = macros.find(atoi(cmd.c_str()));
  if (it == macros.end())
  {
    cerr << "*** Macro error: Macro " << cmd << " not found.\n";
    processEvent("macro_not_found");
    return;
  }

  string macro(it->second);
  string::iterator colon = find(macro.begin(), macro.end(), ':');
  if (colon == macro.end())
  {
    cerr << "*** Macro error: No colon found in macro (" << macro << ").\n";
    processEvent("macro_syntax_error");
    return;
  }
  
  string module_name(macro.begin(), colon);
  string module_cmd(colon+1, macro.end());
  Module *module = findModule(module_name);
  if (module == 0)
  {
    cerr << "*** Macro error: Module " << module_name << " not found.\n";
    processEvent("macro_module_not_found");
    return;
  }
  
  if (active_module == 0)
  {
    if (!activateModule(module))
    {
      cerr << "*** Macro error: Activation of module " << module_name
           << " failed.\n";
      processEvent("macro_module_activation_failed");
      return;
    }
  }
  else if (active_module != module)
  {
    cerr << "*** Macro error: Another module is active ("
         << active_module->name() << ").\n";
    processEvent("macro_another_active_module");
    return;
  }
  
  module->dtmfCmdReceived(module_cmd);

} /* Logic::processMacroCmd */


void Logic::putCmdOnQueue(Timer *t)
{
  delete exec_cmd_on_sql_close_timer;
  exec_cmd_on_sql_close_timer = 0;
  
  cmd_queue.push_back(received_digits);
  received_digits = "";
  cmd_tmo_timer->setEnable(false);
  prev_digit = '?';
  
  processCommandQueue();
  
} /* Logic::putCmdOnQueue */


void Logic::sendRgrSound(Timer *t)
{
  //printf("RepeaterLogic::sendRogerSound\n");
  processEvent("send_rgr_sound");
  enableRgrSoundTimer(false);
} /* Logic::sendRogerSound */



/*
 * This file has not been truncated
 */
