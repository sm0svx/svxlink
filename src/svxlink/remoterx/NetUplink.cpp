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



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpServer.h>
#include <NetRxMsg.h>


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
using namespace NetRxMsg;



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

NetUplink::NetUplink(Config &cfg, const string &name, Rx *rx)
  : server(0), con(0), recv_cnt(0), recv_exp(0), rx(rx)
{
  
} /* NetUplink::NetUplink */


NetUplink::~NetUplink(void)
{
  delete server;
} /* NetUplink::~NetUplink */


bool NetUplink::initialize(void)
{
  server = new TcpServer(NET_RX_DEFAULT_TCP_PORT);
  server->clientConnected.connect(slot(this, &NetUplink::clientConnected));
  
  rx->reset();
  rx->squelchOpen.connect(slot(this, &NetUplink::squelchOpen));
  rx->audioReceived.connect(slot(this, &NetUplink::audioReceived));
  rx->dtmfDigitDetected.connect(slot(this, &NetUplink::dtmfDigitDetected));
  rx->toneDetected.connect(slot(this, &NetUplink::toneDetected));
  
  return true;
  
} /* NetUplink::initialize */



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

void NetUplink::clientConnected(TcpConnection *incoming_con)
{
  cout << "Client connected: " << incoming_con->remoteHost() << ":"
       << incoming_con->remotePort() << endl;
  
  if (con == 0)
  {
    con = incoming_con;
    con->disconnected.connect(slot(this, &NetUplink::clientDisconnected));
    con->dataReceived.connect(slot(this, &NetUplink::tcpDataReceived));
    recv_exp = sizeof(Msg);
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
  con = 0;
  recv_exp = 0;
  rx->reset();
} /* NetUplink::clientDisconnected */


int NetUplink::tcpDataReceived(TcpConnection *con, void *data, int size)
{
  cout << "NetRx::tcpDataReceived: size=" << size << endl;
  
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
      msg = reinterpret_cast<MsgHeartbeat*>(msg);
      break;
    }
    case MsgAuth::TYPE:
    {
      msg = reinterpret_cast<MsgAuth*>(msg);
      break;
    }
    case MsgMute::TYPE:
    {
      MsgMute *mute_msg = reinterpret_cast<MsgMute*>(msg);
      cout << "Mute=" << (mute_msg->doMute() ? "true" : "false") << endl;
      rx->mute(mute_msg->doMute());
      break;
    }
    case MsgAddToneDetector::TYPE:
    {
      MsgAddToneDetector *atd = reinterpret_cast<MsgAddToneDetector*>(msg);
      cout << "AddToneDetector: fq=" << atd->fq()
      	   << " bw=" << atd->bw()
	   << " required_duration=" << atd->requiredDuration() << endl;
      rx->addToneDetector(atd->fq(), atd->bw(), atd->requiredDuration());
      break;
    }
    default:
      cerr << "*** ERROR: Unknown TCP message received. Type="
      	   << msg->type() << ", Size=" << msg->size() << endl;
      break;
  }
  
} /* NetUplink::handleMsg */


void NetUplink::sendMsg(Msg *msg)
{
  if (con != 0)
  {
    cout << con->write(msg, msg->size()) << " bytes written\n";
  }
  
  delete msg;
  
} /* NetUplink::sendMsg */


void NetUplink::squelchOpen(bool is_open)
{
  MsgSquelch *msg = new MsgSquelch(is_open, rx->signalStrength(),
      	      	      	      	   rx->sqlRxId());
  sendMsg(msg);  
} /* NetUplink::squelchOpen */


void NetUplink::dtmfDigitDetected(char digit)
{
  MsgDtmf *msg = new MsgDtmf(digit);
  sendMsg(msg);
} /* NetUplink::dtmfDigitDetected */


void NetUplink::toneDetected(int tone_fq)
{
  MsgTone *msg = new MsgTone(tone_fq);
  sendMsg(msg);
} /* NetUplink::toneDetected */


int NetUplink::audioReceived(short *samples, int count)
{
  cout << "NetUplink::audioReceived: count=" << count << endl;
  count = min(count, 512);
  MsgAudio *msg = new MsgAudio(samples, count);
  sendMsg(msg);
  return count;
} /* NetUplink::audioReceived */





/*
 * This file has not been truncated
 */

