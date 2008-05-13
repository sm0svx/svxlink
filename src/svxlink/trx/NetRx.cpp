/**
@file	 NetRx.cpp
@brief   Contains a class that connect to a remote receiver via IP
@author  Tobias Blomberg
@date	 2006-04-14

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

#include <iostream>
#include <cassert>
#include <cstring>
#include <cstdlib>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "NetRx.h"
#include "NetTrxMsg.h"
#include "NetTrxTcpClient.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace NetTrxMsg;



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

class ToneDet
{
  public:
    float fq;
    int   bw;
    float thresh;
    int   required_duration;
    
    ToneDet(float fq, int bw, float thresh, int required_duration)
      : fq(fq), bw(bw), thresh(thresh), required_duration(required_duration) {}
    
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

NetRx::NetRx(Config &cfg, const string& name)
  : Rx(name), cfg(cfg), is_muted(true), tcp_con(0), last_signal_strength(0.0),
    last_sql_rx_id(0), unflushed_samples(false), sql_is_open(false)
{
} /* NetRx::NetRx */


NetRx::~NetRx(void)
{
  // FIXME
  //delete tcp_con;
  
  list<ToneDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    delete *it;
  }
  tone_detectors.clear();
  
} /* NetRx::~NetRx */


bool NetRx::initialize(void)
{
  string host;
  if (!cfg.getValue(name(), "HOST", host))
  {
    cerr << "*** ERROR: Config variable " << name() << "/HOST not set\n";
    return false;
  }

  string tcp_port(NET_TRX_DEFAULT_TCP_PORT);
  cfg.getValue(name(), "TCP_PORT", tcp_port);
  
  string udp_port(NET_TRX_DEFAULT_UDP_PORT);
  cfg.getValue(name(), "UDP_PORT", udp_port);
  
  tcp_con = NetTrxTcpClient::instance(host, atoi(tcp_port.c_str()));
  tcp_con->connected.connect(slot(*this, &NetRx::tcpConnected));
  tcp_con->disconnected.connect(slot(*this, &NetRx::tcpDisconnected));
  tcp_con->msgReceived.connect(slot(*this, &NetRx::handleMsg));
  tcp_con->connect();
  
  return true;
  
} /* NetRx:initialize */


void NetRx::mute(bool do_mute)
{
  /*
  cout << "NetRx::mute[" << name() << "]: do_mute="
       << (do_mute ? "TRUE" : "FALSE") << "\n";
  */
  
  if (do_mute == is_muted)
  {
    return;
  }
  
  is_muted = do_mute;
  
  if (do_mute)
  {
    last_signal_strength = 0.0;
    last_sql_rx_id = 0;
    sql_is_open = false;
    
    if (unflushed_samples)
    {
      sinkFlushSamples();
    }
    else
    {
      setSquelchState(false);
    }
  }
    
  MsgMute *msg = new MsgMute(do_mute);
  sendMsg(msg);
  
} /* NetRx::mute */


bool NetRx::addToneDetector(float fq, int bw, float thresh,
      	      	      	    int required_duration)
{
  //cout << "addToneDetector: fq=" << fq << " bw=" << bw
  //     << " required_duration=" << required_duration << endl;
       
  ToneDet *det = new ToneDet(fq, bw, thresh, required_duration);
  tone_detectors.push_back(det);
  
  MsgAddToneDetector *msg =
      new MsgAddToneDetector(fq, bw, thresh, required_duration);
  sendMsg(msg);
  
  return true;

} /* NetRx::addToneDetector */


void NetRx::reset(void)
{
  list<ToneDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    delete *it;
  }
  tone_detectors.clear();
  
  is_muted = true;
  last_signal_strength = 0;
  last_sql_rx_id = 0;
  sql_is_open = false;
  
  if (unflushed_samples)
  {
    sinkFlushSamples();
  }
  else
  {
    setSquelchState(false);
  }

  MsgReset *msg = new MsgReset;
  sendMsg(msg);

} /* NetRx::reset */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void NetRx::allSamplesFlushed(void)
{
  unflushed_samples = false;
  if (!sql_is_open)
  {
    setSquelchState(false);
  }
} /* NetRx::allSamplesFlushed */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void NetRx::tcpConnected(void)
{
  cout << name() << ": Connected to remote receiver at "
       << tcp_con->remoteHost() << ":" << tcp_con->remotePort() << "\n";
  
  if (!is_muted)
  {
    MsgMute *msg = new MsgMute(false);
    sendMsg(msg);
  }
  
  list<ToneDet*>::iterator it;
  for (it=tone_detectors.begin(); it!=tone_detectors.end(); ++it)
  {
    MsgAddToneDetector *msg =
      	new MsgAddToneDetector((*it)->fq, (*it)->bw, (*it)->thresh,
      	      	      	       (*it)->required_duration);
    sendMsg(msg);
  }
  
} /* NetRx::tcpConnected */


void NetRx::tcpDisconnected(TcpConnection *con,
      	      	      	    TcpConnection::DisconnectReason reason)
{
  cout << name() << ": Disconnected from remote receiver "
       << con->remoteHost() << ":" << con->remotePort()
       << ": " << TcpConnection::disconnectReasonStr(reason) << "\n";
  
  sql_is_open = false;
  if (unflushed_samples)
  {
    sinkFlushSamples();
  }
  else
  {
    setSquelchState(false);
  }
} /* NetRx::tcpDisconnected */


void NetRx::handleMsg(Msg *msg)
{
  switch (msg->type())
  {
    case MsgSquelch::TYPE:
    {
      if (!is_muted)
      {
	MsgSquelch *sql_msg = reinterpret_cast<MsgSquelch*>(msg);
	last_signal_strength = sql_msg->signalStrength();
	last_sql_rx_id = sql_msg->sqlRxId();
	sql_is_open = sql_msg->isOpen();
	
	if (sql_msg->isOpen())
	{
	  setSquelchState(true);
	}
	else
	{
	  if (unflushed_samples)
	  {
            sinkFlushSamples();
	  }
	  else
	  {
	    setSquelchState(false);
	  }
	}
      }
      break;
    }
    
    case MsgDtmf::TYPE:
    {
      if (!is_muted)
      {
      	MsgDtmf *dtmf_msg = reinterpret_cast<MsgDtmf*>(msg);
      	dtmfDigitDetected(dtmf_msg->digit(), dtmf_msg->duration());
      }
      break;
    }
    
    case MsgTone::TYPE:
    {
      if (!is_muted)
      {
	MsgTone *tone_msg = reinterpret_cast<MsgTone*>(msg);
	toneDetected(tone_msg->toneFq());
      }
      break;
    }
    
    case MsgAudio::TYPE:
    {
      if (!is_muted && sql_is_open)
      {
	MsgAudio *audio_msg = reinterpret_cast<MsgAudio*>(msg);
	unflushed_samples = true;
	sinkWriteSamples(audio_msg->samples(), audio_msg->count());
      }
      break;
    }
    
    /*
    default:
      cerr << "*** ERROR: Unknown TCP message received. Type="
      	   << msg->type() << ", Size=" << msg->size() << endl;
      break;
    */
  }
  
} /* NetRx::handleMsg */


void NetRx::sendMsg(Msg *msg)
{
  tcp_con->sendMsg(msg);
} /* NetUplink::sendMsg */



/*
 * This file has not been truncated
 */

