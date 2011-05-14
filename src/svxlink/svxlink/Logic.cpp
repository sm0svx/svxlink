/**
@file	 Logic.cpp
@brief   The logic core of the SvxLink Server application
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

This is the logic core of the SvxLink Server application. This is where
everything is tied together. It is also the base class for implementing
specific logic core classes (e.g. SimplexLogic and RepeaterLogic).

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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
#include <list>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <Rx.h>
#include <Tx.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioMixer.h>
#include <AsyncAudioAmp.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioStreamStateDetector.h>
#include <AsyncAudioPacer.h>
#include <AsyncAudioDebugger.h>
#include <AsyncAudioRecorder.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "EventHandler.h"
#include "Module.h"
#include "MsgHandler.h"
#include "LogicCmds.h"
#include "Logic.h"
#include "QsoRecorder.h"


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

bool Logic::connectLogics(const vector<string> &link_list, int timeout)
{
  assert(link_list.size() > 0);
  bool check_connect = false;
  vector<string>::const_iterator it;
  for (it = link_list.begin(); it != link_list.end(); it++)
  {
    for (vector<string>::const_iterator it1 = it + 1; it1 != link_list.end();
	 it1++)
    {
      assert(*it != *it1);
      if (!logicsAreConnected(*it, *it1))
      {
	cout << "Activating link " << *it << " --> " << *it1 << endl;
	audio_switch_matrix.connect(*it, *it1);
	audio_switch_matrix.connect(*it1, *it);
	check_connect = true;
      }
    }
  }
  return check_connect;
} /* Logic::connectLogics */


bool Logic::disconnectLogics(const std::vector<std::string> &link_list)
{
  assert(link_list.size() > 0);
  bool check_connect = false;
  for (vector<string>::const_iterator it = link_list.begin();
       it != link_list.end(); it++)
  {
    for (vector<string>::const_iterator it1 = it + 1; it1 != link_list.end();
	 it1++)
    {
      if (logicsAreConnected(*it, *it1))
      {
	cout << "Deactivating link " << *it << " --> " << *it1 << endl;
	audio_switch_matrix.disconnect(*it, *it1);
	check_connect = true;
      }
    }
  }
  return check_connect;
} /* Logic::disconnectLogics */


bool Logic::logicsAreConnected(const string& l1, const string& l2)
{
  return audio_switch_matrix.isConnected(l1, l2);
} /* Logic::logicsAreConnected */


Logic::Logic(Config &cfg, const string& name)
  : m_cfg(cfg),       	      	    m_name(name),
    m_rx(0),  	      	      	    m_tx(0),
    msg_handler(0), 	      	    active_module(0),
    cmd_tmo_timer(0), 	      	    anti_flutter(false),
    prev_digit('?'),                exec_cmd_on_sql_close(0),
    exec_cmd_on_sql_close_timer(0), rgr_sound_timer(0),
    rgr_sound_delay(-1),            report_ctcss(0),
    event_handler(0),               every_minute_timer(0),
    recorder(0), 		    tx_audio_mixer(0),
    tx_audio_selector(0),     	    rx_splitter(0),
    audio_from_module_selector(0),  audio_to_module_splitter(0),
    audio_to_module_selector(0),    state_det(0),
    is_idle(true),                  fx_gain_normal(0),
    fx_gain_low(-12), 	      	    long_cmd_digits(100),
    report_events_as_idle(false),   qso_recorder(0),
    tx_ctcss(TX_CTCSS_ALWAYS), 	    tx_ctcss_mask(0)
{
  logic_con_in = new AudioSplitter;
  logic_con_out = new AudioSelector;
} /* Logic::Logic */


Logic::~Logic(void)
{
  cleanup();
  delete logic_con_out;
  delete logic_con_in;
} /* Logic::~Logic */


