/**
@file	 LocalTx.cpp
@brief   Implements a local transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that implements a local transmitter.

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

#include <termios.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <cstdio>
#include <cstring>
#include <algorithm>
#include <cmath>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>
#include <AsyncConfig.h>
#include <AudioClipper.h>
#include <AudioCompressor.h>
#include <AudioFilter.h>
#include <SigCAudioSink.h>
#include <SigCAudioSource.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioValve.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DtmfEncoder.h"
#include "LocalTx.h"



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

class SineGenerator : public Async::AudioSource
{
  public:
    explicit SineGenerator(const string& audio_dev)
      : audio_io(audio_dev), pos(0), fq(0.0), level(0.0),
      	sample_rate(0)
    {
      sample_rate = audio_io.sampleRate();
      audio_io.registerSource(this);
    }
    
    ~SineGenerator(void)
    {
      enable(false);
    }
    
    void setFq(double tone_fq)
    {
      fq = tone_fq;
    }
    
    void setLevel(int level_percent)
    {
      level = level_percent / 100.0;
    }
    
    void enable(bool enable)
    {
      if (enable == (audio_io.mode() != AudioIO::MODE_NONE))
      {
      	return;
      }
      
      if (enable && (fq != 0))
      {
      	audio_io.open(AudioIO::MODE_WR);
	pos = 0;
      	writeSamples();
      }
      else
      {
      	audio_io.close();
      }
    }

    void resumeOutput(void)
    {
      if (audio_io.mode() != AudioIO::MODE_NONE)
      {
      	writeSamples();
      }
    }
    
    void allSamplesFlushed(void)
    {
    
    }
    
    
  private:
    static const int BLOCK_SIZE = 128;
    
    AudioIO   audio_io;
    unsigned  pos;
    double    fq;
    double    level;
    int       sample_rate;
    
    void writeSamples(void)
    {
      int written;
      do {
	float buf[BLOCK_SIZE];
	for (int i=0; i<BLOCK_SIZE; ++i)
	{
      	  buf[i] = level * sin(2 * M_PI * fq * (pos+i) / sample_rate);
	}
	written = sinkWriteSamples(buf, BLOCK_SIZE);
	pos += written;
      } while (written != 0);
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

LocalTx::LocalTx(Config& cfg, const string& name)
  : Tx(name), name(name), cfg(cfg), audio_io(0), is_transmitting(false),
    serial(0), ptt_pin1(Serial::PIN_NONE), ptt_pin1_rev(false),
    ptt_pin2(Serial::PIN_NONE), ptt_pin2_rev(false), txtot(0),
    tx_timeout_occured(false), tx_timeout(0), tx_delay(0), ctcss_enable(false),
    sigc_src(0), is_flushing(false), dtmf_encoder(0), selector(0), dtmf_valve(0)
{

} /* LocalTx::LocalTx */


LocalTx::~LocalTx(void)
{
  transmit(false);
  
  delete dtmf_encoder;
  delete sigc_src;
  delete txtot;
  delete serial;
  delete selector;
  delete dtmf_valve;
  delete audio_io;
  delete sine_gen;
} /* LocalTx::~LocalTx */


