/**
@file	 AsyncFileReader.cpp
@brief   A class for asynchronous reading from binary files
@author  Tobias Blomberg / SM0SVX
@date	 2011-07-20

This file contains a class that is used for buffered reading
from binary files in a completely non-blocking way.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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
#include <errno.h>
#include <cstring>
#include <iostream>


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

#include "AsyncFileReader.h"



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

FileReader::FileReader(int buf_size)
  : fd(-1), rd_watch(0), buffer(0), head(0), tail(0), buf_size(buf_size),
    is_full(false), is_eof(false)
{
  buffer = new char [buf_size];
} /* FileReader::FileReader */


FileReader::~FileReader(void)
{
  close();
  delete [] buffer;
} /* FileReader::~FileReader */


bool FileReader::open(const string& name)
{
  close();
  
  fd = ::open(name.c_str(), O_RDONLY | O_NONBLOCK);
  if (fd == -1)
  {
    return false;
  }
  
  rd_watch = new FdWatch(fd, FdWatch::FD_WATCH_RD);
  rd_watch->activity.connect(mem_fun(*this, &FileReader::onDataAvail));

  return fillBuffer();
  
} /* FileReader::openPort */


bool FileReader::close(void)
{
  if (fd == -1)
  {
    return true;
  }
  
  if (::close(fd) < 0)
  {
    return false;
  }
  
  fd = -1;
  head = tail = 0;
  is_full = false;
  is_eof = false;
  
  delete rd_watch;
  return true;
  
} /* FileReader::close */


int FileReader::read(void *buf, int len)
{
  if (!fillBuffer())
  {
    return -1;
  }

  int avail = bytesInBuffer();
  if (!is_eof && (avail < len))
  {
    cerr << "FileReader: Buffer underrun" << endl;
    return -1;
  }
  
  int bytes_from_buffer = min(avail, len);
  int written = 0;
  while (bytes_from_buffer > 0)
  {
    int to_end_of_buffer = min(bytes_from_buffer, buf_size - tail);
    memcpy((char *)buf + written, buffer + tail, to_end_of_buffer);
      
    tail += to_end_of_buffer;
    tail %= buf_size;
    bytes_from_buffer -= to_end_of_buffer;
    written += to_end_of_buffer;
  }

  is_full &= (written == 0);

  return written;
}


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void FileReader::onDataAvail(FdWatch *watch)
{
  fillBuffer();

} /* FileReader::onDataAvail */


bool FileReader::fillBuffer(void)
{
  int space = buf_size - bytesInBuffer();

  int bytes_to_buffer = space;
  int written = 0;
  while ((bytes_to_buffer > 0) && isOpen())
  {
    int to_end_of_buffer = min(bytes_to_buffer, buf_size - head);
    int cnt = ::read(fd, buffer + head, to_end_of_buffer);

    if (cnt <= 0)
    {
      if (cnt < 0)
      {
        if (errno == EAGAIN)
          rd_watch->setEnabled(true);
        if ((errno == EIO) || (errno == EBADF) || (errno == EINVAL))
          close();
      }
      is_eof |= (cnt == 0);
      break;
    }
    
    head += cnt;
    head %= buf_size;
    bytes_to_buffer -= cnt;
    written += cnt;
  }
  
  if (written == space)
  {
    is_full = true;
    rd_watch->setEnabled(false);
  }

  return isOpen();

} /* FileReader::fillBuffer */


int FileReader::bytesInBuffer(void) const
{
  return is_full ? buf_size : (head - tail + buf_size) % buf_size;
  
} /* FileReader::bytesInBuffer */


/*
 * This file has not been truncated
 */
