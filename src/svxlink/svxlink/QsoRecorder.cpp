/**
@file	 QsoRecorder.cpp
@brief   The QSO recorder is used to write radio traffic to file
@author  Tobias Blomberg / SM0SVX
@date	 2009-06-06

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#include <dirent.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <ctime>
#include <cstdio>
#include <iostream>
#include <cstdlib>
#include <cstring>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSelector.h>
#include <AsyncAudioRecorder.h>
#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncExec.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "QsoRecorder.h"
#include "Logic.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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

class QsoRecorder::FileEncoder : public Exec
{
  public:
    string basename;
    FileEncoder(const char *shell, string basename)
      : Exec(shell), basename(basename)
    {}
};


/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

namespace {
  int directory_filter(const struct dirent *ent);
  void replace_all(std::string& str, const std::string& from,
                   const std::string& to);
};


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

QsoRecorder::QsoRecorder(Logic *logic)
  : recorder(0), hard_chunk_limit(0), soft_chunk_limit(0), max_dirsize(0),
    default_active(false), tmo_timer(0), logic(logic), qso_tmo_timer(0),
    min_samples(0)
{
  selector = new AudioSelector;
} /* QsoRecorder::QsoRecorder */


QsoRecorder::~QsoRecorder(void)
{
  setEnabled(false);
  delete selector;
  delete tmo_timer;
  delete qso_tmo_timer;
} /* QsoRecorder::~QsoRecorder */


bool QsoRecorder::initialize(const Config &cfg, const string &name)
{
  if (!cfg.getValue(name, "REC_DIR", rec_dir))
  {
    cerr << "*** ERROR: Config variable " << name << "/REC_DIR not set\n";
    return false;
  }

  unsigned max_time = 0;
  cfg.getValue(name, "MAX_TIME", max_time);
  unsigned soft_time = 0;
  cfg.getValue(name, "SOFT_TIME", soft_time);
  setMaxChunkTime(max_time, soft_time);

  unsigned max_dirsize = 0;
  cfg.getValue(name, "MAX_DIRSIZE", max_dirsize);
  setMaxRecDirSize(max_dirsize * 1024 * 1024);

  cfg.getValue(name, "DEFAULT_ACTIVE", default_active);
  setEnabled(default_active);

  unsigned timeout = 0;
  cfg.getValue(name, "TIMEOUT", timeout);
  if (timeout > 0)
  {
    tmo_timer = new Timer(1000 * timeout);
    tmo_timer->setEnable(false);
    tmo_timer->expired.connect(
        hide(mem_fun(*this, &QsoRecorder::timerExpired)));
  }

  unsigned qso_timeout = 0;
  cfg.getValue(name, "QSO_TIMEOUT", qso_timeout);
  if (qso_timeout > 0)
  {
    qso_tmo_timer = new Timer(1000 * qso_timeout);
    qso_tmo_timer->setEnable(false);
    qso_tmo_timer->expired.connect(
        hide(mem_fun(*this, &QsoRecorder::openNewFile)));
  }

  unsigned min_time = 0;
  cfg.getValue(name, "MIN_TIME", min_time);
  min_samples = min_time * INTERNAL_SAMPLE_RATE / 1000;

  cfg.getValue(name, "ENCODER_CMD", encoder_cmd);

  logic->idleStateChanged.connect(
      hide(mem_fun(*this, &QsoRecorder::checkTimeoutTimers)));

  return true;
} /* QsoRecorder::initialize */


void QsoRecorder::addSource(Async::AudioSource *source, int prio)
{
  selector->addSource(source);
  selector->enableAutoSelect(source, prio);
} /* QsoRecorder::addSource */


void QsoRecorder::setEnabled(bool enable)
{
  if ((recorder == 0) && enable)
  {
    cout << logic->name() << ": Activating QSO recorder\n";
    openFile();
  }
  else if ((recorder != 0) && !enable)
  {
    cout << logic->name() << ": Deactivating QSO recorder\n";
    closeFile();
  }

  checkTimeoutTimers();
} /* QsoRecorder::setEnabled */


