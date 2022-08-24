/**
@file	 AsyncPty.cpp
@brief   A class that wrap up some functionality to use a PTY
@author  Tobias Blomberg / SM0SVX
@date	 2014-06-07

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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
#include <poll.h>

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

#include "AsyncPty.h"



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

Pty::Pty(const std::string &slave_link)
  : slave_link(slave_link), master(-1), watch(0),
    pollhup_timer(POLLHUP_CHECK_INTERVAL, Timer::TYPE_PERIODIC)
{
  pollhup_timer.setEnable(false);
  pollhup_timer.expired.connect(
      sigc::hide(mem_fun(*this, &Pty::checkIfSlaveEndOpen)));
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

    // Set the PTY to RAW mode
  struct termios port_settings = {0};
  if (tcgetattr(master, &port_settings))
  {
    cerr << "*** ERROR: tcgetattr failed for PTY: "
         << strerror(errno) << endl;
    close();
    return false;
  }
  cfmakeraw(&port_settings);
  if (tcsetattr(master, TCSANOW, &port_settings) == -1)
  {
    cerr << "*** ERROR: tcsetattr failed for PTY: "
         << strerror(errno) << endl;
    close();
    return false;
  }

    // Set non-blocking mode
  int master_fd_flags = fcntl(master, F_GETFL, 0);
  if ((master_fd_flags == -1) ||
      (fcntl(master, F_SETFL, master_fd_flags|O_NONBLOCK) == -1))
  {
    cerr << "*** ERROR: fcntl failed for PTY: "
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
  ::close(slave);

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
  }

  pollhup_timer.setEnable(true);

  return true;
} /* Pty::open */


void Pty::close(void)
{
  if (!slave_link.empty())
  {
    unlink(slave_link.c_str());
  }
  pollhup_timer.setEnable(false);
  delete watch;
  watch = 0;
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


ssize_t Pty::write(const void *buf, size_t count)
{
  if ((pollMaster() & POLLHUP) != 0)
  {
    return count;
  }
  return ::write(master, buf, count);
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
void Pty::charactersReceived(void)
{
    // Read file descriptor status for the master end
  short revent = pollMaster();

    // If the slave side is not open, stop watching the descriptor
    // and start polling instead
  if ((revent & POLLHUP) != 0)
  {
    delete watch;
    watch = 0;
    pollhup_timer.setEnable(true);
  }

    // If there is no data to read, bail out
  if ((revent & POLLIN) == 0)
  {
    return;
  }

  char buf[256];
  int rd = read(master, buf, sizeof(buf));
  if (rd < 0)
  {
    std::cerr << "*** ERROR: Failed to read master PTY: "
              << std::strerror(errno) << ". "
              << "Trying to reopen the PTY.\n";
    reopen();
    return;
  }
  else if (rd == 0)
  {
    reopen();
    return;
  }

  if (m_is_line_buffered)
  {
    for (int i = 0; i < rd; ++i)
    {
      const char& ch = buf[i];
      if ((ch == '\r') || (ch == '\n'))
      {
        if (!m_line_buffer.empty())
        {
          dataReceived(m_line_buffer.c_str(), m_line_buffer.size());
          m_line_buffer.clear();
        }
      }
      else
      {
        m_line_buffer += ch;
      }
    }
  }
  else
  {
    dataReceived(buf, rd);
  }
} /* Pty::charactersReceived */


/**
 * @brief   Read file descriptor status for the master end of the PTY
 */
short Pty::pollMaster(void)
{
  assert(master >= 0);
  struct pollfd fds = {0};
  fds.fd = master;
  fds.events = POLLIN;
  int ret = ::poll(&fds, 1, 0);
  if (ret > 0)
  {
    return fds.revents;
  }
  else if (ret < 0)
  {
    cout << "*** ERROR: Failed to poll master end of PTY: "
         << strerror(errno) << endl;
    return 0;
  }
  return 0;
} /* Pty::pollMaster */


/**
 * @brief Check if slave end of the PTY is open
 *
 * This function will check if the slave end of the PTY is open and if so will
 * start watching the master file descriptor and stop polling.
 * It will also check if there is data available to read on the master PTY and
 * if so it will call the charactersReceived function.
 */
void Pty::checkIfSlaveEndOpen(void)
{
  short revents = pollMaster();
  if ((revents & POLLHUP) == 0)
  {
    watch = new Async::FdWatch(master, Async::FdWatch::FD_WATCH_RD);
    assert(watch != 0);
    watch->activity.connect(
        sigc::hide(mem_fun(*this, &Pty::charactersReceived)));
    pollhup_timer.setEnable(false);
  }
  if ((revents & POLLIN) != 0)
  {
    charactersReceived();
  }
} /* Pty::checkIfSlaveEndOpen */



/*
 * This file has not been truncated
 */

