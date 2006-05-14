/**
@file	 LocalRx.cpp
@brief   A receiver class to handle local receivers
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that handle local receivers. A local receiver is
a receiver that is directly connected to the sound card on the computer where
the SvxLink core is running.

\verbatim
<A brief description of the program or library this file belongs to>
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cassert>
#include <cmath>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioIO.h>
#include <AudioFilter.h>
#include <SigCAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDet.h"
#include "DtmfDecoder.h"
#include "ToneDetector.h"
#include "SquelchVox.h"
#include "SquelchCtcss.h"
#include "SquelchSerial.h"
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

class ToneDurationDet : public ToneDetector
{
  public:
    ToneDurationDet(float fq, int bw, int duration)
      : ToneDetector(fq, 8000 / bw), required_duration(duration)
    {
      timerclear(&activation_timestamp);
      ToneDetector::activated.connect(
      	      slot(this, &ToneDurationDet::toneActivated));
    }
    
    SigC::Signal1<void, float> detected;
    
  private:
    int     	    required_duration;
    struct timeval  activation_timestamp;
    
    void toneActivated(bool is_activated)
    {
      //printf("%.1f tone %s...\n", toneFq(),
      //	     is_activated ? "ACTIVATED" : "DEACTIVATED");
      if (is_activated)
      {
	gettimeofday(&activation_timestamp, NULL);
      }
      else
      {
      	assert(timerisset(&activation_timestamp));
	struct timeval tv, tv_diff;
	gettimeofday(&tv, NULL);
	timersub(&tv, &activation_timestamp, &tv_diff);
	long diff = tv_diff.tv_sec * 1000 + tv_diff.tv_usec / 1000;
	//printf("The %d tone was active for %ld milliseconds\n",
	//      	 toneFq(), diff);
	if (diff >= required_duration)
	{
	  detected(toneFq());
	}
      	timerclear(&activation_timestamp);
      }
    }
};




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
  : Rx(cfg, name), audio_io(0), is_muted(true), dtmf_dec(0),
    squelch(0), siglevdet(0), siglev_offset(0.0), siglev_slope(1.0),
    deemph_filt(0), sigc_sink(0)
{
  resetHighpassFilter();
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  list<ToneDurationDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    delete *it;
  }
  tone_detectors.clear();
  
  delete siglevdet;
  delete squelch;
  delete dtmf_dec;
  delete sigc_sink;
  delete deemph_filt;
  delete audio_io;
} /* LocalRx::~LocalRx */


bool LocalRx::initialize(void)
{
  string audio_dev;
  if (!cfg().getValue(name(), "AUDIO_DEV", audio_dev))
  {
    cerr << "*** ERROR: Config variable " << name() << "/AUDIO_DEV not set\n";
    return false;
  }
  
  string sql_det_str;
  if (!cfg().getValue(name(), "SQL_DET", sql_det_str))
  {
    cerr << "*** ERROR: Config variable " << name() << "/SQL_DET not set\n";
    return false;
  }

  if (sql_det_str == "VOX")
  {
    squelch = new SquelchVox;
  }
  else if (sql_det_str == "CTCSS")
  {
    squelch = new SquelchCtcss;
  }
  else if (sql_det_str == "SERIAL")
  {
    squelch = new SquelchSerial;
  }
  else
  {
    cerr << "*** ERROR: Unknown squelch type specified in config variable "
      	 << name() << "/SQL_DET. Legal values are: VOX, CTCSS and SERIAL\n";
    return false;
  }
  
  string value;
  if (cfg().getValue(name(), "SIGLEV_OFFSET", value))
  {
    siglev_offset = atof(value.c_str());
  }
  
  if (cfg().getValue(name(), "SIGLEV_SLOPE", value))
  {
    siglev_slope = atof(value.c_str());
  }
  
  bool deemphasis = false;
  if (cfg().getValue(name(), "DEEMPHASIS", value))
  {
    deemphasis = (atoi(value.c_str()) != 0);
  }
  
  if (!squelch->initialize(cfg(), name()))
  {
    cerr << "*** ERROR: Squelch detector initialization failed for RX \""
      	 << name() << "\"\n";
    delete squelch;
    squelch = 0;
    return false;
  }
    
  audio_io = new AudioIO(audio_dev);
  AudioSource *prev_src = audio_io;

  if (deemphasis)
  {
    deemph_filt = new AudioFilter("LpBu1/300");
    deemph_filt->registerSource(prev_src);
    prev_src = deemph_filt;
  }
  
  sigc_sink = new SigCAudioSink;
  sigc_sink->registerSource(prev_src);
  sigc_sink->sigWriteSamples.connect(slot(this, &LocalRx::audioRead));
  
  dtmf_dec = new DtmfDecoder;
  dtmf_dec->digitDetected.connect(dtmfDigitDetected.slot());
  
  siglevdet = new SigLevDet;
  
  return true;
  
} /* LocalRx:initialize */


