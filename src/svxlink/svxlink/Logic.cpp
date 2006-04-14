/**
@file	 Logic.cpp
@brief   The logic core of the SvxLink Server application
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

This is the logic core of the SvxLink Server application. This is where
everything is tied together. It is also the base class for implementing
specific logic core classes (e.g. SimplexLogic and RepeaterLogic).

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
#include <sigc++/bind.h>
#include <sys/time.h>

#include <iostream>
#include <algorithm>
#include <cctype>
#include <cassert>
#include <sstream>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncSampleFifo.h>
#include <Rx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Recorder.h"
#include "EventHandler.h"
#include "Module.h"
#include "MsgHandler.h"
#include "LocalTx.h"
#include "LogicCmds.h"
#include "Logic.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace SigC;



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

AudioSwitchMatrix Logic::audio_switch_matrix;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

void Logic::connectLogics(const string& l1, const string& l2, int timeout)
{
  cout << "Activating link " << l1 << " <--> " << l2 << endl;
  audio_switch_matrix.connect(l1, l2);
  audio_switch_matrix.connect(l2, l1);
} /* Logic::connectLogics */


void Logic::disconnectLogics(const string& l1, const string& l2)
{
  cout << "Deactivating link " << l1 << " <--> " << l2 << endl;
  audio_switch_matrix.disconnect(l1, l2);
  audio_switch_matrix.disconnect(l2, l1);
} /* Logic::connectLogics */


bool Logic::logicsAreConnected(const string& l1, const string& l2)
{
  return audio_switch_matrix.isConnected(l1, l2);
} /* Logic::logicsAreConnected */



Logic::Logic(Config &cfg, const string& name)
  : m_cfg(cfg), m_name(name), m_rx(0), m_tx(0), msg_handler(0),
    write_msg_flush_timer(0), active_module(0), module_tx_fifo(0),
    cmd_tmo_timer(0), logic_transmit(false), anti_flutter(false),
    prev_digit('?'), exec_cmd_on_sql_close(0), exec_cmd_on_sql_close_timer(0),
    rgr_sound_timer(0), rgr_sound_delay(-1), report_ctcss(0), event_handler(0),
    remote_logic_tx(false), every_minute_timer(0), recorder(0),
    tx_ctcss(TX_CTCSS_NEVER)
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
  delete every_minute_timer;
} /* Logic::~Logic */


