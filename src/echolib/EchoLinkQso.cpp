/**
@file	 EchoLinkQso.cpp
@brief   Contains a class for creating an EchoLink connection
@author  Tobias Blomberg
@date	 2003-03-11

This file contains a class for creating an EchoLink connection. For more
information, see the documentation for class EchoLink::Qso.

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003  Tobias Blomberg / SM0SVX

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
Qso::Qso(const IpAddress& addr, const string& callsign, const string& name,
      	 const string& info)
  : init_ok(false),   	sdes_packet(0),       state(STATE_DISCONNECTED),
    next_audio_seq(0),  keep_alive_timer(0),  con_timeout_timer(0),
    callsign(callsign), name(name),   	      local_stn_info(info),
    send_buffer_cnt(0), remote_ip(addr),      rx_indicator_timer(0),
    remote_name("?"), 	remote_call("?"),     is_remote_initiated(false),
    receiving_audio(false)
{
  if (!addr.isUnicast())
  {
    cerr << "IP address is not a unicast address: " << addr << endl;
    return;
  }
  
  setLocalCallsign(callsign);
      
  gsmh = gsm_create();
    
  Dispatcher::instance()->registerConnection(this, &Qso::handleCtrlInput,
      &Qso::handleAudioInput);
  
  init_ok = true;
  return;
  
} /* Qso::Qso */


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
Qso::~Qso(void)
{
  disconnect();
  
  if (sdes_packet != 0)
  {
    free(sdes_packet);
  }
  
  if (init_ok)
  {
    Dispatcher::instance()->unregisterConnection(this);
  }
  
} /* Qso::~Qso */


bool Qso::setLocalCallsign(const std::string& callsign)
{
  this->callsign = callsign;
  if (sdes_packet != 0)
  {
    free(sdes_packet);
    sdes_packet = 0;
  }
  sdes_length = rtp_make_sdes(&sdes_packet, 0, 1, callsign.c_str(),
      name.c_str());
  if(sdes_length <= 0)
  {
    cerr << "Could not create SDES packet\n";
    return false;
  }
  
  return true;
  
} /* Qso::setLocalCallsign */


bool Qso::setLocalName(const std::string& name)
{
  this->name = name;
  if (sdes_packet != 0)
  {
    free(sdes_packet);
    sdes_packet = 0;
  }
  sdes_length = rtp_make_sdes(&sdes_packet, 0, 1, callsign.c_str(),
      name.c_str());
  if(sdes_length <= 0)
  {
    cerr << "Could not create SDES packet\n";
    return false;
  }
  
  return true;
  
} /* Qso::setLocalName */


void Qso::setLocalInfo(const std::string& info)
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
    //sendInfoData();
    unsigned char bye[50];
    int length = rtp_make_bye(bye, 0, "jan2002", 1);
    int ret = Dispatcher::instance()->sendCtrlMsg(remote_ip, bye, length);
    if (ret == -1)
    {
      perror("sendCtrlMsg in Qso::disconnect");
      return false;
    }
  }
  
  cleanupConnection();
  
  return true;
  
} /* Qso::disconnect */


bool Qso::sendInfoData(const string& info)
{
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

  int ret = Dispatcher::instance()->sendAudioMsg(remote_ip, info_msg.c_str(),
      info_msg.length()+1);
  if (ret == -1)
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
  int ret = Dispatcher::instance()->sendAudioMsg(remote_ip, buf.c_str(),
      buf.length()+1);
  if (ret == -1)
  {
    perror("sendAudioMsg in Qso::sendChatData");
    return false;
  }
  
  return true;
  
} /* Qso::sendChatData */


int Qso::sendAudio(short *buf, int len)
{
  int samples_read = 0;
  
  if (state != STATE_CONNECTED)
  {
    return 0;
  }
  
  while (samples_read < len)
  {
    int read_cnt = min(SEND_BUFFER_SIZE - send_buffer_cnt, len-samples_read);
    memcpy(send_buffer + send_buffer_cnt, buf + samples_read,
	read_cnt * sizeof(*send_buffer));
    send_buffer_cnt += read_cnt;
    samples_read += read_cnt;
    
    if (send_buffer_cnt == SEND_BUFFER_SIZE)
    {
      bool packet_sent = sendGsmPacket();
      if (!packet_sent)
      {
	break;
      }
      send_buffer_cnt = 0;
    }
  }
  
  //printf("Samples read = %d\n", samples_read);
  
  return samples_read;
  
} /* Qso::sendAudio */


bool Qso::sendAudioRaw(GsmVoicePacket *packet)
{
  packet->seqNum = htons(next_audio_seq++);
  
  int ret = Dispatcher::instance()->sendAudioMsg(remote_ip, packet,
      	      	      	      	      	      	 sizeof(*packet));
  if (ret == -1)
  {
    perror("sendAudioMsg in Qso::sendAudioRaw");
    return false;
  }
  
  return true;

} /* Qso::sendAudioRaw */


bool Qso::flushAudioSendBuffer(void)
{
  if (state != STATE_CONNECTED)
  {
    return false;
  }
  
  bool success = true;
  if (send_buffer_cnt > 0)
  {
    memset(send_buffer + send_buffer_cnt, 0,
	sizeof(send_buffer) - sizeof(*send_buffer) * send_buffer_cnt);
    send_buffer_cnt = SEND_BUFFER_SIZE;
    success = sendGsmPacket();
    send_buffer_cnt = 0;
  }
  
  return success;
  
} /* Qso::flushAudioSendBuffer */




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


