/**
@file	 LocalTx.cpp
@brief   Implements a local transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

This file contains a class that implements a local transmitter.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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
#include <limits>

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
#include <AsyncAudioFifo.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioAmp.h>
#include <AsyncAudioMixer.h>
#include <AsyncAudioDebugger.h>
#include <AsyncAudioPacer.h>
#include <common.h>
#include <HdlcFramer.h>
#include <AfskModulator.h>
#include <AsyncAudioFsf.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LocalTx.h"
#include "DtmfEncoder.h"
#include "multirate_filter_coeff.h"
#include "PttCtrl.h"
#include "SigLevDetAfsk.h"
#include "Rx.h"
#include "Emphasis.h"
#include "Ptt.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace sigc;
using namespace SvxLink;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define USE_AUDIO_VALVE 0


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class SineGenerator : public Async::AudioSource
{
  public:
    explicit SineGenerator(const string& audio_dev, int channel)
      : audio_io(audio_dev, channel), pos(0), fq(0.0), level(0.0),
        sample_rate(0), audio_dev_keep_open(false)
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

    void setKeepOpen(bool keep_open)
    {
      audio_dev_keep_open = keep_open;
    }
    
    void enable(bool enable)
    {
      if (enable == (audio_io.mode() != AudioIO::MODE_NONE))
      {
      	return;
      }
      
      if (enable && (fq != 0))
      {
      	if (audio_io.open(AudioIO::MODE_WR))
        {
          pos = 0;
          writeSamples();
        }
      }
      else if (!audio_dev_keep_open)
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
    bool      audio_dev_keep_open;
    
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
  : Tx(name), cfg(cfg), audio_io(0), txtot(0),
    tx_timeout_occured(false), tx_timeout(0), sine_gen(0), ctcss_enable(false),
    dtmf_encoder(0), selector(0), dtmf_valve(0), mixer(0), hdlc_framer(0),
    fsk_mod(0), /*fsk_valve(0),*/ input_handler(0), ptt_ctrl(0),
    audio_valve(0), siglev_sine_gen(0), ptt_hangtimer(0), ptt(0),
    last_rx_id(Rx::ID_UNKNOWN), fsk_first_packet_transmitted(false),
    hdlc_framer_ib(0), fsk_mod_ib(0), ctrl_pty(0), audio_dev_keep_open(false)
{

} /* LocalTx::LocalTx */


LocalTx::~LocalTx(void)
{
  transmit(false);
  if (audio_io != 0)
  {
    audio_io->close();
  }
  
  clearHandler();
  delete input_handler;
  delete selector;
  delete dtmf_encoder;
  delete fsk_mod;
  delete hdlc_framer;
  delete fsk_mod_ib;
  delete hdlc_framer_ib;
  delete mixer;
  
  delete txtot;
  delete ptt;
  delete sine_gen;
  delete siglev_sine_gen;
  delete ptt_hangtimer;
  if (ctrl_pty != 0) ctrl_pty->destroy();
} /* LocalTx::~LocalTx */


bool LocalTx::initialize(void)
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

  ptt = PttFactoryBase::createNamedPtt(cfg, name());
  if ((ptt == 0) || (!ptt->initialize(cfg, name())))
  {
    return false;
  }

  int ptt_hangtime = 0;
  if (cfg.getValue(name(), "PTT_HANGTIME", ptt_hangtime) && (ptt_hangtime > 0))
  {
    ptt_hangtimer = new Timer(ptt_hangtime);
    ptt_hangtimer->expired.connect(
        mem_fun(*this, &LocalTx::pttHangtimeExpired));
    ptt_hangtimer->setEnable(false);
  }

  if (cfg.getValue(name(), "TIMEOUT", value))
  {
    tx_timeout = 1000 * atoi(value.c_str());
  }
  
  int tx_delay = 0;
  if (cfg.getValue(name(), "TX_DELAY", value))
  {
    tx_delay = atoi(value.c_str());
  }

  if (!setPtt(false))
  {
      // FIXME: Maybe "perror" is not the right thing to use here
    perror("setPin");
    return false;
  }
  
  int dtmf_tone_length = 100;
  if (cfg.getValue(name(), "DTMF_TONE_LENGTH", value))
  {
    dtmf_tone_length = atoi(value.c_str());
  }
  
  int dtmf_tone_spacing = 50;
  if (cfg.getValue(name(), "DTMF_TONE_SPACING", value))
  {
    dtmf_tone_spacing = atoi(value.c_str());
  }
  
  int dtmf_tone_amp = -18;
  int dtmf_digit_pwr = -15;
  if (cfg.getValue(name(), "DTMF_TONE_AMP", dtmf_tone_amp))
  {
    cout << "*** WARNING: The DTMF_TONE_AMP configuration variable set in "
            "transmitter " << name() << " is deprecated. Use DTMF_DIGIT_PWR "
            "instead. To get the same output level uing the new configuration "
            "variable, add 3dB to the old value." << endl;
    dtmf_digit_pwr = dtmf_tone_amp + 3;
  }
  cfg.getValue(name(), "DTMF_DIGIT_PWR", dtmf_digit_pwr);
  
  audio_io = new AudioIO(audio_dev, audio_channel);
  // FIXME: Check that the audio device has been correctly initialized
  //        before continuing.
