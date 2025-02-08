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



}; /* End of anonymous namespace */

/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

LogWriter::LogWriter(void)
{
    // Create a pipe to route stdout and stderr through
  if (pipe(pipefd) == -1)
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
  tstamp_format = format;
} /* LogWriter::setTimestampFormat */


void LogWriter::setFilename(const std::string& name)
{
  logfile_name = name;
} /* LogWriter::setFilename */


void LogWriter::start(void)
{
  logthread = std::thread(&LogWriter::writerThread, this);
} /* LogWriter::start */


void LogWriter::stop(void)
{
  logfileFlush();

  if (pipefd[1] != -1)
  {
    write(pipefd[1], "\0", 1);
    close(pipefd[1]);
    pipefd[1] = -1;
  }

  if (logthread.joinable())
  {
    logthread.join();
  }

  if (pipefd[0] != -1)
  {
    close(pipefd[0]);
    pipefd[0] = -1;
  }

  logfileClose();
} /* LogWriter::stop */


void LogWriter::reopenLogfile(void)
{
  reopen_logfile = true;
} /* LogWriter::reopenLogfile */


void LogWriter::redirectStdout(void)
{
    // Redirect stdout to the logpipe
  if (dup2(pipefd[1], STDOUT_FILENO) == -1)
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
  if (dup2(pipefd[1], STDERR_FILENO) == -1)
  {
    perror("dup2(stderr)");
    exit(1);
  }
} /* LogWriter::redirectStderr */


void LogWriter::writerThread(void)
{
  if (!logfileOpen())
  {
    abort();
  }

  for (;;)
  {
    char buf[256];
    auto len = read(pipefd[0], buf, sizeof(buf)-1);
    if ((len <= 0) || (buf[len-1] == '\0'))
    {
      //if (len < 0)
      //{
      //  char err[256];
      //  char* errp = strerror_r(errno, err, sizeof(err));
      //  std::stringstream ss;
      //  ss << "*** ERROR: Logpipe read failed with [" << errno
      //     << "]: " << errp << std::endl;
      //  logfileWrite(ss.str().c_str());
      //}
      break;
    }
    buf[len] = 0;
    logfileWrite(buf);
    if (reopen_logfile)
    {
      logfileReopen("Reopen requested");
      reopen_logfile = false;
    }
  }

  //logfileWrite("### logwriter exited\n");
} /* LogWriter::writerThread */




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

bool LogWriter::logfileOpen(void)
{
  logfileClose();
  logfd = open(logfile_name.c_str(), O_WRONLY | O_APPEND | O_CREAT, 00644);
  if (logfd == -1)
  {
    //std::cerr << "open(\"" << logfile_name << "\"): " << strerror(errno)
    //          << std::endl;
    return false;
  }

  return true;
} /* LogWriter::logfileOpen */


void LogWriter::logfileClose(void)
{
  if (logfd != -1)
  {
    close(logfd);
    logfd = -1;
  }
} /* LogWriter::logfileClose */


void LogWriter::logfileReopen(std::string reason)
{
  if (!reason.empty())
  {
    reason += ". ";
  }

  logfileWriteTimestamp();
  std::string msg(reason);
  msg += "Reopening logfile...\n";
  if (write(logfd, msg.c_str(), msg.size()) == -1)
  {
    abort();
  }

  if (!logfileOpen())
  {
    abort();
  }

  logfileWriteTimestamp();
  msg = reason;
  msg += "Logfile reopened.\n";
  if (write(logfd, msg.c_str(), msg.size()) == -1)
  {
    abort();
  }
} /* LogWriter::logfileReopen */


bool LogWriter::logfileWriteTimestamp(void)
{
  if (!tstamp_format.empty())
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    std::string fmt(tstamp_format);
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
    ssize_t ret = write(logfd, tstr, tlen);
    if (ret != static_cast<ssize_t>(tlen))
    {
      return false;
    }
    ret = write(logfd, ": ", 2);
    if (ret != 2)
    {
      return false;
    }
  }
  return true;
} /* LogWriter::logfileWriteTimestamp */


void LogWriter::logfileWrite(const char *buf)
{
  if (logfd == -1)
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
      if (!logfileWriteTimestamp())
      {
        logfileReopen("Write error");
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
    ret = write(logfd, ptr, write_len);
    if (ret != static_cast<ssize_t>(write_len))
    {
      logfileReopen("Write error");
      return;
    }
    ptr += write_len;
  }
} /* LogWriter::logfileWrite */


void LogWriter::logfileFlush(void)
{
  std::cout.flush();
  std::cerr.flush();
} /* LogWriter::logfileFlush */


/*
 * This file has not been truncated
 */