void QsoRecorder::setMaxChunkTime(unsigned max_time, unsigned soft_time)
{
  hard_chunk_limit = max_time * 1000;
  if (max_time > soft_time)
  {
    soft_chunk_limit = (max_time - soft_time) * 1000;
  }
} /* QsoRecorder::setChunkTime */


void QsoRecorder::setMaxRecDirSize(unsigned max_size)
{
  max_dirsize = max_size;
} /* QsoRecorder::setMaxRecDirSize */



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

void QsoRecorder::openNewFile(void)
{
  closeFile();
  openFile();
} /* QsoRecorder::openNewFile */


void QsoRecorder::openFile(void)
{
  if (recorder == 0)
  {
    string filename(rec_dir);
    filename += "/.qsorec_";
    filename += logic->name();
    filename += ".wav";
    recorder = new AudioRecorder(filename);
    recorder->setMaxRecordingTime(hard_chunk_limit, soft_chunk_limit);
    recorder->maxRecordingTimeReached.connect(
        mem_fun(*this, &QsoRecorder::openNewFile));
    recorder->errorOccurred.connect(mem_fun(*this, &QsoRecorder::onError));
    selector->registerSink(recorder, true);
    if (!recorder->initialize())
    {
      cerr << "*** ERROR: Could not open QsoRecorder file \"" << filename 
           << " for writing in logic " << logic->name() << ": "
           << recorder->errorMsg() << endl;
    }
  }
} /* QsoRecorder::openFile */


void QsoRecorder::closeFile(void)
{
  if (recorder != 0)
  {
    string oldpath(rec_dir + "/.qsorec_" + logic->name() + ".wav");

    if (!recorder->closeFile())
    {
      cerr << "*** ERROR: Failed to close QsoRecorder file \"" << oldpath
           << "\" in logic " << logic->name() << ": " << recorder->errorMsg()
           << endl;
    }

    if (recorder->samplesWritten() > min_samples)
    {
      string basename("qsorec_" + logic->name() + "_");

      const struct timeval &begin_time = recorder->beginTimestamp();
      struct tm tm;
      localtime_r(&begin_time.tv_sec, &tm);
      char timestamp[256];
      strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H%M%S", &tm);
      basename += timestamp;

      basename += "_";

      const struct timeval &end_time = recorder->endTimestamp();
      localtime_r(&end_time.tv_sec, &tm);
      strftime(timestamp, sizeof(timestamp), "%Y-%m-%d_%H%M%S", &tm);
      basename += timestamp;
      string newpath = rec_dir + "/" + basename + ".wav";
      if (rename(oldpath.c_str(), newpath.c_str()) != 0)
      {
        perror("QsoRecorder rename");
      }

      cout << logic->name() << ": Wrote QSO recorder file "
           << basename << ".wav\n";

        // Execute external audio file handler (e.g. encoder) if configured
      if (!encoder_cmd.empty())
      {
        cout << logic->name() << ": Starting encoding for file "
             << basename << ".wav\n";
        const char *shell = getenv("SHELL");
        if (shell == NULL)
        {
          shell = "/bin/sh";
        }
        FileEncoder *enc = new FileEncoder(shell, basename);
        enc->appendArgument("-c");
        string cmdline(encoder_cmd);
        replace_all(cmdline, "%f", newpath);
        replace_all(cmdline, "%d", rec_dir);
        replace_all(cmdline, "%b", basename);
        replace_all(cmdline, "%n", basename + ".wav");
        enc->appendArgument(cmdline);
        enc->stdoutData.connect(
            mem_fun(*this, &QsoRecorder::handleEncoderPrintouts));
        enc->stderrData.connect(
            mem_fun(*this, &QsoRecorder::handleEncoderPrintouts));
        enc->exited.connect(
            sigc::bind(mem_fun(*this, &QsoRecorder::encoderExited), enc));
        enc->nice();
        enc->setTimeout(60*60); // One hour timeout
        enc->run();
      }
    }
    else
    {
      if (unlink(oldpath.c_str()) != 0)
      {
        perror("QsoRecorder unlink");
      }
    }

    delete recorder;
    recorder = 0;

    cleanupDirectory();
  }
} /* QsoRecorder::closeFile */