#if 0
  cout << "Sample rate = " << audio_io->sampleRate() << endl;
  if (audio_io->sampleRate() < 0)
  {
    cerr << "*** ERROR: Failed to initialize audio device for transmitter \""
	 << name() << "\".\n";
    return false;
  }
#endif

  cfg.getValue(name(), "AUDIO_DEV_KEEP_OPEN", audio_dev_keep_open);
  if (audio_dev_keep_open && !audio_io->open(AudioIO::MODE_WR))
  {
    cerr << "*** ERROR: Could not open audio device for transmitter \""
         << name() << "\"\n";
    return false;
  }

  sine_gen = new SineGenerator(audio_dev, audio_channel);
  sine_gen->setKeepOpen(audio_dev_keep_open);
  
  if (cfg.getValue(name(), "CTCSS_FQ", value))
  {
    sine_gen->setFq(atof(value.c_str()));
  }  
  
  if (cfg.getValue(name(), "CTCSS_LEVEL", value))
  {
    int level = atoi(value.c_str());
    sine_gen->setLevel(level);
    //audio_io->setGain((100.0 - level) / 100.0);
  }

#if INTERNAL_SAMPLE_RATE >= 16000
  if (cfg.getValue(name(), "TONE_SIGLEV_MAP", value))
  {
    int siglev_level = 10;
    cfg.getValue(name(), "TONE_SIGLEV_LEVEL", siglev_level, true);
    size_t list_len = splitStr(tone_siglev_map, value, ", ");
    if (list_len == 10)
    {
      siglev_sine_gen = new SineGenerator(audio_dev, audio_channel);
      siglev_sine_gen->setLevel(siglev_level);
      siglev_sine_gen->setFq(5500);
    }
    else if (list_len != 0)
    {
      cerr << "*** ERROR: Config variable " << name()
           << "/TONE_SIGLEV_MAP must "
           << "contain exactly ten comma separated siglev values.\n";
    }
  }
