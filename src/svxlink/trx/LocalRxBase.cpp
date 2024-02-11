/**
@file	 LocalRxBase.cpp
@brief   A base receiver class to handle local receivers
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

This file contains a class that handle local receivers. A local receiver is
a receiver that is directly connected to the sound card on the computer where
the SvxLink core is running. It can also be a DDR (Digital Drop Receiver).

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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
#include <cstring>
#include <limits>
#include <json/json.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioFilter.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioAmp.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncAudioDecimator.h>
#include <AsyncAudioClipper.h>
#include <AsyncAudioCompressor.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioStreamStateDetector.h>
#include <AsyncAudioFsf.h>
#include <AsyncUdpSocket.h>
#include <common.h>
#ifdef LADSPA_VERSION
#include <AsyncAudioLADSPAPlugin.h>
#endif


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDet.h"
#include "DtmfDecoder.h"
#include "ToneDetector.h"
#include "SquelchCtcss.h"
#include "LocalRxBase.h"
#include "multirate_filter_coeff.h"
#include "Sel5Decoder.h"
#include "AfskDemodulator.h"
#include "Synchronizer.h"
#include "HdlcDeframer.h"
#include "Tx.h"
#include "Emphasis.h"


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

#define DTMF_MUTING_POST        200
#define TONE_1750_MUTING_PRE    75
#define TONE_1750_MUTING_POST   100
#define DEFAULT_LIMITER_THRESH  -1.0


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

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
	     << ": Distortion detected! Please lower the input volume!\n";
      }
      
      return ret;
    }
  
  private:
    string name;
    
};


class AudioUdpSink : public UdpSocket, public AudioSink
{
  public:
    AudioUdpSink(const IpAddress &remote_ip, uint16_t remote_port,
                 uint16_t local_port=0, const IpAddress &bind_ip=IpAddress())
      : UdpSocket(local_port, bind_ip), remote_ip(remote_ip),
        remote_port(remote_port)
    {
    }

    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count)
    {
      const char *buf = reinterpret_cast<const char *>(samples);
      size_t len = count * sizeof(*samples);
      UdpSocket::write(remote_ip, remote_port, buf, len);
      return count;
    }

    /**
     * @brief   Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void)
    {
      UdpSocket::write(remote_ip, remote_port, NULL, 0);
    }

  private:
    Async::IpAddress  remote_ip;
    uint16_t          remote_port;

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

namespace {
  typedef const char *CfgTag;
  CfgTag CFG_SQL_EXTENDED_HANGTIME_THRESH = "SQL_EXTENDED_HANGTIME_THRESH";
};


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

LocalRxBase::LocalRxBase(Config &cfg, const std::string& name)
  : Rx(cfg, name),
    squelch_det(0), siglevdet(0), /* siglev_offset(0.0), siglev_slope(1.0), */
    tone_dets(0), sql_valve(0), delay(0), sql_tail_elim(0),
    preamp_gain(0), mute_valve(0), sql_hangtime(0), sql_extended_hangtime(0),
    sql_extended_hangtime_thresh(0), input_fifo(0), dtmf_muting_pre(0),
    ob_afsk_deframer(0), ib_afsk_deframer(0), audio_dev_keep_open(false)
{
} /* LocalRxBase::LocalRxBase */


LocalRxBase::~LocalRxBase(void)
{
  clearHandler();
  delete input_fifo;  // This will delete the whole chain of audio objects
  input_fifo = 0;
  delete ob_afsk_deframer;
  ob_afsk_deframer = 0;
  delete ib_afsk_deframer;
  ib_afsk_deframer = 0;
} /* LocalRxBase::~LocalRxBase */


