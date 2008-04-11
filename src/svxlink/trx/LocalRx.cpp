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
#include <AsyncAudioFilter.h>
#include <SigCAudioSink.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioAmp.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioDecimator.h>


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
#include "SquelchSigLev.h"
#include "LocalRx.h"
#include "multirate_filter_coeff.h"


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
      : ToneDetector(fq, 8000 / bw), required_duration(duration),
      	duration_timer(0)
    {
      //timerclear(&activation_timestamp);
      ToneDetector::activated.connect(
      	      slot(*this, &ToneDurationDet::toneActivated));
    }
    
    ~ToneDurationDet(void)
    {
      delete duration_timer;
    }
    
    SigC::Signal1<void, float> detected;
    
  private:
    int     	    required_duration;
    //struct timeval  activation_timestamp;
    Timer     	    *duration_timer;
    
    void toneActivated(bool is_activated)
    {
      //printf("%.1f tone %s...\n", toneFq(),
      //	     is_activated ? "ACTIVATED" : "DEACTIVATED");
      if (is_activated)
      {
	//gettimeofday(&activation_timestamp, NULL);
	duration_timer = new Timer(required_duration);
	duration_timer->expired.connect(
	    slot(*this, &ToneDurationDet::toneDetected));
      }
      else
      {
      	/*
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
	*/
	
	delete duration_timer;
	duration_timer = 0;
      }
    }
    
    void toneDetected(Timer *t)
    {
      delete duration_timer;
      duration_timer = 0;
      detected(toneFq());
    }
};


class SigCAudioValve : public AudioValve, public SigC::Object
{
  public:
    void allSamplesFlushed(void)
    {
      //printf("SigCAudioValve::allSamplesFlushed\n");
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
      
      if (i < ret)
      {
      	cout << name
	     << ": Distorsion detected! Please lower the input volume!\n";
      }
      
      return ret;
    }
  
  private:
    string name;
    
};


class SigCAudioSinkNoFlow : public SigCAudioSink
{
  public:
    virtual int writeSamples(const float *samples, int len)
    {
      sigWriteSamples(const_cast<float *>(samples), len);
      return len;
    }