bool Logic::initialize(void)
{
  ChangeLangCmd *lang_cmd = new ChangeLangCmd(&cmd_parser, this);
  if (!lang_cmd->addToParser())
  {
    cerr << "*** ERROR: Could not add the language change command \""
	 << lang_cmd->cmdStr() << "\".\n";
    delete lang_cmd;  // FIXME: Do this in cleanup() instead
    return false;
  }
  
  string value;
  if (cfg().getValue(name(), "LINKS", value))
  {
    LinkCmd *link_cmd = new LinkCmd(&cmd_parser, this);
    if (!link_cmd->initialize(cfg(), value))
    {
      delete link_cmd;  // FIXME: Do this in cleanup() instead
      return false;
    }
  }

  string event_handler_str;
  if (!cfg().getValue(name(), "EVENT_HANDLER", event_handler_str))
  {
    cerr << "*** ERROR: Config variable " << name()
      	 << "/EVENT_HANDLER not set\n";
    cleanup();
    return false;
  }

  string rx_name;
  if (!cfg().getValue(name(), "RX", rx_name))
  {
    cerr << "*** ERROR: Config variable " << name() << "/RX not set\n";
    cleanup();
    return false;
  }

  string tx_name;
  if (!cfg().getValue(name(), "TX", tx_name))
  {
    cerr << "*** ERROR: Config variable " << name() << "/TX not set\n";
    cleanup();
    return false;
  }

  if (!cfg().getValue(name(), "CALLSIGN", m_callsign))
  {
    cerr << "*** ERROR: Config variable " << name() << "/CALLSIGN not set\n";
    cleanup();
    return false;
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

  if (cfg().getValue(name(), "ACTIVATE_MODULE_ON_LONG_CMD", value))
  {
    string::iterator colon = find(value.begin(), value.end(), ':');
    if (colon == value.end())
    {
      cerr << "*** ERROR: Format error for configuration variable \n"
      	<< "           " << name() << "/ACTIVATE_MODULE_ON_LONG_CMD: "
	      	       "No colon found. \n"
	<< "           The format should be digits:module (e.g. 4:EchoLink)\n";
      return false;
    }

    string digits(value.begin(), colon);
    string module(colon+1, value.end());
    long_cmd_digits = atoi(digits.c_str());
    long_cmd_module = module;
  }

  string macro_section;
  if (cfg().getValue(name(), "MACROS", macro_section))
  {
    list<string> macro_list = cfg().listSection(macro_section);
    list<string>::iterator mlit;
    for (mlit=macro_list.begin(); mlit!=macro_list.end(); ++mlit)
    {
      cfg().getValue(macro_section, *mlit, value);
      macros[atoi(mlit->c_str())] = value;
    }
  }

  string sel5_macros;
  if (cfg().getValue(name(), "SEL5_MACRO_RANGE", sel5_macros))
  {
    size_t comma = sel5_macros.find(",");
    sel5_from = sel5_macros.substr(0, int(comma));
    sel5_to = sel5_macros.substr(int(comma) + 1, sel5_macros.length());
    cout << "Sel5 macro range from " << sel5_from << " to " << sel5_to << endl;
  }

  if (cfg().getValue(name(), "TX_CTCSS", value))
  {
    string::iterator comma;
    string::iterator begin = value.begin();
    do
    {
      comma = find(begin, value.end(), ',');
      string tx_ctcss_type;
      if (comma == value.end())
      {
        tx_ctcss_type = string(begin, value.end());
      }
      else
      {
        tx_ctcss_type = string(begin, comma);
        begin = comma + 1;
      }

      if (tx_ctcss_type == "ALWAYS")
      {
        tx_ctcss_mask |= TX_CTCSS_ALWAYS;
      }
      else if (tx_ctcss_type == "SQL_OPEN")
      {
        tx_ctcss_mask |= TX_CTCSS_SQL_OPEN;
      }
      else if (tx_ctcss_type == "LOGIC")
      {
        tx_ctcss_mask |= TX_CTCSS_LOGIC;
      }
      else if (tx_ctcss_type == "MODULE")
      {
        tx_ctcss_mask |= TX_CTCSS_MODULE;
      }
      else if (tx_ctcss_type == "ANNOUNCEMENT")
      {
        tx_ctcss_mask |= TX_CTCSS_ANNOUNCEMENT;
      }
      else
      {
        cerr << "*** WARNING: Unknown value in configuration variable "
	     << name() << "/TX_CTCSS: " << tx_ctcss_type << endl;
      }
    } while (comma != value.end());
  }

  if (cfg().getValue(name(), "FX_GAIN_NORMAL", value))
  {
    fx_gain_normal = atoi(value.c_str());
  }

  if (cfg().getValue(name(), "FX_GAIN_LOW", value))
  {
    fx_gain_low = atoi(value.c_str());
  }

  AudioSource *prev_rx_src = 0;

    // Create the RX object
  m_rx = RxFactory::createNamedRx(cfg(), rx_name);
  if ((m_rx == 0) || !rx().initialize())
  {
    delete m_rx;
    m_rx = 0;
    cerr << "*** ERROR: Could not initialize RX \"" << rx_name << "\"\n";
    cleanup();
    return false;
  }
  rx().squelchOpen.connect(slot(*this, &Logic::squelchOpen));
  rx().dtmfDigitDetected.connect(slot(*this, &Logic::dtmfDigitDetectedP));
  rx().selcallSequenceDetected.connect(
	slot(*this, &Logic::selcallSequenceDetected));
  rx().mute(false);
  prev_rx_src = m_rx;

    // This valve is used to turn RX audio on/off into the logic core
  rx_valve = new AudioValve;
  rx_valve->setOpen(false);
  prev_rx_src->registerSink(rx_valve, true);
  prev_rx_src = rx_valve;

    // Split the RX audio stream to multiple sinks
  rx_splitter = new AudioSplitter;
  prev_rx_src->registerSink(rx_splitter, true);
  prev_rx_src = 0;

    // Create a selector for audio to the module
  audio_to_module_selector = new AudioSelector;

    // Connect RX audio to the modules
  AudioPassthrough *passthrough = new AudioPassthrough;
  rx_splitter->addSink(passthrough, true);
  audio_to_module_selector->addSource(passthrough);
  audio_to_module_selector->enableAutoSelect(passthrough, 10);

    // Connect inter logic audio input to the modules
  passthrough = new AudioPassthrough;
  logic_con_in->addSink(passthrough, true);
  audio_to_module_selector->addSource(passthrough);
  audio_to_module_selector->enableAutoSelect(passthrough, 0);

    // Split audio to all modules
  audio_to_module_splitter = new AudioSplitter;
  audio_to_module_selector->registerSink(audio_to_module_splitter, true);

    // Connect RX audio to inter logic audio output
  passthrough = new AudioPassthrough;
  rx_splitter->addSink(passthrough, true);
  logic_con_out->addSource(passthrough);
  logic_con_out->enableAutoSelect(passthrough, 10);

    // A valve that is used to turn direct RX to TX audio on or off.
    // Used by the repeater logic.
  rpt_valve = new AudioValve;
  rpt_valve->setOpen(false);
  rx_splitter->addSink(rpt_valve, true);

    // This selector is used to select audio source for TX audio
  tx_audio_selector = new AudioSelector;
  AudioSource *prev_tx_src = tx_audio_selector;

    // Connect the direct RX to TX audio valve to the TX audio selector
  tx_audio_selector->addSource(rpt_valve);
  tx_audio_selector->enableAutoSelect(rpt_valve, 20);

    // Connect incoming intra logic audio to the TX audio selector
    // through a stream state detector
  AudioStreamStateDetector *logic_con_in_idle_det =
	new AudioStreamStateDetector;
  logic_con_in_idle_det->sigStreamStateChanged.connect(
	slot(*this, &Logic::logicConInStreamStateChanged));
  logic_con_in->addSink(logic_con_in_idle_det, true);
  tx_audio_selector->addSource(logic_con_in_idle_det);
  tx_audio_selector->enableAutoSelect(logic_con_in_idle_det, 10);

    // Create a selector and a splitter to handle audio from modules
  audio_from_module_selector = new AudioSelector;
  AudioSplitter *audio_from_module_splitter = new AudioSplitter;
  audio_from_module_selector->registerSink(audio_from_module_splitter, true);

    // Connect audio from modules to the TX audio selector
    // via an audio stream state detector
  AudioStreamStateDetector *audio_from_module_idle_det =
	new AudioStreamStateDetector;
  audio_from_module_idle_det->sigStreamStateChanged.connect(
	slot(*this, &Logic::audioFromModuleStreamStateChanged));
  audio_from_module_splitter->addSink(audio_from_module_idle_det, true);
  tx_audio_selector->addSource(audio_from_module_idle_det);
  tx_audio_selector->enableAutoSelect(audio_from_module_idle_det, 0);

    // Connect audio from modules to the inter logic audio output
  passthrough = new AudioPassthrough;
  audio_from_module_splitter->addSink(passthrough, true);
  logic_con_out->addSource(passthrough);
  logic_con_out->enableAutoSelect(passthrough, 0);

  if (cfg().getValue(name(), "QSO_RECORDER_DIR", value) && !value.empty())
  {
      // Create the qso recorder
    qso_recorder = new QsoRecorder(value);
    if (cfg().getValue(name(), "QSO_RECORDER_CMD", value))
    {
      QsoRecorderCmd *qso_recorder_cmd =
          new QsoRecorderCmd(&cmd_parser, this, qso_recorder);
      if (!qso_recorder_cmd->initialize(value))
      {
	cerr << "*** ERROR: Could not add activation command for the QSO "
	     << "recorder in logic \"" << name() << "\". You probably have "
	     << "the same command set up in more than one place.\n";
	delete qso_recorder_cmd;  // FIXME: Do this in cleanup() instead
	cleanup();
	return false;
      }
    }

      // Connect RX audio and link audio to the qso recorder
    passthrough = new AudioPassthrough;
    audio_to_module_splitter->addSink(passthrough, true);
    qso_recorder->addSource(passthrough, 10);

      // Connect audio from modules to the qso recorder
    passthrough = new AudioPassthrough;
    audio_from_module_splitter->addSink(passthrough, true);
    qso_recorder->addSource(passthrough, 0);
  }

    // Create the state detector
  state_det = new AudioStreamStateDetector;
  state_det->sigStreamStateChanged.connect(
    slot(*this, &Logic::audioStreamStateChange));
  prev_tx_src->registerSink(state_det, true);
  prev_tx_src = state_det;

    // Add a pre-buffered FIFO to avoid underrun
  AudioFifo *tx_fifo = new AudioFifo(1024 * INTERNAL_SAMPLE_RATE / 8000);
  tx_fifo->setPrebufSamples(512 * INTERNAL_SAMPLE_RATE / 8000);
  prev_tx_src->registerSink(tx_fifo, true);
  prev_tx_src = tx_fifo;

    // Create the TX audio mixer
  tx_audio_mixer = new AudioMixer;
  tx_audio_mixer->addSource(prev_tx_src);
  prev_tx_src = tx_audio_mixer;

    // Create the TX object
  m_tx = TxFactory::createNamedTx(cfg(), tx_name);
  if ((m_tx == 0) || !tx().initialize())
  {
    delete m_tx;
    m_tx = 0;
    cerr << "*** ERROR: Could not initialize TX \"" << tx_name << "\"\n";
    cleanup();
    return false;
  }
  tx().transmitterStateChange.connect(
      slot(*this, &Logic::transmitterStateChange));
  prev_tx_src->registerSink(m_tx, true);
  prev_tx_src = 0;

    // Create the message handler
  msg_handler = new MsgHandler(INTERNAL_SAMPLE_RATE);
  msg_handler->allMsgsWritten.connect(slot(*this, &Logic::allMsgsWritten));
  prev_tx_src = msg_handler;

    // This gain control is used to reduce the audio volume of effects
    // and announcements when mixed with normal audio
  fx_gain_ctrl = new AudioAmp;
  fx_gain_ctrl->setGain(fx_gain_normal);
  prev_tx_src->registerSink(fx_gain_ctrl, true);
  prev_tx_src = fx_gain_ctrl;

    // Pace the audio so that we don't fill up the audio output pipe.
  AudioPacer *msg_pacer = new AudioPacer(INTERNAL_SAMPLE_RATE,
      	      	      	      	      	 256 * INTERNAL_SAMPLE_RATE / 8000, 0);
  prev_tx_src->registerSink(msg_pacer, true);
  tx_audio_mixer->addSource(msg_pacer);
  prev_tx_src = 0;

  event_handler = new EventHandler(event_handler_str, this);
  event_handler->playFile.connect(slot(*this, &Logic::playFile));
  event_handler->playSilence.connect(slot(*this, &Logic::playSilence));
  event_handler->playTone.connect(slot(*this, &Logic::playTone));
  event_handler->recordStart.connect(slot(*this, &Logic::recordStart));
  event_handler->recordStop.connect(slot(*this, &Logic::recordStop));
  event_handler->deactivateModule.connect(
          bind(slot(*this, &Logic::deactivateModule), (Module *)0));
  event_handler->setVariable("mycall", m_callsign);
  char str[256];
  sprintf(str, "%.1f", report_ctcss);
  event_handler->setVariable("report_ctcss", str);
  event_handler->setVariable("active_module", "");
  event_handler->setVariable("is_core_event_handler", "1");
  event_handler->setVariable("logic_name", name().c_str());

  cmd_tmo_timer = new Timer(10000);
  cmd_tmo_timer->expired.connect(slot(*this, &Logic::cmdTimeout));
  cmd_tmo_timer->setEnable(false);

  updateTxCtcss(true, TX_CTCSS_ALWAYS);

  loadModules();

  string loaded_modules;
  list<Module*>::const_iterator mit;
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
  list<string> cfgvars = cfg().listSection(name());
  list<string>::const_iterator cfgit;
  for (cfgit=cfgvars.begin(); cfgit!=cfgvars.end(); ++cfgit)
  {
    string var = "Logic::CFG_" + *cfgit;
    string value;
    cfg().getValue(name(), *cfgit, value);
    event_handler->setVariable(var, value);
  }

  if (!event_handler->initialize())
  {
    cleanup();
    return false;
  }

  audio_switch_matrix.addSource(name(), logic_con_out);
  audio_switch_matrix.addSink(name(), logic_con_in);

  everyMinute(0);

  return true;

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
  msg_handler->playFile(path, report_events_as_idle);

  if (!msg_handler->isIdle())
  {
    updateTxCtcss(true, TX_CTCSS_ANNOUNCEMENT);
  }

  checkIdle();
} /* Logic::playFile */


void Logic::playSilence(int length)
{
  msg_handler->playSilence(length, report_events_as_idle);

  if (!msg_handler->isIdle())
  {
    updateTxCtcss(true, TX_CTCSS_ANNOUNCEMENT);
  }

  checkIdle();
} /* Logic::playSilence */


void Logic::playTone(int fq, int amp, int len)
{
  msg_handler->playTone(fq, amp, len, report_events_as_idle);

  if (!msg_handler->isIdle())
  {
    updateTxCtcss(true, TX_CTCSS_ANNOUNCEMENT);
  }

  checkIdle();
} /* Logic::playSilence */


void Logic::recordStart(const string& filename, unsigned max_time)
{
  recordStop();
  recorder = new AudioRecorder(filename);
  if (!recorder->initialize())
  {
    cerr << "*** ERROR: Could not open file for recording: "
      	 << filename << endl;
    recordStop();
    return;
  }
  recorder->setMaxRecordingTime(max_time);
  rx_splitter->addSink(recorder, true);
} /* Logic::recordStart */


void Logic::recordStop(void)
{
  rx_splitter->removeSink(recorder);
  recorder = 0;
} /* Logic::recordStart */


bool Logic::activateModule(Module *module)
{
  if (active_module == module)
  {
    return true;
  }

  if (active_module == 0)
  {
    active_module = module;
    audio_to_module_splitter->enableSink(module, true);
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
    audio_to_module_splitter->enableSink(module, false);
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


void Logic::dtmfDigitDetected(char digit, int duration)
{
  if (active_module != 0)
  {
    if (active_module->dtmfDigitReceived(digit, duration))
    {
      return;
    }
  }

  cmd_tmo_timer->reset();
  cmd_tmo_timer->setEnable(true);

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
  else if (received_digits.size() < 20)
  {
    if (digit == 'H')	// Make it possible to include a hash mark in a macro
    {
      received_digits += '#';
    }
    else if (digit == 'B')
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


void Logic::selcallSequenceDetected(std::string sequence)
{
  if ((sequence.compare(sel5_from) >= 0) && (sequence.compare(sel5_to) <= 0))
  {
    string s = "D" + sequence + "#";
    processMacroCmd(s);
  }
  else
  {
    cout << "Sel5 sequence \"" << sequence << "\" out of defined range\n";
  }
} /* Logic::selcallSequenceDetected */


void Logic::disconnectAllLogics(void)
{
  cout << "Deactivating all links to/from \"" << name() << "\"\n";
  audio_switch_matrix.disconnectSource(name());
  audio_switch_matrix.disconnectSink(name());
} /* Logic::disconnectAllLogics */


void Logic::sendDtmf(const std::string& digits)
{
  if (!digits.empty())
  {
    tx().sendDtmf(digits);
  }
} /* Logic::sendDtmf */


bool Logic::isWritingMessage(void)
{
  return msg_handler->isWritingMessage();
} /* Logic::isWritingMessage */






/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void Logic::squelchOpen(bool is_open)
{
  if (active_module != 0)
  {
    active_module->squelchOpen(is_open);
  }

  stringstream ss;
  ss << "squelch_open " << rx().sqlRxId() << " " << (is_open ? "1" : "0");
  processEvent(ss.str());

  if (!is_open)
  {
    if (((exec_cmd_on_sql_close > 0) || (received_digits == "*")) &&
        !anti_flutter && !received_digits.empty())
    {
      exec_cmd_on_sql_close_timer = new Timer(exec_cmd_on_sql_close);
      exec_cmd_on_sql_close_timer->expired.connect(
	  slot(*this, &Logic::putCmdOnQueue));
    }
    processCommandQueue();
  }
  else
  {
    delete exec_cmd_on_sql_close_timer;
    exec_cmd_on_sql_close_timer = 0;
  }

  updateTxCtcss(is_open, TX_CTCSS_SQL_OPEN);

  checkIdle();

} /* Logic::squelchOpen */


bool Logic::getIdleState(void) const
{
  return !rx().squelchIsOpen() &&
      	 state_det->isIdle() &&
      	 msg_handler->isIdle();

} /* Logic::getIdleState */


void Logic::transmitterStateChange(bool is_transmitting)
{
  stringstream ss;
  ss << "transmit " << (is_transmitting ? "1" : "0");
  processEvent(ss.str());
} /* Logic::transmitterStateChange */


void Logic::clearPendingSamples(void)
{
  msg_handler->clear();
} /* Logic::clearPendingSamples */


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
      rgr_sound_timer->expired.connect(slot(*this, &Logic::sendRgrSound));
    }
    else
    {
      sendRgrSound();
    }
  }
} /* Logic::enableRgrSoundTimer */


#if 0
bool Logic::remoteLogicIsTransmitting(void) const
{
  return logic_con_in->isTransmitting();
} /* Logic::remoteLogicIsTransmitting */
#endif


void Logic::rxValveSetOpen(bool do_open)
{
  rx_valve->setOpen(do_open);
} /* Logic::rxValveSetOpen */


void Logic::rptValveSetOpen(bool do_open)
{
  rpt_valve->setOpen(do_open);
} /* Logic::rptValveSetOpen */


void Logic::checkIdle(void)
{
  bool new_idle_state = getIdleState();
  if (new_idle_state != is_idle)
  {
    is_idle = new_idle_state;
    //printf("Logic::checkIdle: is_idle=%s\n", is_idle ? "TRUE" : "FALSE");
    idleStateChanged(is_idle);
  }
} /* Logic::checkIdle */





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

  //sigc_msg_handler->flushSamples();
  //module_tx_fifo->stopOutput(false);

  if (active_module != 0)
  {
     active_module->allMsgsWritten();
  }

  updateTxCtcss(false, TX_CTCSS_ANNOUNCEMENT);
  checkIdle();

} /* Logic::allMsgsWritten */


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

  stringstream ss;
  ss << module->id();
  ModuleActivateCmd *cmd = new ModuleActivateCmd(&cmd_parser, ss.str(), this);
  if (!cmd->addToParser())
  {
    cerr << "\n*** ERROR: Failed to add module activation command for module \""
	 << module_cfg_name << "\". This is probably due to having set up two "
	 << "modules with the same module id or choosing a module id that "
	 << "is the same as another command.\n\n";
    delete cmd;
    delete module;
    dlclose(handle);
    return;
  }

    // Connect module audio output to the module audio selector
  audio_from_module_selector->addSource(module);
  audio_from_module_selector->enableAutoSelect(module, 0);

    // Connect module audio input to the module audio splitter
  audio_to_module_splitter->addSink(module);
  audio_to_module_splitter->enableSink(module, false);

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

  while (!cmd_queue.empty())
  {
    string cmd(cmd_queue.front());
    cmd_queue.pop_front();

    stringstream ss;
    ss << "dtmf_cmd_received \"" << cmd << "\"";
    processEvent(ss.str());
    if (atoi(event_handler->eventResult().c_str()) != 0)
    {
      continue;
    }

    processCommand(cmd);

  }
} /* Logic::processCommandQueue */