bool LocalRxBase::initialize(void)
{
  if (!Rx::initialize())
  {
    return false;
  }
  
  bool deemphasis = false;
  cfg().getValue(name(), "DEEMPHASIS", deemphasis);
  
  int delay_line_len = 0;
  bool  mute_1750 = false;
  cfg().getValue(name(), "1750_MUTING", mute_1750);
  if (mute_1750)
  {
    delay_line_len = max(delay_line_len, TONE_1750_MUTING_PRE);
  }

  cfg().getValue(name(), "SQL_TAIL_ELIM", sql_tail_elim);
  if (sql_tail_elim > 0)
  {
    delay_line_len = max(delay_line_len, sql_tail_elim);
  }
  
  cfg().getValue(name(), "PREAMP", preamp_gain);
  
  bool peak_meter = false;
  cfg().getValue(name(), "PEAK_METER", peak_meter);
  
    // Get the audio source object
  AudioSource *prev_src = audioSource();
  assert(prev_src != 0);

    // Valve used to mute the audio device on MUTE_ALL
  mute_valve = new Async::AudioValve;
  mute_valve->setOpen(false);
  prev_src->registerSink(mute_valve, true);
  prev_src = mute_valve;

    // Create a fifo buffer to handle large audio blocks
  input_fifo = new AudioFifo(1024);
//  input_fifo->setOverwrite(true);
  prev_src->registerSink(input_fifo);
  prev_src = input_fifo;

  SvxLink::SepPair<string, uint16_t> raw_audio_fwd_dest;
  if (cfg().getValue(name(), "RAW_AUDIO_UDP_DEST", raw_audio_fwd_dest))
  {
    AudioSplitter *raw_audio_splitter = new AudioSplitter;
    prev_src->registerSink(raw_audio_splitter, true);
    AudioPassthrough *pass = new AudioPassthrough;
    raw_audio_splitter->addSink(pass, true);
    prev_src = pass;
    AudioUdpSink *udp = new AudioUdpSink(IpAddress(raw_audio_fwd_dest.first),
                                         raw_audio_fwd_dest.second);
    if (!udp->initOk())
    {
      cerr << "*** ERROR: Could not open UDP socket for raw audio output\n";
      return false;

    }
    raw_audio_splitter->addSink(udp, true);
  }
  
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
  if (audioSampleRate() > 16000)
  {
    AudioDecimator *d1 = new AudioDecimator(3, coeff_48_16_wide,
					    coeff_48_16_wide_taps);
    prev_src->registerSink(d1, true);
    prev_src = d1;
  }

  AudioSplitter *siglevdet_splitter = 0;
  siglevdet_splitter = new AudioSplitter;
  prev_src->registerSink(siglevdet_splitter, true);
  prev_src = 0;

    // Create the signal level detector
  siglevdet = createSigLevDet(cfg(), name());
  if (siglevdet == 0)
  {
    return false;
  }
  siglevdet->setIntegrationTime(0);
  siglevdet->signalLevelUpdated.connect(
      mem_fun(*this, &LocalRxBase::onSignalLevelUpdated));
  siglevdet_splitter->addSink(siglevdet, true);
  dataReceived.connect(mem_fun(siglevdet, &SigLevDet::frameReceived));

    // Add a passthrough element to use as a connector between the splitter and
    // the rest of the audio pipe
  auto siglevdet_splitter_pass = new Async::AudioPassthrough;
  siglevdet_splitter->addSink(siglevdet_splitter_pass, true);
  prev_src = siglevdet_splitter_pass;

#if (INTERNAL_SAMPLE_RATE != 16000)
    // If the sound card sample rate is higher than 8kHz (16 or 48kHz assumed)
    // decimate it down to 8kHz.
    // 16kHz audio to other consumers.
  if (audioSampleRate() > 8000)
  {
    AudioDecimator *d2 = new AudioDecimator(2, coeff_16_8, coeff_16_8_taps);
    prev_src->registerSink(d2, true);
    prev_src = d2;
  }
#endif

    // If a deemphasis filter was configured, create it
  if (deemphasis)
  {
    //AudioFilter *deemph_filt = new AudioFilter("LpBu1/300");
    //AudioFilter *deemph_filt = new AudioFilter("HsBq1/0.01/-18/3500");
    //AudioFilter *deemph_filt = new AudioFilter("HsBq1/0.05/-36/3500");
    //deemph_filt->setOutputGain(9.0f);
    //AudioFilter *deemph_filt = new AudioFilter("HpBu1/50 x LpBu1/150");
    //deemph_filt->setOutputGain(7.0f);

    DeemphasisFilter *deemph_filt = new DeemphasisFilter;
    prev_src->registerSink(deemph_filt, true);
    prev_src = deemph_filt;
  }
  
    // Create a splitter to distribute full bandwidth audio to all consumers
  fullband_splitter = new AudioSplitter;
  prev_src->registerSink(fullband_splitter, true);
  prev_src = fullband_splitter;

    // Create the configured squelch detector and initialize it
  string sql_det_str;
  if (!cfg().getValue(name(), "SQL_DET", sql_det_str))
  {
    cerr << "*** ERROR: Config variable " << name() << "/SQL_DET not set\n";
    return false;
  }

  squelch_det = createSquelch(sql_det_str);
  if (squelch_det == 0)
  {
    cerr << "*** ERROR: Unknown squelch type specified in config variable "
         << name() << "/SQL_DET. Legal squelch types are: "
         << SquelchFactory::validFactories() << std::endl;
    // FIXME: Cleanup
    return false;
  }
  if (!squelch_det->initialize(cfg(), name()))
  {
    std::cerr << "*** ERROR: Squelch detector initialization failed for RX \""
              << name() << "\"" << std::endl;
    delete squelch_det;
    squelch_det = 0;
    // FIXME: Cleanup
    return false;
  }
  if (sql_det_str == SquelchCtcss::OBJNAME)
  {
    SquelchCtcss *squelch_ctcss = dynamic_cast<SquelchCtcss*>(squelch_det);
    squelch_ctcss->snrUpdated.connect(ctcssSnrUpdated.make_slot());
  }

  readyStateChanged.connect(mem_fun(*this, &LocalRxBase::rxReadyStateChanged));

  cfg().getValue(name(), CFG_SQL_EXTENDED_HANGTIME_THRESH,
                         sql_extended_hangtime_thresh);

  squelch_det->squelchOpen.connect(mem_fun(*this, &LocalRxBase::onSquelchOpen));
  squelch_det->toneDetected.connect(mem_fun(*this, &LocalRxBase::onToneDetected));
  fullband_splitter->addSink(squelch_det, true);

  squelchOpen.connect(
      sigc::hide(sigc::mem_fun(*this, &LocalRxBase::publishSquelchState)));

    // Set up out of band AFSK demodulator if configured
  float voice_gain = 0.0f;
  bool ob_afsk_enable = false;
  if (cfg().getValue(name(), "OB_AFSK_ENABLE", ob_afsk_enable) && ob_afsk_enable)
  {
    unsigned fc = 5500;
    //cfg().getValue(name(), "OB_AFSK_CENTER_FQ", fc);
    unsigned shift = 170;
    //cfg().getValue(name(), "OB_AFSK_SHIFT", shift);
    unsigned baudrate = 300;
    //cfg().getValue(name(), "OB_AFSK_BAUDRATE", baudrate);
    voice_gain = 6.0f;
    cfg().getValue(name(), "OB_AFSK_VOICE_GAIN", voice_gain);

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
    //prev_src->registerSink(fsf, true);
    fullband_splitter->addSink(fsf, true);
    AudioSource *prev_src = fsf;

    AfskDemodulator *fsk_demod =
      new AfskDemodulator(fc - shift/2, fc + shift/2, baudrate);
    //fullband_splitter->addSink(fsk_demod, true);
    prev_src->registerSink(fsk_demod, true);
    prev_src = fsk_demod;

    Synchronizer *sync = new Synchronizer(baudrate);
    prev_src->registerSink(sync, true);
    prev_src = 0;

    ob_afsk_deframer = new HdlcDeframer;
    ob_afsk_deframer->frameReceived.connect(
        mem_fun(*this, &LocalRxBase::dataFrameReceived));
    sync->bitsReceived.connect(
        mem_fun(ob_afsk_deframer, &HdlcDeframer::bitsReceived));
  }

  bool ib_afsk_enable = false;
  if (cfg().getValue(name(), "IB_AFSK_ENABLE", ib_afsk_enable) && ib_afsk_enable)
  {
    unsigned fc = 1700;
    //cfg().getValue(name(), "IB_AFSK_CENTER_FQ", fc);
    unsigned shift = 1000;
    //cfg().getValue(name(), "IB_AFSK_SHIFT", shift);
    unsigned baudrate = 1200;
    //cfg().getValue(name(), "IB_AFSK_BAUDRATE", baudrate);

    AfskDemodulator *fsk_demod =
      new AfskDemodulator(fc - shift/2, fc + shift/2, baudrate);
    fullband_splitter->addSink(fsk_demod, true);
    AudioSource *prev_src = fsk_demod;

    Synchronizer *sync = new Synchronizer(baudrate);
    prev_src->registerSink(sync, true);
    prev_src = 0;

    ib_afsk_deframer = new HdlcDeframer;
    ib_afsk_deframer->frameReceived.connect(
        mem_fun(*this, &LocalRxBase::dataFrameReceivedIb));
    sync->bitsReceived.connect(
        mem_fun(ib_afsk_deframer, &HdlcDeframer::bitsReceived));
  }

    // Create a new audio splitter to handle tone detectors
  tone_dets = new AudioSplitter;
  prev_src->registerSink(tone_dets, true);
  prev_src = tone_dets;

    // Filter out the voice band, removing high- and subaudible frequencies,
    // for example CTCSS.
#if (INTERNAL_SAMPLE_RATE == 16000)
  AudioFilter *voiceband_filter = new AudioFilter("BpCh12/-0.1/300-5000");
#else
  AudioFilter *voiceband_filter = new AudioFilter("BpCh12/-0.1/300-3500");
#endif
  prev_src->registerSink(voiceband_filter, true);
  prev_src = voiceband_filter;

    // Create an audio splitter to distribute the voiceband audio to all
    // other consumers
  AudioSplitter *voiceband_splitter = new AudioSplitter;
  prev_src->registerSink(voiceband_splitter, true);
  prev_src = voiceband_splitter;

    // Create the configured type of DTMF decoder and add it to the splitter
  string dtmf_dec_type("NONE");
  cfg().getValue(name(), "DTMF_DEC_TYPE", dtmf_dec_type);
  if (dtmf_dec_type != "NONE")
  {
    DtmfDecoder *dtmf_dec = DtmfDecoder::create(this, cfg(), name());
    if ((dtmf_dec == 0) || !dtmf_dec->initialize())
    {
      // FIXME: Cleanup?
      delete dtmf_dec;
      return false;
    }
    dtmf_dec->digitActivated.connect(
        mem_fun(*this, &LocalRxBase::dtmfDigitActivated));
    dtmf_dec->digitDeactivated.connect(
        mem_fun(*this, &LocalRxBase::dtmfDigitDeactivated));
    voiceband_splitter->addSink(dtmf_dec, true);

    bool dtmf_muting = false;
    cfg().getValue(name(), "DTMF_MUTING", dtmf_muting);
    if (dtmf_muting)
    {
      dtmf_muting_pre = dtmf_dec->detectionTime();
      delay_line_len = max(delay_line_len, dtmf_muting_pre);
    }
  }
  
    // Create a selective multiple tone detector object
  string sel5_dec_type("NONE");
  cfg().getValue(name(), "SEL5_DEC_TYPE", sel5_dec_type);
  if (sel5_dec_type != "NONE")
  {
    Sel5Decoder *sel5_dec = Sel5Decoder::create(cfg(), name());
    if (sel5_dec == 0 || !sel5_dec->initialize())
    {
      cerr << "*** ERROR: Sel5 decoder initialization failed for RX \""
          << name() << "\"\n";
      return false;
    }
    sel5_dec->sequenceDetected.connect(
        mem_fun(*this, &LocalRxBase::sel5Detected));
    voiceband_splitter->addSink(sel5_dec, true);
  }

    // Create an audio valve to use as squelch and connect it to the splitter
  sql_valve = new AudioValve;
  sql_valve->setOpen(false);
  prev_src->registerSink(sql_valve, true);
  prev_src = sql_valve;

    // Create the state detector
  AudioStreamStateDetector *state_det = new AudioStreamStateDetector;
  state_det->sigStreamStateChanged.connect(
            mem_fun(*this, &LocalRxBase::audioStreamStateChange));
  prev_src->registerSink(state_det, true);
  prev_src = state_det;

    // If we need a delay line (e.g. for DTMF muting and/or squelch tail
    // elimination), create it
  if (delay_line_len > 0)
  {
    std::cout << name() << ": Delay line (for DTMF muting etc) set to "
              << delay_line_len << " ms" << std::endl;
    delay = new AudioDelayLine(delay_line_len);
    prev_src->registerSink(delay, true);
    prev_src = delay;
  }

#ifdef LADSPA_VERSION
  std::vector<std::string> ladspa_plugin_cfg;
  if (cfg().getValue(name(), "LADSPA_PLUGINS", ladspa_plugin_cfg))
  {
    for (const auto& pcfg : ladspa_plugin_cfg)
    {
      std::istringstream is(pcfg);
      std::string label;
      std::getline(is, label, ':');
      //std::cout << "### pcfg=" << pcfg << "  label=" << label << std::endl;
      auto plug = new Async::AudioLADSPAPlugin(label);
      if (!plug->initialize())
      {
        std::cout << "*** ERROR: Could not instantiate LADSPA plugin instance "
                     "with label \"" << label << "\"" << std::endl;
        return false;
      }
      unsigned long portno = 0;
      LADSPA_Data val;
      while (is >> val)
      {
        while ((portno < plug->portCount()) &&
               !(plug->portIsControl(portno) && plug->portIsInput(portno)))
        {
          ++portno;
        }
        if (portno >= plug->portCount())
        {
          std::cerr << "*** ERROR: Too many parameters specified for LADSPA "
                       "plugin \"" << plug->label()
                    << "\" in configuration variable " << name()
                    << "/LADSPA_PLUGINS." << std::endl;
          return false;
        }
        plug->setControl(portno++, val);
        char colon = 0;
        if ((is >> colon) && (colon != ':'))
        {
          std::cerr << "*** ERROR: Illegal format for " << name()
                    << "/LADSPA_PLUGINS configuration variable" << std::endl;
          return false;
        }
      }

      plug->print(name() + ": ");

      prev_src->registerSink(plug, true);
      prev_src = plug;
    }
  }
#endif

    // Add a limiter to smoothly limit the audio before hard clipping it
  double limiter_thresh = DEFAULT_LIMITER_THRESH;
  cfg().getValue(name(), "LIMITER_THRESH", limiter_thresh);
  if (limiter_thresh != 0.0)
  {
    AudioCompressor *limit = new AudioCompressor;
    limit->setThreshold(limiter_thresh);
    limit->setRatio(0.1);
    limit->setAttack(2);
    limit->setDecay(20);
    limit->setOutputGain(1);
    prev_src->registerSink(limit, true);
    prev_src = limit;
  }

    // Clip audio to limit its amplitude
  AudioClipper *clipper = new AudioClipper;
  clipper->setClipLevel(0.98);
  prev_src->registerSink(clipper, true);
  prev_src = clipper;

    // Remove high frequencies generated by the previous clipping
#if (INTERNAL_SAMPLE_RATE == 16000)
  AudioFilter *splatter_filter = new AudioFilter("LpCh9/-0.05/5000");
#else
  AudioFilter *splatter_filter = new AudioFilter("LpCh9/-0.05/3500");
#endif
  prev_src->registerSink(splatter_filter, true);
  prev_src = splatter_filter;
  
    // Set the previous audio pipe object to handle audio distribution for
    // the LocalRxBase class
  setAudioSourceHandler(prev_src);
  
  cfg().getValue(name(), "AUDIO_DEV_KEEP_OPEN", audio_dev_keep_open);

    // Open the audio device for reading
  if (!audioOpen())
  {
    // FIXME: Cleanup?
    return false;
  }
  
  if (mute_1750)
  {
    ToneDetector *calldet = new ToneDetector(1750, 50, 100);
    assert(calldet != 0);
    calldet->setPeakThresh(13);
    calldet->activated.connect(mem_fun(*this, &LocalRxBase::tone1750detected));
    voiceband_splitter->addSink(calldet, true);
    //cout << "### Enabling 1750Hz muting\n";
  }

  cfg().valueUpdated.connect(sigc::mem_fun(*this, &LocalRxBase::cfgUpdated));

  return true;

} /* LocalRxBase:initialize */


