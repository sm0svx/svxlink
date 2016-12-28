/**
@file	 LocalRxSim.cpp
@brief   A class to simulate a local receiver
@author  Tobias Blomberg / SM0SVX
@date	 2015-10-03

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
#include <AsyncAudioPacer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LocalRxSim.h"


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

unsigned int LocalRxSim::next_seed = 0;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

LocalRxSim::LocalRxSim(Config &cfg, const std::string& name)
  : LocalRxBase(cfg, name), cfg(cfg), pacer(0)
{
} /* LocalRxSim::LocalRxSim */


LocalRxSim::~LocalRxSim(void)
{
} /* LocalRxSim::~LocalRxSim */


bool LocalRxSim::initialize(void)
{
  int tone_fq = 1000;
  cfg.getValue(name(), "SIM_TONE_FQ", tone_fq);
  audio_gen.setFq(tone_fq);

  string waveform("SIN");
  cfg.getValue(name(), "SIM_WAVEFORM", waveform);
  if (waveform == "SIN")
  {
    audio_gen.setWaveform(AudioGenerator::SIN);
  }
  else if (waveform == "SQUARE")
  {
    audio_gen.setWaveform(AudioGenerator::SQUARE);
  }
  else
  {
    cerr << "*** ERROR: Unknown waveform specified in "
         << name() << "/" << "SIM_WAVEFORM (\"" << waveform 
         << "\"). Valid values are: SIN, SQUARE.\n";
    return false;
  }

  float sim_tone_pwr_db = -20.0f;
  cfg.getValue(name(), "SIM_TONE_PWR", sim_tone_pwr_db);
  audio_gen.setPower(sim_tone_pwr_db);

  pacer = new Async::AudioPacer(INTERNAL_SAMPLE_RATE, 128, 0);
  audio_gen.registerSink(pacer, true);

  if (!LocalRxBase::initialize())
  {
    return false;
  }

  return true;
  
} /* LocalRxSim:initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

bool LocalRxSim::audioOpen(void)
{
  audio_gen.enable(true);
  return true;
} /* LocalRxSim::audioOpen */


void LocalRxSim::audioClose(void)
{
  audio_gen.enable(false);
} /* LocalRxSim::audioClose */


int LocalRxSim::audioSampleRate(void)
{
  return INTERNAL_SAMPLE_RATE;
} /* LocalRxSim::audioSampleRate */


Async::AudioSource *LocalRxSim::audioSource(void)
{
  return pacer;
} /* LocalRxSim::audioSource */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

