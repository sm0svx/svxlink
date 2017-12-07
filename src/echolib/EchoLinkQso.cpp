/**
@file	 EchoLinkQso.cpp
@brief   Contains a class for creating an EchoLink connection
@author  Tobias Blomberg
@date	 2003-03-11

This file contains a class for creating an EchoLink connection. For more
information, see the documentation for class EchoLink::Qso.

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#include <string>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <cassert>
#include <cstring>

#ifdef SPEEX_MAJOR
#include <speex/speex.h>
#endif


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "rtp.h"
#include "rtpacket.h"
#include "EchoLinkDispatcher.h"
#include "EchoLinkQso.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;


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

struct Qso::Private
{
  typedef enum
  {
    CODEC_NONE,
    CODEC_GSM
#ifdef SPEEX_MAJOR
    ,CODEC_SPEEX
#endif
  } Codec;

  Codec     remote_codec;
#ifdef SPEEX_MAJOR
  SpeexBits enc_bits;
  SpeexBits dec_bits;
  void *    enc_state;
  void *    dec_state;
#endif

  Private(void)
    : remote_codec(CODEC_GSM)
#if SPEEX_MAJOR
      , enc_bits(), dec_bits(), enc_state(0), dec_state(0)
#endif
  {}
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


Qso::Qso(const IpAddress& addr, const string& callsign, const string& name,
      	 const string& info)
  : init_ok(false), sdes_length(0), state(STATE_DISCONNECTED), gsmh(0),
    next_audio_seq(0), keep_alive_timer(0), connect_retry_cnt(0),
    con_timeout_timer(0), callsign(callsign), name(name), local_stn_info(info),
    send_buffer_cnt(0), remote_ip(addr), rx_indicator_timer(0),
    remote_name("?"), remote_call("?"), is_remote_initiated(false),
    receiving_audio(false), use_gsm_only(false), p(new Private),
    rx_timeout_left(0)
{
  if (!addr.isUnicast())
  {
    cerr << "IP address is not a unicast address: " << addr << endl;
    return;
  }
  
  setLocalCallsign(callsign);
      
  gsmh = gsm_create();

#ifdef SPEEX_MAJOR
  speex_bits_init(&p->enc_bits);
  speex_bits_init(&p->dec_bits);
    
  p->enc_state = speex_encoder_init(&speex_nb_mode);
  p->dec_state = speex_decoder_init(&speex_nb_mode);

  int val = 25000;
  speex_encoder_ctl(p->enc_state, SPEEX_SET_BITRATE, &val);
  val = 8;
  speex_encoder_ctl(p->enc_state, SPEEX_SET_QUALITY, &val);
  val = 4;
  speex_encoder_ctl(p->enc_state, SPEEX_SET_COMPLEXITY, &val);
#endif
    
  if (!Dispatcher::instance()->registerConnection(this, &Qso::handleCtrlInput,
      &Qso::handleAudioInput))
  {
    cerr << "Cannot create a new Qso object because registration with the "
      	    "dispatcher object failed for some reason.\n";
    return;
  }

  init_ok = true;
  return;
  
} /* Qso::Qso */


Qso::~Qso(void)
{
  disconnect();
  
  gsm_destroy(gsmh);
  gsmh = 0;

#ifdef SPEEX_MAJOR
  speex_bits_destroy(&p->enc_bits);
  speex_bits_destroy(&p->dec_bits);

  speex_encoder_destroy(p->enc_state);
  speex_decoder_destroy(p->dec_state);
#endif
  
  if (init_ok)
  {
    Dispatcher::instance()->unregisterConnection(this);
  }  

  delete p;
  p = 0;
} /* Qso::~Qso */


bool Qso::setLocalCallsign(const string& callsign)
{
#ifdef SPEEX_MAJOR
  const char *priv = "SPEEX";
#else
  const char *priv = 0;
#endif  
  this->callsign.resize(callsign.size());
  transform(callsign.begin(), callsign.end(), this->callsign.begin(),
      	   ::toupper);
  sdes_length = rtp_make_sdes(sdes_packet, callsign.c_str(),
      name.c_str(), priv);
  if(sdes_length <= 0)
  {
    cerr << "Could not create SDES packet\n";
    return false;
  }
  
  return true;
  
} /* Qso::setLocalCallsign */