void LocalRxBase::setMuteState(MuteState new_mute_state)
{
  auto mute_state = muteState();

  //std::cout << "### LocalRxBase::setMuteState[" << name()
  //          << "]: new_mute_state=" << new_mute_state
  //          << " mute_state=" << mute_state
  //          << std::endl;

  while (mute_state != new_mute_state)
  {
    assert((mute_state >= MUTE_NONE) && (mute_state <= MUTE_ALL));

    if (new_mute_state > mute_state)  // Muting requested
    {
      mute_state = static_cast<MuteState>(mute_state + 1);
      switch (mute_state)
      {
        case MUTE_CONTENT:  // MUTE_NONE -> MUTE_CONTENT
          if (delay != 0)
          {
            delay->clear();
          }
          sql_valve->setOpen(false);
          break;

        case MUTE_ALL:  // MUTE_CONTENT -> MUTE_ALL
          mute_valve->setOpen(false);
          if (!audio_dev_keep_open)
          {
            audioClose();
          }
          squelch_det->reset();
          siglevdet->reset();
          setSquelchState(false, "MUTED");
          break;

        default:
          break;
      }
    }
    else                              // Unmuting requested
    {
      mute_state = static_cast<MuteState>(mute_state - 1);
      switch (mute_state)
      {
        case MUTE_CONTENT:  // MUTE_ALL -> MUTE_CONTENT
          mute_valve->setOpen(true);
          if (!audioOpen())
          {
            Rx::setMuteState(MUTE_ALL);
            return;
          }
          squelch_det->restart();
          break;

        case MUTE_NONE:   // MUTE_CONTENT -> MUTE_NONE
          //mute_valve->setOpen(true);
          if (squelchIsOpen())
          {
            sql_valve->setOpen(true);
          }
          break;

        default:
          break;
      }
    }
  }
  Rx::setMuteState(mute_state);
} /* LocalRxBase::setMuteState */