void QsoRecorder::cleanupDirectory(void)
{
  if (max_dirsize == 0)
  {
    return;
  }

  struct dirent **namelist;
  int n = scandir(rec_dir.c_str(), &namelist, directory_filter, alphasort);
  if (n < 0)
  {
    perror("QsoRecorder scandir");
    return;
  }

  unsigned tot_size = 0;
  for (int i=0; i<n; ++i)
  {
    int idx = n - i - 1;
    //cout << namelist[idx]->d_name;
    string path(rec_dir);
    path += "/";
    path += namelist[idx]->d_name;
    free(namelist[idx]);

    struct stat buf;
      // coverity[fs_check_call]
    if (stat(path.c_str(), &buf) < 0)
    {
      perror("QsoRecorder stat");
      continue;
    }
    tot_size += buf.st_size;
    //cout << " " << buf.st_size;
    //cout << " " << tot_size;
    if (tot_size > max_dirsize)
    {
      //cout << " (delete)";
      unlink(path.c_str());
    }
    //cout << endl;
  }
  free(namelist);
} /* QsoRecorder::cleanupDirectory */


void QsoRecorder::timerExpired(void)
{
  if (recorderIsActive() != default_active)
  {
    string ev("qso_recorder_timeout_");
    ev += (default_active ? "activate" : "deactivate");
    logic->processEvent(ev);
    setEnabled(default_active);
  }
} /* QsoRecorder::timerExpired */


void QsoRecorder::checkTimeoutTimers(void)
{
  if (tmo_timer != 0)
  {
    tmo_timer->setEnable((recorderIsActive() != default_active)
                         && logic->isIdle());
  }

  if (qso_tmo_timer != 0)
  {
    qso_tmo_timer->setEnable(recorderIsActive()
                             && (recorder->samplesWritten() > 0)
                             && logic->isIdle());
  }
} /* QsoRecorder::checkTimeoutTimers */


void QsoRecorder::handleEncoderPrintouts(const char *buf, int cnt)
{
  cout << buf;
} /* Exec::handleEncoderPrintouts */


void QsoRecorder::encoderExited(QsoRecorder::FileEncoder *enc)
{
  cout << logic->name() << ": Encoding done for file "
             << enc->basename << ".wav\n";
  if (enc->ifExited() && (enc->exitStatus() != 0))
  {
    cerr << "*** ERROR: QSO recorder external audio file handler in logic "
         << logic->name() << " failed with "
         << "exit code " << enc->exitStatus() << endl;
  }
  else if (enc->ifSignaled())
  {
    cerr << "*** ERROR: QSO recorder external audio file handler in logic "
         << logic->name() << " exited on "
         << "signal " << enc->termSig() << endl;
  }
  delete enc;
} /* QsoRecorder::encoderExited */


void QsoRecorder::onError(void)
{
  cerr << "*** ERROR: The QsoRecorder in logic " << logic->name() 
       << " failed: " << recorder->errorMsg() << endl;
} /* QsoRecorder::onError */



/****************************************************************************
 *
 * Private functions
 *
 ****************************************************************************/

namespace {
  int directory_filter(const struct dirent *ent)
  {
    return strstr(ent->d_name, "qsorec_") == ent->d_name;
  } /* directory_filter */

  void replace_all(std::string& str, const std::string& from,
                   const std::string& to)
  {
    if(from.empty())
    {
      return;
    }
    size_t start_pos = 0;
    while((start_pos = str.find(from, start_pos)) != std::string::npos)
    {
      str.replace(start_pos, from.length(), to);
      start_pos += to.length();
    }
  } /* replace_all */
};



/*
 * This file has not been truncated
 */

