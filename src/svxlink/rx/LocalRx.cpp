/**
@file	 LocalRx.cpp
@brief   A receiver class to handle local receivers
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that handle local receivers. A local receiver is
a receiver that is directly connected to the sound card on the computer where
the SvxLink core is running.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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
#include <AudioSplitter.h>
#include <AsyncAudioAmp.h>
#include <AsyncAudioPassthrough.h>


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

#define DTMF_MUTING_PRE   100
#define DTMF_MUTING_POST  100

#if DTMF_MUTING_PRE > DTMF_MUTING_POST
#define DTMF_DELAY_LINE_LEN DTMF_MUTING_PRE
#else
#define DTMF_DELAY_LINE_LEN DTMF_MUTING_POST
#endif


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


class SigCAudioValve : public AudioValve, public SigC::Object
{
  public:
    void allSamplesFlushed(void)
    {
      AudioValve::allSamplesFlushed();
      sigAllSamplesFlushed();
    }
    
    SigC::Signal0<void> sigAllSamplesFlushed;
};


class PeakMeter : public AudioPassthrough
{
  public:
    PeakMeter(const string& name) : name(name) {}
    
    int writeSamples(const float *samples, int count)
    {
      int ret = sinkWriteSamples(samples, count);
      
      int i;
      for (i=0; i<ret; ++i)
      {
      	if (abs(samples[i]) > 0.997)
	{
	  break;
	}
      }
      
      if (i < count)
      {
      	cout << name
	     << ": Distorsion detected! Please lower the input volume!\n";
      }
      
      return ret;
    }
  
  private:
    string name;
    
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
  : Rx(cfg, name), audio_io(0), is_muted(true),
    squelch(0), siglevdet(0), siglev_offset(0.0), siglev_slope(1.0),
    tone_dets(0), sql_valve(0), delay(0), mute_dtmf(false), sql_tail_elim(0),
    preamp_gain(0)
{
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  /*
  list<ToneDurationDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    delete *it;
  }
  tone_detectors.clear();
  */
  
  delete audio_io;  // This will delete the whole chain of audio objects
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
  
  int delay_line_len = 0;
  if (cfg().getValue(name(), "MUTE_DTMF", value))
  {
    mute_dtmf = (atoi(value.c_str()) != 0);
    delay_line_len = DTMF_DELAY_LINE_LEN;
  }
  
  if (cfg().getValue(name(), "SQL_TAIL_ELIM", value))
  {
    sql_tail_elim = atoi(value.c_str());
    delay_line_len = max(delay_line_len, sql_tail_elim);
  }
  
  if (cfg().getValue(name(), "PREAMP", value))
  {
    preamp_gain = atoi(value.c_str());
  }
  
  bool peak_meter = false;
  if (cfg().getValue(name(), "PEAK_METER", value))
  {
    peak_meter = (atoi(value.c_str()) != 0);
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
  
  if (preamp_gain != 0)
  {
    AudioAmp *preamp = new AudioAmp;
    preamp->setGain(preamp_gain);
    prev_src->registerSink(preamp, true);
    prev_src = preamp;
  }
  
  if (peak_meter)
  {
    PeakMeter *peak_meter = new PeakMeter(name());
    prev_src->registerSink(peak_meter, true);
    prev_src = peak_meter;
  }
      
  if (deemphasis)
  {
    AudioFilter *deemph_filt = new AudioFilter("LpBu1/300");
    deemph_filt->setOutputGain(4);
    prev_src->registerSink(deemph_filt, true);
    prev_src = deemph_filt;
  }
  
  AudioSplitter *splitter = new AudioSplitter;
  prev_src->registerSink(splitter, true);
  
  siglevdet = new SigLevDet;
  splitter->addSink(siglevdet, true);
  
  squelch->squelchOpen.connect(slot(this, &LocalRx::onSquelchOpen));
  splitter->addSink(squelch, true);
  
  DtmfDecoder *dtmf_dec = new DtmfDecoder;
  dtmf_dec->digitActivated.connect(slot(this, &LocalRx::dtmfDigitActivated));
  dtmf_dec->digitDeactivated.connect(
      slot(this, &LocalRx::dtmfDigitDeactivated));
  splitter->addSink(dtmf_dec, true);
  
  tone_dets = new AudioSplitter;
  splitter->addSink(tone_dets, true);
  
  AudioFilter *ctcss_filt = new AudioFilter("HpBu4/300");
  splitter->addSink(ctcss_filt, true);
  prev_src = ctcss_filt;
  
  sql_valve = new SigCAudioValve;
  sql_valve->sigAllSamplesFlushed.connect(
      slot(this, &LocalRx::allSamplesFlushed));
  prev_src->registerSink(sql_valve, true);
  prev_src = sql_valve;
  
  if (delay_line_len > 0)
  {
    delay = new AudioDelayLine(delay_line_len);
    prev_src->registerSink(delay, true);
    prev_src = delay;
  }
    
  SigCAudioSink *sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(slot(this, &LocalRx::audioRead));
  sigc_sink->sigWriteSamples.connect(audioReceived.slot());
  sigc_sink->sigFlushSamples.connect(
      slot(sigc_sink, &SigCAudioSink::allSamplesFlushed));
  prev_src->registerSink(sigc_sink, true);
  
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
    if (delay != 0)
    {
      delay->clear();
    }
    sql_valve->setOpen(false);
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
  
  //tone_detectors.push_back(det);
  tone_dets->addSink(det, true);
  
  return true;

} /* LocalRx::addToneDetector */