bool Logic::initialize(void)
{
  string rx_name;
  string tx_name;
  string value;
  string macro_section;
  string event_handler_str;
  list<string> macro_list;
  list<string>::iterator mlit;
  string loaded_modules;
  list<Module*>::const_iterator mit;
  list<string> cfgvars;
  list<string>::const_iterator cfgit;
  
  if (cfg().getValue(name(), "LINKS", value))
  {
    LinkCmd *link_cmd = new LinkCmd(&cmd_parser, this);
    link_cmd->initialize(cfg(), value);
  }
  
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
  
  if (cfg().getValue(name(), "TX_CTCSS", value))
  {
    if (value == "ALWAYS")
    {
      tx_ctcss = TX_CTCSS_ALWAYS;
    }
    else if (value == "SQL_OPEN")
    {
      tx_ctcss = TX_CTCSS_SQL_OPEN;
    }
  }
  
  m_rx = Rx::create(cfg(), rx_name);
  if ((m_rx == 0) || !rx().initialize())
  {
    cerr << "*** ERROR: Could not initialize RX \"" << rx_name << "\"\n";
    goto rx_init_failed;
  }
  rx().squelchOpen.connect(slot(this, &Logic::squelchOpen));
  rx().audioReceived.connect(slot(this, &Logic::audioReceived));
  rx().dtmfDigitDetected.connect(slot(this, &Logic::dtmfDigitDetected));
  rx().mute(false);
    
  m_tx = new LocalTx(cfg(), tx_name);
  if (!tx().initialize())
  {
    cerr << "*** ERROR: Could not initialize TX \"" << tx_name << "\"\n";
    goto tx_init_failed;
  }
  tx().allSamplesFlushed.connect(slot(this, &Logic::allTxSamplesFlushed));
  
  if (tx_ctcss == TX_CTCSS_ALWAYS)
  {
    tx().enableCtcss(true);
  }
  
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
  
  msg_handler = new MsgHandler(8000);
  msg_handler->writeAudio.connect(slot(this, &Logic::transmitAudio));
  msg_handler->allMsgsWritten.connect(slot(this, &Logic::allMsgsWritten));
  tx().transmitBufferFull.connect(
      	  slot(msg_handler, &MsgHandler::writeBufferFull));
  
  event_handler = new EventHandler(event_handler_str, this);
  event_handler->playFile.connect(slot(this, &Logic::playFile));
  event_handler->playSilence.connect(slot(this, &Logic::playSilence));
  event_handler->playTone.connect(slot(this, &Logic::playTone));
  event_handler->recordStart.connect(slot(this, &Logic::recordStart));
  event_handler->recordStop.connect(slot(this, &Logic::recordStop));
  event_handler->deactivateModule.connect(
          bind(slot(this, &Logic::deactivateModule), (Module *)0));
  event_handler->setVariable("mycall", m_callsign);
  char str[256];
  sprintf(str, "%.1f", report_ctcss);
  event_handler->setVariable("report_ctcss", str);
  event_handler->setVariable("active_module", "");
  event_handler->setVariable("is_core_event_handler", "1");

  loadModules();
  
  for (mit=modules.begin(); mit!=modules.end(); ++mit)
  {
    if (!loaded_modules.empty())
    {
      loaded_modules += " ";
    }
    loaded_modules += (*mit)->name();
  }
  event_handler->setVariable("loaded_modules", loaded_modules);

  event_handler->processEvent("namespace eval Logic {}");
  cfgvars = cfg().listSection(name());
  for (cfgit=cfgvars.begin(); cfgit!=cfgvars.end(); ++cfgit)
  {
    string var = "Logic::CFG_" + *cfgit;
    string value;
    cfg().getValue(name(), *cfgit, value);
    event_handler->setVariable(var, value);
  }

  event_handler->initialize();

  audio_switch_matrix.addSource(name(), &logic_con_out);
  logic_con_in.sigWriteSamples.connect(
      	  slot(this, &Logic::remoteLogicWriteSamples));
  logic_con_in.sigFlushSamples.connect(
      	  slot(this, &Logic::remoteLogicFlushSamples));
  audio_switch_matrix.addSink(name(), &logic_con_in);
  
  processEvent("startup");
  
  everyMinute(0);
  
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
    event_handler->processEvent(name() + "::" + event);
  }
  else
  {
    event_handler->processEvent(string(module->name()) + "::" + event);
  }
  msg_handler->end();
}


void Logic::setEventVariable(const string& name, const string& value)
{
  event_handler->setVariable(name, value);
} /* Logic::setEventVariable */


void Logic::playFile(const string& path)
{
  module_tx_fifo->stopOutput(true);
  msg_handler->playFile(path);
  transmitCheck();
} /* Logic::playFile */


void Logic::playSilence(int length)
{
  module_tx_fifo->stopOutput(true);
  msg_handler->playSilence(length);
  transmitCheck();
} /* Logic::playSilence */


void Logic::playTone(int fq, int amp, int len)
{
  module_tx_fifo->stopOutput(true);
  msg_handler->playTone(fq, amp, len);
  transmitCheck();
} /* Logic::playSilence */


void Logic::recordStart(const string& filename)
{
  recordStop();
  recorder = new Recorder(filename);
  if (!recorder->initialize())
  {
    cerr << "*** ERROR: Could not open file for recording: "
      	 << filename << endl;
    recordStop();
    return;
  }
  rx().audioReceived.connect(slot(recorder, &Recorder::writeSamples));
} /* Logic::recordStart */


