/**
@file	 AsyncPtyStreamBuf.h
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

/** @example AsyncPtyStreamBuf_demo.cpp
An example of how to use the AsyncPtyStreamBuf class
*/


#ifndef ASYNC_PTY_STREAM_BUF_INCLUDED
#define ASYNC_PTY_STREAM_BUF_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <streambuf>
#include <vector>


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



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Pty;
  

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	A stream buffer class to stream characters to a PTY
@author Tobias Blomberg / SM0SVX
@date   2014-12-20

This class can be used to write data to a UNIX 98 PTY using the standard
streaming interface. The typical usage pattern is to first create an instance
of a Pty. Then an instance of this class is created, the stream buffer. Lastly
a std::ostream can be created with the stream buffer as an argument to the
constructor. The std::ostream can then be used as any other output stream.

\include AsyncPtyStreamBuf_demo.cpp
*/
class PtyStreamBuf : public std::streambuf
{
  public:
    /**
     * @brief 	Default constructor
     * @param   pty A previously created PTY object
     * @param   buf_size The buffer size
     */
    explicit PtyStreamBuf(Pty *pty, std::size_t buf_size=256);
  
    /**
     * @brief 	Destructor
     */
    ~PtyStreamBuf(void);
  
    /**
     * @brief 	Return the PTY this stream is attached to
     * @return	The PTY
     */
    Pty *pty(void) const { return m_pty; }
    
  protected:
    
  private:
    Pty *             m_pty;
    std::vector<char> m_buf;

    PtyStreamBuf(const PtyStreamBuf&);
    PtyStreamBuf& operator=(const PtyStreamBuf&);

    virtual int_type overflow(int_type ch);
    virtual int sync(void);
    bool writeToPty(void);
    
};  /* class PtyStreamBuf */


} /* namespace */

#endif /* ASYNC_PTY_STREAM_BUF_INCLUDED */



/*
 * This file has not been truncated
 */
