/**
@file	 NetTx.cpp
@brief   Contains a class that connect to a remote transmitter via IP
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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
#include <AsyncAudioPacer.h>
#include <AsyncAudioEncoder.h>


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
  : Tx(name), cfg(cfg), tcp_con(0), log_disconnects_once(false),
    log_disconnect(true), is_transmitting(false), mode(Tx::TX_OFF),
    ctcss_enable(false), pacer(0), is_connected(false), pending_flush(false),
    unflushed_samples(false), audio_enc(0), fq(0),
    modulation(Modulation::MOD_UNKNOWN)
{
} /* NetTx::NetTx */


NetTx::~NetTx(void)
{
  clearHandler();
  delete audio_enc;
  delete pacer;
  tcp_con->deleteInstance();
} /* NetTx::~NetTx */


bool NetTx::initialize(void)
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
  
  cfg.getValue(name(), "LOG_DISCONNECTS_ONCE", log_disconnects_once);

  string audio_enc_name;
  cfg.getValue(name(), "CODEC", audio_enc_name);
  if (audio_enc_name.empty())
  {
    audio_enc_name = "RAW";
  }
  
  string auth_key;
  cfg.getValue(name(), "AUTH_KEY", auth_key);
  
  pacer = new AudioPacer(INTERNAL_SAMPLE_RATE, 512, 50);
  setHandler(pacer);
  
  audio_enc = AudioEncoder::create(audio_enc_name);
  if (audio_enc == 0)
  {
    cerr << "*** ERROR: Illegal audio codec (" << audio_enc_name
          << ") specified for transmitter " << name() << "\n";
    return false;
  }
  audio_enc->writeEncodedSamples.connect(
          mem_fun(*this, &NetTx::writeEncodedSamples));
  audio_enc->flushEncodedSamples.connect(
          mem_fun(*this, &NetTx::flushEncodedSamples));
  string opt_prefix(audio_enc->name());
  opt_prefix += "_ENC_";
  list<string> names = cfg.listSection(name());
  list<string>::const_iterator nit;
  for (nit=names.begin(); nit!=names.end(); ++nit)
  {
    if ((*nit).find(opt_prefix) == 0)
    {
      string opt_value;
      cfg.getValue(name(), *nit, opt_value);
      string opt_name((*nit).substr(opt_prefix.size()));
      audio_enc->setOption(opt_name, opt_value);
    }
  }
  audio_enc->printCodecParams();
  pacer->registerSink(audio_enc);
  
  tcp_con = NetTrxTcpClient::instance(host, atoi(tcp_port.c_str()));
  if (tcp_con == 0)
  {
    return false;
  }
  tcp_con->setAuthKey(auth_key);
  tcp_con->isReady.connect(mem_fun(*this, &NetTx::connectionReady));
  tcp_con->msgReceived.connect(mem_fun(*this, &NetTx::handleMsg));
  tcp_con->connect();
  
  return true;
  
} /* NetTx:initialize */


