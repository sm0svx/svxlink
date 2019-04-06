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

#include <dlfcn.h>
#include <link.h>
#include <sigc++/bind.h>
#include <sys/time.h>

#include <iostream>
#include <iomanip>
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
#include <common.h>
#include <config.h>


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
#include "LinkManager.h"
#include "DtmfDigitHandler.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace sigc;
using namespace SvxLink;



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
  : LogicBase(cfg, name),
    m_rx(0),  	      	      	            m_tx(0),
    msg_handler(0), 	      	            active_module(0),
    exec_cmd_on_sql_close_timer(-1),        rgr_sound_timer(-1),
    report_ctcss(0.0f),                     event_handler(0),
    recorder(0),                            tx_audio_mixer(0),
    fx_gain_ctrl(0),                        tx_audio_selector(0),
    rx_splitter(0),                         rx_valve(0),
    rpt_valve(0),                           audio_from_module_selector(0),
    audio_to_module_splitter(0),            audio_to_module_selector(0),
    state_det(0),
    fx_gain_normal(0),                      fx_gain_low(-12),
    long_cmd_digits(100),                   report_events_as_idle(false),
    qso_recorder(0),                        tx_ctcss(TX_CTCSS_ALWAYS),
    tx_ctcss_mask(0),
    currently_set_tx_ctrl_mode(Tx::TX_OFF), is_online(true),
    dtmf_digit_handler(0),                  state_pty(0),
    dtmf_ctrl_pty(0)
{
  rgr_sound_timer.expired.connect(sigc::hide(
        mem_fun(*this, &Logic::sendRgrSound)));
  logic_con_in = new AudioSplitter;
  logic_con_out = new AudioSelector;
} /* Logic::Logic */


Logic::~Logic(void)
{
  cleanup();
  delete logic_con_out;
  delete logic_con_in;
  delete dtmf_digit_handler;
} /* Logic::~Logic */