void Logic::recordStop(void)
{
  delete recorder;
  recorder = 0;
} /* Logic::recordStart */


void Logic::audioFromModule(short *samples, int count)
{
  module_tx_fifo->addSamples(samples, count);
} /* Logic::audioFromModule */


void Logic::moduleTransmitRequest(bool do_transmit)
{
  /*
  cout << "Logic::moduleTransmitRequest: do_transmit="
       << (do_transmit ? "TRUE" : "FALSE") << endl;
  */
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
  if (module == 0)
  {
    module = active_module;
  }
  if ((module != 0) && (module == active_module))
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
  cout << "digit=" << digit << endl;
  
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
    else if (isdigit(digit) || ((digit == '*') && (received_digits != "*")))
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
  
  if (!cmd_queue.empty() && !rx().squelchIsOpen())
  {
    processCommandQueue();
  }
    
} /* Logic::dtmfDigitDetected */


void Logic::disconnectAllLogics(void)
{
  cout << "Deactivating all links to/from \"" << name() << "\"\n";
  audio_switch_matrix.disconnectSource(name());
  audio_switch_matrix.disconnectSink(name());
} /* Logic::disconnectAllLogics */





/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void Logic::squelchOpen(bool is_open)
{
  stringstream ss;
  ss << "squelch_open " << rx().sqlRxId() << " " << (is_open ? "1" : "0");
  processEvent(ss.str());

  if (!is_open)
  {
    logic_con_out.sinkFlushSamples();
    if (((exec_cmd_on_sql_close > 0) || (received_digits == "*")) && 
        !anti_flutter && !received_digits.empty())
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
  
  if (tx_ctcss == TX_CTCSS_SQL_OPEN)
  {
    tx().enableCtcss(is_open);
  }
  
} /* Logic::squelchOpen */


void Logic::transmit(bool do_transmit)
{
  //cout << "Logic::transmit: do_transmit="
    //   << (do_transmit ? "true" : "false") << endl;
  
  bool was_transmitting = tx().isTransmitting();

  tx().transmit(do_transmit);

  if (do_transmit != was_transmitting)
  {
    stringstream ss;
    ss << "transmit " << (do_transmit ? "1" : "0");
    processEvent(ss.str());
  }

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
  //cout << "Logic::allMsgsWritten\n";

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
  //cout << "Logic::allModuleSamplesWritten\n";
  
  tx().flushSamples();
  transmitCheck();  
} /* Logic::allModuleSamplesWritten */


void Logic::transmitCheck(void)
{
  /*
  cout << "Logic::transmitCheck:\n";
  if (active_module != 0)
  {
    cout << "\tactive_module->isTransmitting   = "
      	 << (active_module->isTransmitting() ? "TRUE" : "FALSE") << endl;
  }
  cout << "\tmsg_handler->isWritingMessage() = "
       << (msg_handler->isWritingMessage() ? "TRUE" : "FALSE") << endl;
  cout << "\t!module_tx_fifo->empty()        = "
       << (!module_tx_fifo->empty() ? "TRUE" : "FALSE") << endl;
  cout << "\tlogic_transmit                  = "
       << (logic_transmit ? "TRUE" : "FALSE") << endl;
  cout << "\tremote_logic_tx                 = "
       << (remote_logic_tx ? "TRUE" : "FALSE") << endl;
  cout << "\ttx().isFlushing()               = "
       << (tx().isFlushing() ? "TRUE" : "FALSE") << endl;
  */
   
  if (((active_module != 0) && active_module->isTransmitting()) ||
      msg_handler->isWritingMessage() ||
      !module_tx_fifo->empty() ||
      logic_transmit ||
      remote_logic_tx ||
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
  //cout << "Logic::allTxSamplesFlushed\n";
  transmitCheck();
} /* Logic::allTxSamplesFlushed */


void Logic::remoteLogicTransmitRequest(bool do_tx)
{
  remote_logic_tx = do_tx;
  transmitCheck();
} /* Logic::remoteLogicTx */


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
  cout << "Loading module \"" << module_cfg_name << "\" into logic \""
       << name() << "\"\n";

  string module_path;
  if (!cfg().getValue("GLOBAL", "MODULE_PATH", module_path))
  {
    module_path = "";
  }
  
  string plugin_name = module_cfg_name;
  cfg().getValue(module_cfg_name, "NAME", plugin_name);
  
    // Define the module namespace so that we can set some variables in it
  event_handler->processEvent("namespace eval " + plugin_name + " {}");
  
  cfg().getValue(module_cfg_name, "PLUGIN_NAME", plugin_name);
  
  string plugin_filename;
  if (!module_path.empty())
  {
    plugin_filename = module_path + "/"; 
  }
  plugin_filename +=  "Module" + plugin_name + ".so";
  
  void *handle = dlopen(plugin_filename.c_str(), RTLD_NOW);
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
  
  stringstream ss;
  ss << module->id();
  new ModuleActivateCmd(&cmd_parser, ss.str(), this);
  
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
    else if (!(*it).empty())
    {
      if (!cmd_parser.processCmd(*it))
      {
      	stringstream ss;
	ss << "unknown_command " << *it;
      	processEvent(ss.str());
      }
      
      /*
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
      */
    }
  }
  
  cmd_queue.clear();
  
} /* Logic::processCommandQueue */


