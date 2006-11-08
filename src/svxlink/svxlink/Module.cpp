
#include <iostream>

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <version/SVXLINK.h>

#include <Rx.h>

#include "Logic.h"
#include "Module.h"


using namespace std;
using namespace Async;


Module::Module(void *dl_handle, Logic *logic, const string& cfg_name)
  : m_dl_handle(dl_handle), m_logic(logic), m_id(-1), m_name(cfg_name),
    m_is_transmitting(false), m_is_active(false), m_cfg_name(cfg_name),
    m_tmo_timer(0)
{
  
} /* Module::Module */


Module::~Module(void)
{
  delete m_tmo_timer;
} /* Module::~Module */


bool Module::initialize(void)
{
  if (strcmp(compiledForVersion(), SVXLINK_VERSION) != 0)
  {
    cerr << "*** ERROR: This module is compiled for version "
         << compiledForVersion() << " of SvxLink but the running version "
         << "of the SvxLink core is " << SVXLINK_VERSION << ".\n";
    return false;
  }

  string id_str;
  if (!cfg().getValue(cfgName(), "ID", id_str))
  {
    cerr << "*** Error: Config variable " << cfgName()
      	 << "/ID not set\n";
    return false;
  }
  m_id = atoi(id_str.c_str());

  cfg().getValue(cfgName(), "NAME", m_name);
  
  string timeout_str;
  if (cfg().getValue(cfgName(), "TIMEOUT", timeout_str))
  {
    m_tmo_timer = new Timer(1000 * atoi(timeout_str.c_str()));
    m_tmo_timer->setEnable(false);
    m_tmo_timer->expired.connect(slot(*this, &Module::moduleTimeout));
  }
  
  list<string> vars = cfg().listSection(cfgName());
  list<string>::const_iterator cfgit;
  for (cfgit=vars.begin(); cfgit!=vars.end(); ++cfgit)
  {
    string var = name() + "::CFG_" + *cfgit;
    string value;
    cfg().getValue(cfgName(), *cfgit, value);
    setEventVariable(var, value);
  }

  return true;
  
} /* Module::initialize */


void Module::activate(void)
{
  cout << "Activating module " << name() << "...\n";
  
  m_is_active = true;
  
  processEvent("activating_module");
  
  m_audio_con = logic()->rx().audioReceived.connect(
      	  slot(*this, &Module::audioFromRx));
  
  setIdle(true);
  activateInit();
}


void Module::deactivate(void)
{
  cout << "Deactivating module " << name() << "...\n";
  
  deactivateCleanup();
  setIdle(false);
  transmit(false);
  
  m_audio_con.disconnect();
  
  processEvent("deactivating_module");
  
  m_is_active = false;
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
  if (m_is_active)
  {
    logic()->playFile(path);
  }
} /* Module::playFile */


void Module::sendDtmf(const std::string& digits)
{
  if (m_is_active)
  {
    logic()->sendDtmf(digits);
  }
} /* Module::sendDtmf */


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
  if (m_is_active && (m_tmo_timer != 0))
  {
    m_tmo_timer->setEnable(is_idle);
  }
} /* Module::setIdle */


void Module::moduleTimeout(Timer *t)
{
  cout << "Module timeout: " << name() << endl;
  processEvent("timeout");
  deactivateMe();
} /* ModuleParrot::moduleTimeout */