void Logic::processCommand(const std::string &cmd, bool force_core_cmd)
{
  if (cmd.substr(0, 1) == "*")
  {
    string rest(cmd, 1);
    if (rest.empty())
    {
      processEvent("manual_identification");
    }
    else
    {
      processCommand(rest, true);
    }
  }
  else if (cmd.substr(0, 1) == "D")
  {
    processMacroCmd(cmd);
  }
  else if ((!force_core_cmd) && (active_module != 0))
  {
    active_module->dtmfCmdReceived(cmd);
  }
  else if (!cmd.empty())
  {
    if ((!force_core_cmd) && (cmd.size() >= long_cmd_digits))
    {
      Module *module = findModule(long_cmd_module);
      if (module != 0)
      {
	activateModule(module);
	if (active_module != 0)
	{
	  active_module->dtmfCmdReceived(cmd);
	}
      }
      else
      {
	cerr << "*** WARNING: Could not find module \"" << long_cmd_module
	      << "\" specified in configuration variable "
	      << "ACTIVATE_MODULE_ON_LONG_CMD.\n";
	stringstream ss;
	ss << "command_failed " << cmd;
	processEvent(ss.str());
      }
    }
    else if (!cmd_parser.processCmd(cmd))
    {
      stringstream ss;
      ss << "unknown_command " << cmd;
      processEvent(ss.str());
    }
  }

} /* Logic::processCommand */