bool LocalTx::initialize(void)
{
  string audio_dev;
  if (!cfg.getValue(name, "AUDIO_DEV", audio_dev))
  {
    cerr << "*** ERROR: Config variable " << name << "/AUDIO_DEV not set\n";
    return false;
  }
  
  string ptt_port;
  if (!cfg.getValue(name, "PTT_PORT", ptt_port))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PORT not set\n";
    return false;
  }
  
  string ptt_pin_str;
  if (!cfg.getValue(name, "PTT_PIN", ptt_pin_str))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PIN not set\n";
    return false;
  }
  const char *ptr = ptt_pin_str.c_str();
  int cnt;
  cnt = parsePttPin(ptr, ptt_pin1, ptt_pin1_rev);
  if (cnt == 0)
  {
    return false;
  }
  ptr += cnt;
  if (*ptr != 0)
  {
    if (parsePttPin(ptr, ptt_pin2, ptt_pin2_rev) == 0)
    {
      return false;
    }
  }

  string tx_timeout_str;
  if (cfg.getValue(name, "TIMEOUT", tx_timeout_str))
  {
    tx_timeout = 1000 * atoi(tx_timeout_str.c_str());
  }
  
  string tx_delay_str;
  if (cfg.getValue(name, "TX_DELAY", tx_delay_str))
  {
    tx_delay = max(0, min(atoi(tx_delay_str.c_str()), 1000));
  }
  
  if (ptt_port != "NONE")
  {
    serial = new Serial(ptt_port.c_str());
    if (!serial->open())
    {
      perror("open serial port");
      return false;
    }
  }
  if (!setPtt(false))
  {
    perror("setPin");
    delete serial;
    serial = 0;
    return false;
  }
  
  audio_io = new AudioIO(audio_dev);
  
  sine_gen = new SineGenerator(audio_dev);
  
  string value;
  if (cfg.getValue(name, "CTCSS_FQ", value))
  {
    sine_gen->setFq(atof(value.c_str()));
  }  
  
  if (cfg.getValue(name, "CTCSS_LEVEL", value))
  {
    int level = atoi(value.c_str());
    sine_gen->setLevel(level);
    audio_io->setGain((100.0 - level) / 100.0);
  }  
  
  sigc_src = new SigCAudioSource;
  sigc_src->sigWriteBufferFull.connect(transmitBufferFull.slot());
  sigc_src->sigAllSamplesFlushed.connect(
      slot(*this, &LocalTx::onAllSamplesFlushed));
  AudioSource *prev_src = sigc_src;
  
  /*
  AudioCompressor *comp = new AudioCompressor;
  comp->setThreshold(-10);
  comp->setRatio(0.25);
  comp->setAttack(10);
  comp->setDecay(100);
  comp->setOutputGain(0);
  prev_src->registerSink(comp, true);
  prev_src = comp;
  */
  
  if (cfg.getValue(name, "PREEMPHASIS", value) && (atoi(value.c_str()) != 0))
  {
    AudioFilter *preemph = new AudioFilter("HpBu1/3000");
    preemph->setOutputGain(4);
    prev_src->registerSink(preemph, true);
    prev_src = preemph;
  }
  
  /*
  AudioCompressor *limit = new AudioCompressor;
  limit->setThreshold(-1);
  limit->setRatio(0.1);
  limit->setAttack(2);
  limit->setDecay(20);
  limit->setOutputGain(1);
  prev_src->registerSink(limit, true);
  prev_src = limit;
  */
  
  AudioClipper *clipper = new AudioClipper;
  prev_src->registerSink(clipper, true);
  prev_src = clipper;

  AudioFilter *sf = new AudioFilter("LpBu4/3000");
  prev_src->registerSink(sf, true);
  prev_src = sf;
  
  audio_stream = prev_src;
  selector = new AudioSelector;
  selector->addSource(audio_stream);
  selector->selectSource(audio_stream);
  prev_src = 0;
  
  dtmf_encoder = new DtmfEncoder(audio_io->sampleRate());
  dtmf_encoder->setToneLength(100);
  dtmf_encoder->setGapLength(50);
  dtmf_encoder->setToneAmplitude(-18);
  dtmf_encoder->allDigitsSent.connect(
      slot(*this, &LocalTx::onAllDtmfDigitsSent));
  
  dtmf_valve = new AudioValve(true);
  dtmf_valve->setOpen(false);
  dtmf_encoder->registerSink(dtmf_valve);
  selector->addSource(dtmf_valve);
  
  audio_io->registerSource(selector);

  return true;
  
} /* LocalTx::initialize */