bool Logic::initialize(void)
{
  if (!LogicBase::initialize())
  {
    return false;
  }

  if (cfg().getValue(name(), "ONLINE_CMD", online_cmd))
  {
    OnlineCmd *cmd = new OnlineCmd(&cmd_parser, this, online_cmd);
    if (!cmd->addToParser())
    {
      cerr << "*** ERROR: Could not add the logic online command \""
           << cmd->cmdStr() << "\" for logic " << name() << ".\n";
      delete cmd;
      return false;
    }
  }

  ChangeLangCmd *lang_cmd = new ChangeLangCmd(&cmd_parser, this);
  if (!lang_cmd->addToParser())
  {
    cerr << "*** ERROR: Could not add the language change command \""
	 << lang_cmd->cmdStr() << "\" for logic " << name() << ".\n";
    delete lang_cmd;
    return false;
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

  int exec_cmd_on_sql_close = -1;
  if (cfg().getValue(name(), "EXEC_CMD_ON_SQL_CLOSE", exec_cmd_on_sql_close))
  {
    exec_cmd_on_sql_close_timer.setTimeout(exec_cmd_on_sql_close);
  }
  int rgr_sound_delay = -1;
  if (cfg().getValue(name(), "RGR_SOUND_DELAY", rgr_sound_delay))
  {
    rgr_sound_timer.setTimeout(rgr_sound_delay);
  }
  cfg().getValue(name(), "REPORT_CTCSS", report_ctcss);

  string state_pty_path;
  cfg().getValue(name(), "STATE_PTY", state_pty_path);
  if (!state_pty_path.empty())
  {
    state_pty = new Pty(state_pty_path);
    if (!state_pty->open())
    {
      cerr << "*** ERROR: Could not open state PTY "
           << state_pty_path << " as spcified in configuration variable "
           << name() << "/" << "STATE_PTY" << endl;
      cleanup();
      return false;
    }
  }

  string dtmf_ctrl_pty_path;
  cfg().getValue(name(), "DTMF_CTRL_PTY", dtmf_ctrl_pty_path);
  if (!dtmf_ctrl_pty_path.empty())
  {
    dtmf_ctrl_pty = new Pty(dtmf_ctrl_pty_path);
    if (!dtmf_ctrl_pty->open())
    {
      cerr << "*** ERROR: Could not open control PTY "
           << dtmf_ctrl_pty_path << " as spcified in configuration variable "
           << name() << "/" << "DTMF_CTRL_PTY" << endl;
      cleanup();
      return false;
    }
    dtmf_ctrl_pty->dataReceived.connect(
        mem_fun(*this, &Logic::dtmfCtrlPtyCmdReceived));
  }

  string value;
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

  cfg().getValue(name(), "FX_GAIN_NORMAL", fx_gain_normal);
  cfg().getValue(name(), "FX_GAIN_LOW", fx_gain_low);

  AudioSource *prev_rx_src = 0;

    // Create the RX object
  cout << "Loading RX: " << rx_name << endl;
  m_rx = RxFactory::createNamedRx(cfg(), rx_name);
  if ((m_rx == 0) || !rx().initialize())
  {
    delete m_rx;
    m_rx = 0;
    cerr << "*** ERROR: Could not initialize RX \"" << rx_name << "\"\n";
    cleanup();
    return false;
  }
  rx().squelchOpen.connect(mem_fun(*this, &Logic::squelchOpen));
  rx().dtmfDigitDetected.connect(mem_fun(*this, &Logic::dtmfDigitDetectedP));
  rx().selcallSequenceDetected.connect(
	mem_fun(*this, &Logic::selcallSequenceDetected));
  rx().setMuteState(Rx::MUTE_NONE);
  rx().publishStateEvent.connect(mem_fun(*this, &Logic::publishStateEvent));
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
	mem_fun(*this, &Logic::logicConInStreamStateChanged));
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
	mem_fun(*this, &Logic::audioFromModuleStreamStateChanged));
  audio_from_module_splitter->addSink(audio_from_module_idle_det, true);
  tx_audio_selector->addSource(audio_from_module_idle_det);
  tx_audio_selector->enableAutoSelect(audio_from_module_idle_det, 0);

    // Connect audio from modules to the inter logic audio output
  passthrough = new AudioPassthrough;
  audio_from_module_splitter->addSink(passthrough, true);
  logic_con_out->addSource(passthrough);
  logic_con_out->enableAutoSelect(passthrough, 0);

    // Create the qso recorder if QSO_RECORDER is properly set
  SepPair<string, string> qso_rec_cfg;
  if (!cfg().getValue(name(), "QSO_RECORDER", qso_rec_cfg, true))
  {
    cerr << "*** ERROR: Bad format for config variable QSO_RECORDER. "
         << "Valid format for value is command:config_section.\n";
    cleanup();
    return false;
  }
  if (!qso_rec_cfg.second.empty())
  {
    qso_recorder = new QsoRecorder(this);
    if (!qso_recorder->initialize(cfg(), qso_rec_cfg.second))
    {
      cleanup();
      return false;
    }
    if (!qso_rec_cfg.first.empty())
    {
      QsoRecorderCmd *qso_recorder_cmd =
          new QsoRecorderCmd(&cmd_parser, this, qso_recorder);
      if (!qso_recorder_cmd->initialize(qso_rec_cfg.first))
      {
        cerr << "*** ERROR: Could not add activation command for the QSO "
             << "recorder in logic \"" << name() << "\". You probably have "
             << "the same command set up in more than one place.\n";
        delete qso_recorder_cmd;
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
    mem_fun(*this, &Logic::audioStreamStateChange));
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
  cout << "Loading TX: " << tx_name << endl;
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
      mem_fun(*this, &Logic::transmitterStateChange));
  prev_tx_src->registerSink(m_tx);
  prev_tx_src = 0;

    // Create the message handler
  msg_handler = new MsgHandler(INTERNAL_SAMPLE_RATE);
  msg_handler->allMsgsWritten.connect(mem_fun(*this, &Logic::allMsgsWritten));
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
  event_handler->playFile.connect(mem_fun(*this, &Logic::playFile));
  event_handler->playSilence.connect(mem_fun(*this, &Logic::playSilence));
  event_handler->playTone.connect(mem_fun(*this, &Logic::playTone));
  event_handler->recordStart.connect(mem_fun(*this, &Logic::recordStart));
  event_handler->recordStop.connect(mem_fun(*this, &Logic::recordStop));
  event_handler->deactivateModule.connect(
          bind(mem_fun(*this, &Logic::deactivateModule), (Module *)0));
  event_handler->publishStateEvent.connect(
          mem_fun(*this, &Logic::publishStateEvent));
  event_handler->playDtmf.connect(mem_fun(*this, &Logic::playDtmf));
  event_handler->injectDtmf.connect(mem_fun(*this, &Logic::injectDtmf));
  event_handler->setVariable("mycall", m_callsign);
  char str[256];
  sprintf(str, "%.1f", report_ctcss);
  event_handler->setVariable("report_ctcss", str);
  event_handler->setVariable("active_module", "");
  event_handler->setVariable("is_core_event_handler", "1");
  event_handler->setVariable("logic_name", name().c_str());

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

  if (LocationInfo::has_instance())
  {
     LocationInfo::AprsStatistics lis;
     LocationInfo::instance()->aprs_stats.insert(
         pair<string,LocationInfo::AprsStatistics>(name(), lis));
     LocationInfo::instance()->aprs_stats[name()].reset();
  }

  every_minute_timer.setExpireOffset(100);
  every_minute_timer.expired.connect(mem_fun(*this, &Logic::everyMinute));
  timeoutNextMinute();
  every_minute_timer.start();

  every_second_timer.setExpireOffset(100);
  every_second_timer.expired.connect(mem_fun(*this, &Logic::everySecond));
  timeoutNextSecond();
  every_second_timer.start();
  
  dtmf_digit_handler = new DtmfDigitHandler;
  dtmf_digit_handler->commandComplete.connect(
      mem_fun(*this, &Logic::putCmdOnQueue));
  exec_cmd_on_sql_close_timer.expired.connect(sigc::hide(
      mem_fun(dtmf_digit_handler, &DtmfDigitHandler::forceCommandComplete)));

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


void Logic::playDtmf(const std::string& digits, int amp, int len)
{
  for (string::size_type i=0; i < digits.size(); ++i)
  {
    msg_handler->playDtmf(digits[i], amp, len);
    msg_handler->playSilence(50, report_events_as_idle);
  }

  if (!msg_handler->isIdle())
  {
    updateTxCtcss(true, TX_CTCSS_ANNOUNCEMENT);
  }

  checkIdle();
} /* Logic::playDtmf */


void Logic::recordStart(const string& filename, unsigned max_time)
{
  recordStop();
  recorder = new AudioRecorder(filename);
  if (!recorder->initialize())
  {
    cerr << "*** ERROR: Could not open file " << filename
         << " for recording in logic " << name() << ": "
         << recorder->errorMsg() << endl;
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
} /* Logic::recordStop */


void Logic::injectDtmf(const std::string& digits, int len)
{
  for (string::size_type i=0; i < digits.size(); ++i)
  {
    dtmfDigitDetected(digits[i], len);
  }
} /* Logic::injectDtmf */


bool Logic::activateModule(Module *module)
{
  if (active_module == module)
  {
    return true;
  }

  if ((active_module == 0) && is_online)
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
  if (id < 0)
  {
    return 0;
  }

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

  dtmf_digit_handler->digitReceived(digit);

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
    cout << name() << ": Sel5 sequence \"" << sequence
         << "\" out of defined range\n";
  }
} /* Logic::selcallSequenceDetected */


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


Async::AudioSink *Logic::logicConIn(void)
{
  return logic_con_in;
} /* Logic::logicConIn */


void Logic::setOnline(bool online)
{
  is_online = online;
  if (online)
  {
    tx().setTxCtrlMode(currently_set_tx_ctrl_mode);
  }
  else
  {
    tx().setTxCtrlMode(Tx::TX_OFF);
    deactivateModule(0);
  }
  stringstream ss;
  ss << "logic_online ";
  ss << (is_online ? 1 : 0);
  processEvent(ss.str());
} /* Logic::setOnline */


Async::AudioSource *Logic::logicConOut(void)
{
  return logic_con_out;
} /* Logic::logicConOut */



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
    const string &received_digits = dtmf_digit_handler->command();
    if (!dtmf_digit_handler->antiFlutterActive() &&
        !received_digits.empty())
    {
      if (exec_cmd_on_sql_close_timer.timeout() > 0)
      {
        exec_cmd_on_sql_close_timer.setEnable(true);
      }
      else if (received_digits == "*")
      {
        dtmf_digit_handler->forceCommandComplete();
      }
    }
    processCommandQueue();
  }
  else
  {
    exec_cmd_on_sql_close_timer.setEnable(false);
  }

  if (LocationInfo::has_instance())
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    LocationInfo::instance()->setReceiving(name(), tv, is_open);
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
  if (LocationInfo::has_instance() &&
      (LocationInfo::instance()->getTransmitting(name()) != is_transmitting))
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    LocationInfo::instance()->setTransmitting(name(), tv, is_transmitting);
  }

  stringstream ss;
  ss << "transmit " << (is_transmitting ? "1" : "0");
  processEvent(ss.str());
} /* Logic::transmitterStateChange */


