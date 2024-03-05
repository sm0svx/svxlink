/**
@file	 Module.cpp
@brief   This file contains the base class for implementing a SvxLink module.
@author  Tobias Blomberg / SM0SVX
@date	 2005-02-18

This file contains a class for implementing a SvxLink modules. The module
should inherit the Module class and implement the abstract methods.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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


#include <iostream>
#include <cstring>
#include <cstdlib>

#include <AsyncConfig.h>
#include <AsyncTimer.h>

#include <Rx.h>

#include "version/SVXLINK.h"
#include "Logic.h"
#include "Module.h"


using namespace std;
using namespace Async;


Module::Module(void *dl_handle, Logic *logic, const string& cfg_name)
  : m_dl_handle(dl_handle), m_logic(logic), m_id(-1), m_name(cfg_name),
    m_is_transmitting(false), m_is_active(false), m_cfg_name(cfg_name),
    m_tmo_timer(0), m_mute_linking(true)
{
  
} /* Module::Module */


Module::~Module(void)
{
  delete m_tmo_timer;
} /* Module::~Module */


bool Module::initialize(void)
{
  if (strcmp(compiledForVersion(), SVXLINK_APP_VERSION) != 0)
  {
    cerr << "*** ERROR: This module is compiled for version "
         << compiledForVersion() << " of SvxLink but the running version "
         << "of the SvxLink core is " << SVXLINK_APP_VERSION << ".\n";
    return false;
  }

  cfg().getValue(cfgName(), "ID", m_id);
  cfg().getValue(cfgName(), "NAME", m_name);
  
  string timeout_str;
  if (cfg().getValue(cfgName(), "TIMEOUT", timeout_str))
  {
    m_tmo_timer = new Timer(1000 * atoi(timeout_str.c_str()));
    m_tmo_timer->setEnable(false);
    m_tmo_timer->expired.connect(mem_fun(*this, &Module::moduleTimeout));
  }

  cfg().getValue(cfgName(), "MUTE_LOGIC_LINKING", m_mute_linking);

  list<string> vars = cfg().listSection(cfgName());
  list<string>::const_iterator cfgit;
  for (cfgit=vars.begin(); cfgit!=vars.end(); ++cfgit)
  {
    string var = name() + "::CFG_" + *cfgit;
    string value;
    cfg().getValue(cfgName(), *cfgit, value);
    setEventVariable(var, value);
  }

  cfg().valueUpdated.connect(sigc::mem_fun(*this, &Module::cfgUpdated));

  return true;
  
} /* Module::initialize */


void Module::activate(void)
{
  cout << logic()->name() << ": Activating module " << name() << "...\n";
  
  m_is_active = true;
  
  processEvent("activating_module");
  
  /*
  m_audio_con = logic()->rx().audioReceived.connect(
      	  mem_fun(*this, &Module::audioFromRx));
  */

  m_logic_idle_con = logic()->idleStateChanged.connect(
      mem_fun(*this, &Module::logicIdleStateChanged));

  m_logic->setMuteLinking(m_mute_linking);
  setIdle(logic()->isIdle());
  activateInit();
}


void Module::deactivate(void)
{
  cout << logic()->name() << ": Deactivating module " << name() << "...\n";
  
  deactivateCleanup();
  //transmit(false);
  
  //m_audio_con.disconnect();
  m_logic_idle_con.disconnect();
  
  processEvent("deactivating_module");
  
  m_is_active = false;

  setIdle(true);
  m_logic->setMuteLinking(false);
}


Config &Module::cfg(void) const
{
  return logic()->cfg();
} /* Module::cfg */


const string& Module::logicName(void) const
{
  return logic()->name();
} /* Module::logicName */


void Module::playHelpMsg(void)
{
  processEvent("play_help");
} /* Module::playHelpMsg */


void Module::dtmfCmdReceivedWhenIdle(const std::string &cmd)
{
  std::stringstream ss;
  ss << "command_failed " << id() << cmd;
  logic()->processEvent(ss.str());
} /* Module::dtmfCmdReceivedWhenIdle */


void Module::processEvent(const string& event)
{
  logic()->processEvent(event, this);
} /* Module::playFile */


void Module::setEventVariable(const string& name, const string& value)
{
  logic()->setEventVariable(name, value);
} /* Module::setEventVariable */


void Module::playFile(const string& path)
{
  logic()->playFile(path);
} /* Module::playFile */


void Module::sendDtmf(const std::string& digits)
{
  logic()->sendDtmf(digits);
} /* Module::sendDtmf */


#if 0
int Module::audioFromModule(float *samples, int count)
{
  if (m_is_active)
  {
    logic()->audioFromModule(samples, count);
  }
  return count;
}


void Module::transmit(bool tx)
{
  if (m_is_active && (tx != m_is_transmitting))
  {
    m_is_transmitting = tx;
    logic()->moduleTransmitRequest(tx);
  }
} /* transmit */
#endif


bool Module::activateMe(void)
{
  return logic()->activateModule(this);
} /* Module::activateMe */


void Module::deactivateMe(void)
{
  if (m_is_active)
  {
    logic()->deactivateModule(this);
  }
} /* Module::deactivateMe */


Module *Module::findModule(int id)
{
  return logic()->findModule(id);
} /* Module::findModule */


list<Module*> Module::moduleList(void)
{
  return logic()->moduleList();
} /* Module::moduleList */


void Module::setIdle(bool is_idle)
{
  if (m_tmo_timer != 0)
  {
    m_tmo_timer->setEnable(m_is_active && is_idle);
  }
} /* Module::setIdle */


bool Module::logicIsIdle(void) const
{
  return logic()->isIdle();
} /* Module::logicIsIdle */


void Module::logicIdleStateChanged(bool is_idle)
{
  /*
  printf("Module::logicIdleStateChanged: is_idle=%s\n",
      is_idle ? "TRUE" : "FALSE");
  */
  setIdle(is_idle);
} /* Module::logicIdleStateChanged */


bool Module::squelchIsOpen(void)
{
  return logic()->rx().squelchIsOpen();
} /* Module::squelchIsOpen */


void Module::cfgUpdated(const std::string& section, const std::string& tag)
{
  if (section == cfgName())
  {
    std::string value;
    if (cfg().getValue(cfgName(), tag, value))
    {
      setEventVariable(name() + "::CFG_" + tag, value);
      processEvent("config_updated CFG_" + tag + " \"" + value + "\"");
    }
  }
} /* Module::cfgUpdated */


bool Module::isWritingMessage(void)
{
  return logic()->isWritingMessage();
} /* Module::isWritingMessage */


void Module::moduleTimeout(Timer *t)
{
  cout << logic()->name() << ": Module timeout: " << name() << endl;
  processEvent("timeout");
  deactivateMe();
} /* ModuleParrot::moduleTimeout */


/*
 * This file has not been truncated
 */