void LocalTx::transmit(bool do_transmit)
{
  if (do_transmit == is_transmitting)
  {
    return;
  }
  
  cout << name << ": Turning the transmitter " << (do_transmit ? "ON" : "OFF")
       << endl;
  
  is_transmitting = do_transmit;
  
  if (do_transmit)
  {
    if (!audio_io->open(AudioIO::MODE_WR))
    {
      cerr << "*** ERROR: Could not open audio device for transmitter \""
      	   << name << "\"\n";
      is_transmitting = false;
      return;
    }
    
    if (ctcss_enable)
    {
      sine_gen->enable(true);
    }

    if ((txtot == 0) && (tx_timeout > 0))
    {
      txtot = new Timer(tx_timeout);
      txtot->expired.connect(slot(*this, &LocalTx::txTimeoutOccured));
    }
    
    if (tx_delay > 0)
    {
      float samples[8000];
      memset(samples, 0, sizeof(samples));
      audio_io->writeSamples(samples, 8000 * tx_delay / 1000);
      flushSamples();
    }
    
    dtmf_valve->setOpen(true);

    transmitBufferFull(false);
  }
  else
  {
    dtmf_valve->setOpen(false);
    
    audio_io->close();
    
    if (ctcss_enable)
    {
      sine_gen->enable(false);
    }
    
    delete txtot;
    txtot = 0;
    tx_timeout_occured = false;
  }
  
  if (!setPtt(is_transmitting && !tx_timeout_occured))
  {
    perror("setPin");
  }

} /* LocalTx::transmit */


int LocalTx::transmitAudio(float *samples, int count)
{
  is_flushing = false;
  
  if (!is_transmitting)
  {
    transmitBufferFull(true);
    return 0;
  }
  
  int ret = sigc_src->writeSamples(samples, count);
  /*
  if (ret != count)
  {
    printf("ret=%d  count=%d\n", ret, count);
  }
  */
  return ret;
  
} /* LocalTx::transmitAudio */


void LocalTx::flushSamples(void)
{
  is_flushing = true;
  sigc_src->flushSamples();
} /* LocalTx::flushSamples */


void LocalTx::enableCtcss(bool enable)
{
  ctcss_enable = enable;
  if (is_transmitting)
  {
    sine_gen->enable(enable);
  }
} /* LocalTx::enableCtcss */


void LocalTx::sendDtmf(const string& digits)
{
  selector->selectSource(dtmf_valve);
  dtmf_encoder->send(digits);
} /* LocalTx::sendDtmf */


bool LocalTx::isSendingDtmf(void) const
{
  return dtmf_encoder->isSending();
} /* LocalTx::isSendingDtmf */



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


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void LocalTx::txTimeoutOccured(Timer *t)
{
  delete txtot;
  txtot = 0;
  
  if (tx_timeout_occured)
  {
    return;
  }
  
  cerr << "*** ERROR: The transmitter have been active for too long. Turning "
      	  "it off...\n";
  
  if (!setPtt(false))
  {
    perror("setPin");
  }
  
  tx_timeout_occured = true;
  txTimeout();
} /* LocalTx::txTimeoutOccured */


int LocalTx::parsePttPin(const char *str, Serial::Pin &pin, bool &rev)
{
  int cnt = 0;
  if (*str == '!')
  {
    rev = true;
    str++;
    cnt++;
  }
  if (strncmp(str, "RTS", 3) == 0)
  {
    pin = Serial::PIN_RTS;
    str += 3;
    cnt += 3;
  }
  else if (strncmp(str, "DTR", 3) == 0)
  {
    pin = Serial::PIN_DTR;
    str += 3;
    cnt += 3;
  }
  else
  {
    cerr << "*** ERROR: Accepted values for config variable "
      	 << name << "/PTT_PIN are \"[!]RTS\" and/or \"[!]DTR\".\n";
    return 0;
  }

  return cnt;

} /* LocalTx::parsePttPin */


bool LocalTx::setPtt(bool tx)
{
  if ((serial != 0) && !serial->setPin(ptt_pin1, tx ^ ptt_pin1_rev))
  {
    return false;
  }

  if ((serial != 0) && !serial->setPin(ptt_pin2, tx ^ ptt_pin2_rev))
  {
    return false;
  }

  return true;

} /* LocalTx::setPtt  */


void LocalTx::onAllSamplesFlushed(void)
{
  is_flushing = false;
  allSamplesFlushed();
} /* LocalTx::allSamplesFlushed */


void LocalTx::onAllDtmfDigitsSent(void)
{
  selector->selectSource(audio_stream);
  allDtmfDigitsSent();
} /* LocalTx::allDtmfDigitsSent */



/*
 * This file has not been truncated
 */