void Logic::dtmfCtrlPtyCmdReceived(const void *buf, size_t count)
{
  const char *buffer = reinterpret_cast<const char*>(buf);
  for (size_t i=0; i<count; ++i)
  {
    const char &ch = buffer[i];
    if (::isdigit(ch) || (ch == '*') || (ch == '#') ||
        ((ch >= 'A') && (ch <= 'F')))
    {
      dtmfDigitDetectedP(ch, 100);
    }
  }
} /* Logic::dtmfCtrlPtyCmdReceived */


void Logic::clearPendingSamples(void)
{
  msg_handler->clear();
} /* Logic::clearPendingSamples */


void Logic::enableRgrSoundTimer(bool enable)
{
  if (rgr_sound_timer.timeout() < 0)
  {
    return;
  }

  rgr_sound_timer.setEnable(false);

  if (enable)
  {
    if (rgr_sound_timer.timeout() > 0)
    {
      rgr_sound_timer.setEnable(true);
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
  setIdle(getIdleState());
} /* Logic::checkIdle */


void Logic::setTxCtrlMode(Tx::TxCtrlMode mode)
{
  currently_set_tx_ctrl_mode = mode;
  if (is_online)
  {
    tx().setTxCtrlMode(mode);
  }
} /* Logic::setTxCtrlMode */



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
  cfg().getValue("GLOBAL", "MODULE_PATH", module_path);

  string plugin_name = module_cfg_name;
  cfg().getValue(module_cfg_name, "NAME", plugin_name);

    // Define the module namespace so that we can set some variables in it
  event_handler->processEvent("namespace eval " + plugin_name + " {}");

  cfg().getValue(module_cfg_name, "PLUGIN_NAME", plugin_name);

  void *handle = NULL;
  string plugin_filename = "Module" + plugin_name + ".so";
  if (!module_path.empty())
  {
    string plugin_abs_filename = module_path + "/" + plugin_filename;
    handle = dlopen(plugin_abs_filename.c_str(), RTLD_NOW);
    if (handle == NULL)
    {
      cerr << "*** ERROR: Failed to load module "
        << module_cfg_name.c_str() << " into logic " << name() << ": "
        << dlerror() << endl;
      return;
    }
  }
  else
  {
    handle = dlopen(plugin_filename.c_str(), RTLD_NOW);
    if (handle == NULL)
    {
      string plugin_abs_filename = string(SVX_MODULE_INSTALL_DIR "/")
                                   + plugin_filename;
      handle = dlopen(plugin_abs_filename.c_str(), RTLD_NOW);
      if (handle == NULL)
      {
        cerr << "*** ERROR: Failed to load module "
          << module_cfg_name.c_str() << " into logic " << name() << ": "
          << dlerror() << endl;
        return;
      }
    }
  }

  struct link_map *link_map;
  if (dlinfo(handle, RTLD_DI_LINKMAP, &link_map) == -1)
  {
    cerr << "*** ERROR: Could not read information for module "
      	 << module_cfg_name.c_str() << " in logic " << name() << ": "
         << dlerror() << endl;
    dlclose(handle);
    return;
  }
  cout << "\tFound " << link_map->l_name << endl;

  Module::InitFunc init = (Module::InitFunc)dlsym(handle, "module_init");
  if (init == NULL)
  {
    cerr << "*** ERROR: Could not find init func for module "
      	 << module_cfg_name.c_str() << " in logic " << name() << ": "
         << dlerror() << endl;
    dlclose(handle);
    return;
  }

  Module *module = init(handle, this, module_cfg_name.c_str());
  if (module == 0)
  {
    cerr << "*** ERROR: Creation failed for module "
      	 << module_cfg_name.c_str() << " in logic " << name() << endl;
    dlclose(handle);
    return;
  }

  if (!module->initialize())
  {
    cerr << "*** ERROR: Initialization failed for module "
      	 << module_cfg_name.c_str() << " in logic " << name() << endl;
    delete module;
    dlclose(handle);
    return;
  }

  if (module->id() >= 0)
  {
    stringstream ss;
    ss << module->id();
    ModuleActivateCmd *cmd = new ModuleActivateCmd(&cmd_parser, ss.str(), this);
    if (!cmd->addToParser())
    {
      cerr << "\n*** ERROR: Failed to add module activation command for "
           << "module \"" << module_cfg_name << "\" in logic \"" << name()
           << "\". This is probably due to having set up two modules with the "
           << "same module id or choosing a module id that is the same as "
           << "another command.\n\n";
      delete cmd;
      delete module;
      dlclose(handle);
      return;
    }
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
  deactivateModule(0);
  list<Module *>::iterator it;
  for (it=modules.begin(); it!=modules.end(); ++it)
  {
    Module *module = *it;
    void *plugin_handle = module->pluginHandle();
    delete module;
    dlclose(plugin_handle);
  }
  modules.clear();
} /* logic::unloadModules */


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
  if (cmd[0] == '*')
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
  else if (cmd[0] == 'D')
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
	      << "\" in logic \"" << name() << "\" specified in configuration "
	      << "variable ACTIVATE_MODULE_ON_LONG_CMD.\n";
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
  cout << name() << ": Processing macro command: " << macro_cmd << "...\n";
  assert(!macro_cmd.empty() && (macro_cmd[0] == 'D'));
  string cmd(macro_cmd, 1);
  if (cmd.empty())
  {
    cerr << "*** Macro error in logic " << name() << ": Empty command.\n";
    processEvent("macro_empty");
    return;
  }

  map<int, string>::iterator it = macros.find(atoi(cmd.c_str()));
  if (it == macros.end())
  {
    cerr << "*** Macro error in logic " << name() << ": Macro "
         << cmd << " not found.\n";
    processEvent("macro_not_found");
    return;
  }
  string macro(it->second);
  cout << name() << ": Macro command found: \"" << macro << "\"\n";

  string::iterator colon = find(macro.begin(), macro.end(), ':');
  if (colon == macro.end())
  {
    cerr << "*** Macro error in logic " << name()
         << ": No colon found in macro (" << macro << ").\n";
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
      cerr << "*** Macro error in logic " << name() << ": Module "
           << module_name << " not found.\n";
      processEvent("macro_module_not_found");
      return;
    }

    if (active_module == 0)
    {
      if (!activateModule(module))
      {
	cerr << "*** Macro error in logic " << name()
             << ": Activation of module " << module_name << " failed.\n";
	processEvent("macro_module_activation_failed");
	return;
      }
    }
    else if (active_module != module)
    {
      cerr << "*** Macro error in logic " << name()
           << ": Another module is active (" << active_module->name()
           << ").\n";
      processEvent("macro_another_active_module");
      return;
    }
  }

  for (unsigned i=0; i<module_cmd.size(); ++i)
  {
    dtmfDigitDetected(module_cmd[i], 100);
  }
} /* Logic::processMacroCmd */


