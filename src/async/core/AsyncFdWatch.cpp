/**
@file	 AsyncFdWatch.cpp
@brief   Contains a watch for file descriptors
@author  Tobias Blomberg
@date	 2003-03-19

This file contains a watch for file descriptors. When activity is found on the
file descriptor, a signal is emitted.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2022 Tobias Blomberg

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

#include <cassert>
//#include <iostream>


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

#include "AsyncApplication.h"
#include "AsyncFdWatch.h"



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

FdWatch::FdWatch(void)
  : m_fd(-1), m_type(FD_WATCH_RD), m_enabled(false)
{
} /* FdWatch::FdWatch */


FdWatch::FdWatch(int fd, FdWatchType type)
  : m_fd(fd), m_type(type), m_enabled(true)
{
  Application::app().addFdWatch(this);
} /* FdWatch::FdWatch */


FdWatch::~FdWatch(void)
{
  if (m_enabled)
  {
    Application::app().delFdWatch(this);
  }
} /* FdWatch::~FdWatch */


FdWatch& FdWatch::operator=(FdWatch&& other)
{
  //std::cout << "### FdWatch::operator=(FdWatch&&): other.fd()="
  //          << other.fd() << std::endl;
  bool other_was_enabled = other.m_enabled;
  setEnabled(false);
  other.setEnabled(false);
  m_fd = other.m_fd;
  other.m_fd = -1;
  m_type = other.m_type;
  other.m_type = FD_WATCH_RD;
  setEnabled(other_was_enabled);
  return *this;
} /* FdWatch::operator=(FdWatch&&) */


void FdWatch::setEnabled(bool enabled)
{
  if (!m_enabled && enabled)
  {
    assert(m_fd >= 0);
    Application::app().addFdWatch(this);
    m_enabled = enabled;
  }
  else if (m_enabled && !enabled)
  {
    Application::app().delFdWatch(this);
    m_enabled = enabled;
  }
} /* FdWatch::setEnabled */


void FdWatch::setFd(int fd, FdWatchType type)
{
  bool was_enabled = m_enabled;
  setEnabled(false);
  m_fd = fd;
  m_type = type;
  if (m_fd >= 0)
  {
    setEnabled(was_enabled);
  }
} /* FdWatch::setFd */



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
