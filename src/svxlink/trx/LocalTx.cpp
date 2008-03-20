/**
@file	 LocalTx.cpp
@brief   Implements a local transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that implements a local transmitter.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>
#include <AsyncConfig.h>
#include <AsyncAudioClipper.h>
#include <AsyncAudioCompressor.h>
#include <AsyncAudioFilter.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioDebugger.h>
#include <AsyncAudioFifo.h>



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
using namespace SigC;



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


class LocalTx::InputHandler : public AudioPassthrough
{
  public:
    InputHandler(void) : is_flushing(false) /*, is_writing_audio(false)*/ {}
    
    bool isFlushing(void) const { return is_flushing; }
    //bool isWritingAudio(void) const { return is_writing_audio; }
    
    int writeSamples(const float *samples, int count)
    {
      is_flushing = false;
      //is_writing_audio = true;
      return sinkWriteSamples(samples, count);
    }
    
    void flushSamples(void)
    {
      is_flushing = true;
      sinkFlushSamples();
    }
    
    void allSamplesFlushed(void)
    {
      is_flushing = false;
      //is_writing_audio = false;
      sourceAllSamplesFlushed();
    }
  
  private:;
    bool is_flushing;
    //bool is_writing_audio;
    
}; /* class LocalTx::InputHandler */


class LocalTx::PttCtrl : public AudioSink, public AudioSource,
      	      	      	 public SigC::Object
{
  public:
    explicit PttCtrl(LocalTx *tx, int tx_delay)
      : tx(tx), tx_ctrl_mode(TX_OFF), is_transmitting(false), tx_delay_timer(0),
      	tx_delay(tx_delay), fifo(0)
    {
      valve.setBlockWhenClosed(false);
      valve.setOpen(false);
      
      if (tx_delay > 0)
      {
      	fifo = new AudioFifo((tx_delay + 500) * 8);
      	fifo->registerSink(&valve);
      	AudioSink::setHandler(fifo);
      }
      else
      {
      	AudioSink::setHandler(&valve);
      }
      
      AudioSource::setHandler(&valve);
    }
    
    ~PttCtrl(void)
    {
      AudioSink::clearHandler();
      AudioSource::clearHandler();
      delete fifo;
      delete tx_delay_timer;
    }
    
    void setTxDelay(int delay_ms)
    {
      tx_delay = delay_ms;
    }
        
    void setTxCtrlMode(TxCtrlMode mode)
    {
      if (mode == tx_ctrl_mode)
      {
      	return;
      }
      tx_ctrl_mode = mode;
      
      switch (mode)
      {
	case TX_OFF:
	  transmit(false);
	  break;

	case TX_ON:
	  transmit(true);
	  break;

	case TX_AUTO:
      	  transmit(!valve.isIdle());
	  break;
      }
    }
    
    int writeSamples(const float *samples, int count)
    {
      if ((tx_ctrl_mode == TX_AUTO) && !is_transmitting)
      {
      	transmit(true);
      }
      
      if (fifo != 0)
      {
      	return fifo->writeSamples(samples, count);
      }
      
      return valve.writeSamples(samples, count);
    }
    
    void allSamplesFlushed(void)
    {
      valve.allSamplesFlushed();
      if ((tx_ctrl_mode == TX_AUTO) && is_transmitting && valve.isIdle())
      {
      	transmit(false);
      }
    }
    
  private:
    LocalTx   	*tx;
    TxCtrlMode	tx_ctrl_mode;
    bool      	is_transmitting;
    Timer     	*tx_delay_timer;
    int       	tx_delay;
    AudioFifo 	*fifo;
    AudioValve	valve;
    
    void transmit(bool do_transmit)
    {
      if (do_transmit == is_transmitting)
      {
      	return;
      }
      
      is_transmitting = do_transmit;
      tx->transmit(do_transmit);
      if (do_transmit)
      {
	if (tx_delay > 0)
	{
	  fifo->enableBuffering(true);
      	  valve.setBlockWhenClosed(true);
      	  tx_delay_timer = new Timer(tx_delay);
	  tx_delay_timer->expired.connect(
	      slot(*this, &PttCtrl::txDelayExpired));
	}
	else
	{
      	  valve.setOpen(true);
	}
      }
      else
      {
      	if (tx_delay_timer != 0)
	{
      	  delete tx_delay_timer;
	  tx_delay_timer = 0;
	}
      	valve.setBlockWhenClosed(false);
      	valve.setOpen(false);
      }
    }
    
    void txDelayExpired(Timer *t)
    {
      delete tx_delay_timer;
      tx_delay_timer = 0;
      fifo->enableBuffering(false);
      valve.setOpen(true);
      valve.setBlockWhenClosed(false);
    }
    
}; /* class LocalTx::PttCtrl */




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
  : name(name), cfg(cfg), audio_io(0), is_transmitting(false),
    serial(0), ptt_pin1(Serial::PIN_NONE), ptt_pin1_rev(false),
    ptt_pin2(Serial::PIN_NONE), ptt_pin2_rev(false), txtot(0),
    tx_timeout_occured(false), tx_timeout(0), ctcss_enable(false),
    dtmf_encoder(0), selector(0), dtmf_valve(0), input_handler(0)
{

} /* LocalTx::LocalTx */