void Logic::processMacroCmd(string& cmd)
{
  cout << "Processing macro command: " << cmd << "...\n";
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
  cout << "Macro command found: \"" << macro << "\"\n";

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
  
  if ((received_digits != "*") ||
      (find(cmd_queue.begin(), cmd_queue.end(), "*") == cmd_queue.end()))
  {
    cmd_queue.push_back(received_digits);
  }
  received_digits = "";
  cmd_tmo_timer->setEnable(false);
  prev_digit = '?';
  
  processCommandQueue();
  
} /* Logic::putCmdOnQueue */


void Logic::sendRgrSound(Timer *t)
{
  //cout << "RepeaterLogic::sendRogerSound\n";
  processEvent("send_rgr_sound");
  enableRgrSoundTimer(false);
} /* Logic::sendRogerSound */


int Logic::remoteLogicWriteSamples(const short *samples, int len)
{
  if (msg_handler->isWritingMessage() || rx().squelchIsOpen() ||
      ((active_module != 0) && active_module->isTransmitting()) ||
      (len <= 0))
  {
    return len;
  }
  if (!remote_logic_tx)
  {
    remoteLogicTransmitRequest(true);
  }
  short *buf = new short[len];
  memcpy(buf, samples, len * sizeof(*samples));
  int cnt = transmitAudio(buf, len);
  delete [] buf;
  return cnt;
} /* Logic::remoteLogicWriteSamples */


void Logic::remoteLogicFlushSamples(void)
{
  remoteLogicTransmitRequest(false);
  tx().flushSamples();
} /* Logic::remoteLogicFlushSamples */


int Logic::audioReceived(short *samples, int len)
{
  short *buf = new short[len];
  memcpy(buf, samples, len * sizeof(*samples));
  len = logic_con_out.sinkWriteSamples(buf, len);
  delete [] buf;
  return len;
} /* Logic::audioReceived */


void Logic::everyMinute(Timer *t)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int msec = (59 - tv.tv_sec % 60) * 1000 + (999999 - tv.tv_usec) / 1000 + 1;

  if ((t != 0) && (msec > 1000))
  {
    processEvent("every_minute");
  }
  delete every_minute_timer;
  
  every_minute_timer = new Timer(msec);
  every_minute_timer->expired.connect(slot(this, &Logic::everyMinute));
  
} /* Logic::everyMinute */


/*
 * This file has not been truncated
 */
