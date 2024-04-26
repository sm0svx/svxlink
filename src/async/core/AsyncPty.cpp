/**
@file	 AsyncPty.cpp
@brief   A class that wrap up some functionality to use a PTY
@author  Tobias Blomberg / SM0SVX
@date	 2014-06-07

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX

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

#include <AsyncApplication.h>
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
  : m_slave_link(slave_link),
    m_pollhup_timer(POLLHUP_CHECK_INTERVAL, Timer::TYPE_PERIODIC)
{
  m_watch.activity.connect(
      sigc::hide(sigc::mem_fun(*this, &Pty::charactersReceived)));
  m_pollhup_timer.setEnable(false);
  m_pollhup_timer.expired.connect(
      sigc::hide(sigc::mem_fun(*this, &Pty::checkIfSlaveEndOpen)));
} /* Pty::Pty */


Pty::~Pty(void)
{
  close();
} /* Pty::~Pty */


bool Pty::open(void)
{
  close();

    // Create the master pty
  m_master = posix_openpt(O_RDWR|O_NOCTTY);

  char *slave_path = NULL;
  if ((m_master < 0) ||
      (grantpt(m_master) < 0) ||
      (unlockpt(m_master) < 0) ||
      (slave_path = ptsname(m_master)) == NULL)
  {
    close();
    return false;
  }

    // Set the PTY to RAW mode
  struct termios port_settings = {0};
  if (tcgetattr(m_master, &port_settings))
  {
    std::cerr << "*** ERROR: tcgetattr failed for PTY: "
              << strerror(errno) << std::endl;
    close();
    return false;
  }
  cfmakeraw(&port_settings);
  if (tcsetattr(m_master, TCSANOW, &port_settings) == -1)
  {
    std::cerr << "*** ERROR: tcsetattr failed for PTY: "
              << strerror(errno) << std::endl;
    close();
    return false;
  }

    // Set non-blocking mode
  int master_fd_flags = fcntl(m_master, F_GETFL, 0);
  if ((master_fd_flags == -1) ||
      (fcntl(m_master, F_SETFL, master_fd_flags|O_NONBLOCK) == -1))
  {
    std::cerr << "*** ERROR: fcntl failed for PTY: "
              << strerror(errno) << std::endl;
    close();
    return false;
  }

    // Open the slave device to keep it open even if the external script
    // close the device. If we do not do this an I/O error will occur
    // if the script close the device.
  int slave = ::open(slave_path, O_RDWR|O_NOCTTY);
  if (slave == -1)
  {
    std::cerr << "*** ERROR: Could not open slave PTY " << slave_path
              << std::endl;
    close();
    return false;
  }
  ::close(slave);

    // Create symlink to make the access for user scripts a bit easier
  if (!m_slave_link.empty())
  {
    if (symlink(slave_path, m_slave_link.c_str()) == -1)
    {
      std::cerr << "*** ERROR: Failed to create PTY slave symlink "
                << slave_path << " -> " << m_slave_link << std::endl;
      close();
      return false;
    }
  }

  m_slave_path = slave_path;

  m_watch.setFd(m_master, Async::FdWatch::FD_WATCH_RD);
  m_watch.setEnabled(false);
  m_pollhup_timer.setEnable(true);

  return true;
} /* Pty::open */


void Pty::close(void)
{
  if (!m_slave_link.empty())
  {
    unlink(m_slave_link.c_str());
  }
  m_slave_path = "";
  m_pollhup_timer.setEnable(false);
  m_watch.setEnabled(false);
  if (m_master >= 0)
  {
    ::close(m_master);
    m_master = -1;
  }
} /* Pty::close */


bool Pty::reopen(void)
{
  if (!open())
  {
    std::cerr << "*** ERROR: Failed to reopen the PTY" << std::endl;
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
  return ::write(m_master, buf, count);
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
 */
void Pty::charactersReceived(void)
{
    // Read file descriptor status for the master end
  short revent = pollMaster();

    // If the slave side is not open, stop watching the descriptor
    // and start polling instead
  if ((revent & POLLHUP) != 0)
  {
    m_watch.setEnabled(false);
    m_pollhup_timer.setEnable(true);
  }

    // If there is no data to read, bail out
  if ((revent & POLLIN) == 0)
  {
    return;
  }

  char buf[256];
  int rd = ::read(m_master, buf, sizeof(buf));
  if (rd < 0)
  {
    std::cerr << "*** ERROR: Failed to read master PTY: "
              << std::strerror(errno) << ". Trying to reopen the PTY."
              << std::endl;
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
  assert(m_master >= 0);
  struct pollfd fds = {0};
  fds.fd = m_master;
  fds.events = POLLIN;
  int ret = ::poll(&fds, 1, 0);
  if (ret > 0)
  {
    return fds.revents;
  }
  else if (ret < 0)
  {
    std::cout << "*** ERROR: Failed to poll master end of PTY: "
              << strerror(errno) << std::endl;
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
    m_watch.setEnabled(true);
    m_pollhup_timer.setEnable(false);
  }
  if ((revents & POLLIN) != 0)
  {
    charactersReceived();
  }
} /* Pty::checkIfSlaveEndOpen */


/*
 * This file has not been truncated
 */