#endif

  bool ob_afsk_enable = false;
  cfg.getValue(name(), "OB_AFSK_ENABLE", ob_afsk_enable);
  if (ob_afsk_enable && (siglev_sine_gen != 0))
  {
    cerr << "*** ERROR: Cannot have both siglev tone (TONE_SIGLEV_MAP) and "
            "AFSK (IB/OB_AFSK_ENABLE) enabled at the same time for receiver "
         << name() << ".\n";
      // FIXME: Should we bother to clean up or do we trust that the
      // creator of this object will delete it if the initialization fail?
    return false;
  }
  
  AudioSource *prev_src = 0;
  
    // The input handler is where audio enters this TX object
  input_handler = new AudioPassthrough;
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
  if (cfg.getValue(name(), "PREEMPHASIS", value) && (atoi(value.c_str()) != 0))
  {
    //AudioFilter *preemph = new AudioFilter("HsBq1/0.05/36/3500");
    //preemph->setOutputGain(-9.0f);
    /*
#if INTERNAL_SAMPLE_RATE < 16000
    AudioFilter *preemph = new AudioFilter("LpBu1/3000 x HpBu1/3000");
    preemph->setOutputGain(26);
#else
    AudioFilter *preemph = new AudioFilter("LpBu3/5500 x HpBu1/3000");
    preemph->setOutputGain(21);
#endif
    */

    PreemphasisFilter *preemph = new PreemphasisFilter;
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
  
#if 1
    // Filter out high frequencies generated by the previous clipping
#if (INTERNAL_SAMPLE_RATE == 16000)
  //AudioFilter *splatter_filter = new AudioFilter("LpBu10/5500");
  AudioFilter *splatter_filter = new AudioFilter("LpCh9/-0.05/5500");
#else
  AudioFilter *splatter_filter = new AudioFilter("LpBu20/3500");
#endif
  prev_src->registerSink(splatter_filter, true);
  prev_src = splatter_filter;
#endif
  
    // Create a valve so that we can control when to transmit audio
  #if USE_AUDIO_VALVE
  audio_valve = new AudioValve;
  audio_valve->setBlockWhenClosed(true);
  audio_valve->setOpen(true);
  prev_src->registerSink(audio_valve, true);
  prev_src = audio_valve;
  #endif
  
    // We need a selector to choose if DTMF or normal audio should be
    // transmitted
  selector = new AudioSelector;
  //selector->setDebug(true);
  //prev_src = new AudioDebugger(prev_src, "content");
  selector->addSource(prev_src);
  selector->enableAutoSelect(prev_src, 0);
  prev_src = selector;
  //prev_src = new AudioDebugger(prev_src, "selector");
  
    // Create the DTMF encoder
  dtmf_encoder = new DtmfEncoder(INTERNAL_SAMPLE_RATE);
  dtmf_encoder->allDigitsSent.connect(
      mem_fun(*this, &LocalTx::allDtmfDigitsSent));
  dtmf_encoder->setDigitDuration(dtmf_tone_length);
  dtmf_encoder->setDigitSpacing(dtmf_tone_spacing);
  dtmf_encoder->setDigitPower(dtmf_digit_pwr);
  
    // Create a valve so that we can control when to transmit DTMF
  dtmf_valve = new AudioValve;
  dtmf_valve->setBlockWhenClosed(true);
  dtmf_valve->setOpen(false);
  dtmf_encoder->registerSink(dtmf_valve, true);
  selector->addSource(dtmf_valve);
  selector->enableAutoSelect(dtmf_valve, 10);
  selector->setFlushWait(dtmf_valve, false);
  
  if (ob_afsk_enable)
  {
    unsigned fc = 5500;
    //cfg.getValue(name(), "OB_AFSK_CENTER_FQ", fc);
    unsigned shift = 170;
    //cfg.getValue(name(), "OB_AFSK_SHIFT", shift);
    unsigned baudrate = 300;
    //cfg.getValue(name(), "OB_AFSK_BAUDRATE", baudrate);
    float voice_gain = -6.0f;
    cfg.getValue(name(), "OB_AFSK_VOICE_GAIN", voice_gain);
    float afsk_level = -12.0f;
    cfg.getValue(name(), "OB_AFSK_LEVEL", afsk_level);
    unsigned afsk_tx_delay = 100;
    cfg.getValue(name(), "OB_AFSK_TX_DELAY", afsk_tx_delay);

    AudioFilter *voice_filter = new AudioFilter("LpCh10/-0.5/4500");
    voice_filter->setOutputGain(voice_gain);
    prev_src->registerSink(voice_filter, true);
    prev_src = voice_filter;
    AudioFilter *voice_filter2 = new AudioFilter("LpCh10/-0.5/4500");
    prev_src->registerSink(voice_filter2, true);
    prev_src = voice_filter2;

      // Create a mixer so that we can mix other audio with the voice audio
    mixer = new AudioMixer;
    mixer->addSource(prev_src);
    prev_src = mixer;

      // Create the HDLC framer
    hdlc_framer = new HdlcFramer;
    hdlc_framer->setStartFlagCnt(
        static_cast<size_t>(ceil(afsk_tx_delay * baudrate / 8000.0)));

      // Create the AFSK modulator
    fsk_mod = new AfskModulator(fc - shift / 2, fc + shift / 2, baudrate,
                                afsk_level);
    hdlc_framer->sendBits.connect(mem_fun(fsk_mod, &AfskModulator::sendBits));

      // Frequency sampling filter with passband center 5500Hz, about 400Hz
      // wide and about 40dB stop band attenuation
    const size_t N = 128;
    float coeff[N/2+1];
    memset(coeff, 0, sizeof(coeff));
    coeff[42] = 0.39811024;
    coeff[43] = 1.0;
    coeff[44] = 1.0;
    coeff[45] = 1.0;
    coeff[46] = 0.39811024;
    AudioFsf *fsf = new AudioFsf(N, coeff);
    fsk_mod->registerSink(fsf, true);

    mixer->addSource(fsf);

    /*
    AudioPacer *fsk_pacer = new AudioPacer(INTERNAL_SAMPLE_RATE, 256, 20);
    fsk_mod->registerSink(fsk_pacer, true);
    mixer->addSource(fsk_pacer);
    */

      // Create a valve so that we can control when to transmit AFSK
    /*
    fsk_valve = new AudioValve;
    fsk_valve->setBlockWhenClosed(true);
    fsk_valve->setOpen(false);
    fsk_valve->registerSink(fsk_valve, true);
    selector->addSource(fsk_valve);
    selector->enableAutoSelect(fsk_valve, 20);
    */
  }

  bool ib_afsk_enable = false;
  cfg.getValue(name(), "IB_AFSK_ENABLE", ib_afsk_enable);
  if (ob_afsk_enable && ib_afsk_enable)
  {
    unsigned fc = 1700;
    //cfg.getValue(name(), "IB_AFSK_CENTER_FQ", fc);
    unsigned shift = 1000;
    //cfg.getValue(name(), "IB_AFSK_SHIFT", shift);
    unsigned baudrate = 1200;
    //cfg.getValue(name(), "IB_AFSK_BAUDRATE", baudrate);
    float afsk_level = -6;
    cfg.getValue(name(), "IB_AFSK_LEVEL", afsk_level);
    unsigned afsk_tx_delay = 100;
    cfg.getValue(name(), "IB_AFSK_TX_DELAY", afsk_tx_delay);

      // Create the inband HDLC framer
    hdlc_framer_ib = new HdlcFramer;
    hdlc_framer_ib->setStartFlagCnt(
        static_cast<size_t>(ceil(afsk_tx_delay * baudrate / 8000.0)));

      // Create the inband AFSK modulator
    fsk_mod_ib = new AfskModulator(fc - shift / 2, fc + shift / 2, baudrate,
                                afsk_level);
    hdlc_framer_ib->sendBits.connect(
        mem_fun(fsk_mod_ib, &AfskModulator::sendBits));

    AudioPacer *pacer = new AudioPacer(INTERNAL_SAMPLE_RATE, 256, 0);
    fsk_mod_ib->registerSink(pacer);

      // Connect the inband AFSK modulator to the main audio selector
    selector->addSource(pacer);
    selector->enableAutoSelect(pacer, 20);
    selector->setFlushWait(pacer, false);
  }

  /*
  AudioDebugger *d1 = new AudioDebugger;
  prev_src->registerSink(d1, true);
  prev_src = d1;
  */

    // Create the PTT controller
  ptt_ctrl = new PttCtrl(tx_delay);
  ptt_ctrl->transmitterStateChange.connect(mem_fun(*this, &LocalTx::transmit));
  ptt_ctrl->preTransmitterStateChange.connect(
      mem_fun(*this, &LocalTx::preTransmitterStateChange));
  prev_src->registerSink(ptt_ctrl, true);
  prev_src = ptt_ctrl;

  float master_gain = 0.0f;
  if (cfg.getValue(name(), "MASTER_GAIN", master_gain))
  {
    AudioAmp *master_gain_stage = new AudioAmp;
    master_gain_stage->setGain(master_gain);
    prev_src->registerSink(master_gain_stage, true);
    prev_src = master_gain_stage;
  }

#if (INTERNAL_SAMPLE_RATE != 16000)  
  if (audio_io->sampleRate() > 8000)
  {
      // Interpolate sample rate to 16kHz
    AudioInterpolator *i1 = new AudioInterpolator(2, coeff_16_8,
                                                  coeff_16_8_taps);
    prev_src->registerSink(i1, true);
    prev_src = i1;
  }
#endif

  if (audio_io->sampleRate() > 16000)
  {
      // Interpolate sample rate to 48kHz
#if (INTERNAL_SAMPLE_RATE == 8000)
    AudioInterpolator *i2 = new AudioInterpolator(3, coeff_48_16_int,
                                                  coeff_48_16_int_taps);
#else
    AudioInterpolator *i2 = new AudioInterpolator(3, coeff_48_16,
                                                  coeff_48_16_taps);
#endif
    prev_src->registerSink(i2, true);
    prev_src = i2;
  }
  
    // Finally connect the whole audio pipe to the audio device
  prev_src->registerSink(audio_io, true);

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


void LocalTx::enableCtcss(bool enable)
{
  ctcss_enable = enable;
  if (isTransmitting())
  {
    sine_gen->enable(enable);
  }
} /* LocalTx::enableCtcss */


void LocalTx::sendDtmf(const string& digits, unsigned duration)
{
  if (fsk_mod != 0)
  {
    if (duration == 0)
    {
      duration = dtmf_encoder->digitDuration();
    }
    sendFskDtmf(digits, duration);
  }
  else
  {
    #if USE_AUDIO_VALVE
    audio_valve->setOpen(false);
    #endif
    dtmf_encoder->send(digits, duration);
  }
} /* LocalTx::sendDtmf */


void LocalTx::sendData(const std::vector<uint8_t> &msg)
{
  if (hdlc_framer != 0)
  {
    hdlc_framer->sendBytes(msg);
  }
} /* LocalTx::sendData */


void LocalTx::setTransmittedSignalStrength(char rx_id, float siglev)
{
  //cout << "### LocalTx::setTransmittedSignalStrength: rx_id=" << rx_id
  //     << " siglev=" << siglev
  //     << endl;

#if INTERNAL_SAMPLE_RATE >= 16000
  if (hdlc_framer != 0)
  {
    fsk_trailer_transmitted = false;

    uint8_t siglevui;
    if (siglev < 0.0f)
    {
      siglevui = 0;
    }
    else if (siglev > 255.0f)
    {
      siglevui = 255;
    }
    else
    {
      siglevui = static_cast<uint8_t>(siglev);
    }
    sendFskSiglev(rx_id, siglevui);
    last_rx_id = rx_id;
  }
  else if (siglev_sine_gen != 0)
  {
    int siglevi = static_cast<int>(siglev);
    if (tone_siglev_map[0] > tone_siglev_map[9])
    {
      for (int i=0; i<10; ++i)
      {
        if (tone_siglev_map[i] <= siglevi)
        {
          siglev_sine_gen->setFq(5500 + i*100);
          siglev_sine_gen->enable(true);
          return;
        }
      }
    }
    else
    {
      for (int i=9; i>=0; --i)
      {
        if (tone_siglev_map[i] <= siglevi)
        {
          siglev_sine_gen->setFq(5500 + i*100);
          siglev_sine_gen->enable(true);
          return;
        }
      }
    }

    siglev_sine_gen->enable(false);
  }
#endif
} /* LocalTx::setTransmittedSignalLevel */


void LocalTx::setFq(unsigned fq)
{
  if (ctrl_pty != 0)
  {
    ostringstream ss;
    ss << "F" << fq << ";";
    ssize_t ret = ctrl_pty->write(ss.str().c_str(), ss.str().size());
    if (ret != static_cast<ssize_t>(ss.str().size()))
    {
      cerr << "*** WARNING[" << name() << "]: Failed to write transmit "
              "frequency command to PTY " << ctrl_pty->name()
           << ": " << ss.str() << endl;
    }
  }
} /* LocalTx::setFq */


void LocalTx::setModulation(Modulation::Type mod)
{
  if (ctrl_pty != 0)
  {
    ostringstream ss;
    ss << "M" << Modulation::toString(mod) << ";";
    ssize_t ret = ctrl_pty->write(ss.str().c_str(), ss.str().size());
    if (ret != static_cast<ssize_t>(ss.str().size()))
    {
      cerr << "*** WARNING[" << name() << "]: Failed to write set transmit "
              "modulation command to PTY " << ctrl_pty->name()
           << ": " << ss.str() << endl;
    }
  }
} /* LocalTx::setModulation */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void LocalTx::transmit(bool do_transmit)
{
  if (do_transmit == isTransmitting())
  {
    return;
  }
  
  //cout << name() << ": Turning the transmitter " << (do_transmit ? "ON" : "OFF")
  //     << endl;
  
  if (do_transmit)
  {
    fsk_trailer_transmitted = false;

    setIsTransmitting(true);

    if (!audio_io->open(AudioIO::MODE_WR))
    {
      cerr << "*** ERROR: Could not open audio device for transmitter \""
           << name() << "\"\n";
      //is_transmitting = false;
      //return;
    }
    
    if (ctcss_enable)
    {
      sine_gen->enable(true);
    }

    if (siglev_sine_gen != 0)
    {
      siglev_sine_gen->enable(true);
    }

    if ((txtot == 0) && (tx_timeout > 0))
    {
      txtot = new Timer(tx_timeout);
      txtot->expired.connect(mem_fun(*this, &LocalTx::txTimeoutOccured));
    }
  }
  else
  {
    if (!audio_dev_keep_open)
    {
      audio_io->close();
    }

    if (ctcss_enable)
    {
      sine_gen->enable(false);
    }

    if (siglev_sine_gen != 0)
    {
      siglev_sine_gen->enable(false);
    }
    
    delete txtot;
    txtot = 0;
    tx_timeout_occured = false;

    setIsTransmitting(false);
  }
  
  if (!setPtt(isTransmitting() && !tx_timeout_occured, true))
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
  
  cerr << "*** ERROR: Transmitter " << name()
       << " have been active for too long. Turning it off...\n";
  
  if (!setPtt(false))
  {
    perror("setPin");
  }
  
  tx_timeout_occured = true;
  txTimeout();
} /* LocalTx::txTimeoutOccured */


bool LocalTx::setPtt(bool tx, bool with_hangtime)
{
  if (ptt_hangtimer != 0)
  {
    if (!tx && with_hangtime)
    {
      ptt_hangtimer->setEnable(true);
      return true;
    }
    ptt_hangtimer->setEnable(false);
  }

  if (!ptt->setTxOn(tx))
  {
    return false;
  }

  return true;

} /* LocalTx::setPtt */


void LocalTx::allDtmfDigitsSent(void)
{
  #if USE_AUDIO_VALVE
  audio_valve->setOpen(true);
  #endif
} /* LocalTx::allDtmfDigitsSent  */


void LocalTx::pttHangtimeExpired(Timer *t)
{
  setPtt(false);
} /* LocalTx::pttHangtimeExpired */


bool LocalTx::preTransmitterStateChange(bool do_transmit)
{
  //cout << name() << ": LocalTx::preTransmitterStateChange: do_transmit="
  //     << do_transmit << endl;

  if (do_transmit)
  {
    return false;
  }

  //cout << "### fsk_trailer_transmitted=" << fsk_trailer_transmitted << endl;
  if (fsk_mod != 0) {
    if (!fsk_trailer_transmitted)
    {
      //cout << "  Sending AFSK trailer\n";
      fsk_trailer_transmitted = true;
      sendFskSiglev(last_rx_id, 0);
      sendFskSiglev(last_rx_id, 0);
      last_rx_id = Rx::ID_UNKNOWN;
      return true;
    }
    else
    {
      //cout << "### fsk_first_packet_transmitted = false\n";
      fsk_first_packet_transmitted = false;
    }
  }

  return false;

} /* LocalTx::preTransmitterStateChange */


void LocalTx::sendFskSiglev(char rxid, uint8_t siglev)
{
  //cout << "### LocalTx::sendFskSiglev: rxid=" << rxid
  //     << " siglev=" << (int)siglev << endl;
  if ((rxid < '!') || (rxid > '~'))
  {
    rxid = Rx::ID_UNKNOWN;
  }

  vector<uint8_t> frame;
  frame.push_back(DATA_CMD_SIGLEV);
  frame.push_back(static_cast<uint8_t>(rxid));
  frame.push_back(siglev);
  //cout << "### fsk_first_packet_transmitted=" << fsk_first_packet_transmitted
  //     << endl;
  if (fsk_first_packet_transmitted || (hdlc_framer_ib == 0))
  {
    hdlc_framer->sendBytes(frame);
  }
  else
  {
    //cout << "### Send first packet ---\n";
    fsk_first_packet_transmitted = true;
    hdlc_framer_ib->sendBytes(frame);
  }
} /* LocalTx::sendFskSiglev */


void LocalTx::sendFskDtmf(const string &digits, unsigned duration)
{
  if (duration > numeric_limits<uint16_t>::max())
  {
    duration = numeric_limits<uint16_t>::max();
  }

  vector<uint8_t> frame;
  frame.push_back(DATA_CMD_DTMF);
  frame.push_back(duration & 0xff);
  frame.push_back(duration >> 8);
  for (size_t i=0; i<digits.size(); ++i)
  {
    frame.push_back(digits[i]);
  }
  hdlc_framer->sendBytes(frame);
} /* LocalTx::sendFskDtmf */



/*
 * This file has not been truncated
 */