bool LocalRxBase::addToneDetector(float fq, int bw, float thresh,
      	      	      	      int required_duration)
{
  //printf("Adding tone detector with fq=%d  bw=%d  req_dur=%d\n",
  //    	 fq, bw, required_duration);
  ToneDetector *det = new ToneDetector(fq, 2*bw, required_duration);
  assert(det != 0);
  det->setPeakThresh(thresh);
  det->setDetectOverlapPercent(75);
  det->setDetectToneFrequencyTolerancePercent(50.0f * bw / fq);
  det->detected.connect(sigc::mem_fun(*this, &LocalRxBase::onToneDetected));
  
  tone_dets->addSink(det, true);
  
  return true;

} /* LocalRxBase::addToneDetector */


float LocalRxBase::signalStrength(void) const
{
  if (squelchIsOpen())
  {
    return siglevdet->siglevIntegrated();
  }
  return siglevdet->lastSiglev();
} /* LocalRxBase::signalStrength */
    

char LocalRxBase::sqlRxId(void) const
{
  return siglevdet->lastRxId();
} /* LocalRxBase::sqlRxId */


void LocalRxBase::reset(void)
{
  setMuteState(Rx::MUTE_ALL);
  tone_dets->removeAllSinks();
  if (delay != 0)
  {
    delay->mute(false);
  }
} /* LocalRxBase::reset */