    virtual void flushSamples(void)
    {
      sourceAllSamplesFlushed();
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
  : Rx(name), cfg(cfg), audio_io(0), is_muted(true),
    squelch(0), siglevdet(0), siglev_offset(0.0), siglev_slope(1.0),
    tone_dets(0), sql_valve(0), delay(0), mute_dtmf(false), sql_tail_elim(0),
    preamp_gain(0)
{
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  delete audio_io;  // This will delete the whole chain of audio objects
} /* LocalRx::~LocalRx */


bool LocalRx::initialize(void)
{
  string value;

  string audio_dev;
  if (!cfg.getValue(name(), "AUDIO_DEV", audio_dev))
  {
    cerr << "*** ERROR: Config variable " << name() << "/AUDIO_DEV not set\n";
    return false;
  }
  
  if (!cfg.getValue(name(), "AUDIO_CHANNEL", value))
  {
    cerr << "*** ERROR: Config variable " << name()
         << "/AUDIO_CHANNEL not set\n";
    return false;
  }
  int audio_channel = atoi(value.c_str());
  
  if (cfg.getValue(name(), "SIGLEV_OFFSET", value))
  {
    siglev_offset = atof(value.c_str());
  }
  
  if (cfg.getValue(name(), "SIGLEV_SLOPE", value))
  {
    siglev_slope = atof(value.c_str());
  }
  
  bool deemphasis = false;
  if (cfg.getValue(name(), "DEEMPHASIS", value))
  {
    deemphasis = (atoi(value.c_str()) != 0);
  }
  
  int delay_line_len = 0;
  if (cfg.getValue(name(), "MUTE_DTMF", value))
  {
    mute_dtmf = (atoi(value.c_str()) != 0);
    delay_line_len = DTMF_DELAY_LINE_LEN;
  }
  
  if (cfg.getValue(name(), "SQL_TAIL_ELIM", value))
  {
    sql_tail_elim = atoi(value.c_str());
    delay_line_len = max(delay_line_len, sql_tail_elim);
  }
  
  if (cfg.getValue(name(), "PREAMP", value))
  {
    preamp_gain = atoi(value.c_str());
  }
  
  bool peak_meter = false;
  if (cfg.getValue(name(), "PEAK_METER", value))
  {
    peak_meter = (atoi(value.c_str()) != 0);
  }
  
  int dtmf_hangtime = 100;
  if (cfg.getValue(name(), "DTMF_HANGTIME", value))
  {
    dtmf_hangtime = atoi(value.c_str());
  }
  
  audio_io = new AudioIO(audio_dev, audio_channel);
  AudioSource *prev_src = audio_io;
  
  if (audio_io->sampleRate() > 16000)
  {
    AudioDecimator *d1 = new AudioDecimator(3, coeff_48_16, coeff_48_16_taps);
    prev_src->registerSink(d1, true);
    prev_src = d1;
  }
  
  AudioSplitter *rate_16k_splitter = 0;
  
  if (audio_io->sampleRate() > 8000)
  {
    rate_16k_splitter = new AudioSplitter;
    prev_src->registerSink(rate_16k_splitter, true);

    AudioDecimator *d2 = new AudioDecimator(2, coeff_16_8, coeff_16_8_taps);
    //prev_src->registerSink(d2, true);
    rate_16k_splitter->addSink(d2, true);
    prev_src = d2;
  }

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
    deemph_filt->setOutputGain(6);
    prev_src->registerSink(deemph_filt, true);
    prev_src = deemph_filt;
  }
  
  AudioSplitter *splitter = new AudioSplitter;
  prev_src->registerSink(splitter, true);
  
  if (rate_16k_splitter != 0)
  {
    siglevdet = new SigLevDet(16000);
    rate_16k_splitter->addSink(siglevdet, true);
  }
  else
  {
    siglevdet = new SigLevDet(8000);
    splitter->addSink(siglevdet, true);
  }
  siglevdet->setDetectorSlope(siglev_slope);
  siglevdet->setDetectorOffset(siglev_offset);
  
  string sql_det_str;
  if (!cfg.getValue(name(), "SQL_DET", sql_det_str))
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
  else if (sql_det_str == "SIGLEV")
  {
    squelch = new SquelchSigLev(siglevdet);
  }
  else
  {
    cerr << "*** ERROR: Unknown squelch type specified in config variable "
      	 << name() << "/SQL_DET. Legal values are: VOX, CTCSS, SIGLEV "
	 << "and SERIAL\n";
    // FIXME: Cleanup
    return false;
  }
  
  if (!squelch->initialize(cfg, name()))
  {
    cerr << "*** ERROR: Squelch detector initialization failed for RX \""
      	 << name() << "\"\n";
    delete squelch;
    squelch = 0;
    // FIXME: Cleanup
    return false;
  }
  
  squelch->squelchOpen.connect(slot(*this, &LocalRx::onSquelchOpen));
  splitter->addSink(squelch, true);
  
  tone_dets = new AudioSplitter;
  splitter->addSink(tone_dets, true);
  
  AudioFilter *ctcss_filt = new AudioFilter("HpBu4/300");
  splitter->addSink(ctcss_filt, true);
  prev_src = ctcss_filt;
  
  AudioSplitter *ctcss_splitter = new AudioSplitter;
  prev_src->registerSink(ctcss_splitter, true);

  DtmfDecoder *dtmf_dec = DtmfDecoder::create(cfg, name());
  if ((dtmf_dec == 0) || !dtmf_dec->initialize())
  {
    // FIXME: Cleanup?
    return false;
  }

  dtmf_dec->digitActivated.connect(slot(*this, &LocalRx::dtmfDigitActivated));
  dtmf_dec->digitDeactivated.connect(
      slot(*this, &LocalRx::dtmfDigitDeactivated));
  ctcss_splitter->addSink(dtmf_dec, true);
  
  sql_valve = new SigCAudioValve;
  sql_valve->setOpen(false);
  sql_valve->sigAllSamplesFlushed.connect(
      slot(*this, &LocalRx::allSamplesFlushed));
  ctcss_splitter->addSink(sql_valve, true);
  prev_src = sql_valve;
  
  if (delay_line_len > 0)
  {
    delay = new AudioDelayLine(delay_line_len);
    prev_src->registerSink(delay, true);
    prev_src = delay;
  }
  
  setHandler(prev_src);
  
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
    setSquelchState(false);
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


bool LocalRx::addToneDetector(float fq, int bw, float thresh,
      	      	      	      int required_duration)
{
  //printf("Adding tone detector with fq=%d  bw=%d  req_dur=%d\n",
  //    	 fq, bw, required_duration);
  ToneDurationDet *det = new ToneDurationDet(fq, bw, required_duration);
  assert(det != 0);
  det->setSnrThresh(thresh);
  det->detected.connect(toneDetected.slot());
  
  tone_dets->addSink(det, true);
  
  return true;

} /* LocalRx::addToneDetector */


float LocalRx::signalStrength(void) const
{
  //return siglev_offset - siglev_slope * log10(siglevdet->lastSiglev());
  return siglevdet->lastSiglev();
} /* LocalRx::signalStrength */
    

void LocalRx::reset(void)
{
  mute(true);
  tone_dets->removeAllSinks();
  if (delay != 0)
  {
    delay->mute(false);
  }
} /* LocalRx::reset */



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
  if (!is_muted)
  {
    dtmfDigitDetected(digit, duration_ms);
  }
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
  if (is_open)
  {
    if (delay != 0)
    {
      delay->clear();
    }
    if (!is_muted)
    {
      setSquelchState(true);
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