float LocalRx::signalStrength(void) const
{
  return siglev_offset - siglev_slope * log10(siglevdet->lastSiglev());
} /* LocalRx::signalStrength */
    

void LocalRx::reset(void)
{
  mute(true);
  
  /*
  list<ToneDurationDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    delete *it;
  }
  tone_detectors.clear();
  */
  tone_dets->removeAllSinks();
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
  //bool was_open = squelch->isOpen();
  
  //siglevdet->writeSamples(samples, count);
  
  /*
  squelch->audioIn(samples, count);
  if (!was_open && squelch->isOpen())
  {
    setSquelchState(true);
    siglevdet->reset();
  }
  */
  
  //dtmf_dec->processSamples(samples, count);
  
  /*
  list<ToneDurationDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    (*it)->writeSamples(samples, count);
  }
  */
  
  if (is_muted)
  {
    printf("*** Gaaaahhh. Audio sent while muted. Not good...\n");
  }
  
  //if (/*squelch->isOpen() &&*/ !is_muted)
  //{
    /*
    float filtered[count];
    for (int i=0; i<count; ++i)
    {
      filtered[i] = samples[i];
    }
    highpassFilter(filtered, count);
    count = audioReceived(filtered, count);
    */
    
    //count = audioReceived(samples, count);
  //}
  
  /*
  if (was_open && !squelch->isOpen())
  {
    setSquelchState(false);
  }
  */
  
  return count;
  
} /* LocalRx::audioRead */


void LocalRx::dtmfDigitActivated(char digit)
{
  //printf("DTMF digit %c activated.\n", digit);
  if (mute_dtmf)
  {
    delay->mute(true, DTMF_MUTING_PRE);
  }
} /* LocalRx::dtmfDigitActivated */


void LocalRx::dtmfDigitDeactivated(char digit, int duration_ms)
{
  //printf("DTMF digit %c deactivated. Duration = %d ms\n", digit, duration_ms);
  dtmfDigitDetected(digit, duration_ms);
  if (mute_dtmf)
  {
    delay->mute(false, DTMF_MUTING_POST);
  }
} /* LocalRx::dtmfDigitActivated */


void LocalRx::allSamplesFlushed(void)
{
  //printf("LocalRx::allSamplesFlushed\n");
  setSquelchState(false);
} /* LocalRx::allSamplesFlushed */


void LocalRx::onSquelchOpen(bool is_open)
{
  //printf("LocalRx::onSquelchOpen\n");
  if (is_open)
  {
    setSquelchState(true);
    if (delay != 0)
    {
      delay->clear();
    }
    if (!is_muted)
    {
      sql_valve->setOpen(true);
    }
  }
  else
  {
    if (sql_tail_elim > 0)
    {
      delay->clear(sql_tail_elim);
    }
    sql_valve->setOpen(false);
  }
} /* LocalRx::onSquelchOpen */



/*
 * This file has not been truncated
 */