bool Qso::setLocalName(const string& name)
{
  const char *priv = 0;
#ifdef SPEEX_MAJOR
  if (!use_gsm_only)
  {
    priv = "SPEEX";
  }
#endif  

  this->name = name;
  sdes_length = rtp_make_sdes(sdes_packet, callsign.c_str(),
      name.c_str(), priv);
  if(sdes_length <= 0)
  {
    cerr << "Could not create SDES packet\n";
    return false;
  }
  
  return true;
  
} /* Qso::setLocalName */


void Qso::setLocalInfo(const string& info)
{
  local_stn_info = info;
} /* Qso::setLocalInfo */


bool Qso::connect(void)
{
  if (state != STATE_DISCONNECTED)
  {
    return true;
  }
  
  is_remote_initiated = false;
  connect_retry_cnt = 0;
  bool setup_connection_ok = setupConnection();
  if (setup_connection_ok)
  {
    setState(STATE_CONNECTING);
  }
  
  return setup_connection_ok;
  
} /* Qso::connect */


bool Qso::accept(void)
{
  if (state != STATE_DISCONNECTED)
  {
    return true;
  }

  is_remote_initiated = true;
  bool setup_connection_ok = setupConnection();
  if (setup_connection_ok)
  {
    setState(STATE_CONNECTED);
  }
  
  return setup_connection_ok;
  
} /* Qso::accept */


bool Qso::disconnect(void)
{
  if (state == STATE_DISCONNECTED)
  {
    return true;
  }
  
  if (state != STATE_BYE_RECEIVED)
  {
    if (!sendByePacket())
    {
      return false;
    }
  }
  
  cleanupConnection();
  
  return true;
  
} /* Qso::disconnect */


bool Qso::sendInfoData(const string& info)
{
  if (state != STATE_CONNECTED)
  {
    return false;
  }
  
  string info_msg("oNDATA\r");
  if (info.empty())
  {
    info_msg += local_stn_info;
  }
  else
  {
    info_msg += info;
  }
  replace(info_msg.begin(), info_msg.end(), '\n', '\r');

  bool success = Dispatcher::instance()->sendAudioMsg(
      remote_ip, info_msg.c_str(), info_msg.length()+1);
  if (!success)
  {
    perror("sendAudioMsg in Qso::sendInfoData");
    return false;
  }
  
  return true;
  
} /* Qso::sendInfoData */


bool Qso::sendChatData(const string& msg)
{
  if (state != STATE_CONNECTED)
  {
    return false;
  }
  
  string buf("oNDATA" + callsign + '>' + msg + "\r\n");
  bool success = Dispatcher::instance()->sendAudioMsg(remote_ip, buf.c_str(),
      buf.length()+1);
  if (!success)
  {
    perror("sendAudioMsg in Qso::sendChatData");
    return false;
  }
  
  return true;
  
} /* Qso::sendChatData */


bool Qso::sendAudioRaw(RawPacket *raw_packet)
{
  if (state != STATE_CONNECTED)
  {
    return false;
  }
  
#ifdef SPEEX_MAJOR
  if ((raw_packet->voice_packet->header.pt == 0x96) &&
      (p->remote_codec == Private::CODEC_GSM))
  {
    // transcode SPEEX -> GSM
    VoicePacket voice_packet;
    size_t nbytes = 0;
    
    for(int i=0; i<FRAME_COUNT; i++)
    {
      gsm_encode(gsmh, raw_packet->samples + i*160, voice_packet.data + i*33);
      nbytes += 33;
    }
    voice_packet.header.version = 0xc0;
    voice_packet.header.pt = 0x03;
    voice_packet.header.time = htonl(0);
    voice_packet.header.ssrc = htonl(0);
    voice_packet.header.seqNum = htons(next_audio_seq++);
    
    bool success = Dispatcher::instance()->sendAudioMsg(remote_ip, &voice_packet,
        nbytes + sizeof(voice_packet.header));
    if (!success)
    {
      perror("sendAudioMsg in Qso::sendAudioRaw");
      return false;
    }
  }
  else
#endif
  {
    raw_packet->voice_packet->header.seqNum = htons(next_audio_seq++);
  
    bool success = Dispatcher::instance()->sendAudioMsg(remote_ip,
        raw_packet->voice_packet, raw_packet->length);
    if (!success)
    {
      perror("sendAudioMsg in Qso::sendAudioRaw");
      return false;
    }
  }
  
  return true;

} /* Qso::sendAudioRaw */


