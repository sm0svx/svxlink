/**
@file	 AsyncFileReader.h
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


#ifndef FILE_READER_INCLUDED
#define FILE_READER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <unistd.h>

#include <string>


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

class FdWatch;
  

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

class FileReader : public sigc::trackable
{
  public:

    /**
      * @brief   Constuctor
      * @param   buf_size The device name of the serial port to use
      *
      * This is the constructor for the file reader class. The buffer size
      * should be assigned at least twice as large as the maximum block size
      * to be read.
      */
    FileReader(int buf_size);

    /**
      * @brief   Destructor
      */
    ~FileReader(void);

    /**
      * @brief   Open a file for binary reading
      * @param   name The file name to be opened
      * @return  Return \em true on success or else \em false on failue.
      */
    bool open(const std::string& name);
    
    /**
      * @brief   Close a previously opened file
      * @return  Return \em true on success or else \em false on failue.
      */
    bool close(void);

    /**
     * @brief 	Check if a file is currently opened
     * @return	Returns \em true if a file is currently opened or
     *	      	\em false if no file has been opened or if the file
     *          was already closed.
     */
    bool isOpen(void) const { return (fd != -1); }
    
    /**
      * @brief   Read data from a previously opened file
      * @param   buf  A read target data buffer
      * @param   len  The number of bytes to be read
      * @return  The number of read bytes is returned on success. If an error
      *          occurs, -1 is returned.
      */
    int read(void *buf, int len);

      
  private:
    int     fd;
    FdWatch *rd_watch;
    char    *buffer;
    int     head, tail;
    int     buf_size;
    bool    is_full;
    bool    is_eof;
    
    void onDataAvail(FdWatch *watch);
    bool fillBuffer(void);
    int bytesInBuffer(void) const;
    
};  /* class FileReader */


} /* namespace */

#endif /* FILE_READER_INCLUDED */



/*
 * This file has not been truncated
 */

