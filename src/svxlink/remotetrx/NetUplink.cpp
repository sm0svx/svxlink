/**
@file	 NetUplink.cpp
@brief   Contains a class the implements a remote receiver uplink via IP
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
<A brief description of the program or library this file belongs to>
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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpServer.h>
#include <SigCAudioSink.h>
#include <SigCAudioSource.h>
#include <AsyncAudioFifo.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "NetUplink.h"
#include "Rx.h"


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

NetUplink::NetUplink(Config &cfg, const string &name, Rx *rx, Tx *tx,
      	      	     const string& port_str)
  : server(0), con(0), recv_cnt(0), recv_exp(0), rx(rx), tx(tx), fifo(0),
    sigc_src(0), cfg(cfg), name(name)
{
  
} /* NetUplink::NetUplink */


NetUplink::~NetUplink(void)
{
  delete fifo;
  delete sigc_sink;
  delete server;
} /* NetUplink::~NetUplink */


bool NetUplink::initialize(void)
{
  string listen_port;
  if (!cfg.getValue(name, "LISTEN_PORT", listen_port))
  {
    cerr << "*** ERROR: Configuration variable " << name
      	 << "/LISTEN_PORT is missing.\n";
    return false;
  }
  
  server = new TcpServer(listen_port);
  server->clientConnected.connect(slot(*this, &NetUplink::clientConnected));
  server->clientDisconnected.connect(
      slot(*this, &NetUplink::clientDisconnected));
  
  sigc_sink = new SigCAudioSink;
  sigc_sink->registerSource(rx);
  
  rx->reset();
  rx->squelchOpen.connect(slot(*this, &NetUplink::squelchOpen));
  sigc_sink->sigWriteSamples.connect(slot(*this, &NetUplink::audioReceived));
  sigc_sink->sigFlushSamples.connect(
	slot(*sigc_sink, &SigCAudioSink::allSamplesFlushed));
  rx->dtmfDigitDetected.connect(slot(*this, &NetUplink::dtmfDigitDetected));
  rx->toneDetected.connect(slot(*this, &NetUplink::toneDetected));
  
  sigc_src = new SigCAudioSource;
  sigc_src->sigAllSamplesFlushed.connect(
      slot(*this, &NetUplink::allSamplesFlushed));
  
  fifo = new AudioFifo(16000);
  sigc_src->registerSink(fifo);
  fifo->registerSink(tx);

  tx->txTimeout.connect(slot(*this, &NetUplink::txTimeout));
  tx->transmitterStateChange.connect(
      slot(*this, &NetUplink::transmitterStateChange));
    
  return true;
  
} /* NetUplink::initialize */



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


void NetUplink::clientConnected(TcpConnection *incoming_con)
{
  cout << "Client connected: " << incoming_con->remoteHost() << ":"
       << incoming_con->remotePort() << endl;
  
  if (con == 0)
  {
    con = incoming_con;
    //con->disconnected.connect(slot(*this, &NetUplink::clientDisconnected));
    con->dataReceived.connect(slot(*this, &NetUplink::tcpDataReceived));
    recv_exp = sizeof(Msg);
    recv_cnt = 0;
  }
  else
  {
    cout << "Only one client allowed. Disconnecting...\n";
    incoming_con->disconnect();
  }
} /* NetUplink::clientConnected */


void NetUplink::clientDisconnected(TcpConnection *the_con,
      	      	      	      	   TcpConnection::DisconnectReason reason)
{
  cout << "Client disconnected: " << con->remoteHost() << ":"
       << con->remotePort() << endl;

  assert(the_con == con);  
  con = 0;
  recv_exp = 0;
  rx->reset();
  
  tx->setTxCtrlMode(Tx::TX_OFF);
  tx->enableCtcss(false);
  fifo->clear();
  fifo->flushSamples();
  
} /* NetUplink::clientDisconnected */


int NetUplink::tcpDataReceived(TcpConnection *con, void *data, int size)
{
  //cout << "NetRx::tcpDataReceived: size=" << size << endl;
  
  //Msg *msg = reinterpret_cast<Msg*>(data);
  //cout << "Received a TCP message with type " << msg->type()
  //     << " and size " << msg->size() << endl;
  
  if (recv_exp == 0)
  {
    cerr << "*** ERROR: Unexpected TCP data received. Throwing it away...\n";
    return size;
  }
  
  int orig_size = size;
  
  char *buf = static_cast<char*>(data);
  while (size > 0)
  {
    int read_cnt = min(size, recv_exp-recv_cnt);
    if (recv_cnt+read_cnt > static_cast<int>(sizeof(recv_buf)))
    {
      cerr << "*** ERROR: TCP receive buffer overflow. Disconnecting...\n";
      con->disconnect();
      clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
      return orig_size;
    }
    memcpy(recv_buf+recv_cnt, buf, read_cnt);
    size -= read_cnt;
    recv_cnt += read_cnt;
    buf += read_cnt;
    
    if (recv_cnt == recv_exp)
    {
      if (recv_exp == sizeof(Msg))
      {
      	Msg *msg = reinterpret_cast<Msg*>(recv_buf);
	if (recv_exp == static_cast<int>(msg->size()))
	{
	  handleMsg(msg);
	  recv_cnt = 0;
	  recv_exp = sizeof(Msg);
	}
	else
	{
      	  recv_exp = msg->size();
	}
      }
      else
      {
      	Msg *msg = reinterpret_cast<Msg*>(recv_buf);
      	handleMsg(msg);
	recv_cnt = 0;
	recv_exp = sizeof(Msg);
      }
    }
  }
  
  return orig_size;
  
} /* NetUplink::tcpDataReceived */


