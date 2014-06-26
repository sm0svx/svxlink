/**
@file	 Pty.cpp
@brief   A class that wrap up some functionality to use a PTY
@author  Tobias Blomberg / SM0SVX
@date	 2014-06-07

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

#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <termios.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <cassert>


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

#include "Pty.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



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

Pty::Pty(const std::string &slave_link)
  : slave_link(slave_link), master(-1), slave(-1), watch(0)
{
} /* Pty::Pty */


Pty::~Pty(void)
{
  close();
} /* Pty::~Pty */


bool Pty::open(void)
{
  close();

    // Create the master pty
  master = posix_openpt(O_RDWR|O_NOCTTY);

  char *slave_path = NULL;
  if ((master < 0) ||
      (grantpt(master) < 0) ||
      (unlockpt(master) < 0) ||
      (slave_path = ptsname(master)) == NULL)
  {
    close();
    return false;
  }

    // Turn off line buffering on the PTY (noncanonical mode)
  struct termios port_settings;
  memset(&port_settings, 0, sizeof(port_settings));
  if (tcgetattr(master, &port_settings))
  {
    cerr << "*** ERROR: tcgetattr failed for PTY: "
         << strerror(errno) << endl;
    close();
    return false;
  }
  port_settings.c_lflag &= ~ICANON;
  if (tcsetattr(master, TCSANOW, &port_settings) == -1)
  {
    cerr << "*** ERROR: tcsetattr failed for PTY: "
         << strerror(errno) << endl;
    close();
    return false;
  }

    // Open the slave device to keep it open even if the external script
    // close the device. If we do not do this an I/O error will occur
    // if the script close the device.
  int slave = ::open(slave_path, O_RDWR|O_NOCTTY);
  if (slave == -1)
  {
    cerr << "*** ERROR: Could not open slave PTY " << slave_path << endl;
    close();
    return false;
  }

    // Watch the master pty
  watch = new Async::FdWatch(master, Async::FdWatch::FD_WATCH_RD);
  assert(watch != 0);
  watch->activity.connect(mem_fun(*this, &Pty::charactersReceived));

    // Create symlink to make the access for user scripts a bit easier
  if (!slave_link.empty())
  {
    if (symlink(slave_path, slave_link.c_str()) == -1)
    {
      cerr << "*** ERROR: Failed to create PTY slave symlink " << slave_path
           << " -> " << slave_link << endl;
      close();
      return false;
    }
    cout << "### Created pseudo tty slave link "
         << slave_path << " -> " << slave_link << endl;
  }

  return true;
} /* Pty::open */


void Pty::close(void)
{
  if (!slave_link.empty())
  {
    unlink(slave_link.c_str());
  }
  delete watch;
  watch = 0;
  if (slave >= 0)
  {
    ::close(slave);
    slave = -1;
  }
  if (master >= 0)
  {
    ::close(master);
    master = -1;
  }
} /* Pty::close */


bool Pty::reopen(void)
{
  if (!open())
  {
    cerr << "*** ERROR: Failed to reopen the PTY\n";
    return false;
  }
  return true;
} /* Pty::reopen */


bool Pty::write(char cmd)
{
  return (::write(master, &cmd, 1) == 1);
} /* Pty::write */


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

/**
 * @brief   Called when characters are received on the master PTY
 * @param   w The watch that triggered the event
 */
void Pty::charactersReceived(Async::FdWatch *w)
{
  char cmd;
  int rd = read(w->fd(), &cmd, 1);
  if (rd != 1)
  {
    std::cerr << "*** ERROR: Failed to read master PTY: "
              << std::strerror(errno) << ". "
              << "Trying to reopen the PTY.\n";
    reopen();
    return;
  }
  cmdReceived(cmd);
} /* Pty::charactersReceived */


/*
 * This file has not been truncated
 */

