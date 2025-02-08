/**
@file   LogWriter.h
@brief  A class for writing stdout and stderr to a logfile
@author Tobias Blomberg / SM0SVX
@date   2025-06-11

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

#ifndef LOG_WRITER_INCLUDED
#define LOG_WRITER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <thread>
#include <atomic>


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



/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



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
@brief  A class for writing stdout and stderr to a logfile
@author Tobias Blomberg / SM0SVX
@date   2025-06-11

This is a threaded logging class for routing stdout and stderr to a logfile.
Each row will be prefixed with a timestamp.
*/
class LogWriter
{
  public:
    /**
     * @brief   Default constructor
     */
    LogWriter(void);

    /**
     * @brief   Disallow copy construction
     */
    LogWriter(const LogWriter&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    LogWriter& operator=(const LogWriter&) = delete;

    /**
     * @brief   Destructor
     */
    ~LogWriter(void);

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    void setTimestampFormat(const std::string& format);

    void setFilename(const std::string& name);

    void start(void);

    void stop(void);

    void reopenLogfile(void);

    void redirectStdout(void);

    void redirectStderr(void);

    void writerThread(void);

  private:
    std::string       logfile_name;
    int               logfd           = -1;
    std::string       tstamp_format   = "%c";
    std::atomic_bool  reopen_logfile;
    int               pipefd[2]       = {-1, -1};
    std::thread       logthread;

    bool logfileOpen(void);
    void logfileClose(void);
    void logfileReopen(std::string reason);
    bool logfileWriteTimestamp(void);
    void logfileWrite(const char *buf);
    void logfileFlush(void);

}; /* class LogWriter */


#endif /* LOG_WRITER_INCLUDED */

/*
 * This file has not been truncated
 */