void Qso::setRemoteParams(const string& priv)
{
#ifdef SPEEX_MAJOR  
  if ((priv.find("SPEEX") != string::npos)
      && (p->remote_codec == Private::CODEC_GSM)
      && !use_gsm_only)
  {
    cerr << "Switching to SPEEX audio codec for EchoLink Qso." << endl;
    p->remote_codec = Private::CODEC_SPEEX;
  }
#endif
} /* Qso::setRemoteParams */


int Qso::writeSamples(const float *samples, int count)
{
  int samples_read = 0;
  
  if (state != STATE_CONNECTED)
  {
    return count;
  }
  
  while (samples_read < count)
  {
    int read_cnt = min(BUFFER_SIZE - send_buffer_cnt, count-samples_read);
    for (int i=0; i<read_cnt; ++i)
    {
      float sample = samples[samples_read++];
      if (sample > 1)
      {
      	send_buffer[send_buffer_cnt++] = 32767;
      }
      else if (sample < -1)
      {
      	send_buffer[send_buffer_cnt++] = -32767;
      }
      else
      {
      	send_buffer[send_buffer_cnt++] = static_cast<int16_t>(32767.0 * sample);
      }
    }
    
    if (send_buffer_cnt == BUFFER_SIZE)
    {
      bool packet_sent = sendVoicePacket();
      if (!packet_sent)
      {
	break;
      }
      send_buffer_cnt = 0;
    }
  }
  
  //printf("Samples read = %d\n", samples_read);
  
  return samples_read;
  
} /* Qso::writeSamples */


void Qso::flushSamples(void)
{
  if (state == STATE_CONNECTED)
  {
    if (send_buffer_cnt > 0)
    {
      memset(send_buffer + send_buffer_cnt, 0,
	  sizeof(send_buffer) - sizeof(*send_buffer) * send_buffer_cnt);
      send_buffer_cnt = BUFFER_SIZE;
      sendVoicePacket();
      send_buffer_cnt = 0;
    }
  }
  
  sourceAllSamplesFlushed();
  
} /* Qso::flushSamples */


void Qso::resumeOutput(void)
{
} /* Qso::resumeOutput */
    

void Qso::setUseGsmOnly(void)
{
  use_gsm_only = true;
} /* Qso::setGsmCodec */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void Qso::allSamplesFlushed(void)
{

} /* Qso::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void Qso::printData(const unsigned char *buf, int len)
{
  ios::fmtflags old_flags(cerr.flags());

  for(int i=0; i<len; i++)
  {
    if (isprint(buf[i]))
    {
      cerr << buf[i];
    }
    else
    {
      unsigned ch = (unsigned char)buf[i];
      cerr << "<" << hex << setfill('0') << setw(2) << ch << ">";
    }
  }
  cerr << endl;

  cerr.flags(old_flags);
} /* Qso::printData */


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
void Qso::handleCtrlInput(unsigned char *buf, int len)
{
  //printData(buf, recv_len);
  
  if(isRTCPByepacket(buf, len))
  {
    handleByePacket(buf, len);
  } 
  else if(isRTCPSdespacket(buf, len))
  {
    handleSdesPacket(buf, len);
  }
  else
  {
    cerr << "Unknown packet type received from " << remote_ip << endl;
    return;
  }
} /* Qso::handleCtrlInput */


inline void Qso::handleByePacket(unsigned char *buf, int len)
{
  if (state != STATE_DISCONNECTED)
  {
    setState(STATE_BYE_RECEIVED);
    disconnect();
  }
  else
  {
    sendByePacket();
  }
} /* Qso::handleByePacket */


