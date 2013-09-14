/**
@file	 QsoRecorder.cpp
@brief   The QSO recorder is used to write radio traffic to file
@author  Tobias Blomberg / SM0SVX
@date	 2009-06-06

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

namespace {
  int directory_filter(const struct dirent *ent);
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
    default_active(false), tmo_timer(0), logic(logic)
{
  selector = new AudioSelector;
} /* QsoRecorder::QsoRecorder */


QsoRecorder::~QsoRecorder(void)
{
  setEnabled(false);
  delete selector;
  delete tmo_timer;
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
    logic->idleStateChanged.connect(
        hide(mem_fun(*this, &QsoRecorder::checkTimeoutTimer)));
  }

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

  checkTimeoutTimer();
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

void QsoRecorder::maxRecordingTimeReached(void)
{
  closeFile();
  openFile();
} /* QsoRecorder::maxRecordingTimeReached */


void QsoRecorder::openFile(void)
{
  if (recorder == 0)
  {
    time_t epoch = time(NULL);
    struct tm *tm = localtime(&epoch);
    char timestamp[256];
    strftime(timestamp, sizeof(timestamp), "%y%m%d%H%M%S", tm);
    rec_timestamp = timestamp;
    string filename(rec_dir);
    filename += "/tmp_";
    filename += rec_timestamp;
    filename += ".wav";
    recorder = new AudioRecorder(filename);
    recorder->setMaxRecordingTime(hard_chunk_limit, soft_chunk_limit);
    recorder->maxRecordingTimeReached.connect(
        mem_fun(*this, &QsoRecorder::maxRecordingTimeReached));
    selector->registerSink(recorder, true);
    if (!recorder->initialize())
    {
      cerr << "*** QsoRecorder ERROR: Could not open file for recording: "
        << filename << endl;
    }
  }
} /* QsoRecorder::openFile */


void QsoRecorder::closeFile(void)
{
  if (recorder != 0)
  {
    recorder->closeFile();

    string oldpath(rec_dir + "/tmp_" + rec_timestamp + ".wav");
    if (recorder->samplesWritten() > 0)
    {
      time_t epoch = time(NULL);
      struct tm *tm = localtime(&epoch);
      char timestamp[256];
      strftime(timestamp, sizeof(timestamp), "%y%m%d%H%M%S", tm);
      string newpath(rec_dir + "/qsorec_" + rec_timestamp + "_" + timestamp +
                     ".wav");
      if (rename(oldpath.c_str(), newpath.c_str()) != 0)
      {
        perror("QsoRecorder rename");
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
    rec_timestamp = "";

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


void QsoRecorder::checkTimeoutTimer(void)
{
  if (tmo_timer != 0)
  {
    tmo_timer->setEnable((recorderIsActive() != default_active)
                         && logic->isIdle());
  }
} /* QsoRecorder::checkTimeoutTimer */



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
};



/*
 * This file has not been truncated
 */

