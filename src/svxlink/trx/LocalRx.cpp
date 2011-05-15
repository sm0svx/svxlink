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
#include <AsyncAudioClipper.h>
#include <AsyncAudioCompressor.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioStreamStateDetector.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDetNoise.h"
#include "SigLevDetTone.h"
#include "DtmfDecoder.h"
#include "ToneDetector.h"
#include "SquelchVox.h"
#include "SquelchCtcss.h"
#include "SquelchSerial.h"
#include "SquelchSigLev.h"
#include "SquelchEvDev.h"
#include "LocalRx.h"
#include "multirate_filter_coeff.h"
#include "Sel5Decoder.h"


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

#define DTMF_MUTING_PRE   75
#define DTMF_MUTING_POST  200


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class ToneDurationDet : public ToneDetector
{
  public:
    ToneDurationDet(float fq, float bw, int duration)
      : ToneDetector(fq, bw),
      	required_duration(duration), duration_timer(0)
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
  : Rx(cfg, name), cfg(cfg), audio_io(0), is_muted(true),
    squelch_det(0), siglevdet(0), /* siglev_offset(0.0), siglev_slope(1.0), */
    tone_dets(0), sql_valve(0), delay(0), mute_dtmf(false), sql_tail_elim(0),
    preamp_gain(0)
{
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  clearHandler();
  delete audio_io;  // This will delete the whole chain of audio objects
} /* LocalRx::~LocalRx */


bool LocalRx::initialize(void)
{
  if (!Rx::initialize())
  {
    return false;
  }
  
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
  
  bool deemphasis = false;
  if (cfg.getValue(name(), "DEEMPHASIS", value))
  {
    deemphasis = (atoi(value.c_str()) != 0);
  }
  
  int delay_line_len = 0;
  if (cfg.getValue(name(), "MUTE_DTMF", value))
  {
    cerr << "*** ERROR: The MUTE_DTMF configuration variable has been\n"
      	 << "           renamed to DTMF_MUTING. Change this in configuration\n"
	 << "           section \"" << name() << "\".\n";
    return false;
  }
  
  if (cfg.getValue(name(), "DTMF_MUTING", value))
  {
    mute_dtmf = (atoi(value.c_str()) != 0);
    delay_line_len = DTMF_MUTING_PRE;
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
  
    // Create the audio IO object
  audio_io = new AudioIO(audio_dev, audio_channel);
  //FIXME: Check that the audio device is correctly initialized
  //       before continuing.
  AudioSource *prev_src = audio_io;
  
    // Create a fifo buffer to handle large audio blocks
  AudioFifo *input_fifo = new AudioFifo(1024);
//  input_fifo->setOverwrite(true);
  prev_src->registerSink(input_fifo, true);
  prev_src = input_fifo;
  
    // If a preamp was configured, create it
  if (preamp_gain != 0)
  {
    AudioAmp *preamp = new AudioAmp;
    preamp->setGain(preamp_gain);
    prev_src->registerSink(preamp, true);
    prev_src = preamp;
  }
  
    // If a peak meter was configured, create it
  if (peak_meter)
  {
    PeakMeter *peak_meter = new PeakMeter(name());
    prev_src->registerSink(peak_meter, true);
    prev_src = peak_meter;
  }
  
    // If the sound card sample rate is higher than 16kHz (48kHz assumed),
    // decimate it down to 16kHz
  if (audio_io->sampleRate() > 16000)
  {
    AudioDecimator *d1 = new AudioDecimator(3, coeff_48_16_wide,
					    coeff_48_16_wide_taps);
    prev_src->registerSink(d1, true);
    prev_src = d1;
  }
  
    // If the sound card sample rate is higher than 8kHz (16 or 48kHz assumed)
    // decimate it down to 8kHz. Also create a splitter to distribute the
    // 16kHz audio to other consumers.
#if (INTERNAL_SAMPLE_RATE != 16000)
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
#endif

    // If a deemphasis filter was configured, create it
  if (deemphasis)
  {
    //AudioFilter *deemph_filt = new AudioFilter("LpBu1/300");
    //AudioFilter *deemph_filt = new AudioFilter("HsBq1/0.01/-18/3500");
    AudioFilter *deemph_filt = new AudioFilter("HsBq1/0.05/-36/3500");
    deemph_filt->setOutputGain(2.88);
    prev_src->registerSink(deemph_filt, true);
    prev_src = deemph_filt;
  }
  
    // Create an audio splitter to distribute the 8kHz audio to all consumers
  AudioSplitter *splitter = new AudioSplitter;
  prev_src->registerSink(splitter, true);
  prev_src = 0;
  
    // Create the signal level detector. Connect it to the 16 or 8kHz splitter
    // depending on how the sound card sample rate is setup.
#if (INTERNAL_SAMPLE_RATE != 16000)
  if (rate_16k_splitter != 0)
  {
    siglevdet = createSigLevDet(name(), 16000);
    if (siglevdet == 0)
    {
      return false;
    }
    rate_16k_splitter->addSink(siglevdet, true);
  }
  else
  {
    siglevdet = createSigLevDet(name(), 8000);
    if (siglevdet == 0)
    {
      return false;
    }
    splitter->addSink(siglevdet, true);
  }
#else
  siglevdet = createSigLevDet(name(), 16000);
  if (siglevdet == 0)
  {
    return false;
  }
  splitter->addSink(siglevdet, true);
#endif
  
    // Create the configured squech detector and initialize it. Then connect
    // it to the 8kHz audio splitter
  string sql_det_str;
  if (!cfg.getValue(name(), "SQL_DET", sql_det_str))
  {
    cerr << "*** ERROR: Config variable " << name() << "/SQL_DET not set\n";
    return false;
  }

  if (sql_det_str == "VOX")
  {
    squelch_det = new SquelchVox;
  }
  else if (sql_det_str == "CTCSS")
  {
    squelch_det = new SquelchCtcss;
  }
  else if (sql_det_str == "SERIAL")
  {
    squelch_det = new SquelchSerial;
  }
  else if (sql_det_str == "SIGLEV")
  {
    squelch_det = new SquelchSigLev(siglevdet);
  }
  else if (sql_det_str == "EVDEV")
  {
    squelch_det = new SquelchEvDev;
  }
  else
  {
    cerr << "*** ERROR: Unknown squelch type specified in config variable "
      	 << name() << "/SQL_DET. Legal values are: VOX, CTCSS, SIGLEV, "
	 << "EVDEV and SERIAL\n";
    // FIXME: Cleanup
    return false;
  }
  
  if (!squelch_det->initialize(cfg, name()))
  {
    cerr << "*** ERROR: Squelch detector initialization failed for RX \""
      	 << name() << "\"\n";
    delete squelch_det;
    squelch_det = 0;
    // FIXME: Cleanup
    return false;
  }
  
  squelch_det->squelchOpen.connect(slot(*this, &LocalRx::onSquelchOpen));
  splitter->addSink(squelch_det, true);

    // Create the configured type of DTMF decoder and add it to the splitter
  DtmfDecoder *dtmf_dec = DtmfDecoder::create(cfg, name());
  if ((dtmf_dec == 0) || !dtmf_dec->initialize())
  {
    // FIXME: Cleanup?
    return false;
  }

  dtmf_dec->digitActivated.connect(slot(*this, &LocalRx::dtmfDigitActivated));
  dtmf_dec->digitDeactivated.connect(
      slot(*this, &LocalRx::dtmfDigitDeactivated));
  splitter->addSink(dtmf_dec, true);
  
   // creates a selective multiple tone detector object
  string sel5_det_str;
  if (cfg.getValue(name(), "SEL5_DEC_TYPE", sel5_det_str))
  {
    Sel5Decoder *sel5_dec = Sel5Decoder::create(cfg, name());
    if (sel5_dec == 0 || !sel5_dec->initialize())
    {
      cerr << "*** ERROR: Sel5 decoder initialization failed for RX \""
          << name() << "\"\n";
      return false;
    }
    sel5_dec->sequenceDetected.connect(slot(*this, &LocalRx::sel5Detected));
    splitter->addSink(sel5_dec, true);
  }

    // Create a new audio splitter to handle tone detectors then add it to
    // the splitter
  tone_dets = new AudioSplitter;
  splitter->addSink(tone_dets, true);
  
    // Create an audio valve to use as squelch and connect it to the splitter
  sql_valve = new AudioValve;
  sql_valve->setOpen(false);
  splitter->addSink(sql_valve, true);
  prev_src = sql_valve;

    // Create the state detector
  AudioStreamStateDetector *state_det = new AudioStreamStateDetector;
  state_det->sigStreamStateChanged.connect(
            slot(*this, &LocalRx::audioStreamStateChange));
  prev_src->registerSink(state_det, true);
  prev_src = state_det;

    // Create the highpass CTCSS filter that cuts off audio below 300Hz
  AudioFilter *ctcss_filt = new AudioFilter("HpBu20/300");
  prev_src->registerSink(ctcss_filt, true);
  prev_src = ctcss_filt;
  
    // If we need a delay line (e.g. for DTMF muting and/or squelch tail
    // elimination), create it
  if (delay_line_len > 0)
  {
    delay = new AudioDelayLine(delay_line_len);
    prev_src->registerSink(delay, true);
    prev_src = delay;
  }
  
    // Add a limiter to smoothly limiting the audio before hard clipping it
  AudioCompressor *limit = new AudioCompressor;
  limit->setThreshold(-1);
  limit->setRatio(0.1);
  limit->setAttack(2);
  limit->setDecay(20);
  limit->setOutputGain(1);
  prev_src->registerSink(limit, true);
  prev_src = limit;

    // Clip audio to limit its amplitude
  AudioClipper *clipper = new AudioClipper;
  clipper->setClipLevel(0.98);
  prev_src->registerSink(clipper, true);
  prev_src = clipper;

    // Remove high frequencies generated by the previous clipping
#if (INTERNAL_SAMPLE_RATE == 16000)
  AudioFilter *splatter_filter = new AudioFilter("LpBu20/5500");
#else
  AudioFilter *splatter_filter = new AudioFilter("LpBu20/3500");
#endif
  prev_src->registerSink(splatter_filter, true);
  prev_src = splatter_filter;
  
    // Set the previous audio pipe object to handle audio distribution for
    // the LocalRx class
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
    squelch_det->reset();
    setSquelchState(false);
  }
  else
  {
    if (!audio_io->open(AudioIO::MODE_RD))
    {
      cerr << "*** ERROR: Could not open audio device for receiver \""
      	   << name() << "\"\n";
      return;
    }
    
    squelch_det->reset();
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

void LocalRx::sel5Detected(std::string sequence)
{
  selcallSequenceDetected(sequence);
} /* LocalRx::sel5Detected */


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


void LocalRx::audioStreamStateChange(bool is_active, bool is_idle)
{
  if (is_idle)
  {
    setSquelchState(false);
  }
} /* LocalRx::audioStreamStateChange */


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


SigLevDet *LocalRx::createSigLevDet(const string &name, int sample_rate)
{
  string siglev_det_type;
  if (!cfg.getValue(name, "SIGLEV_DET", siglev_det_type))
  {
    siglev_det_type = "NOISE";
  }
  
  SigLevDet *siglevdet = 0;
  if (siglev_det_type == "TONE")
  {
    if (sample_rate != 16000)
    {
      cerr << "*** ERROR: The tone signal level detector only work at 16kHz "
              "sampling rate\n";
      return 0;
    }
    siglevdet = new SigLevDetTone;
  }
  else if (siglev_det_type == "NOISE")
  {
    SigLevDetNoise *det = new SigLevDetNoise(sample_rate);
  
    string value;
    if (cfg.getValue(name, "SIGLEV_OFFSET", value))
    {
      det->setDetectorOffset(atof(value.c_str()));
    }
    
    if (cfg.getValue(name, "SIGLEV_SLOPE", value))
    {
      det->setDetectorSlope(atof(value.c_str()));
    }
    
    siglevdet = det;
  }
  else
  {
    cerr << "*** ERROR: Unknown signal level detector type \""
         << siglev_det_type << "\" specified in " << name << "/SIGLEV_DET.";
    return 0;
  }
  
  if (!siglevdet->initialize(cfg, name))
  {
    delete siglevdet;
    siglevdet = 0;
  }
  
  return siglevdet;
  
} /* LocalRx::createSigLevDet */



/*
 * This file has not been truncated
 */

