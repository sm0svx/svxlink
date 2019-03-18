/**
@file	 LocalRx.cpp
@brief   A receiver class to handle local receivers
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that handle local analogue receivers. A local
receiver is a receiver that is directly connected to the sound card on the
computer where the SvxLink core is running.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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

#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioIO.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LocalRx.h"


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

LocalRx::LocalRx(Config &cfg, const std::string& name)
  : LocalRxBase(cfg, name), cfg(cfg), audio_io(0), ctrl_pty(0)
{
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  delete audio_io;
  audio_io = 0;
  if (ctrl_pty != 0) ctrl_pty->destroy();
} /* LocalRx::~LocalRx */


bool LocalRx::initialize(void)
{
  string audio_dev;
  if (!cfg.getValue(name(), "AUDIO_DEV", audio_dev))
  {
    cerr << "*** ERROR: Config variable " << name() << "/AUDIO_DEV not set\n";
    return false;
  }
  
  int audio_channel = 0;
  if (!cfg.getValue(name(), "AUDIO_CHANNEL", audio_channel))
  {
    cerr << "*** ERROR: Config variable " << name()
         << "/AUDIO_CHANNEL not set\n";
    return false;
  }
  
  string ctrl_pty_name;
  if (cfg.getValue(name(), "CTRL_PTY", ctrl_pty_name))
  {
    ctrl_pty = RefCountingPty::instance(ctrl_pty_name);
    if (ctrl_pty == 0)
    {
      cerr << "*** ERROR: Could not create control PTY in " << name() << endl;
      return false;
    }
  }

    // Create the audio IO object
    //FIXME: Check that the audio device is correctly initialized
    //       before continuing.
  audio_io = new AudioIO(audio_dev, audio_channel);

  if (!LocalRxBase::initialize())
  {
    return false;
  }

  return true;
  
} /* LocalRx:initialize */


void LocalRx::setFq(unsigned fq)
{
  if (ctrl_pty != 0)
  {
    ostringstream ss;
    ss << "f" << fq << ";";
    ssize_t ret = ctrl_pty->write(ss.str().c_str(), ss.str().size());
    if (ret != static_cast<ssize_t>(ss.str().size()))
    {
      cerr << "*** WARNING[" << name() << "]: Failed to write set receiver "
              "frequency command to PTY " << ctrl_pty->name()
           << ": " << ss.str() << endl;
    }
  }
} /* LocalRx::setFq */


void LocalRx::setModulation(Modulation::Type mod)
{
  if (ctrl_pty != 0)
  {
    ostringstream ss;
    ss << "m" << Modulation::toString(mod) << ";";
    ssize_t ret = ctrl_pty->write(ss.str().c_str(), ss.str().size());
    if (ret != static_cast<ssize_t>(ss.str().size()))
    {
      cerr << "*** WARNING[" << name() << "]: Failed to write set receiver "
              "modulation command to PTY " << ctrl_pty->name()
           << ": " << ss.str() << endl;
    }
  }
} /* LocalRx::setModulation */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

bool LocalRx::audioOpen(void)
{
    // Open the audio device for reading
  if (!audio_io->open(AudioIO::MODE_RD))
  {
    cerr << "*** ERROR: Could not open audio device for receiver \""
      	 << name() << "\"\n";
    // FIXME: Cleanup?
    return false;
  }
  return true;
} /* LocalRx::audioOpen */


void LocalRx::audioClose(void)
{
  audio_io->close();
} /* LocalRx::audioClose */


int LocalRx::audioSampleRate(void)
{
  return audio_io->sampleRate();
} /* LocalRx::audioSampleRate */


Async::AudioSource *LocalRx::audioSource(void)
{
  return audio_io;
} /* LocalRx::audioSource */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