void LocalRxBase::registerFullbandSink(Async::AudioSink* sink)
{
  fullband_splitter->addSink(sink);
} /* LocalRxBase::registerFullbandSink */


void LocalRxBase::unregisterFullbandSink(Async::AudioSink* sink)
{
  fullband_splitter->removeSink(sink);
} /* LocalRxBase::unregisterFullbandSink */


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

void LocalRxBase::sel5Detected(std::string sequence)
{
  if (muteState() == MUTE_NONE)
  {
    selcallSequenceDetected(sequence);
  }
} /* LocalRxBase::sel5Detected */


void LocalRxBase::dtmfDigitActivated(char digit)
{
  //printf("DTMF digit %c activated.\n", digit);
  if (dtmf_muting_pre > 0)
  {
    delay->mute(true, dtmf_muting_pre);
  }
} /* LocalRxBase::dtmfDigitActivated */


void LocalRxBase::dtmfDigitDeactivated(char digit, int duration_ms)
{
  //printf("DTMF digit %c deactivated. Duration = %d ms\n", digit, duration_ms);
  if (muteState() == MUTE_NONE)
  {
    dtmfDigitDetected(digit, duration_ms);
  }
  if (dtmf_muting_pre > 0)
  {
    delay->mute(false, DTMF_MUTING_POST);
  }
} /* LocalRxBase::dtmfDigitDeactivated */


