/**
@file	 UsrpLogic.cpp
@brief   A logic core that connect to the SvxUsrp
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2021-04-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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

#include <sstream>
#include <iostream>
#include <fstream>
#include <iomanip>
#include <cstring>
#include <cmath>
#include <algorithm>
#include <iterator>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncUdpSocket.h>
#include <AsyncAudioValve.h>
#include <version/SVXLINK.h>
#include <AsyncAudioInterpolator.h>
#include <AsyncAudioDecimator.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "UsrpLogic.h"
#include "../trx/multirate_filter_coeff.h"


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

UsrpLogic::UsrpLogic(Async::Config& cfg, const std::string& name)
  : LogicBase(cfg, name), m_msg_type(0), m_udp_sock(0),
    m_logic_con_in(0), m_logic_con_out(0),
    m_next_udp_tx_seq(0), m_next_udp_rx_seq(0),
    m_dec(0),
    m_flush_timeout_timer(3000, Timer::TYPE_ONESHOT, false),
    m_enc(0), m_default_tg(0),
    m_tg_select_timeout(DEFAULT_TG_SELECT_TIMEOUT),
    m_tg_select_timeout_cnt(0), m_selected_tg(0), m_previous_tg(0),
    m_tg_local_activity(false), m_last_qsy(0),
    m_mute_first_tx_loc(true), m_mute_first_tx_rem(false), m_use_prio(true),
    udp_seq(0), stored_samples(0)
{
  m_flush_timeout_timer.expired.connect(
      mem_fun(*this, &UsrpLogic::flushTimeout));
  timerclear(&m_last_talker_timestamp);
} /* UsrpLogic::UsrpLogic */


UsrpLogic::~UsrpLogic(void)
{
  delete m_udp_sock;
  m_udp_sock = 0;
  delete m_udp_rxsock;
  m_udp_rxsock = 0;
  delete m_logic_con_in;
  m_logic_con_in = 0;
  delete m_enc;
  m_enc = 0;
  delete m_dec;
  m_dec = 0;
} /* UsrpLogic::~UsrpLogic */


bool UsrpLogic::initialize(void)
{
  if (!cfg().getValue(name(), "USRP_HOST", m_usrp_host))
  {
    cerr << "*** ERROR: " << name() << "/HOST missing in configuration" << endl;
    return false;
  }

  m_usrp_port = 41234;
  cfg().getValue(name(), "USRP_TX_PORT", m_usrp_port);

  m_usrp_rx_port = 41233;
  cfg().getValue(name(), "USRP_RX_PORT", m_usrp_rx_port);
  
  m_udp_sock = new UdpSocket(m_usrp_port);
  cout << "Usrp Tx socket=" << m_usrp_host << ":" << m_usrp_port << endl;

  if (m_usrp_rx_port == m_usrp_port)
  {
    m_udp_sock->dataReceived.connect(
      mem_fun(*this, &UsrpLogic::udpDatagramReceived));
  }
  else
  {
    m_udp_rxsock = new UdpSocket(m_usrp_rx_port);
    m_udp_rxsock->dataReceived.connect(
       mem_fun(*this, &UsrpLogic::udpDatagramReceived));
  }
  
    // Create logic connection incoming audio passthrough
  m_logic_con_in = new Async::AudioStreamStateDetector;
  m_logic_con_in->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &UsrpLogic::onLogicConInStreamStateChanged));
  AudioSource *prev_src = m_logic_con_in;

  //if (INTERNAL_SAMPLE_RATE == 16000)
  //{
  //  AudioDecimator *d1 = new AudioDecimator(2, coeff_16_8,
	//				    coeff_16_8_taps);
  //  prev_src->registerSink(d1, true);
  //  prev_src = d1;
  //}
  m_enc_endpoint = prev_src;

  prev_src = 0;
    // Create dummy audio codec used before setting the real encoder
  if (!setAudioCodec()) { return false; }
  prev_src = m_dec;

    // Create jitter buffer
  AudioFifo *fifo = new Async::AudioFifo(2*INTERNAL_SAMPLE_RATE);
  prev_src->registerSink(fifo, true);
  prev_src = fifo;
  unsigned jitter_buffer_delay = 0;
  cfg().getValue(name(), "JITTER_BUFFER_DELAY", jitter_buffer_delay);
  if (jitter_buffer_delay > 0)
  {
    fifo->setPrebufSamples(jitter_buffer_delay * INTERNAL_SAMPLE_RATE / 1000);
  }

  //if (INTERNAL_SAMPLE_RATE == 16000)  
  //{
      // Interpolate sample rate to 16kHz
  //  AudioInterpolator *i1 = new AudioInterpolator(2, coeff_16_8,
   //                                               coeff_16_8_taps);
   // prev_src->registerSink(i1, true);
  //  prev_src = i1;
  //}

  m_logic_con_out = new Async::AudioStreamStateDetector;
  m_logic_con_out->sigStreamStateChanged.connect(
      sigc::mem_fun(*this, &UsrpLogic::onLogicConOutStreamStateChanged));
  prev_src->registerSink(m_logic_con_out, true);
  prev_src = 0;

  r_buf = new int16_t[USRP_AUDIO_FRAME_LEN*2];

  if (!LogicBase::initialize())
  {
    cout << "*** ERROR: Initializing Logic " << name() << endl;
    return false;
  }

  return true;
} /* UsrpLogic::initialize */


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


