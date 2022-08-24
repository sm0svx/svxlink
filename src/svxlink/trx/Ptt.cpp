/**
@file	 Ptt.cpp
@brief   Base class for PTT hw control
@author  Tobias Blomberg / SM0SVX
@date	 2014-01-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#include <iostream>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Ptt.h"
#include "PttSerialPin.h"
#include "PttGpio.h"
#include "PttPty.h"
#ifdef HAS_HIDRAW_SUPPORT
#include "PttHidraw.h"
#endif
#ifdef HAS_GPIOD_SUPPORT
#include "PttGpiod.h"
#endif



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

namespace {
  class PttDummy : public Ptt
  {
    public:
      struct Factory : public PttFactory<PttDummy>
      {
        Factory(void) : PttFactory<PttDummy>("Dummy") {}
      };

      virtual bool initialize(Config &cfg, const string name) { return true; }
      virtual bool setTxOn(bool tx_on) { return true; }
  };
};



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

Ptt *PttFactoryBase::createNamedPtt(Config& cfg, const string& name)
{
  PttDummy::Factory dummy_ptt_factory;
  PttSerialPin::Factory serial_ptt_factory;
  PttGpio::Factory gpio_ptt_factory;
  PttPty::Factory pty_ptt_factory;
#ifdef HAS_HIDRAW_SUPPORT
  PttHidraw::Factory hidraw_ptt_factory;
#endif
#ifdef HAS_GPIOD_SUPPORT
  PttGpiod::Factory gpiod_ptt_factory;
#endif

  string ptt_type;
  if (!cfg.getValue(name, "PTT_TYPE", ptt_type) || ptt_type.empty())
  {
    cerr << "*** ERROR: PTT_TYPE not specified for transmitter "
         << name << ". Legal values are: "
         << validFactories() << "or \"NONE\"" << endl;
    return 0;
  }

  if (ptt_type == "NONE")
  {
    ptt_type = "Dummy";
  }
  
  Ptt *ptt = createNamedObject(ptt_type);
  if (ptt == 0)
  {
    cerr << "*** ERROR: Unknown PTT_TYPE \"" << ptt_type << "\" specified for "
         << "transmitter " << name << ". Legal values are: "
         << validFactories() << "or \"NONE\"" << endl;
  }
  
  return ptt;
} /* PttFactoryBase::createNamedPtt */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