void LocalRxBase::onToneDetected(float fq)
{
  if (muteState() == MUTE_NONE)
  {
    toneDetected(fq);
  }
} /* LocalRxBase::onToneDetected */


void LocalRxBase::dataFrameReceived(vector<uint8_t> frame)
{
  vector<uint8_t>::const_iterator it = frame.begin();
  if ((frame.size() == 5) && (*it++ == Tx::DATA_CMD_TONE_DETECTED))
  {
    float fq = 0.0f;
    uint8_t *fq_ptr = reinterpret_cast<uint8_t*>(&fq);
    *fq_ptr++ = *it++;
    *fq_ptr++ = *it++;
    *fq_ptr++ = *it++;
    *fq_ptr++ = *it++;
    cout << "### LocalRxBase::dataFrameReceived: len=" << frame.size()
         << " cmd=" << Tx::DATA_CMD_TONE_DETECTED
         << " fq=" << fq
         << endl;
    if (muteState() == MUTE_NONE)
    {
      toneDetected(fq);
    }
  }
  dataReceived(frame);
} /* LocalRxBase::dataFrameReceived */


void LocalRxBase::dataFrameReceivedIb(vector<uint8_t> frame)
{
  cout << "### Inband data frame received: len=" << frame.size() << endl;
  dataFrameReceived(frame);
} /* LocalRxBase::dataFrameReceived */


