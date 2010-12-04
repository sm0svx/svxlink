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

#include <ctime>
#include <cstdio>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSelector.h>
#include <AsyncAudioRecorder.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "QsoRecorder.h"



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

QsoRecorder::QsoRecorder(const string &rec_dir)
  : recorder(0), rec_dir(rec_dir)
{
  selector = new AudioSelector;
} /* QsoRecorder::QsoRecorder */


QsoRecorder::~QsoRecorder(void)
{
  setEnabled(false);
  delete selector;
} /* QsoRecorder::~QsoRecorder */


void QsoRecorder::addSource(Async::AudioSource *source, int prio)
{
  selector->addSource(source);
  selector->enableAutoSelect(source, prio);
} /* QsoRecorder::addSource */


void QsoRecorder::setEnabled(bool enable)
{
  if (enable && (recorder == 0))
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
    selector->registerSink(recorder, true);
    if (!recorder->initialize())
    {
      cerr << "*** QsoRecorder ERROR: Could not open file for recording: "
          << filename << endl;
    }
  }
  else if (recorder != 0)
  {
    delete recorder;
    recorder = 0;

    string oldpath(rec_dir + "/tmp_" + rec_timestamp + ".wav");
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

    rec_timestamp = "";
  }
} /* QsoRecorder::enable */



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



/*
 * This file has not been truncated
 */

