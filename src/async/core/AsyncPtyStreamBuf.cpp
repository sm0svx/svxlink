/**
@file	 AsyncPtyStreamBuf.cpp
@brief   A stream buffer for writing to a PTY
@author  Tobias Blomberg / SM0SVX
@date	 2014-12-20

\verbatim
Async - A library for programming event driven applications
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

#include <functional>
#include <cassert>
#include <iostream>


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

#include "AsyncPtyStreamBuf.h"
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

PtyStreamBuf::PtyStreamBuf(Pty *pty, size_t buf_size)
  : m_pty(pty), m_buf(buf_size + 1)
{
  assert(m_pty != 0);
  char *base = &m_buf.front();
  setp(base, base + m_buf.size() - 1);
} /* PtyStreamBuf::PtyStreamBuf */


PtyStreamBuf::~PtyStreamBuf(void)
{
} /* PtyStreamBuf::~PtyStreamBuf */




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

PtyStreamBuf::int_type PtyStreamBuf::overflow(int_type ch)
{
  if (m_pty->isOpen() && (ch != traits_type::eof()))
  {
    assert(std::less_equal<char *>()(pptr(), epptr()));
    *pptr() = ch;
    pbump(1);
    if (writeToPty())
    {
      return ch;
    }
  }

  return traits_type::eof();
} /* PtyStreamBuf::overflow */


int PtyStreamBuf::sync(void)
{
  return (m_pty->isOpen() && writeToPty()) ? 0 : -1;
} /* PtyStreamBuf::sync */


bool PtyStreamBuf::writeToPty(void)
{
  ptrdiff_t n = pptr() - pbase();
  pbump(-n);
  ssize_t written = m_pty->write(pbase(), n);
  return (written == n);
} /* PtyStreamBuf::writeToPty */



/*
 * This file has not been truncated
 */