void UsrpLogic::handleMsgError(std::istream& is)
{
} /* UsrpLogic::handleMsgError */


void UsrpLogic::handleMsgTalkerStart(int tg)
{
  cout << name() << ": Talker start on TG #" << tg << endl;
} /* UsrpLogic::handleMsgTalkerStart */


void UsrpLogic::handleMsgTalkerStop(int tg)
{
  cout << name() << ": Talker stop on TG #" << tg << endl;
} /* UsrpLogic::handleMsgTalkerStop */


void UsrpLogic::handleMsgRequestQsy(int tg)
{
} /* UsrpLogic::handleMsgRequestQsy */


void UsrpLogic::sendEncodedAudio(const void *buf, int count)
{
  std::array<int16_t, 160> audiodata;
  UsrpMsg usrp;
  usrp.setType(USRP_TYPE_VOICE);
  usrp.setKeyup(true);
  int len = (int)(count * sizeof(char) / 2);

  if (m_flush_timeout_timer.isEnabled())
  {
    m_flush_timeout_timer.setEnable(false);
  }

  const int16_t *t = reinterpret_cast<const int16_t*>(buf);

  memcpy(r_buf+stored_samples, t, sizeof(int16_t)*len);
  stored_samples += len;

  while (stored_samples >= USRP_AUDIO_FRAME_LEN)
  {
    for(size_t x=0; x<USRP_AUDIO_FRAME_LEN; x++)
    {
      audiodata[x] = r_buf[x];
    }
    usrp.setAudiodata(audiodata);

    sendMsg(usrp);
    memmove(r_buf, r_buf + USRP_AUDIO_FRAME_LEN, 
              sizeof(int16_t)*(stored_samples-USRP_AUDIO_FRAME_LEN));
    stored_samples -= USRP_AUDIO_FRAME_LEN;
  }
} /* UsrpLogic::sendEncodedAudio */


void UsrpLogic::flushEncodedAudio(void)
{
  m_enc->allEncodedSamplesFlushed();
  m_flush_timeout_timer.setEnable(true);
} /* UsrpLogic::flushEncodedAudio */


void UsrpLogic::udpDatagramReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
{
  //cout << "incoming packet from " << addr.toString() << ", len=" << count 
  //     << endl;

  stringstream ss;
  ss.write(reinterpret_cast<const char*>(buf), count);
  UsrpMsg usrp;

  if (!usrp.unpack(ss))
  {
    cout << "*** WARNING[" << name()
         << "]: Unpacking failed for UDP Usrp message" << endl;
    return;
  }

  switch (usrp.type())
  {
    case USRP_TYPE_VOICE:
      if (count <= USRP_HEADER_LEN) handleStreamStop();
      else handleVoiceStream(usrp);
    break;

    case USRP_TYPE_DTMF:
      cout << "USRP_TYPE_DTMF" << endl;
    break;
   
    case USRP_TYPE_TEXT:
      cout << "USRP_TYPE_TEXT : " << endl;
      handleTextMsg(usrp);
    break;
   
    case USRP_TYPE_TLV:
      cout << "USRP_TYPE_TLV" << endl;
    break;
   
    case USRP_TYPE_PING:
      cout << "USRP_TYPE_PING" << endl;
      sendHeartbeat();
    break;
    
    default:
      cout << "*** unknown type of message:" << usrp.type() << endl; 
    break;
  }
} /* UsrpLogic::udpDatagramReceived */


void UsrpLogic::handleVoiceStream(UsrpMsg usrp)
{
  gettimeofday(&m_last_talker_timestamp, NULL);  
  m_dec->writeEncodedSamples(&usrp.audioData(), 
                        sizeof(int16_t)*USRP_AUDIO_FRAME_LEN);
} /* UsrpLogic::handleVoiceStream */


void UsrpLogic::handleStreamStop(void)
{
  m_dec->flushEncodedSamples();
  checkIdle();
  m_enc->allEncodedSamplesFlushed();
  timerclear(&m_last_talker_timestamp);
} /* UsrpLogic::handleStreamStop */


void UsrpLogic::handleTextMsg(UsrpMsg usrp)
{
} /* UsrpLogic::handleTextMsg */


