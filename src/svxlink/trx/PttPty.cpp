/**
@file	 PttPty.cpp
@brief   A PTT hardware controller using a PTY to signal an external script
@author  Tobias Blomberg / SM0SVX & Steve Koehler / DH1DM & Adi Bier / DL1HRC
@date	 2014-05-05

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <RefCountingPty.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "PttPty.h"



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

PttPty::PttPty(void)
  : pty(0)
{
} /* PttPty::PttPty */


PttPty::~PttPty(void)
{
  pty->destroy();
} /* PttPty::~PttPty */


bool PttPty::initialize(Async::Config &cfg, const std::string name)
{

  string ptt_pty;
  if (!cfg.getValue(name, "PTT_PTY", ptt_pty))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PTY not set\n";
    return false;
  }

  pty = RefCountingPty::instance(ptt_pty);
  return (pty != 0);
} /* PttPty::initialize */


/*
 * This functions sends a character over the pty-device:
 * T  to direct the controller to enable the TX
 * R  to direct the controller to disable the TX
 */
bool PttPty::setTxOn(bool tx_on)
{
  char cmd(tx_on ? 'T' : 'R');
  return (pty->write(&cmd, 1) == 1);
} /* PttPty::setTxOn */



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