LocalTx::~LocalTx(void)
{
  transmit(false);
  
  clearHandler();
  delete input_handler;
  delete selector;
  delete dtmf_encoder;
  
  delete txtot;
  delete serial;
  delete sine_gen;
} /* LocalTx::~LocalTx */


bool LocalTx::initialize(void)
{
  string value;
  
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

  if (cfg.getValue(name, "TIMEOUT", value))
  {
    tx_timeout = 1000 * atoi(value.c_str());
  }
  
  int tx_delay = 0;
  if (cfg.getValue(name, "TX_DELAY", value))
  {
    tx_delay = atoi(value.c_str());
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
  
  int dtmf_tone_length = 100;
  if (cfg.getValue(name, "DTMF_TONE_LENGTH", value))
  {
    dtmf_tone_length = atoi(value.c_str());
  }
  
  int dtmf_tone_spacing = 50;
  if (cfg.getValue(name, "DTMF_TONE_SPACING", value))
  {
    dtmf_tone_spacing = atoi(value.c_str());
  }
  
  int dtmf_tone_amp = -18;
  if (cfg.getValue(name, "DTMF_TONE_AMP", value))
  {
    dtmf_tone_amp = min(atoi(value.c_str()), 0);
  }
  
  audio_io = new AudioIO(audio_dev);
  
  sine_gen = new SineGenerator(audio_dev);
  
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
  
  AudioSource *prev_src = 0;
  
    // The input handler is where audio enters this TX object
  input_handler = new InputHandler;
  setHandler(input_handler);
  prev_src = input_handler;
  
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
  
    // If preemphasis is enabled, create the preemphasis filter
  if (cfg.getValue(name, "PREEMPHASIS", value) && (atoi(value.c_str()) != 0))
  {
    AudioFilter *preemph = new AudioFilter("HpBu1/3000");
    preemph->setOutputGain(6);
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
  
    // Clip audio to limit its amplitude
  AudioClipper *clipper = new AudioClipper;
  prev_src->registerSink(clipper, true);
  prev_src = clipper;
  
    // Filter out high frequencies generated by the previous clipping
  AudioFilter *sf = new AudioFilter("LpBu4/3000");
  prev_src->registerSink(sf, true);
  prev_src = sf;
  
    // We need a selector to choose if DTMF or normal audio should be
    // transmitted
  selector = new AudioSelector;
  selector->addSource(prev_src);
  selector->enableAutoSelect(prev_src, 0);
  prev_src = selector;
  
    // Create the DTMF encoder
  dtmf_encoder = new DtmfEncoder(audio_io->sampleRate());
  dtmf_encoder->setToneLength(dtmf_tone_length);
  dtmf_encoder->setToneSpacing(dtmf_tone_spacing);
  dtmf_encoder->setToneAmplitude(dtmf_tone_amp);
  
    // Create a valve so that we can control when to transmit DTMF
  dtmf_valve = new AudioValve;
  dtmf_valve->setBlockWhenClosed(true);
  dtmf_valve->setOpen(false);
  dtmf_encoder->registerSink(dtmf_valve, true);
  selector->addSource(dtmf_valve);
  selector->enableAutoSelect(dtmf_valve, 10);
  
    // Cteate the PTT controller
  ptt_ctrl = new PttCtrl(this, tx_delay);
  prev_src->registerSink(ptt_ctrl, true);
  prev_src = ptt_ctrl;
  
    // Finally connect the whole audio pipe to the audio device
  prev_src->registerSink(audio_io, true);

  return true;
  
} /* LocalTx::initialize */


void LocalTx::setTxCtrlMode(Tx::TxCtrlMode mode)
{
  ptt_ctrl->setTxCtrlMode(mode);
  switch (mode)
  {
    case TX_OFF:
      dtmf_valve->setOpen(false);
      break;
      
    case TX_ON:
    case TX_AUTO:
      dtmf_valve->setOpen(true);
      break;
  }
  
} /* LocalTx::setTxCtrlMode */


/*
bool LocalTx::isWritingAudio(void) const
{
  return input_handler->isWritingAudio();
}
*/


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
  dtmf_encoder->send(digits);
} /* LocalTx::sendDtmf */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


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
    transmitterStateChange(true);

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
  }
  else
  {
    audio_io->close();
    
    if (ctcss_enable)
    {
      sine_gen->enable(false);
    }
    
    delete txtot;
    txtot = 0;
    tx_timeout_occured = false;
    
    transmitterStateChange(false);
  }
  
  if (!setPtt(is_transmitting && !tx_timeout_occured))
  {
    perror("setPin");
  }
  
} /* LocalTx::transmit */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


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



/*
 * This file has not been truncated
 */

