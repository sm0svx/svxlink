/**
@file	 RfUplink.cpp
@brief   Uplink type that communicates to the SvxLink core through a transceiver
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-20

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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
#include <algorithm>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Rx.h>
#include <Tx.h>
#include <AsyncAudioDebugger.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioSelector.h>
#include <AsyncAudioPassthrough.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "RfUplink.h"



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

struct CtcssDetPar
{
  float fq;
  unsigned duration;

  CtcssDetPar(void) : fq(0.0f), duration(0) {}

  friend istream &operator>>(istream &input, CtcssDetPar &par)
  {
    char colon;
    input >> par.fq >> colon >> par.duration >> std::ws;
    if (colon != ':')
    {
      input.setstate(ios_base::failbit);
    }
    return input;
  }
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

RfUplink::RfUplink(Config &cfg, const string &name, Rx *rx, Tx *tx)
  : cfg(cfg), name(name), rx(rx), tx(tx), uplink_tx(0), uplink_rx(0),
    tx_audio_sel(0)
{
  
} /* RfUplink::RfUplink */


RfUplink::~RfUplink(void)
{
  delete uplink_tx;
  delete tx_audio_sel;
  delete uplink_rx;
} /* RfUplink::~RfUplink */


bool RfUplink::initialize(void)
{
  string uplink_tx_name;
  if (!cfg.getValue(name, "UPLINK_TX", uplink_tx_name))
  {
    cerr << "*** ERROR: Config variable " << name << "/UPLINK_TX not set.\n";
    return false;
  }

  string uplink_rx_name;
  if (!cfg.getValue(name, "UPLINK_RX", uplink_rx_name))
  {
    cerr << "*** ERROR: Config variable " << name << "/UPLINK_RX not set.\n";
    return false;
  }

  string value;
  bool mute_rx_on_tx = false;
  if (cfg.getValue(name, "MUTE_UPLINK_RX_ON_TX", value))
  {
    mute_rx_on_tx = atoi(value.c_str()) != 0;
  }

  bool loop_rx_to_tx = false;
  if (cfg.getValue(name, "LOOP_RX_TO_TX", value))
  {
    loop_rx_to_tx = atoi(value.c_str()) != 0;
  }

  CtcssDetPar ctcss_det_par;
  if (!cfg.getValue(name, "DETECT_CTCSS", ctcss_det_par, true))
  {
    cerr << "*** ERROR: Format error for config variable "
         << name << "/DETECT_CTCSS. Valid format: "
         << "tone_fq:min_duration\n";
    return false;
  }

  unsigned det_1750_duration = 0;
  cfg.getValue(name, "DETECT_1750", det_1750_duration);

  rx->squelchOpen.connect(
      mem_fun(*this, &RfUplink::rxSquelchOpen));
  rx->signalLevelUpdated.connect(
      mem_fun(*this, &RfUplink::rxSignalLevelUpdated));
  rx->dtmfDigitDetected.connect(
      mem_fun(*this, &RfUplink::rxDtmfDigitDetected));
  rx->toneDetected.connect(
      mem_fun(*this, &RfUplink::rxToneDetected));
  rx->reset();
  rx->setMuteState(Rx::MUTE_NONE);
  if ((ctcss_det_par.fq > 0) && (ctcss_det_par.duration > 0))
  {
    rx->addToneDetector(ctcss_det_par.fq, 4, 10, ctcss_det_par.duration);
  }
  if (det_1750_duration > 0)
  {
    rx->addToneDetector(1750, 50, 10, det_1750_duration);
  }
  AudioSource *prev_src = rx;

  AudioFifo *fifo = new AudioFifo(8000);
  fifo->setPrebufSamples(512);
  prev_src->registerSink(fifo, true);
  prev_src = fifo;
  
  AudioSplitter *splitter = 0;
  if (loop_rx_to_tx)
  {
    splitter = new AudioSplitter;
    prev_src->registerSink(splitter, true);
    prev_src = 0;
  }
  
  uplink_tx = TxFactory::createNamedTx(cfg, uplink_tx_name);
  if ((uplink_tx == 0) || !uplink_tx->initialize())
  {
    cerr << "*** ERROR: Could not initialize uplink transmitter for RfUplink "
         << name << "\n";
    delete uplink_tx;
    uplink_tx = 0;
    return false;
  }
  uplink_tx->setTxCtrlMode(Tx::TX_AUTO);
  uplink_tx->enableCtcss(true);
  if (loop_rx_to_tx)
  {
    splitter->addSink(uplink_tx);
  }
  else
  {
    prev_src->registerSink(uplink_tx);
  }
  prev_src = 0;
  
  
  uplink_rx = RxFactory::createNamedRx(cfg, uplink_rx_name);
  if ((uplink_rx == 0) || !uplink_rx->initialize())
  {
    delete uplink_tx;
    uplink_tx = 0;
    delete uplink_rx;
    uplink_rx = 0;
    return false;
  }
  uplink_rx->squelchOpen.connect(mem_fun(*this, &RfUplink::uplinkRxSquelchOpen));
  uplink_rx->dtmfDigitDetected.connect(
      mem_fun(*this, &RfUplink::uplinkRxDtmfRcvd));
  uplink_rx->signalLevelUpdated.connect(
      mem_fun(*this, &RfUplink::uplinkRxSignalLevelUpdated));
  uplink_rx->reset();
  uplink_rx->setMuteState(Rx::MUTE_NONE);
  if (mute_rx_on_tx)
  {
    uplink_tx->transmitterStateChange.connect(
        mem_fun(*this, &RfUplink::uplinkTxTransmitterStateChange));
  }
  prev_src = uplink_rx;
  
  if (loop_rx_to_tx)
  {
    tx_audio_sel = new AudioSelector;
    tx_audio_sel->addSource(prev_src);
    tx_audio_sel->enableAutoSelect(prev_src, 10);
    AudioPassthrough *connector = new AudioPassthrough;
    splitter->addSink(connector, true);
    tx_audio_sel->addSource(connector);
    tx_audio_sel->enableAutoSelect(connector, 0);
    prev_src = tx_audio_sel;
  }

  tx->setTxCtrlMode(Tx::TX_AUTO);
  prev_src->registerSink(tx);
  
  return true;
  
} /* RfUplink::initialize */



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

