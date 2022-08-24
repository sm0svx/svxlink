/**
@file	 DummyLogic.h
@brief   A simple dummy logic core that does not do anything
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioDebugger.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DummyLogic.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

//using namespace MyNamespace;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Static class variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global functions
 *
 ****************************************************************************/

extern "C" {
  LogicBase* construct(void) { return new DummyLogic; }
}


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

namespace {


/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/



}; /* End of anonymous namespace */

/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

DummyLogic::DummyLogic(void)
{
  m_logic_con_in = new Async::AudioDebugger;
  m_logic_con_in->setName("DummyLogicIn");
  m_logic_con_out = new Async::AudioDebugger;
  m_logic_con_out->setName("DummyLogicOut");
} /* DummyLogic::DummyLogic */


bool DummyLogic::initialize(Async::Config& cfgobj, const std::string& logic_name)
{
  return LogicBase::initialize(cfgobj, logic_name);
} /* DummyLogic::initialize */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

DummyLogic::~DummyLogic(void)
{
  delete m_logic_con_in;
  delete m_logic_con_out;
} /* DummyLogic::~DummyLogic */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