void Logic::processMacroCmd(const string& macro_cmd)
{
  cout << "Processing macro command: " << macro_cmd << "...\n";
  assert(!macro_cmd.empty() && (macro_cmd[0] == 'D'));
  string cmd(macro_cmd, 1);
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

  if (!module_name.empty())
  {
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
  }

  for (unsigned i=0; i<module_cmd.size(); ++i)
  {
    dtmfDigitDetected(module_cmd[i], 100);
  }
  //module->dtmfCmdReceived(module_cmd);

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
  processEvent("send_rgr_sound");
  enableRgrSoundTimer(false);
} /* Logic::sendRogerSound */


void Logic::everyMinute(Timer *t)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int msec = 60000 - (tv.tv_sec % 60) * 1000 - (tv.tv_usec / 1000);

  if ((t != 0) && (msec > 1000))
  {
    processEvent("every_minute");
  }
  delete every_minute_timer;

  every_minute_timer = new Timer(msec);
  every_minute_timer->expired.connect(slot(*this, &Logic::everyMinute));

} /* Logic::everyMinute */


void Logic::dtmfDigitDetectedP(char digit, int duration)
{
  cout << name() << ": digit=" << digit << endl;

  stringstream ss;
  ss << "dtmf_digit_received " << digit << " " << duration;
  processEvent(ss.str());
  if (atoi(event_handler->eventResult().c_str()) != 0)
  {
    return;
  }

  dtmfDigitDetected(digit, duration);

} /* Logic::dtmfDigitDetected */