inline void Qso::handleSdesPacket(unsigned char *buf, int len)
{
  char remote_id[256];
  if(parseSDES(remote_id, buf, RTCP_SDES_NAME))
  {
    //printData(reinterpret_cast<unsigned char*>(remote_id), strlen(remote_id));
#if 0
    char strtok_buf[256];
    char *strtok_buf_ptr = strtok_buf;
    char *remote_call_str = strtok_r(remote_id, " \t\n\r", &strtok_buf_ptr);
    const char *remote_name_str = strtok_r(NULL, " \t\n\r", &strtok_buf_ptr);
    if ((remote_call_str != 0) && (remote_call_str[0] != 0))
    {
      if (remote_name_str == 0)
      {
	remote_name_str = "?";
      }
      remote_call = remote_call_str;
      remote_name = remote_name_str;
    }
#endif
    string str(remote_id);
    size_t pos = str.find_first_of(" \t\n\r");
    if (pos != string::npos)
    {
      remote_call = str.substr(0, pos);
      pos = str.find_first_not_of(" \t\n\r", pos);
      if (pos != string::npos)
      {
        remote_name = str.substr(pos);
      }
    }
  }
  char priv[256];
  if(parseSDES(priv, buf, RTCP_SDES_PRIV))
  {
    setRemoteParams(priv);
  }
      
  switch (state)
  {
    case STATE_CONNECTING:
      setState(STATE_CONNECTED);
      break;
    
    case STATE_DISCONNECTED:
      sendByePacket();
      break;
    
    case STATE_CONNECTED:// Keep-alive
      assert(con_timeout_timer != 0);
      con_timeout_timer->reset();
      break;
    
    case STATE_BYE_RECEIVED:
      break;
  }  
} /* Qso::handleSdesPacket */


void Qso::handleAudioInput(unsigned char *buf, int len)
{
  if (state == STATE_DISCONNECTED)
  {
    cerr << "Ignoring audio/info/chat packet from " << remote_ip <<
      	    " since we are disconnected.\n";
    return;
  }
  
  if(buf[0] != 0xc0) /* Not an audio packet */
  {
    handleNonAudioPacket(buf, len);
  }
  else
  {
    handleAudioPacket(buf, len);
  }
} /* Qso::handleAudioInput */


inline void Qso::handleNonAudioPacket(unsigned char *buf, int len)
{
  //printData(buf, len);

  if(memcmp(buf+1, "NDATA", 5) == 0)
  {
    if (buf[6] == 0x0d) // Remote station info / conference status
    {
      char *info_buf = reinterpret_cast<char *>(buf);
      char *null = (char *)memchr(info_buf, 0, len);
      if (null == 0)
      {
      	cerr << "Malformed info packet received:\n";
	printData(buf, len);
	return;
      }
      string info_msg(info_buf+7, null);
      replace(info_msg.begin(), info_msg.end(), '\r', '\n');
      infoMsgReceived(info_msg);
      /*
      if (null+1 < info_buf+len)
      {
      	string trailing_data(null+1, info_buf+len);
	cerr << "Trailing info data: ";
	printData(reinterpret_cast<const unsigned char*>(null+1),
	      	  info_buf+len-(null+1));
      }
      */
    }
    else  // Chat data
    {
      char *chat_buf = reinterpret_cast<char *>(buf);
      char *null = (char *)memchr(buf, 0, len);
      if (null == 0)
      {
      	cerr << "Malformed chat packet received:\n";
	printData(buf, len);
	return;
      }
      //string chat_msg(buf+6, buf+len);
      string chat_msg(chat_buf+6, null);
      replace(chat_msg.begin(), chat_msg.end(), '\r', '\n');
      chatMsgReceived(chat_msg);
      if (null+1 < chat_buf+len)
      {
      	string trailing_data(null+1, chat_buf+len);
	cerr << "Trailing chat data: ";
	printData(reinterpret_cast<const unsigned char*>(null+1),
	      	  chat_buf+len-(null+1));
      }
    }
  }
  else
  {
    cerr << "Unknown non-audio packet received:\n";
    printData(buf, len);
    return;
  }
  
} /* Qso::handleNonAudioPacket */


