/**
@file   LogWriter.cpp
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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include <syslog.h>

#include <cstring>
#include <iostream>
#include <iomanip>
#include <sstream>


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

#include "LogWriter.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Static class variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

namespace {


/****************************************************************************
 *
 * Local functions
 *
 ****************************************************************************/

class LogWriterWorker
{
  public:
    virtual ~LogWriterWorker(void) {}
    virtual bool logOpen(void) { return true; }
    virtual void logClose(void) {}
    virtual void logReopen(std::string reason) {}
    virtual bool logWriteTimestamp(void) { return true; }
    virtual void logWrite(const char *buf) = 0;
};


class LogWriterWorkerFile : public LogWriterWorker
{
  public:
    LogWriterWorkerFile(const std::string& filename,
                        const std::string& tstamp_format);
    virtual ~LogWriterWorkerFile(void) {}
    virtual bool logOpen(void) override;
    virtual void logClose(void) override;
    virtual void logReopen(std::string reason) override;
    virtual bool logWriteTimestamp(void) override;
    virtual void logWrite(const char *buf) override;

  private:
    std::string       m_logfile_name;
    int               m_logfd           = -1;
    std::string       m_tstamp_format;
};


class LogWriterWorkerSyslog : public LogWriterWorker
{
  public:
    virtual ~LogWriterWorkerSyslog(void) {}
    virtual void logWrite(const char *buf) override;
  private:
    std::string   m_buf;
};


}; /* End of anonymous namespace */


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

LogWriter::LogWriter(void)
{
    // Create a pipe to route stdout and stderr through
  if (pipe(m_pipefd) == -1)
  {
    perror("pipe");
    exit(1);
  }
} /* LogWriter::LogWriter */


LogWriter::~LogWriter(void)
{
  stop();
} /* LogWriter::~LogWriter */


void LogWriter::setTimestampFormat(const std::string& format)
{
  m_tstamp_format = format;
} /* LogWriter::setTimestampFormat */


void LogWriter::setDestinationName(const std::string& name)
{
  m_dest_name = name;
} /* LogWriter::setDestinationName */


void LogWriter::start(void)
{
  m_logthread = std::thread(&LogWriter::writerThread, this);
} /* LogWriter::start */


void LogWriter::stop(void)
{
  logFlush();

  if (m_pipefd[1] != -1)
  {
    write(m_pipefd[1], "\0", 1);
    close(m_pipefd[1]);
    m_pipefd[1] = -1;
  }

  if (m_logthread.joinable())
  {
    m_logthread.join();
  }

  if (m_pipefd[0] != -1)
  {
    close(m_pipefd[0]);
    m_pipefd[0] = -1;
  }

  //worker->logClose();
} /* LogWriter::stop */


void LogWriter::reopenLogfile(void)
{
  m_reopen_log = true;
} /* LogWriter::reopenLogfile */


void LogWriter::redirectStdout(void)
{
    // Redirect stdout to the logpipe
  if (dup2(m_pipefd[1], STDOUT_FILENO) == -1)
  {
    perror("dup2(stdout)");
    exit(1);
  }

    // Force stdout to line buffered mode
  if (setvbuf(stdout, NULL, _IOLBF, 0) != 0)
  {
    perror("setvbuf(stdout, NULL, _IOLBF, 0)");
    exit(1);
  }
} /* LogWriter::redirectStdout */


void LogWriter::redirectStderr(void)
{
    // Redirect stderr to the logpipe
  if (dup2(m_pipefd[1], STDERR_FILENO) == -1)
  {
    perror("dup2(stderr)");
    exit(1);
  }
} /* LogWriter::redirectStderr */



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

void LogWriter::writerThread(void)
{
  LogWriterWorker* worker = nullptr;
  if (m_dest_name == "syslog:")
  {
    worker = new LogWriterWorkerSyslog;
  }
  else
  {
    worker = new LogWriterWorkerFile(m_dest_name, m_tstamp_format);
  }

  if (!worker->logOpen())
  {
    abort();
  }

  for (;;)
  {
    char buf[256];
    auto len = read(m_pipefd[0], buf, sizeof(buf)-1);
    if ((len <= 0) || (buf[len-1] == '\0'))
    {
      //if (len < 0)
      //{
      //  char err[256];
      //  char* errp = strerror_r(errno, err, sizeof(err));
      //  std::stringstream ss;
      //  ss << "*** ERROR: Logpipe read failed with [" << errno
      //     << "]: " << errp << std::endl;
      //  worker->logWrite(ss.str().c_str());
      //}
      break;
    }
    buf[len] = 0;
    worker->logWrite(buf);
    if (m_reopen_log)
    {
      worker->logReopen("Reopen requested");
      m_reopen_log = false;
    }
  }

  //worker->logWrite("### logwriter exited\n");
  delete worker;
  worker = nullptr;
} /* LogWriter::writerThread */