void Qso::printData(const unsigned char *buf, int len)
{
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
  setState(STATE_BYE_RECEIVED);
  disconnect();
} /* Qso::handleByePacket */


inline void Qso::handleSdesPacket(unsigned char *buf, int len)
{
  struct rtcp_sdes_request sdes_items;
  sdes_items.nitems = 1;
  sdes_items.item[0].r_item = RTCP_SDES_NAME;
  sdes_items.item[0].r_text = NULL;
  parseSDES(buf, &sdes_items);
  if(sdes_items.item[0].r_text != NULL)
  {
    char remote_id[40];
    remote_id[0] = 0;
    copySDESitem(sdes_items.item[0].r_text, remote_id);
    //printData(remote_id, strlen(remote_id));
    char strtok_buf[40];
    char *strtok_buf_ptr = strtok_buf;
    char *remote_call_str = strtok_r(remote_id, " \t\n\r", &strtok_buf_ptr);
    char *remote_name_str = strtok_r(NULL, " \t\n\r", &strtok_buf_ptr);
    if ((remote_call_str != 0) && (remote_call_str[0] != 0))
    {
      if (remote_name_str == 0)
      {
	remote_name_str = "?";
      }
      remote_call = remote_call_str;
      remote_name = remote_name_str;
    }
  }
      
  switch (state)
  {
    case STATE_CONNECTING:
      setState(STATE_CONNECTED);
      break;
    
    case STATE_DISCONNECTED:
      accept();
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
      if (null+1 < info_buf+len)
      {
      	string trailing_data(null+1, info_buf+len);
	cerr << "Trailing info data: ";
	printData(reinterpret_cast<const unsigned char*>(null+1),
	      	  info_buf+len-(null+1));
      }
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
  GsmVoicePacket *voice_packet = reinterpret_cast<GsmVoicePacket*>(buf);
  
  audioReceivedRaw(voice_packet);
  
  /* gsm_signal average = 0; */
  for(int i=0; i<4; i++)
  {
    gsm_signal sbuff[160];
    gsm_decode(gsmh, voice_packet->data + i*33, sbuff);
    if (rx_indicator_timer == 0)
    {
      receiving_audio = true;
      isReceiving(true); 
      rx_indicator_timer = new Timer(RX_INDICATOR_HANG_TIME);
      rx_indicator_timer->expired.connect(slot(this, &Qso::checkRxActivity));
    }
    gettimeofday(&last_audio_packet_received, 0);
    
    /*
    for(int j=0; j<640; j+=2)
    {
      average += abs(*(sbuff + j));
    }
    average /= 320;
    if(sendStrength)
    {
      sprintf(line, "%C %d", STRENGTH, average);
      puts(line);
    }
    */
    
    audioReceived(sbuff, 160);
  }
  
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
    keep_alive_timer->expired.connect(slot(this, &Qso::sendKeepAlive));
    con_timeout_timer = new Timer(CON_TIMEOUT_TIME, Timer::TYPE_PERIODIC);
    con_timeout_timer->expired.connect(slot(this, &Qso::connectionTimeout));
  }
    
  return send_sdes_ok;
    
} /* Qso::setupConnection */


void Qso::cleanupConnection(void)
{
  if (rx_indicator_timer != 0)
  {
    receiving_audio = false;
    isReceiving(false);
    delete rx_indicator_timer;
    rx_indicator_timer = 0;
  }
  delete keep_alive_timer;
  keep_alive_timer = 0;
  delete con_timeout_timer;
  con_timeout_timer = 0;
  setState(STATE_DISCONNECTED);
} /* Qso::cleanupConnection */


bool Qso::sendGsmPacket(void)
{
  assert(send_buffer_cnt == SEND_BUFFER_SIZE);

  GsmVoicePacket voice_packet;
  voice_packet.version = 0xc0;
  voice_packet.pt = 0x03;
  voice_packet.time = htonl(0);
  voice_packet.ssrc = htonl(0);
  for(int i=0; i<SEND_BUFFER_SIZE/160; i++)
  {
    gsm_encode(gsmh, send_buffer + i*160, voice_packet.data + i*33);
  }
  voice_packet.seqNum = htons(next_audio_seq++);
  
  int ret = Dispatcher::instance()->sendAudioMsg(remote_ip, &voice_packet,
      sizeof(voice_packet));
  if (ret == -1)
  {
    perror("sendAudioMsg in Qso::sendGsmPacket");
    return false;
  }
  
  return true;
  
} /* Qso::sendGsmPacket */


void Qso::checkRxActivity(Timer *timer)
{
  struct timeval tv;
  gettimeofday(&tv, 0);
  struct timeval diff;
  timersub(&tv, &last_audio_packet_received, &diff);
  long diff_ms = diff.tv_sec * 1000 + diff.tv_usec / 1000;
  if (diff_ms >= RX_INDICATOR_HANG_TIME)
  {
    receiving_audio = false;
    isReceiving(false);
    delete rx_indicator_timer;
    rx_indicator_timer = 0;
  }
  else
  {
    rx_indicator_timer->setTimeout(RX_INDICATOR_HANG_TIME-diff_ms+100);
  }
} /* Qso::checkRxActivity */



/*
 * This file has not been truncated
 */