inline void Qso::handleAudioPacket(unsigned char *buf, int len)
{
  VoicePacket *voice_packet = reinterpret_cast<VoicePacket*>(buf);

  RawPacket raw_packet = { voice_packet, len, receive_buffer };
  short *sbuff = receive_buffer;

  /* Check that we have received a valid header. */
  if ((unsigned)len < sizeof(voice_packet->header))
  {
    cerr << "*** WARNING: Invalid audio packet size." << endl;
    return;
  }

#ifdef SPEEX_MAJOR
  if (voice_packet->header.pt == 0x96)
  {
    speex_bits_read_from(&p->dec_bits, (char*)voice_packet->data,
                         len - sizeof(voice_packet->header));
          
    for (int frameno=0; frameno<FRAME_COUNT; ++frameno)
    {
      int err = speex_decode_int(p->dec_state, &p->dec_bits, sbuff);
      if (err == -1)
      {
        cerr << "*** WARNING: Short frame count. There should be "
             << FRAME_COUNT << " frames in each audio packet, but only "
             << frameno << " frames have been received."
             << endl;
        return;
      }
      if (err == -2)
      {
        cerr << "*** WARNING: Corrupt Speex stream in received audio packet."
             << endl;
        return;
      }

      if (rx_indicator_timer == 0)
      {
        receiving_audio = true;
        isReceiving(true);
        rx_indicator_timer = new Timer(RX_INDICATOR_POLL_TIME,
                                       Timer::TYPE_PERIODIC);
        rx_indicator_timer->expired.connect(
            mem_fun(*this, &Qso::checkRxActivity));
        rx_timeout_left = RX_INDICATOR_SLACK;
      }
      
      float samples[160];
      for (int i = 0; i < 160; i++)
      {
        samples[i] = static_cast<float>(sbuff[i]) / 32768.0;
      }
      sinkWriteSamples(samples, 160);
      sbuff += 160;
    }
  }
  else
#endif
  {
    if ((unsigned)len < sizeof(voice_packet->header)+FRAME_COUNT*33)
    {
      cerr << "*** WARNING: Invalid GSM audio packet size." << endl;
      return;
    }
    for (int frameno=0; frameno<FRAME_COUNT; ++frameno)
    {
      gsm_decode(gsmh, voice_packet->data + frameno*33, sbuff);
      if (rx_indicator_timer == 0)
      {
        receiving_audio = true;
        isReceiving(true); 
        rx_indicator_timer = new Timer(RX_INDICATOR_POLL_TIME,
                                       Timer::TYPE_PERIODIC);
        rx_indicator_timer->expired.connect(
            mem_fun(*this, &Qso::checkRxActivity));
        rx_timeout_left = RX_INDICATOR_SLACK;
      }
      
      float samples[160];
      for (int i=0; i<160; ++i)
      {
        samples[i] = static_cast<float>(sbuff[i]) / 32768.0;
      }
      sinkWriteSamples(samples, 160);
      sbuff += 160;
    }
  }

  rx_timeout_left += BLOCK_TIME;
  if (rx_timeout_left < BLOCK_TIME + RX_INDICATOR_SLACK)
  {
    rx_timeout_left = BLOCK_TIME + RX_INDICATOR_SLACK;
  }
  else if (rx_timeout_left > RX_INDICATOR_MAX_TIME)
  {
    rx_timeout_left = RX_INDICATOR_MAX_TIME;
  }

  audioReceivedRaw(&raw_packet);

} /* Qso::handleAudioPacket */


bool Qso::sendSdesPacket(void)
{
  bool success = Dispatcher::instance()->sendCtrlMsg(remote_ip, sdes_packet,
      sdes_length);
  if (!success)
  {
    perror("sendCtrlMsg in Qso::sendSdesPacket");
    return false;
  }
  
  return true;
  
} /* Qso::sendSdesPacket */


void Qso::sendKeepAlive(Timer *timer)
{
  if ((state == STATE_CONNECTING) &&
      (++connect_retry_cnt == MAX_CONNECT_RETRY_CNT))
  {
    cleanupConnection();
  }
  else
  {
    sendSdesPacket();
  }
} /* Qso::sendKeepAlive */