void LogWriter::logFlush(void)
{
  std::cout.flush();
  std::cerr.flush();
} /* LogWriter::logFlush */


namespace {


LogWriterWorkerFile::LogWriterWorkerFile(const std::string& filename,
                                         const std::string& tstamp_format)
  : m_logfile_name(filename), m_tstamp_format(tstamp_format)
{
} /* LogWriterWorkerFile::LogWriterWorkerFile */


bool LogWriterWorkerFile::logOpen(void)
{
  logClose();
  m_logfd = open(m_logfile_name.c_str(), O_WRONLY | O_APPEND | O_CREAT, 00644);
  if (m_logfd == -1)
  {
    //std::cerr << "open(\"" << m_logfile_name << "\"): " << strerror(errno)
    //          << std::endl;
    return false;
  }

  return true;
} /* LogWriterWorkerFile::logOpen */


void LogWriterWorkerFile::logClose(void)
{
  if (m_logfd != -1)
  {
    close(m_logfd);
    m_logfd = -1;
  }
} /* LogWriterWorkerFile::logClose */


void LogWriterWorkerFile::logReopen(std::string reason)
{
  if (!reason.empty())
  {
    reason += ". ";
  }

  logWriteTimestamp();
  std::string msg(reason);
  msg += "Reopening logfile...\n";
  if (write(m_logfd, msg.c_str(), msg.size()) == -1)
  {
    abort();
  }

  if (!logOpen())
  {
    abort();
  }

  logWriteTimestamp();
  msg = reason;
  msg += "Logfile reopened.\n";
  if (write(m_logfd, msg.c_str(), msg.size()) == -1)
  {
    abort();
  }
} /* LogWriterWorkerFile::logReopen */


bool LogWriterWorkerFile::logWriteTimestamp(void)
{
  if (!m_tstamp_format.empty())
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    std::string fmt(m_tstamp_format);
    const std::string frac_code("%f");
    size_t pos = fmt.find(frac_code);
    if (pos != std::string::npos)
    {
      std::stringstream ss;
      ss << std::setfill('0') << std::setw(3) << (tv.tv_usec / 1000);
      fmt.replace(pos, frac_code.length(), ss.str());
    }
    struct tm tm;
    char tstr[256];
    size_t tlen = strftime(tstr, sizeof(tstr), fmt.c_str(),
                           localtime_r(&tv.tv_sec, &tm));
    ssize_t ret = write(m_logfd, tstr, tlen);
    if (ret != static_cast<ssize_t>(tlen))
    {
      return false;
    }
    ret = write(m_logfd, ": ", 2);
    if (ret != 2)
    {
      return false;
    }
  }
  return true;
} /* LogWriterWorkerFile::logWriteTimestamp */


void LogWriterWorkerFile::logWrite(const char *buf)
{
  if (m_logfd == -1)
  {
    return;
  }

  const char *ptr = buf;
  while (*ptr != 0)
  {
    static bool print_timestamp = true;
    ssize_t ret;

    if (print_timestamp)
    {
      if (!logWriteTimestamp())
      {
        logReopen("Write error");
        return;
      }
      print_timestamp = false;
    }

    size_t write_len = 0;
    const char *nl = strchr(ptr, '\n');
    if (nl != 0)
    {
      write_len = nl-ptr+1;
      print_timestamp = true;
    }
    else
    {
      write_len = strlen(ptr);
    }
    ret = write(m_logfd, ptr, write_len);
    if (ret != static_cast<ssize_t>(write_len))
    {
      logReopen("Write error");
      return;
    }
    ptr += write_len;
  }
} /* LogWriterWorkerFile::logWrite */


void LogWriterWorkerSyslog::logWrite(const char* buf)
{
  m_buf.append(buf);

  std::string::size_type pos;
  while ((pos = m_buf.find ('\n')) != std::string::npos)
  {
    std::string line(m_buf, 0, pos);
    m_buf.erase(0, pos + 1);

    int loglevel = LOG_INFO;
    if (line.rfind("*** ERROR:", 0) == 0)
    {
      loglevel = LOG_ERR;
    }
    else if (line.rfind("*** WARNING:", 0) == 0)
    {
      loglevel = LOG_WARNING;
    }
    else if (line.rfind("### ", 0) == 0)
    {
      loglevel = LOG_DEBUG;
    }
    syslog(LOG_DAEMON | loglevel, "%s", line.c_str());
  }
} /* LogWriterWorkerSyslog::logWrite */


}; /* End of anonymous namespace */


/*
 * This file has not been truncated
 */