void UsrpLogic::sendMsg(UsrpMsg& usrp)
{
  usrp.setTg(m_selected_tg);
  
  if (udp_seq++ > 0x7fff) udp_seq = 0;
  usrp.setSeq(udp_seq);
  
  ostringstream ss;
  if (!usrp.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack UDP Usrp message\n";
    return;
  }
  sendUdpMessage(ss);
} /* UsrpLogic::sendMsg */


void UsrpLogic::sendStopMsg(void)
{
  UsrpStopMsg usrp;
  usrp.setTg(m_selected_tg);

  if (udp_seq++ > 0x7fff) udp_seq = 0;
  usrp.setSeq(udp_seq);

  ostringstream ss;
  if (!usrp.pack(ss))
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to pack UDP Usrp message\n";
    return;
  }
  sendUdpMessage(ss);
} /* UsrpLogic::sendStopMsg */


void UsrpLogic::sendUdpMessage(ostringstream& ss)
{
  if (m_udp_sock == 0)
  {
    return;
  }

  IpAddress usrp_addr(m_usrp_host);
  
  m_udp_sock->write(usrp_addr, m_usrp_port, ss.str().data(), ss.str().size());
} /* UsrpLogic::sendUdpMessage */


void UsrpLogic::sendHeartbeat(void)
{
} /* UsrpLogic::sendHeartbeat */


void UsrpLogic::allEncodedSamplesFlushed(void)
{
  //sendUdpMsg(MsgUdpAllSamplesFlushed());
} /* UsrpLogic::allEncodedSamplesFlushed */


void UsrpLogic::flushTimeout(Async::Timer *t)
{
  m_flush_timeout_timer.setEnable(false);
  m_enc->allEncodedSamplesFlushed();
} /* UsrpLogic::flushTimeout */


void UsrpLogic::handleTimerTick(Async::Timer *t)
{
  if (timerisset(&m_last_talker_timestamp))
  {
    struct timeval now, diff;
    gettimeofday(&now, NULL);
    timersub(&now, &m_last_talker_timestamp, &diff);
    if (diff.tv_sec > 3)
    {
      cout << name() << ": Last talker audio timeout" << endl;
      m_dec->flushEncodedSamples();
      timerclear(&m_last_talker_timestamp);
    }
  }
} /* UsrpLogic::handleTimerTick */


bool UsrpLogic::setAudioCodec(void)
{
  delete m_enc;
  m_enc = Async::AudioEncoder::create("S16");
  if (m_enc == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize audio encoder" 
         << endl;
    m_enc = Async::AudioEncoder::create("DUMMY");
    assert(m_enc != 0);
    return false;
  }
  m_enc->writeEncodedSamples.connect(
      mem_fun(*this, &UsrpLogic::sendEncodedAudio));
  m_enc->flushEncodedSamples.connect(
      mem_fun(*this, &UsrpLogic::flushEncodedAudio));
  m_enc_endpoint->registerSink(m_enc, false);

  AudioSink *sink = 0;
  if (m_dec != 0)
  {
    sink = m_dec->sink();
    m_dec->unregisterSink();
    delete m_dec;
  }
  m_dec = Async::AudioDecoder::create("S16");
  if (m_dec == 0)
  {
    cerr << "*** ERROR[" << name()
         << "]: Failed to initialize audio decoder" 
         << endl;
    m_dec = Async::AudioDecoder::create("DUMMY");
    assert(m_dec != 0);
    return false;
  }
  m_dec->allEncodedSamplesFlushed.connect(
      mem_fun(*this, &UsrpLogic::allEncodedSamplesFlushed));
  if (sink != 0)
  {
    m_dec->registerSink(sink, true);
  }

  return true;
} /* UsrpLogic::setAudioCodec */


void UsrpLogic::onLogicConInStreamStateChanged(bool is_active,
                                                    bool is_idle)
{
 // cout << "### UsrpLogic::onLogicConInStreamStateChanged: is_active="
  //     << is_active << ", is_idle=" << is_idle << endl;

  checkIdle();
  if (is_idle)
  {
    sendStopMsg();
  }
} /* UsrpLogic::onLogicConInStreamStateChanged */


void UsrpLogic::onLogicConOutStreamStateChanged(bool is_active,
                                                     bool is_idle)
{
  //cout << "### UsrpLogic::onLogicConOutStreamStateChanged: is_active="
  //     << is_active << "  is_idle=" << is_idle << endl;
  checkIdle();
} /* UsrpLogic::onLogicConOutStreamStateChanged */


bool UsrpLogic::isIdle(void)
{
  return m_logic_con_out->isIdle() && m_logic_con_in->isIdle();
} /* UsrpLogic::isIdle */


void UsrpLogic::checkIdle(void)
{
  setIdle(isIdle());
} /* UsrpLogic::checkIdle */


/*
 * This file has not been truncated
 */