void Qso::setState(State state)
{
  if (state != this->state)
  {
    this->state = state;
    if (state == STATE_CONNECTED)
    {
      sendInfoData();
    }
    stateChange(state);
  }
} /* Qso::setState */


void Qso::connectionTimeout(Timer *timer)
{
  cleanupConnection();
} /* Qso::connectionTimeout */


bool Qso::setupConnection(void)
{
  send_buffer_cnt = 0;
  
  bool send_sdes_ok = sendSdesPacket();
  if (send_sdes_ok)
  {
    keep_alive_timer = new Timer(KEEP_ALIVE_TIME, Timer::TYPE_PERIODIC);
    keep_alive_timer->expired.connect(mem_fun(*this, &Qso::sendKeepAlive));
    con_timeout_timer = new Timer(CON_TIMEOUT_TIME, Timer::TYPE_PERIODIC);
    con_timeout_timer->expired.connect(mem_fun(*this, &Qso::connectionTimeout));
  }
    
  return send_sdes_ok;
    
} /* Qso::setupConnection */


void Qso::cleanupConnection(void)
{
  if (rx_indicator_timer != 0)
  {
    receiving_audio = false;
    isReceiving(false);
    sinkFlushSamples();
    delete rx_indicator_timer;
    rx_indicator_timer = 0;
  }
  delete keep_alive_timer;
  keep_alive_timer = 0;
  delete con_timeout_timer;
  con_timeout_timer = 0;
  setState(STATE_DISCONNECTED);
} /* Qso::cleanupConnection */


bool Qso::sendVoicePacket(void)
{
  assert(send_buffer_cnt == BUFFER_SIZE);

  size_t nbytes = 0;
  VoicePacket voice_packet;
  voice_packet.header.version = 0xc0;
  voice_packet.header.time = htonl(0);
  voice_packet.header.ssrc = htonl(0);
  voice_packet.header.seqNum = htons(next_audio_seq++);

#ifdef SPEEX_MAJOR
  if (p->remote_codec == Private::CODEC_SPEEX)
  {
    for(int i = 0; i < BUFFER_SIZE; i += 160)
    {
      speex_encode_int(p->enc_state, send_buffer + i, &p->enc_bits);
    }
    speex_bits_insert_terminator(&p->enc_bits);
    size_t nsize = speex_bits_nbytes(&p->enc_bits);
    if (nsize < sizeof(voice_packet.data))
    {
      nbytes = speex_bits_write(&p->enc_bits, (char*)voice_packet.data, nsize);
    }
    speex_bits_reset(&p->enc_bits);
    voice_packet.header.pt = 0x96;
  }
  else
#endif
  {
    for(int i=0; i<FRAME_COUNT; i++)
    {
      gsm_encode(gsmh, send_buffer + i*160, voice_packet.data + i*33);
      nbytes += 33;
    }
    voice_packet.header.pt = 0x03;
  }
  if (!nbytes)
  {
    perror("audio packet size in Qso::sendVoicePacket");
    return false;
  }

  bool success = Dispatcher::instance()->sendAudioMsg(remote_ip, &voice_packet,
      nbytes + sizeof(voice_packet.header));
  if (!success)
  {
    perror("sendAudioMsg in Qso::sendVoicePacket");
    return false;
  }
  
  return true;
  
} /* Qso::sendVoicePacket */


void Qso::checkRxActivity(Timer *timer)
{
  //cout << "### Qso::checkRxActivity: rx_timeout_left="
  //     << rx_timeout_left << endl;
  rx_timeout_left -= 100;
  if (rx_timeout_left <= 0)
  {
    receiving_audio = false;
    isReceiving(false);
    sinkFlushSamples();
    delete rx_indicator_timer;
    rx_indicator_timer = 0;
  }
} /* Qso::checkRxActivity */


bool Qso::sendByePacket(void)
{
  unsigned char bye[64];
  int length = rtp_make_bye(bye);
  bool success = Dispatcher::instance()->sendCtrlMsg(remote_ip, bye, length);
  if (!success)
  {
    perror("sendCtrlMsg in Qso::disconnect");
    return false;
  }

  return true;

} /* Qso::sendByePacket */




/*
 * This file has not been truncated
 */