void RfUplink::uplinkRxSquelchOpen(bool is_open)
{
  if (is_open)
  {
    tx->setTransmittedSignalStrength(uplink_rx->sqlRxId(),
                                     uplink_rx->signalStrength());
  }
  /*
  else
  {
    tx->setTransmittedSignalStrength(uplink_rx->sqlRxId(), 0);
  }
  */
} /* RfUplink::uplinkRxSquelchOpen */


void RfUplink::uplinkRxDtmfRcvd(char digit, int duration)
{
  char digit_str[2] = {digit, 0};
  tx->sendDtmf(digit_str, duration);
} /* RfUplink::uplinkRxDtmfRcvd */


void RfUplink::uplinkRxSignalLevelUpdated(float siglev)
{
  //cout << "### RfUplink::uplinkRxSignalLevelUpdated: siglev="
  //     << siglev << endl;
  tx->setTransmittedSignalStrength('?', siglev);
} /* RfUplink::uplinkRxSignalLevelUpdated */


void RfUplink::rxSquelchOpen(bool is_open)
{
  if (is_open)
  {
    uplink_tx->setTransmittedSignalStrength(rx->sqlRxId(),
                                            rx->signalStrength());
  }
  /*
  else
  {
    uplink_tx->setTransmittedSignalStrength(rx->sqlRxId(), 0);
  }
  */
} /* RfUplink::rxSquelchOpen  */


void RfUplink::rxSignalLevelUpdated(float siglev)
{
  //cout << "### RfUplink::rxSignalLevelUpdated: siglev=" << siglev << endl;
  if (rx->squelchIsOpen())
  {
    uplink_tx->setTransmittedSignalStrength(rx->sqlRxId(), siglev);
  }
} /* RfUplink::rxSignalLevelUpdated */


void RfUplink::rxDtmfDigitDetected(char digit, int duration)
{
  cout << name << ": DTMF digit \"" << digit << "\" received for "
       << duration << "ms" << endl;
    // FIXME: DTMF digits should be retransmitted with the correct duration.
  const char dtmf_str[] = {digit, 0};
  uplink_tx->sendDtmf(dtmf_str, duration);
} /* RfUplink::rxDtmfDigitDetected */


void RfUplink::rxToneDetected(float fq)
{
  vector<uint8_t> msg;
  msg.push_back(Tx::DATA_CMD_TONE_DETECTED);
  uint8_t *fq_ptr = reinterpret_cast<uint8_t*>(&fq);
  msg.push_back(*fq_ptr++);
  msg.push_back(*fq_ptr++);
  msg.push_back(*fq_ptr++);
  msg.push_back(*fq_ptr++);
  uplink_tx->sendData(msg);
} /* RfUplink::rxToneDetected */


void RfUplink::uplinkTxTransmitterStateChange(bool is_transmitting)
{
  uplink_rx->setMuteState(is_transmitting ? Rx::MUTE_ALL : Rx::MUTE_NONE);
} /* RfUplink::uplinkTxTransmitterStateChange */



/*
 * This file has not been truncated
 */

