/**
@file	 NetUplink.cpp
@brief   Contains a class that implements a remote transceiver uplink via IP
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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
#include <cstring>
#include <cerrno>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>
#include <AsyncTcpServer.h>
#include <AsyncAudioFifo.h>
#include <AsyncTimer.h>
#include <AsyncAudioEncoder.h>
#include <AsyncAudioDecoder.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioPassthrough.h>


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
    cfg(cfg), name(name), last_msg_timestamp(), heartbeat_timer(0),
    audio_enc(0), audio_dec(0), loopback_con(0), rx_splitter(0),
    tx_selector(0), state(STATE_DISC), mute_tx_timer(0), tx_muted(false),
    fallback_enabled(false), tx_ctrl_mode(Tx::TX_OFF)
{
  heartbeat_timer = new Timer(10000);
  heartbeat_timer->setEnable(false);
  heartbeat_timer->expired.connect(mem_fun(*this, &NetUplink::heartbeat));

    // FIXME: Shouldn't we use the updates directly from the receiver instead?
    // Why is this even here?!
  //siglev_check_timer = new Timer(1000, Timer::TYPE_PERIODIC);
  //siglev_check_timer->setEnable(true);
  //siglev_check_timer->expired.connect(mem_fun(*this, &NetUplink::checkSiglev));
  
} /* NetUplink::NetUplink */


NetUplink::~NetUplink(void)
{
  delete audio_enc;
  delete audio_dec;
  delete fifo;
  delete tx_selector;
  delete rx_splitter;
  delete loopback_con;
  delete server;
  delete heartbeat_timer;
  delete mute_tx_timer;
  //delete siglev_check_timer;
} /* NetUplink::~NetUplink */


bool NetUplink::initialize(void)
{
    // Initialize the GCrypt library if not already initialized
  if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P))
  {
    gcry_check_version(NULL);
    gcry_error_t err;
    err = gcry_control(GCRYCTL_DISABLE_SECMEM, 0);
    if (err != GPG_ERR_NO_ERROR)
    {
      cerr << "*** ERROR: Failed to initialize the Libgcrypt library: "
           << gcry_strsource(err) << "/" << gcry_strerror(err) << endl;
      return false;
    }
      // Tell Libgcrypt that initialization has completed
    err = gcry_control(GCRYCTL_INITIALIZATION_FINISHED, 0);
    if (err != GPG_ERR_NO_ERROR)
    {
      cerr << "*** ERROR: Failed to initialize the Libgcrypt library: "
           << gcry_strsource(err) << "/" << gcry_strerror(err) << endl;
      return false;
    }
  }

  string listen_port;
  if (!cfg.getValue(name, "LISTEN_PORT", listen_port))
  {
    cerr << "*** ERROR: Configuration variable " << name
      	 << "/LISTEN_PORT is missing.\n";
    return false;
  }

  cfg.getValue(name, "FALLBACK_REPEATER", fallback_enabled, true);
  cfg.getValue(name, "AUTH_KEY", auth_key);

  int mute_tx_on_rx = -1;
  cfg.getValue(name, "MUTE_TX_ON_RX", mute_tx_on_rx, true);
  if (mute_tx_on_rx >= 0)
  {
    mute_tx_timer = new Timer(mute_tx_on_rx);
    mute_tx_timer->setEnable(false);
    mute_tx_timer->expired.connect(mem_fun(*this, &NetUplink::unmuteTx));
  }
  
  server = new TcpServer<>(listen_port);
  server->clientConnected.connect(mem_fun(*this, &NetUplink::clientConnected));
  server->clientDisconnected.connect(
      mem_fun(*this, &NetUplink::clientDisconnected));
  
  rx->reset();
  rx->squelchOpen.connect(mem_fun(*this, &NetUplink::squelchOpen));
  rx->signalLevelUpdated.connect(mem_fun(*this, &NetUplink::signalLevelUpdated));
  rx->dtmfDigitDetected.connect(mem_fun(*this, &NetUplink::dtmfDigitDetected));
  rx->toneDetected.connect(mem_fun(*this, &NetUplink::toneDetected));
  rx->selcallSequenceDetected.connect(
      mem_fun(*this, &NetUplink::selcallSequenceDetected));
  
  tx->txTimeout.connect(mem_fun(*this, &NetUplink::txTimeout));
  tx->transmitterStateChange.connect(
      mem_fun(*this, &NetUplink::transmitterStateChange));
  
  rx_splitter = new AudioSplitter;
  rx->registerSink(rx_splitter);

  loopback_con = new AudioPassthrough;
  
  rx_splitter->addSink(loopback_con);

  tx_selector = new AudioSelector;
  tx_selector->addSource(loopback_con);

  unsigned tx_jitter_buffer_delay = 0;
  cfg.getValue(name, "TX_JITTER_BUFFER_DELAY", tx_jitter_buffer_delay);
  fifo = new AudioFifo(INTERNAL_SAMPLE_RATE);
  fifo->setPrebufSamples(tx_jitter_buffer_delay*INTERNAL_SAMPLE_RATE/1000);
  tx_selector->addSource(fifo);
  tx_selector->selectSource(fifo);

  tx_selector->registerSink(tx);
  
  if (fallback_enabled)
  {
    setFallbackActive(true);
  }
  else
  {
    rx->setMuteState(Rx::MUTE_CONTENT);
  }

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