void Logic::checkIfOnlineCmd(void)
{
  if (dtmf_digit_handler->command() == (online_cmd + "1"))
  {
    cout << name() << ": Setting logic online\n";
    setOnline(true);
  }
} /* Logic::checkIfOnlineCmd */


void Logic::putCmdOnQueue(void)
{
  exec_cmd_on_sql_close_timer.setEnable(false);

  if (!is_online)
  {
    checkIfOnlineCmd();
    return;
  }

  string received_digits = dtmf_digit_handler->command();
  if ((received_digits != "*") ||
      (find(cmd_queue.begin(), cmd_queue.end(), "*") == cmd_queue.end()))
  {
    cmd_queue.push_back(received_digits);
  }

  dtmf_digit_handler->reset();

  processCommandQueue();

} /* Logic::putCmdOnQueue */


void Logic::sendRgrSound(void)
{
  processEvent("send_rgr_sound");
  enableRgrSoundTimer(false);
} /* Logic::sendRogerSound */


void Logic::timeoutNextMinute(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm *tm = localtime(&tv.tv_sec);
  tm->tm_min += 1;
  tm->tm_sec = 0;
  every_minute_timer.setTimeout(*tm);
} /* Logic::timeoutNextMinute */


void Logic::everyMinute(AtTimer *t)
{
  processEvent("every_minute");
  timeoutNextMinute();
} /* Logic::everyMinute */