void LocalRx::mute(bool do_mute)
{
  if (do_mute == is_muted)
  {
    return;
  }
  
  if (do_mute)
  {
    audio_io->close();
  }
  else
  {
    if (!audio_io->open(AudioIO::MODE_RD))
    {
      cerr << "*** Error: Could not open audio device for receiver \""
      	   << name() << "\"\n";
      return;
    }
    
    squelch->reset();
  }

  is_muted = do_mute;

} /* LocalRx::mute */


bool LocalRx::squelchIsOpen(void) const
{
  return squelch->isOpen();
} /* LocalRx::squelchIsOpen */


bool LocalRx::addToneDetector(float fq, int bw, float thresh,
      	      	      	      int required_duration)
{
  //printf("Adding tone detector with fq=%d  bw=%d  req_dur=%d\n",
  //    	 fq, bw, required_duration);
  ToneDurationDet *det = new ToneDurationDet(fq, bw, required_duration);
  assert(det != 0);
  det->setSnrThresh(thresh);
  det->detected.connect(toneDetected.slot());
  
  tone_detectors.push_back(det);
  
  return true;

} /* LocalRx::addToneDetector */


float LocalRx::signalStrength(void) const
{
  return siglev_offset - siglev_slope * log10(siglevdet->lastSiglev());
} /* LocalRx::signalStrength */
    

void LocalRx::reset(void)
{
  mute(true);
  
  list<ToneDurationDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    delete *it;
  }
  tone_detectors.clear();
  
} /* LocalRx::reset */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

int LocalRx::audioRead(float *samples, int count)
{
  bool was_open = squelch->isOpen();
  
  siglevdet->writeSamples(samples, count);
    
  squelch->audioIn(samples, count);
  if (!was_open && squelch->isOpen())
  {
    setSquelchState(true);
    siglevdet->reset();
  }
  
  dtmf_dec->processSamples(samples, count);
  
  list<ToneDurationDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    (*it)->writeSamples(samples, count);
  }
  
  if (squelch->isOpen() && !is_muted)
  {
    float filtered[count];
    for (int i=0; i<count; ++i)
    {
      filtered[i] = samples[i];
    }
    highpassFilter(filtered, count);
    count = audioReceived(filtered, count);
  }
  
  if (was_open && !squelch->isOpen())
  {
    setSquelchState(false);
  }
  
  return count;
  
} /* LocalRx::audioRead */




/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkfilter -Bu -Hp -o 4 \
      	      	  -a 3.7500000000e-02 0.0000000000e+00 -l

      filtertype  = Butterworth
      passtype	  = Highpass
      ripple  	  = 
      order   	  = 4
      samplerate  = 8000
      corner1 	  = 400
      corner2 	  = 
      adzero  	  = 
      logmin  	  = -60


*/

#define GAIN   1.361640944e+00


void LocalRx::resetHighpassFilter(void)
{
  for (int i=0; i<NPOLES+1; ++i)
  {
    xv[i] = 0.0;
    yv[i] = 0.0;
  }
} /* LocalRx::resetHighpassFilter */


void LocalRx::highpassFilter(float *samples, int count)
{ 
  for (int i=0; i<count; ++i)
  {
    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
    xv[4] = samples[i] / GAIN;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
    yv[4] = (xv[0] + xv[4]) - 4 * (xv[1] + xv[3]) + 6 * xv[2]
                   + (-0.5393551283 * yv[0]) + (2.4891382938 * yv[1])
                   + (-4.3370618174 * yv[2]) + (3.3849727283 * yv[3]);
    /*
    if (yv[4] > 1.0)
    {
      printf("*** Distorsion: %.1f\n", yv[4]);
    }
    */
    samples[i] = yv[4];
  }
} /* LocalRx::highpassFilter */



/*
 * This file has not been truncated
 */