void Logic::audioStreamStateChange(bool is_active, bool is_idle)
{
  if (is_active)
  {
    fx_gain_ctrl->setGain(fx_gain_low);
  }
  else
  {
    fx_gain_ctrl->setGain(fx_gain_normal);
  }

  checkIdle();
} /* Logic::audioStreamStateChange */


void Logic::cleanup(void)
{
  if (m_tx != 0)
  {
    tx().setTxCtrlMode(Tx::TX_OFF);
  }

  delete event_handler;       	      event_handler = 0;
  delete cmd_tmo_timer;       	      cmd_tmo_timer = 0;
  unloadModules();
  delete exec_cmd_on_sql_close_timer; exec_cmd_on_sql_close_timer = 0;
  delete rgr_sound_timer;     	      rgr_sound_timer = 0;
  delete every_minute_timer;  	      every_minute_timer = 0;

  if (audio_switch_matrix.sourceIsAdded(name()))
  {
    audio_switch_matrix.removeSource(name());
  }
  if (audio_switch_matrix.sinkIsAdded(name()))
  {
    audio_switch_matrix.removeSink(name());
  }

  delete msg_handler; 	      	      msg_handler = 0;
  delete m_tx;        	      	      m_tx = 0;
  delete m_rx;        	      	      m_rx = 0;
  delete audio_to_module_selector;    audio_to_module_selector = 0;
  delete tx_audio_selector;   	      tx_audio_selector = 0;
  delete audio_from_module_selector;  audio_from_module_selector = 0;
  delete tx_audio_mixer;      	      tx_audio_mixer = 0;
  delete qso_recorder;                qso_recorder = 0;
} /* Logic::cleanup */


void Logic::updateTxCtcss(bool do_set, TxCtcssType type)
{
  if (do_set)
  {
    tx_ctcss |= type;
  }
  else
  {
    tx_ctcss &= ~type;
  }

  tx().enableCtcss((tx_ctcss & tx_ctcss_mask) != 0);

} /* Logic::updateTxCtcss */


void Logic::logicConInStreamStateChanged(bool is_active, bool is_idle)
{
  updateTxCtcss(!is_idle, TX_CTCSS_LOGIC);
} /* Logic::logicConInStreamStateChanged */


void Logic::audioFromModuleStreamStateChanged(bool is_active, bool is_idle)
{
  updateTxCtcss(!is_idle, TX_CTCSS_MODULE);
} /* Logic::audioFromModuleStreamStateChanged */



/*
 * This file has not been truncated
 */

