/**
@file   LogWriter.h
@brief  A class for writing stdout and stderr to a logfile or syslog
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
#include <mutex>


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
@brief  A class for writing stdout and stderr to a logfile or syslog
@author Tobias Blomberg / SM0SVX
@date   2025-06-11

This is a threaded logging class for routing stdout and stderr to a logfile or
to syslog.

When writing to a file, each row will be prefixed with a timestamp.

When writing to syslog, the timestamp format in SvxLink is ignored.
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
     * @brief   Set the format used for writing timestamps to file
     * @param   format A strftime format string
     */
    void setTimestampFormat(const std::string& fmt);

    /**
     * @brief   Set the name of the logging destination
     * @param   name Either a path to a logfile or the literal "syslog:"
     */
    void setDestinationName(const std::string& name);

    /**
     * @brief   Start the logger thread
     */
    void start(void);

    /**
     * @brief   Stop the logger thread
     */
    void stop(void);

    /**
     * @brief   Reopen the logfile before writing the next log row
     */
    void reopenLogfile(void);

    /**
     * @brief   Call this function to redirect stdout to the logger
     */
    void redirectStdout(void);

    /**
     * @brief   Call this function to redirect stderr to the logger
     */
    void redirectStderr(void);

  private:
    class LogWriterWorker;
    class LogWriterWorkerFile;
    class LogWriterWorkerSyslog;

    std::string       m_dest_name;
    std::string       m_tstamp_format {"%c"};
    std::atomic_bool  m_reopen_log;
    int               m_pipefd[2]     {-1, -1};
    std::thread       m_logthread;
    std::mutex        m_mutex;
    LogWriterWorker*  m_worker        {nullptr};

    void writerThread(void);
    void logFlush(void);

}; /* class LogWriter */


#endif /* LOG_WRITER_INCLUDED */

/*
 * This file has not been truncated
 */
