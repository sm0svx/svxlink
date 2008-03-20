/**
@file	 NetTx.cpp
@brief   Contains a class that connect to a remote transmitter via IP
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-09

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioPacer.h>
#include <SigCAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "NetTx.h"
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

NetTx::NetTx(Config &cfg, const string& name)
  : cfg(cfg), name(name), tcp_con(0), is_transmitting(false),
    mode(Tx::TX_OFF), ctcss_enable(false), pacer(0)
{
} /* NetTx::NetTx */


NetTx::~NetTx(void)
{
  clearHandler();
  delete pacer;
  //FIXME delete tcp_con;
} /* NetTx::~NetTx */


bool NetTx::initialize(void)
{
  string host;
  if (!cfg.getValue(name, "HOST", host))
  {
    cerr << "*** ERROR: Config variable " << name << "/HOST not set\n";
    return false;
  }

  string tcp_port(NET_TRX_DEFAULT_TCP_PORT);
  cfg.getValue(name, "TCP_PORT", tcp_port);
  
  string udp_port(NET_TRX_DEFAULT_UDP_PORT);
  cfg.getValue(name, "UDP_PORT", udp_port);
  
  tcp_con = NetTrxTcpClient::instance(host, atoi(tcp_port.c_str()));
  tcp_con->connected.connect(slot(*this, &NetTx::tcpConnected));
  tcp_con->disconnected.connect(slot(*this, &NetTx::tcpDisconnected));
  tcp_con->msgReceived.connect(slot(*this, &NetTx::handleMsg));
  tcp_con->connect();
  
  pacer = new AudioPacer(8000, 512, 50);
  setHandler(pacer);
  
  SigCAudioSink *sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(slot(*this, &NetTx::sendAudio));
  sigc_sink->sigFlushSamples.connect(slot(*this, &NetTx::sendFlush));
  pacer->registerSink(sigc_sink, true);
  
  return true;
  
} /* NetTx:initialize */


void NetTx::setTxCtrlMode(TxCtrlMode mode)
{
  this->mode = mode;
  
  MsgSetTxCtrlMode *msg = new MsgSetTxCtrlMode(mode);
  sendMsg(msg);
} /* NetTx::setTxCtrlMode */


bool NetTx::isTransmitting(void) const
{
  return is_transmitting;
} /* NetTx::isTransmitting */


void NetTx::enableCtcss(bool enable)
{
  ctcss_enable = enable;
  MsgEnableCtcss *msg = new MsgEnableCtcss(enable);
  sendMsg(msg);
} /* NetTx::enableCtcss */


void NetTx::sendDtmf(const std::string& digits)
{
  MsgSendDtmf *msg = new MsgSendDtmf(digits);
  sendMsg(msg);
} /* NetTx::sendDtmf */


int NetTx::sendAudio(float *samples, int count)
{
  assert(count > 0);
  if (count > MsgAudio::MAX_COUNT)
  {
    count = MsgAudio::MAX_COUNT;
  }
  MsgAudio *msg = new MsgAudio(samples, count);
  sendMsg(msg);
  return count;
} /* NetTx::writeSamples */


void NetTx::sendFlush(void)
{
  MsgFlush *msg = new MsgFlush;
  sendMsg(msg);
} /* NetTx::flushSamples */



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

void NetTx::tcpConnected(void)
{
  cout << name << ": Connected to remote transmitter at "
       << tcp_con->remoteHost() << ":" << tcp_con->remotePort() << "\n";
  
  MsgSetTxCtrlMode *mode_msg = new MsgSetTxCtrlMode(mode);
  sendMsg(mode_msg);
  
  MsgEnableCtcss *ctcss_msg = new MsgEnableCtcss(ctcss_enable);
  sendMsg(ctcss_msg);
  
} /* NetTx::tcpConnected */


void NetTx::tcpDisconnected(TcpConnection *con,
      	      	      	    TcpConnection::DisconnectReason reason)
{
  cout << name << ": Disconnected from remote transmitter at "
       << con->remoteHost() << ":" << con->remotePort()
       << ": " << TcpConnection::disconnectReasonStr(reason) << "\n";
  
} /* NetTx::tcpDisconnected */


void NetTx::handleMsg(Msg *msg)
{
  switch (msg->type())
  {
    case MsgHeartbeat::TYPE:
    {
      break;
    }
    
    case MsgAuth::TYPE:
    {
      msg = reinterpret_cast<MsgAuth*>(msg);
      break;
    }
    
    case MsgTxTimeout::TYPE:
    {
      txTimeout();
      break;
    }
    
    case MsgTransmitterStateChange::TYPE:
    {
      MsgTransmitterStateChange *state_msg
      	  = reinterpret_cast<MsgTransmitterStateChange*>(msg);
      cout << name << ": The transmitter is "
      	   << (state_msg->isTransmitting() ? "ON" : "OFF") << endl;
      transmitterStateChange(state_msg->isTransmitting());
      break;
    }
    
    case MsgAllSamplesFlushed::TYPE:
    {
      sourceAllSamplesFlushed();
      break;
    }
    
    /*
    default:
      cerr << "*** ERROR: Unknown TCP message received. Type="
      	   << msg->type() << ", Size=" << msg->size() << endl;
      break;
    */
  }
  
} /* NetTx::handleMsg */


void NetTx::sendMsg(Msg *msg)
{
  if (tcp_con != 0)
  {
    tcp_con->sendMsg(msg);
  }
} /* NetUplink::sendMsg */



/*
 * This file has not been truncated
 */