void Logic::timeoutNextSecond(void)
{
  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm *tm = localtime(&tv.tv_sec);
  tm->tm_sec += 1;
  every_second_timer.setTimeout(*tm);
} /* Logic::timeoutNextSecond */


void Logic::everySecond(AtTimer *t)
{
  processEvent("every_second");
  timeoutNextSecond();
} /* Logic::everySecond */


void Logic::dtmfDigitDetectedP(char digit, int duration)
{
  cout << name() << ": digit=" << digit << endl;

  if (!is_online)
  {
    dtmf_digit_handler->digitReceived(digit);
    return;
  }

  stringstream ss;
  ss << "dtmf_digit_received " << digit << " " << duration;
  processEvent(ss.str());
  if (atoi(event_handler->eventResult().c_str()) != 0)
  {
    return;
  }

  dtmfDigitDetected(digit, duration);

  if (dtmf_ctrl_pty != 0)
  {
    dtmf_ctrl_pty->write(&digit, 1);
  }
} /* Logic::dtmfDigitDetectedP */


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

  if (m_rx != 0)
  {
    m_rx->reset();
  }

  unloadModules();
  exec_cmd_on_sql_close_timer.setEnable(false);
  rgr_sound_timer.setEnable(false);
  every_minute_timer.stop();

  if (LinkManager::hasInstance())
  {
    LinkManager::instance()->deleteLogic(this);
  }

  delete event_handler;               event_handler = 0;
  delete m_tx;        	      	      m_tx = 0;
  delete m_rx;        	      	      m_rx = 0;
  delete msg_handler; 	      	      msg_handler = 0;
  delete audio_to_module_selector;    audio_to_module_selector = 0;
  delete tx_audio_selector;   	      tx_audio_selector = 0;
  delete audio_from_module_selector;  audio_from_module_selector = 0;
  delete tx_audio_mixer;      	      tx_audio_mixer = 0;
  delete qso_recorder;                qso_recorder = 0;
  delete state_pty;                   state_pty = 0;
  delete dtmf_ctrl_pty;               dtmf_ctrl_pty = 0;
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


void Logic::publishStateEvent(const string &event_name, const string &msg)
{
  if (state_pty == 0)
  {
    return;
  }
  struct timeval tv;
  gettimeofday(&tv, NULL);
  stringstream os;
  os << setfill('0');
  os << tv.tv_sec << "." << setw(3) << tv.tv_usec / 1000 << " ";
  os << event_name << " " << msg;
  os << endl;
  state_pty->write(os.str().c_str(), os.str().size());
} /* Logic::publishStateEvent */


/*
 * This file has not been truncated
 */