void NetUplink::handleIncomingConnection(TcpConnection *incoming_con)
{
  assert(con == 0);
  rx->reset();
  if (fallback_enabled) // Deactivate fallback repeater mode
  {
    setFallbackActive(false);
  }
  
  if (audio_enc != 0)
  {
    rx_splitter->removeSink(audio_enc);
    delete audio_enc;
    audio_enc = 0;
  }
  
  delete audio_dec;
  audio_dec = 0;
  
  con = incoming_con;
  con->dataReceived.connect(mem_fun(*this, &NetUplink::tcpDataReceived));
  recv_exp = sizeof(Msg);
  recv_cnt = 0;
  heartbeat_timer->setEnable(true);
  gettimeofday(&last_msg_timestamp, NULL);
  
  setState(STATE_CON_SETUP);

  MsgProtoVer *ver_msg = new MsgProtoVer;
  sendMsg(ver_msg);
  
  if (auth_key.empty())
  {
    MsgAuthOk *auth_msg = new MsgAuthOk;
    sendMsg(auth_msg);
    setState(STATE_READY);
  }
  else
  {
    MsgAuthChallenge *auth_msg = new MsgAuthChallenge;
    memcpy(auth_challenge, auth_msg->challenge(),
           MsgAuthChallenge::CHALLENGE_LEN);
    sendMsg(auth_msg);
  }
} /* NetUplink::handleIncomingConnection */


void NetUplink::clientConnected(TcpConnection *incoming_con)
{
  cout << name << ": Client connected: " << incoming_con->remoteHost() << ":"
       << incoming_con->remotePort() << endl;
  
  switch (state)
  {
    case STATE_DISC:
      handleIncomingConnection(incoming_con);
      break;
    case STATE_CON_SETUP:
    case STATE_READY:
      cout << name << ": Only one client allowed. Disconnecting...\n";
      // Fall through
    case STATE_DISC_CLEANUP:
      incoming_con->disconnect();
      break;
  }
} /* NetUplink::clientConnected */


void NetUplink::disconnectCleanup(void)
{
  con = 0;
  recv_exp = 0;
  setState(STATE_DISC);

  rx->reset();
  tx->enableCtcss(false);
  fifo->clear();
  if (audio_dec != 0)
  {
    audio_dec->flushEncodedSamples();
  }
  tx->setTxCtrlMode(Tx::TX_OFF);
  heartbeat_timer->setEnable(false);

  if (mute_tx_timer != 0)
  {
    mute_tx_timer->setEnable(false);
  }

  tx_muted = false;
  tx_ctrl_mode = Tx::TX_OFF;
    
  if (fallback_enabled)
  {
    setFallbackActive(true);
  }
  else
  {
    rx->setMuteState(Rx::MUTE_CONTENT);
  }
} /* NetUplink::disconnectCleanup */


void NetUplink::clientDisconnected(TcpConnection *the_con,
                                   TcpConnection::DisconnectReason reason)
{
  cout << name << ": Client disconnected: " << the_con->remoteHost() << ":"
       << the_con->remotePort() << endl;
  con = 0;
  setState(STATE_DISC_CLEANUP);
  Application::app().runTask(mem_fun(*this, &NetUplink::disconnectCleanup));
} /* NetUplink::clientDisconnected */