void LocalRxBase::audioStreamStateChange(bool is_active, bool is_idle)
{
  if (is_idle && !squelch_det->isOpen())
  {
    setSquelchState(false, squelch_det->activityInfo());
  }
} /* LocalRxBase::audioStreamStateChange */


void LocalRxBase::onSquelchOpen(bool is_open)
{
  if (muteState() == MUTE_ALL)
  {
    return;
  }

  if (is_open)
  {
    if (delay != 0)
    {
      delay->clear();
    }
    setSquelchState(true, squelch_det->activityInfo());
    if (muteState() == MUTE_NONE)
    {
      sql_valve->setOpen(true);
    }
    setSqlHangtimeFromSiglev(siglevdet->lastSiglev());
    siglevdet->setIntegrationTime(1000);
    siglevdet->setContinuousUpdateInterval(1000);
  }
  else
  {
    if (sql_tail_elim > 0)
    {
      delay->clear(sql_tail_elim);
    }
    if (!sql_valve->isOpen())
    {
      setSquelchState(false, squelch_det->activityInfo());
    }
    else
    {
      sql_valve->setOpen(false);
    }
    siglevdet->setIntegrationTime(0);
    siglevdet->setContinuousUpdateInterval(0);
  }
} /* LocalRxBase::onSquelchOpen */


