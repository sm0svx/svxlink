/**
@file	 PtyDtmfDecoder.cpp
@brief   This file contains a class that add support for the Pty interface
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2014-03-21

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014  Tobias Blomberg / SM0SVX

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
#include <string>
#include <pty.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>  // for stat()
#include <sys/stat.h>   // for stat()


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "PtyDtmfDecoder.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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

PtyDtmfDecoder::PtyDtmfDecoder(Config &cfg, const string &name)
  : HwDtmfDecoder(cfg, name), fd(-1), watch(0)
{
  //cout << "### Pty DTMF decoder loaded...\n";

} /* PtyDtmfDecoder::PtyDtmfDecoder */


PtyDtmfDecoder::~PtyDtmfDecoder(void)
{
  delete watch;
  watch = 0;
  if (fd >= 0)
  {
    close(fd);
    fd = -1;
  }
  master = 0;
} /* PtyDtmfDecoder::~PtyDtmfDecoder */


bool PtyDtmfDecoder::initialize(void)
{
  if (!HwDtmfDecoder::initialize())
  {
    return false;
  }

  string serial_port;
  if (!cfg().getValue(name(), "DTMF_SERIAL", serial_port))
  {
    cerr << "*** ERROR: Config variable " << name()
      	 << "/DTMF_SERIAL not specified\n";
    return false;
  }

  // delete the old symlink if existing
  unlink(serial_port.c_str());

  // creating the pty master
  master = posix_openpt(O_RDWR|O_NOCTTY);

  if (master < 0 ||
      grantpt(master) < 0 ||
      unlockpt(master) < 0 ||
      (slave = ptsname(master)) == NULL)
  {
	master = 0;
	return false;
  }

  if ((fd = open(slave, O_RDWR|O_NOCTTY)) == -1)
  {
    cerr << "*** ERROR: Could not open event device " << slave <<
            " specified in " << name() << endl;
    return false;
  }

  // the created device is ptsname(master)
  std::cout << "created pseudo tty master (DTMF) " << slave << " -> "
            << serial_port << "\n";

  // watch the master pty
  watch = new FdWatch(master, FdWatch::FD_WATCH_RD);
  assert(watch != 0);
  watch->setEnabled(true);
  watch->activity.connect(mem_fun(*this, &PtyDtmfDecoder::charactersReceived));

  // create symlink to make the access for user scripts a bit easier
  if (symlink(slave, serial_port.c_str()) == -1)
  {
    cerr << "*** ERROR: creating symlink " << slave
         << " -> " << serial_port << "\n";
    master = 0;
    return false;
  }

  cout << "PTY DTMF \n";

  return true;

} /* PtyDtmfDecoder::initialize */


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

void PtyDtmfDecoder::charactersReceived(FdWatch *w)
{
  char buf[1];
  int rd = read(w->fd(), buf, 1);
  if (rd < 0)
  {
    cerr << "*** ERROR: reading characters from serial_port " << name()
         << "/PTY-dtmf device" << endl;
    return;
  }

  if (buf[0] == ' ')
  {
    cout << "DTMF idle\n";
    digitIdle(); // DTMF digit deactivated
  }
  else
  {
    cout << "DTMF = " << buf[0];
    digitActive(buf[0]); // DTMF digit activated
  }
} /* PtyDtmfDecoder::charactersReceived */



/*
 * This file has not been truncated
 */

