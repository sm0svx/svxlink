
#include <iostream>

#include <AsyncConfig.h>
#include <AsyncTimer.h>

#include "Rx.h"
#include "Logic.h"
#include "Module.h"


using namespace std;
using namespace Async;


Module::Module(void *dl_handle, Logic *logic, const string& cfg_name)
  : m_dl_handle(dl_handle), m_logic(logic), m_id(-1), m_is_transmitting(false),
    m_is_active(false), m_cfg_name(cfg_name), m_tmo_timer(0)
{
  
} /* Module::Module */


Module::~Module(void)
{
  delete m_tmo_timer;
} /* Module::~Module */


bool Module::initialize(void)
{
  string id_str;
  if (!cfg().getValue(cfgName(), "ID", id_str))
  {
    cerr << "*** Error: Config variable " << cfgName()
      	 << "/ID not set\n";
    return false;
  }
  m_id = atoi(id_str.c_str());

  string timeout_str;
  if (cfg().getValue(cfgName(), "TIMEOUT", timeout_str))
  {
    m_tmo_timer = new Timer(1000 * atoi(timeout_str.c_str()));
    m_tmo_timer->setEnable(false);
    m_tmo_timer->expired.connect(slot(this, &Module::moduleTimeout));
  }
  
  return true;
  
} /* Module::initialize */


void Module::activate(void)
{
  cout << "Activating module " << name() << "...\n";
  
  m_is_active = true;
  
  playMsg("activating_module");
  playModuleName();
  
  m_audio_con = logic()->rx().audioReceived.connect(
      	  slot(this, &Module::audioFromRx));
  m_squelch_con = logic()->rx().squelchOpen.connect(
      	  slot(this, &Module::squelchOpen));
  
  setIdle(true);
  activateInit();
}


void Module::deactivate(void)
{
  cout << "Deactivating module " << name() << "...\n";
  
  deactivateCleanup();
  setIdle(false);
  
  m_audio_con.disconnect();
  m_squelch_con.disconnect();
  
  playMsg("deactivating_module");
  playModuleName();
  
  m_is_active = false;
}


Config &Module::cfg(void) const
{
  return logic()->cfg();
} /* Module::cfg */


void Module::playMsg(const string& msg) const
{
  logic()->playMsg(msg, this);
}


void Module::playNumber(int number) const
{
  logic()->playNumber(number);
}


void Module::spellWord(const string& word) const
{
  logic()->spellWord(word);
}


int Module::audioFromModule(short *samples, int count)
{
  logic()->audioFromModule(samples, count);
  return count;
}


void Module::transmit(bool tx)
{
  logic()->moduleTransmitRequest(tx);
  m_is_transmitting = tx;
} /* transmit */


bool Module::activateMe(void)
{
  return logic()->activateModule(this);
} /* Module::activateMe */


void Module::deactivateMe(void)
{
  logic()->deactivateModule(this);
} /* Module::deactivateMe */


Module *Module::findModule(int id)
{
  return logic()->findModule(id);
} /* Module::findModule */


list<Module*> Module::moduleList(void)
{
  return logic()->moduleList();
} /* Module::moduleList */


const string& Module::logicName(void) const
{
  return logic()->name();
} /* Module::logicName */


void Module::setIdle(bool is_idle)
{
  if (m_tmo_timer != 0)
  {
    m_tmo_timer->setEnable(is_idle);
  }
} /* Module::setIdle */


void Module::moduleTimeout(Timer *t)
{
  cout << "Module timeout: " << name() << endl;
  playMsg("timeout");
  deactivateMe();
} /* ModuleParrot::moduleTimeout */