void LocalRxBase::tone1750detected(bool detected)
{
   if (detected)
   {
     cout << name() << ": Muting 1750Hz tone burst\n";
     delay->mute(true, TONE_1750_MUTING_PRE);
   }
   else
   {
     delay->mute(false, TONE_1750_MUTING_POST);
   }
} /* LocalRxBase::tone1750detected */


void LocalRxBase::onSignalLevelUpdated(float siglev)
{
  setSqlHangtimeFromSiglev(siglev);
  signalLevelUpdated(siglev);
  publishSquelchState();
} /* LocalRxBase::onSignalLevelUpdated */


void LocalRxBase::setSqlHangtimeFromSiglev(float siglev)
{
  if (sql_extended_hangtime_thresh > 0)
  {
    squelch_det->enableExtendedHangtime(
        ((siglev < sql_extended_hangtime_thresh) &&
         (muteState() == MUTE_NONE)));
  }
} /* LocalRxBase::setSqlHangtime */


void LocalRxBase::rxReadyStateChanged(void)
{
  if (!isReady())
  {
    siglevdet->reset();
    squelch_det->reset();
    siglevdet->signalLevelUpdated(siglevdet->lastSiglev());
    squelch_det->squelchOpen(false);
  }
} /* LocalRxBase::rxReadyStateChanged */


void LocalRxBase::publishSquelchState(void)
{
  //std::cout << "### LocalRxBase::publishSquelchState: " << std::endl;
  float siglev = signalStrength();
  Json::Value rx(Json::objectValue);
  rx["name"] = name();
  char rx_id = sqlRxId();
  rx["id"] = std::string(&rx_id, &rx_id+1);
  rx["sql_open"] = squelchIsOpen();
  rx["siglev"] = static_cast<int>(siglev);
  Json::StreamWriterBuilder builder;
  builder["commentStyle"] = "None";
  builder["indentation"] = ""; //The JSON document is written on a single line
  Json::StreamWriter* writer = builder.newStreamWriter();
  stringstream os;
  writer->write(rx, &os);
  delete writer;
  publishStateEvent("Rx:sql_state", os.str());
} /* LocalRxBase::publishSquelchState */


void LocalRxBase::cfgUpdated(const std::string& section, const std::string& tag)
{
  //std::cout << "### LocalRxBase::cfgUpdated: "
  //          << section << "/" << tag << "=" << cfg().getValue(section, tag)
  //          << std::endl;
  if (section == name())
  {
    if (tag == CFG_SQL_EXTENDED_HANGTIME_THRESH)
    {
      cfg().getValue(name(), CFG_SQL_EXTENDED_HANGTIME_THRESH,
                     sql_extended_hangtime_thresh);
      std::cout << "Setting " << CFG_SQL_EXTENDED_HANGTIME_THRESH << " to "
                << sql_extended_hangtime_thresh
                << " for receiver " << name() << std::endl;
    }
  }
} /* LocalRxBase::cfgUpdated */


/*
 * This file has not been truncated
 */