int NetUplink::tcpDataReceived(TcpConnection *con, void *data, int size)
{
  //cout << "NetRx::tcpDataReceived: size=" << size << endl;
  
  //Msg *msg = reinterpret_cast<Msg*>(data);
  //cout << "Received a TCP message with type " << msg->type()
  //     << " and size " << msg->size() << endl;
  
    // Discard data if we are not in one of the "connected" states
  if ((state != STATE_CON_SETUP) && (state != STATE_READY))
  {
    return size;
  }

  if (recv_exp == 0)
  {
    cerr << "*** ERROR: Unexpected TCP data received in NetUplink "
         << name << ". Throwing it away...\n";
    return size;
  }
  
  int orig_size = size;
  
  char *buf = static_cast<char*>(data);
  while (size > 0)
  {
    unsigned read_cnt = min(static_cast<unsigned>(size), recv_exp-recv_cnt);
    if (recv_cnt+read_cnt > sizeof(recv_buf))
    {
      cerr << "*** ERROR: TCP receive buffer overflow in NetUplink "
           << name << ". Disconnecting...\n";
      forceDisconnect();
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
	if (msg->size() == sizeof(Msg))
	{
	  handleMsg(msg);
	  recv_cnt = 0;
	  recv_exp = sizeof(Msg);
	}
	else if (msg->size() > sizeof(Msg))
	{
      	  recv_exp = msg->size();
	}
	else
	{
	  cerr << "*** ERROR: Illegal message header received in NetUplink "
               << name << ". Header length too small (" << msg->size()
               << ")\n";
          forceDisconnect();
	  return orig_size;
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
  switch (state)
  {
    case STATE_DISC:
    case STATE_DISC_CLEANUP:
      return;
      
    case STATE_CON_SETUP:
      if (msg->type() == MsgAuthResponse::TYPE &&
          msg->size() == sizeof(MsgAuthResponse))
      {
        MsgAuthResponse *resp_msg = reinterpret_cast<MsgAuthResponse *>(msg);
        if (!resp_msg->verify(auth_key, auth_challenge))
        {
          cerr << "*** ERROR: Authentication error in NetUplink "
               << name << ".\n";
          forceDisconnect();
          return;
        }
        else
        {
          MsgAuthOk *ok_msg = new MsgAuthOk;
          sendMsg(ok_msg);
        }
        setState(STATE_READY);
      }
      else
      {
        cerr << "*** ERROR: Protocol error in NetUplink " << name << ".\n";
        forceDisconnect();
      }
      return;
    
    case STATE_READY:
      break;
  }
  
  gettimeofday(&last_msg_timestamp, NULL);
  
  switch (msg->type())
  {
    case MsgHeartbeat::TYPE:
    {
      break;
    }
    
    case MsgReset::TYPE:
    {
      rx->reset();
      break;
    }
    
    case MsgSetRxFq::TYPE:
    {
      MsgSetRxFq *fq_msg = reinterpret_cast<MsgSetRxFq*>(msg);
      cout << rx->name() << ": SetRxFq(" << fq_msg->fq() << ")\n";
      rx->setFq(fq_msg->fq());
      break;
    }

    case MsgSetRxModulation::TYPE:
    {
      MsgSetRxModulation *mod_msg = reinterpret_cast<MsgSetRxModulation*>(msg);
      cout << rx->name() << ": SetRxModulation("
           << Modulation::toString(mod_msg->modulation()) << ")\n";
      rx->setModulation(mod_msg->modulation());
      break;
    }

    case MsgSetMuteState::TYPE:
    {
      MsgSetMuteState *mute_msg = reinterpret_cast<MsgSetMuteState*>(msg);
      cout << rx->name() << ": SetMuteState("
           << Rx::muteStateToString(mute_msg->muteState())
      	   << ")\n";
      rx->setMuteState(mute_msg->muteState());
      break;
    }
    
    case MsgAddToneDetector::TYPE:
    {
      MsgAddToneDetector *atd = reinterpret_cast<MsgAddToneDetector*>(msg);
      cout << rx->name() << ": AddToneDetector(" << atd->fq()
      	   << ", " << atd->bw()
	   << ", " << atd->requiredDuration() << ")\n";
      rx->addToneDetector(atd->fq(), atd->bw(), atd->thresh(),
      	      	      	  atd->requiredDuration());
      break;
    }
    
    case MsgSetTxCtrlMode::TYPE:
    {
      MsgSetTxCtrlMode *mode_msg = reinterpret_cast<MsgSetTxCtrlMode *>(msg);
      tx_ctrl_mode = mode_msg->mode();
      if (!tx_muted)
      {
	tx->setTxCtrlMode(tx_ctrl_mode);
      }
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
      tx->sendDtmf(dtmf_msg->digits(), dtmf_msg->duration());
      break;
    }
    
    case MsgRxAudioCodecSelect::TYPE:
    {
      MsgRxAudioCodecSelect *codec_msg = 
          reinterpret_cast<MsgRxAudioCodecSelect *>(msg);
      if (audio_enc != 0)
      {
	rx_splitter->removeSink(audio_enc);
	delete audio_enc;
      }
      audio_enc = AudioEncoder::create(codec_msg->name());
      if (audio_enc != 0)
      {
        audio_enc->writeEncodedSamples.connect(
                mem_fun(*this, &NetUplink::writeEncodedSamples));
        audio_enc->flushEncodedSamples.connect(
                mem_fun(*audio_enc, &AudioEncoder::allEncodedSamplesFlushed));
        //audio_enc->registerSource(rx);
	rx_splitter->addSink(audio_enc);
        cout << name << ": Using CODEC \"" << audio_enc->name()
             << "\" to encode RX audio\n";
	
	MsgRxAudioCodecSelect::Opts opts;
	codec_msg->options(opts);
	MsgRxAudioCodecSelect::Opts::const_iterator it;
	for (it=opts.begin(); it!=opts.end(); ++it)
	{
	  audio_enc->setOption((*it).first, (*it).second);
	}
	audio_enc->printCodecParams();
      }
      else
      {
        cerr << "*** ERROR: Received request for unknown RX audio codec ("
             << codec_msg->name() << ") in NetUplink " << name << "\n";
      }
      break;
    }
    
    case MsgTxAudioCodecSelect::TYPE:
    {
      MsgTxAudioCodecSelect *codec_msg = 
          reinterpret_cast<MsgTxAudioCodecSelect *>(msg);
      delete audio_dec;
      audio_dec = AudioDecoder::create(codec_msg->name());
      if (audio_dec != 0)
      {
        audio_dec->registerSink(fifo);
        audio_dec->allEncodedSamplesFlushed.connect(
            mem_fun(*this, &NetUplink::allEncodedSamplesFlushed));
        cout << name << ": Using CODEC \"" << audio_dec->name()
             << "\" to decode TX audio\n";
	
	MsgRxAudioCodecSelect::Opts opts;
	codec_msg->options(opts);
	MsgTxAudioCodecSelect::Opts::const_iterator it;
	for (it=opts.begin(); it!=opts.end(); ++it)
	{
	  audio_dec->setOption((*it).first, (*it).second);
	}
	audio_dec->printCodecParams();
      }
      else
      {
        cerr << "*** ERROR: Received request for unknown TX audio codec ("
             << codec_msg->name() << ") in NetUplink " << name << "\n";
      }
      break;
    }
    
    case MsgAudio::TYPE:
    {
      //cout << "NetUplink [MsgAudio]\n";
      if (!tx_muted && (audio_dec != 0))
      {
        MsgAudio *audio_msg = reinterpret_cast<MsgAudio*>(msg);
        audio_dec->writeEncodedSamples(audio_msg->buf(), audio_msg->size());
      }
      break;
    }
    
    case MsgFlush::TYPE:
    {
      if (audio_dec != 0)
      {
        audio_dec->flushEncodedSamples();
      }
      break;
    } 

    case MsgTransmittedSignalStrength::TYPE:
    {
      MsgTransmittedSignalStrength *siglev_msg =
        reinterpret_cast<MsgTransmittedSignalStrength *>(msg);
      tx->setTransmittedSignalStrength(siglev_msg->sqlRxId(),
                                       siglev_msg->signalStrength());
      break;
    }
    
    case MsgSetTxFq::TYPE:
    {
      MsgSetTxFq *fq_msg = reinterpret_cast<MsgSetTxFq*>(msg);
      cout << tx->name() << ": SetTxFq(" << fq_msg->fq() << ")\n";
      tx->setFq(fq_msg->fq());
      break;
    }

    case MsgSetTxModulation::TYPE:
    {
      MsgSetTxModulation *mod_msg = reinterpret_cast<MsgSetTxModulation*>(msg);
      cout << tx->name() << ": SetTxModulation("
           << Modulation::toString(mod_msg->modulation()) << ")\n";
      tx->setModulation(mod_msg->modulation());
      break;
    }

    default:
      cerr << "*** ERROR: Unknown TCP message received in NetUplink "
           << name << ". type=" << msg->type() << ", size="
           << msg->size() << endl;
      break;
  }
  
} /* NetUplink::handleMsg */


void NetUplink::sendMsg(Msg *msg)
{
  if ((state == STATE_CON_SETUP) || (state == STATE_READY))
  {
    int written = con->write(msg, msg->size());
    if (written == -1)
    {
      cerr << "*** ERROR: TCP transmit error in NetUplink \"" << name
           << "\": " << strerror(errno) << ".\n";
      forceDisconnect();
    }
    else if (written != static_cast<int>(msg->size()))
    {
      cerr << "*** ERROR: TCP transmit buffer overflow in NetUplink "
           << name << ".\n";
      forceDisconnect();
    }
  }
  
  delete msg;
  
} /* NetUplink::sendMsg */


void NetUplink::squelchOpen(bool is_open)
{
  if (mute_tx_timer != 0)
  {
    if (is_open)
    {
      tx_muted = true;
      tx->setTxCtrlMode(Tx::TX_OFF);
    }
    else
    {
      mute_tx_timer->setEnable(true);
    }
  }

  MsgSquelch *msg = new MsgSquelch(is_open, rx->signalStrength(),
                                   rx->sqlRxId(), rx->squelchActivityInfo());
  sendMsg(msg);
} /* NetUplink::squelchOpen */


void NetUplink::dtmfDigitDetected(char digit, int duration)
{
  cout << name << ": DTMF digit detected: " << digit << " with duration " << duration
       << " milliseconds" << endl;
  MsgDtmf *msg = new MsgDtmf(digit, duration);
  sendMsg(msg);
} /* NetUplink::dtmfDigitDetected */


void NetUplink::toneDetected(float tone_fq)
{
  cout << name << ": Tone detected: " << tone_fq << endl;
  MsgTone *msg = new MsgTone(tone_fq);
  sendMsg(msg);
} /* NetUplink::toneDetected */


void NetUplink::selcallSequenceDetected(std::string sequence)
{
  // cout "Sel5 sequence detected: " << sequence << endl;
  MsgSel5 *msg = new MsgSel5(sequence);
  sendMsg(msg);
} /* NetUplink::selcallSequenceDetected */


void NetUplink::writeEncodedSamples(const void *buf, int size)
{
  //cout << "NetUplink::writeEncodedSamples: size=" << size << endl;
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
} /* NetUplink::writeEncodedSamples */


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


void NetUplink::allEncodedSamplesFlushed(void)
{
  MsgAllSamplesFlushed *msg = new MsgAllSamplesFlushed;
  sendMsg(msg);
} /* NetUplink::allEncodedSamplesFlushed */


void NetUplink::heartbeat(Timer *t)
{
  MsgHeartbeat *msg = new MsgHeartbeat;
  sendMsg(msg);
  
  struct timeval diff_tv;
  struct timeval now;
  gettimeofday(&now, NULL);
  timersub(&now, &last_msg_timestamp, &diff_tv);
  int diff_ms = diff_tv.tv_sec * 1000 + diff_tv.tv_usec / 1000;
  
  if (diff_ms > 15000)
  {
    cerr << "*** ERROR: Heartbeat timeout in NetUplink " << name << "\n";
    forceDisconnect();
  }
  
  t->reset();
  
} /* NetTrxTcpClient::heartbeat */


#if 0
void NetUplink::checkSiglev(Timer *t)
{
  squelchOpen(rx->squelchIsOpen());
} /* NetUplink::checkSiglev */
#endif


void NetUplink::unmuteTx(Timer *t)
{
  mute_tx_timer->setEnable(false);
  tx_muted = false;
  tx->setTxCtrlMode(tx_ctrl_mode);
} /* NetUplink::unmuteTx */


void NetUplink::setFallbackActive(bool activate)
{
  rx->reset();
  if (activate)
  {
    cout << name << ": Activating fallback repeater mode\n";
    tx->setTxCtrlMode(Tx::TX_AUTO);
    tx_selector->selectSource(loopback_con);
    rx->setMuteState(Rx::MUTE_NONE);
  }
  else
  {
    cout << name << ": Deactivating fallback repeater mode\n";
    tx->setTxCtrlMode(Tx::TX_OFF);
    tx_selector->selectSource(fifo);
  }
} /* NetUplink::setFallbackActive */


void NetUplink::signalLevelUpdated(float siglev)
{
  MsgSiglevUpdate *msg = new MsgSiglevUpdate(rx->signalStrength(),
					     rx->sqlRxId());
  sendMsg(msg);  
} /* NetUplink::signalLevelUpdated */


void NetUplink::forceDisconnect(void)
{
  con->disconnect();
  clientDisconnected(con, TcpConnection::DR_ORDERED_DISCONNECT);
} /* NetUplink::forceDisconnect */


/*
 * This file has not been truncated
 */
