/**
@file	 PttPty.cpp
@brief   A PTT hardware controller using a pin in a serial port
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

#include <cstring>


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
{
} /* PttPty::PttPty */


PttPty::~PttPty(void)
{
  master = 0;
  unlink(slave);
} /* PttPty::~PttPty */


bool PttPty::initialize(Async::Config &cfg, const std::string name)
{

  string ptt_port;
  if (!cfg.getValue(name, "PTT_PORT", ptt_port))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PORT not set\n";
    return false;
  }

  // delete symlink if existing
  unlink(ptt_port.c_str());

  master = posix_openpt(O_RDWR|O_NONBLOCK);
  if (master < 0 ||
      grantpt(master) < 0 ||
      unlockpt(master) < 0 ||
      (slave = ptsname(master)) == NULL)
  {
    master = 0;
	cerr << "*** ERROR: Creating the pty device.\n";
	return false;
  }

  if ((fd = open(slave, O_RDWR|O_NONBLOCK)) == -1)
  {
    cerr << "*** ERROR: Could not open event device " << slave <<
            " specified in " << name << endl;
    return false;
  }

  // create symlink to make the access for user scripts a bit easier
  if (symlink(slave, ptt_port.c_str()) == -1)
  {
    cerr << "*** ERROR: creating symlink " << slave
         << " -> " << ptt_port << "\n";
    master = 0;
    return false;
  }

  // the created device is ptsname(master)
  cout << "created pseudo tty master (PTT) " << slave << " -> "
       << ptt_port << endl;

  return true;
} /* PttPty::initialize */


/*
 * This functions sends a character over the pty-device:
 * T\n  to direct the controller to enable the TX
 * R\n  to direct the controller to disable the TX
 *
**/
bool PttPty::setTxOn(bool tx_on)
{
  string s  = "";
  s += (tx_on ? "T\n" : "R\n");
  int ret = write(master, s.c_str(), 2);
  return (ret < 0 ? false : true);
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