void NetUplink::handleMsg(Msg *msg)
{
  switch (msg->type())
  {
    case MsgHeartbeat::TYPE:
    {
      break;
    }
    
    case MsgAuth::TYPE:
    {
      break;
    }
    
    case MsgMute::TYPE:
    {
      MsgMute *mute_msg = reinterpret_cast<MsgMute*>(msg);
      cout << "MsgMute(" << (mute_msg->doMute() ? "true" : "false") << ")\n";
      rx->mute(mute_msg->doMute());
      break;
    }
    
    case MsgAddToneDetector::TYPE:
    {
      MsgAddToneDetector *atd = reinterpret_cast<MsgAddToneDetector*>(msg);
      cout << "AddToneDetector(" << atd->fq()
      	   << ", " << atd->bw()
	   << ", " << atd->requiredDuration() << ")\n";
      rx->addToneDetector(atd->fq(), atd->bw(), atd->thresh(),
      	      	      	  atd->requiredDuration());
      break;
    }
    
    case MsgSetTxCtrlMode::TYPE:
    {
      MsgSetTxCtrlMode *mode_msg = reinterpret_cast<MsgSetTxCtrlMode *>(msg);
      tx->setTxCtrlMode(mode_msg->mode());
      break;
    }
     
    case MsgEnableCtcss::TYPE:
    {
      MsgEnableCtcss *ctcss_msg = reinterpret_cast<MsgEnableCtcss *>(msg);
      tx->enableCtcss(ctcss_msg->enable());
      break;
    }
     
    case MsgSendDtmf::TYPE:
    {
      MsgSendDtmf *dtmf_msg = reinterpret_cast<MsgSendDtmf *>(msg);
      tx->sendDtmf(dtmf_msg->digits());
      break;
    }
     
    case MsgAudio::TYPE:
    {
      MsgAudio *audio_msg = reinterpret_cast<MsgAudio *>(msg);
      sigc_src->writeSamples(audio_msg->samples(), audio_msg->count());
      break;
    }
     
    case MsgFlush::TYPE:
    {
      sigc_src->flushSamples();
      break;
    } 
    
    default:
      cerr << "*** ERROR: Unknown TCP message received. type="
      	   << msg->type() << ", tize=" << msg->size() << endl;
      break;
  }
  
} /* NetUplink::handleMsg */


void NetUplink::sendMsg(Msg *msg)
{
  //cout << "NetUplink::sendMsg: msg->size()=" << msg->size() << endl;
  if (con != 0)
  {
    int written = con->write(msg, msg->size());
    if (written != static_cast<int>(msg->size()))
    {
      cerr << "*** ERROR: TCP transmit buffer overflow.\n";
      con->disconnect();
      clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
    }
  }
  
  delete msg;
  
} /* NetUplink::sendMsg */


void NetUplink::squelchOpen(bool is_open)
{
  MsgSquelch *msg = new MsgSquelch(is_open, rx->signalStrength(),
      	      	      	      	   rx->sqlRxId());
  sendMsg(msg);
} /* NetUplink::squelchOpen */


void NetUplink::dtmfDigitDetected(char digit, int duration)
{
  cout << "DTMF digit detected: " << digit << " with duration " << duration
       << " milliseconds" << endl;
  MsgDtmf *msg = new MsgDtmf(digit, duration);
  sendMsg(msg);
} /* NetUplink::dtmfDigitDetected */


void NetUplink::toneDetected(float tone_fq)
{
  cout << "Tone detected: " << tone_fq << endl;
  MsgTone *msg = new MsgTone(tone_fq);
  sendMsg(msg);
} /* NetUplink::toneDetected */


int NetUplink::audioReceived(float *samples, int count)
{
  count = min(count, 512);
  MsgAudio *msg = new MsgAudio(samples, count);
  sendMsg(msg);
  return count;
} /* NetUplink::audioReceived */


void NetUplink::txTimeout(void)
{
  MsgTxTimeout *msg = new MsgTxTimeout;
  sendMsg(msg);
} /* NetUplink::txTimeout */


void NetUplink::transmitterStateChange(bool is_transmitting)
{
  MsgTransmitterStateChange *msg =
      new MsgTransmitterStateChange(is_transmitting);
  sendMsg(msg);
} /* NetUplink::transmitterStateChange */


void NetUplink::allSamplesFlushed(void)
{
  MsgAllSamplesFlushed *msg = new MsgAllSamplesFlushed;
  sendMsg(msg);
} /* NetUplink::allSamplesFlushed */


/*
 * This file has not been truncated
 */

