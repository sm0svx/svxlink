
#include <iostream>

#include "Rx.h"
#include "Logic.h"
#include "Module.h"

using namespace std;
using namespace Async;

void Module::activate(void)
{
  cout << "Activating module " << name() << "...\n";
  
  playMsg("activating_module");
  playModuleName();
  
  m_audio_con = logic()->rx().audioReceived.connect(
      	  slot(this, &Module::audioFromRx));
  m_squelch_con = logic()->rx().squelchOpen.connect(
      	  slot(this, &Module::squelchOpen));
  
  activateInit();
}


void Module::deactivate(void)
{
  cout << "Deactivating module " << name() << "...\n";
  
  deactivateCleanup();
  
  m_audio_con.disconnect();
  m_squelch_con.disconnect();
  
  playMsg("deactivating_module");
  playModuleName();
}


Config &Module::cfg(void) const
{
  return logic()->cfg();
} /*  */

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