void NetTx::setTxCtrlMode(TxCtrlMode mode)
{
  this->mode = mode;
  
  MsgSetTxCtrlMode *msg = new MsgSetTxCtrlMode(mode);
  sendMsg(msg);
  
  if (!is_connected)
  {
    switch (mode)
    {
      case Tx::TX_OFF:
      	setIsTransmitting(false);
	break;
      case Tx::TX_AUTO:
      	setIsTransmitting(unflushed_samples);
	break;
      case Tx::TX_ON:
      	setIsTransmitting(true);
	break;
    }
  }
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


void NetTx::sendDtmf(const std::string& digits, unsigned duration)
{
  MsgSendDtmf *msg = new MsgSendDtmf(digits, duration);
  sendMsg(msg);
} /* NetTx::sendDtmf */


void NetTx::setTransmittedSignalStrength(char rx_id, float siglev)
{
  //cout << "### NetTx::setTransmittedSignalStrength: rx_id=" << rx_id
  //     << " siglev=" << siglev << endl;
  MsgTransmittedSignalStrength *msg =
    new MsgTransmittedSignalStrength(siglev, rx_id);
  sendMsg(msg);
} /* NetTx::setTransmittedSignalStrength */


void NetTx::sendData(const std::vector<uint8_t> &msg)
{
  /*
  MsgSendDtmf *msg = new MsgSendDtmf(digits, duration);
  sendMsg(msg);
  */
} /* NetTx::sendData */


void NetTx::setFq(unsigned fq)
{
  this->fq = fq;
  MsgSetTxFq *msg = new MsgSetTxFq(fq);
  sendMsg(msg);
} /* NetTx::setFq */


void NetTx::setModulation(Modulation::Type mod)
{
  modulation = mod;
  MsgSetTxModulation *msg = new MsgSetTxModulation(mod);
  sendMsg(msg);
} /* NetTx::setModulation */



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

void NetTx::connectionReady(bool is_ready)
{
  if (is_ready)
  {
    cout << name() << ": Connected to remote transmitter at "
        << tcp_con->remoteHost() << ":" << tcp_con->remotePort() << "\n";
    
    is_connected = true;
    log_disconnect = true;
    
    MsgSetTxCtrlMode *mode_msg = new MsgSetTxCtrlMode(mode);
    sendMsg(mode_msg);
    
    MsgEnableCtcss *ctcss_msg = new MsgEnableCtcss(ctcss_enable);
    sendMsg(ctcss_msg);
    
    if (fq > 0)
    {
      MsgSetTxFq *msg = new MsgSetTxFq(fq);
      sendMsg(msg);
    }

    if (modulation != Modulation::MOD_UNKNOWN)
    {
      MsgSetTxModulation *msg = new MsgSetTxModulation(modulation);
      sendMsg(msg);
    }

    MsgAudioCodecSelect *msg = new MsgTxAudioCodecSelect(audio_enc->name());
    cout << name() << ": Requesting CODEC \"" << msg->name() << "\"\n";
    string opt_prefix(audio_enc->name());
    opt_prefix += "_DEC_";
    list<string> names = cfg.listSection(name());
    list<string>::const_iterator nit;
    for (nit=names.begin(); nit!=names.end(); ++nit)
    {
      if ((*nit).find(opt_prefix) == 0)
      {
	string opt_value;
	cfg.getValue(name(), *nit, opt_value);
	string opt_name((*nit).substr(opt_prefix.size()));
	msg->addOption(opt_name, opt_value);
      }
    }
    sendMsg(msg);
  }
  else
  {
    if (log_disconnect)
    {
      cout << name() << ": Disconnected from remote transmitter at "
          << tcp_con->remoteHost() << ":" << tcp_con->remotePort() << ": "
          << TcpConnection::disconnectReasonStr(tcp_con->disconnectReason())
          << "\n";
    }
    
    is_connected = false;
    log_disconnect = !log_disconnects_once;
  
    if (pending_flush)
    {
      allEncodedSamplesFlushed();
    }
  }
}


void NetTx::handleMsg(Msg *msg)
{
  switch (msg->type())
  {
    case MsgTxTimeout::TYPE:
    {
      txTimeout();
      break;
    }
    
    case MsgTransmitterStateChange::TYPE:
    {
      MsgTransmitterStateChange *state_msg
      	  = reinterpret_cast<MsgTransmitterStateChange*>(msg);
      setIsTransmitting(state_msg->isTransmitting());
      break;
    }
    
    case MsgAllSamplesFlushed::TYPE:
    {
      allEncodedSamplesFlushed();
      break;
    }
    
    /*
    default:
      cerr << name() << ": *** ERROR: Unknown TCP message received. Type="
      	   << msg->type() << ", Size=" << msg->size() << endl;
      break;
    */
  }
  
} /* NetTx::handleMsg */


void NetTx::sendMsg(Msg *msg)
{
  tcp_con->sendMsg(msg);
} /* NetUplink::sendMsg */


void NetTx::writeEncodedSamples(const void *buf, int size)
{
  pending_flush = false;
  unflushed_samples = true;
  
  if (is_connected)
  {
    const char *ptr = reinterpret_cast<const char *>(buf);
    while (size > 0)
    {
      const int bufsize = MsgAudio::BUFSIZE;
      int len = min(size, bufsize);
      MsgAudio *msg = new MsgAudio(ptr, len);
      sendMsg(msg);
      size -= len;
      ptr += len;
    }
  }
  else
  {
    if (mode == Tx::TX_AUTO)
    {
      setIsTransmitting(true);
    }
  }
} /* NetTx::writeEncodedSamples */


void NetTx::flushEncodedSamples(void)
{
  if (is_connected)
  {
    MsgFlush *msg = new MsgFlush;
    sendMsg(msg);
    pending_flush = true;
  }
  else
  {
    allEncodedSamplesFlushed();
  }
} /* NetTx::flushSamples */


void NetTx::setIsTransmitting(bool is_transmitting)
{
  if (is_transmitting != this->is_transmitting)
  {
    cout << name() << ": The transmitter is "
      	 << (is_transmitting ? "ON" : "OFF") << endl;
    this->is_transmitting = is_transmitting;
    transmitterStateChange(is_transmitting);
  }
} /* NetTx::setIsTransmitting */


void NetTx::allEncodedSamplesFlushed(void)
{
  unflushed_samples = false;
  pending_flush = false;
  audio_enc->allEncodedSamplesFlushed();
  
  if (!is_connected && (mode == Tx::TX_AUTO))
  {
    setIsTransmitting(false);
  }
} /* NetTx::allEncodedSamplesFlushed */



/*
 * This file has not been truncated
 */

