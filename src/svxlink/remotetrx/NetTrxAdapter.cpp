/**
@file	 NetTrxAdapter.cpp
@brief   Make it possible to connect a remote NetTx/NetRx as a Rx/Tx
@author  Tobias Blomberg / SM0SVX
@date	 2008-04-15

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
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



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Rx.h>
#include <Tx.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioFifo.h>
#include <AsyncAudioDebugger.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "NetTrxAdapter.h"
#include "Uplink.h"
#include "NetUplink.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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

class RxAdapter : public Rx, public AudioSink
{
  public:
    RxAdapter(const string &name, char rx_id=Rx::ID_UNKNOWN)
      : Rx(cfg, name), siglev_timer(1000, Timer::TYPE_PERIODIC),
        rx_id(rx_id), forced_rx_id(rx_id != Rx::ID_UNKNOWN), siglev(0.0)
    {
      mute_valve.setOpen(false);
      mute_valve.setBlockWhenClosed(false);
      AudioSink::setHandler(&mute_valve);
      AudioSource::setHandler(&mute_valve);
      siglev_timer.setEnable(false);
      siglev_timer.expired.connect(mem_fun(*this, &RxAdapter::reportSiglev));
    }
    
    virtual ~RxAdapter(void) {}

    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void)
    {
      return Rx::initialize();
    }
    
    /**
     * @brief   Set the RX identity
     * @param   rx_id A char identifying the receiver
     *
     * Use this function to set the RX identity dynamically. If an RX identity
     * was given during object construction, it cannot be changed.
     */
    void setRxId(char rx_id)
    {
      if (!forced_rx_id)
      {
        this->rx_id = rx_id;
      }
    }

    /**
     * @brief 	Set the mute state for this receiver
     * @param 	new_mute_state The mute state to set for this receiver
     */
    void setMuteState(MuteState new_mute_state) override
    {
      //cout << name() << ": RxAdapter::mute: do_mute=" << do_mute << endl;
      Rx::setMuteState(new_mute_state);
      mute_valve.setOpen(new_mute_state == MUTE_NONE);
    }

    /**
     * @brief 	Call this function to add a tone detector to the RX
     * @param 	fq The tone frequency to detect
     * @param 	bw The bandwidth of the detector
     * @param 	thresh The detection threshold in dB SNR
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of tone detection or
     *	      	\em false if it's not.
     */
    virtual bool addToneDetector(float fq, int bw, float thresh,
      	      	      	      	 int required_duration)
    {
      //cout << "### RxAdapter::addToneDetector: fq=" << fq << endl;
      if (fq < 300.0f)
      {
        ctcssDetectorFqChanged(fq);
        return true;
      }
      return false;
    }
    
    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    virtual float signalStrength(void) const { return siglev; }
    
    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    virtual char sqlRxId(void) const { return rx_id; }
    
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    virtual void reset(void)
    {
      mute_valve.setOpen(false);
    }

    void setSquelchState(bool is_open)
    {
      if (mute_valve.isOpen())
      {
      	Rx::setSquelchState(is_open);
      }
      siglev_timer.setEnable(squelchIsOpen());
    }

    void setSignalLevel(char rx_id, float new_siglev)
    {
      clock_gettime(CLOCK_MONOTONIC, &last_siglev_time);
      setRxId(rx_id);
      siglev = new_siglev;
      signalLevelUpdated(siglev);
    }
    
    sigc::signal<void, float> ctcssDetectorFqChanged;
   
  private:
    AudioValve      mute_valve;
    Config    	    cfg;
    Timer           siglev_timer;
    char            rx_id;
    bool            forced_rx_id;
    float           siglev;
    struct timespec last_siglev_time;

    void reportSiglev(Timer *t)
    {
      struct timespec ts;
      long diff = 2000;
      if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0)
      {
        diff =  1000 * (ts.tv_sec - last_siglev_time.tv_sec) +
                (ts.tv_nsec - last_siglev_time.tv_nsec) / 1000000;
      }
      if (diff > 1500)
      {
        signalLevelUpdated(siglev);
      }
    }
    
}; /* class RxAdapter */


class TxAdapter : public Tx, public AudioSource
{
  public:
    TxAdapter(const string &name)
      : Tx(name), tx_ctrl_mode(Tx::TX_OFF),
        is_idle(true), ctcss_enabled(false), ctcss_fq(0.0f)
    {
    }

    ~TxAdapter(void) {}

    /**
     * @brief   Set the CTCSS frequency used when reporting detection
     * @param   fq The frequency in Hz
     */
    void setCtcssFq(float fq) { ctcss_fq = fq; }

    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void)
    {
      return true;
    }
    
    /**
     * @brief 	Set the transmit control mode
     * @param 	mode The mode to use to set the transmitter on or off.
     *
     * Use this function to turn the transmitter on (TX_ON) or off (TX__OFF).
     * There is also a third mode (TX_AUTO) that will automatically turn the
     * transmitter on when there is audio to transmit.
     */
    virtual void setTxCtrlMode(TxCtrlMode mode)
    {
      //cout << "Tx::setTxCtrlMode: mode=" << mode << endl;
      if (mode == tx_ctrl_mode)
      {
      	return;
      }
      tx_ctrl_mode = mode;
      
      switch (mode)
      {
	case Tx::TX_OFF:
	  transmit(false);
	  break;

	case Tx::TX_ON:
	  transmit(true);
	  break;

	case Tx::TX_AUTO:
      	  transmit(!is_idle);
	  break;
      }
    }
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    virtual void enableCtcss(bool enable)
    {
      //cout << "### TxAdapter::enableCtcss: enable=" << enable << "\n";
      ctcss_enabled = enable;
    }
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     */
    virtual void sendDtmf(const std::string& digits, unsigned duration)
    {
      for (unsigned i=0; i<digits.size(); ++i)
      {
        sendDtmfDigit(digits[i], duration);
      }
    }

    /**
     * @brief   Set the signal level value that should be transmitted
     * @param   siglev The signal level to transmit
     * @param   rx_id  The id of the receiver that received the signal
     *
     * This function does not set the output power of the transmitter but
     * instead sets a signal level value that is transmitted with the
     * transmission if the specific Tx object supports it. This can be used
     * on a link transmitter to transport signal level measurements to the
     * link receiver.
     */
    virtual void setTransmittedSignalStrength(char rx_id, float siglev)
    {
      //cout << "### TxAdapter::setTransmittedSignalStrength: rx_id=" << rx_id
      //     << " siglev=" << siglev << endl;
      transmittedSignalStrength(rx_id, siglev);
    }

    int writeSamples(const float *samples, int count)
    {
      is_idle = false;
      if ((tx_ctrl_mode == Tx::TX_AUTO) && !isTransmitting())
      {
      	transmit(true);
      }
      
      return sinkWriteSamples(samples, count);
    }
    
    void flushSamples(void)
    {
      sinkFlushSamples();
    }
    
    void resumeOutput()
    {
      sourceResumeOutput();
    }
    
    void allSamplesFlushed(void)
    {
      is_idle = true;
      sourceAllSamplesFlushed();
      if ((tx_ctrl_mode == Tx::TX_AUTO) && isTransmitting())
      {
      	transmit(false);
      }
    }
    
    signal<void, bool>        sigTransmit;
    signal<void, char, int>   sendDtmfDigit;
    signal<void, float>       sendTone;
    signal<void, char, float> transmittedSignalStrength;
    
  private:
    Tx::TxCtrlMode  tx_ctrl_mode;
    bool      	    is_idle;
    bool            ctcss_enabled;
    float           ctcss_fq;

    void transmit(bool do_transmit)
    {
      setIsTransmitting(do_transmit);
      sigTransmit(do_transmit);
      transmitterStateChange(do_transmit);
      if (ctcss_enabled && do_transmit && (ctcss_fq > 0.0f))
      {
        //cout << "### TxAdapter::transmit: Sending tone\n";
        sendTone(ctcss_fq);
      }
    }
    
}; /* class TxAdapter */



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

map<string, NetTrxAdapter*> NetTrxAdapter::net_trx_adapters;



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

NetTrxAdapter *NetTrxAdapter::instance(Async::Config &cfg,
      	      	      	      	       const std::string &net_uplink_name)
{
  if (net_trx_adapters.count(net_uplink_name) > 0)
  {
    return net_trx_adapters[net_uplink_name];
  }
  else
  {
    NetTrxAdapter *adapter = new NetTrxAdapter(cfg, net_uplink_name);
    if (!adapter->initialize())
    {
      delete adapter;
      return 0;
    }
    net_trx_adapters[net_uplink_name] = adapter;
    return adapter;
  }
} /* NetTrxAdapter::instance */


NetTrxAdapter::NetTrxAdapter(Config &cfg, const string &net_uplink_name)
  : ul(0), cfg(cfg), txa1(0), txa2(0), rxa1(0), net_uplink_name(net_uplink_name)
{
  
} /* NetTrxAdapter::NetTrxAdapter */


NetTrxAdapter::~NetTrxAdapter(void)
{
  delete txa1;
  delete txa2;
} /* NetTrxAdapter::~NetTrxAdapter */


bool NetTrxAdapter::initialize(void)
{
  char rx_id = Rx::ID_UNKNOWN;
  if (!cfg.getValue(net_uplink_name, "RX_ID", rx_id, true))
  {
    cerr << "*** ERROR: Invalid RX id specified for RX "
         << net_uplink_name << endl;
    return false;
  }

  float rx_siglev = 1.0f;
  cfg.getValue(net_uplink_name, "RX_SIGLEV", rx_siglev);

  AudioSource *prev_src = 0;

    // NetTx audio chain
  txa1 = new TxAdapter("NetRxAdapter");
  if (!txa1->initialize())
  {
    return false;
  }
  prev_src = txa1;
  
  AudioFifo *fifo1 = new AudioFifo(8000);
  prev_src->registerSink(fifo1, true);
  prev_src = fifo1;
  
  rxa1 = new RxAdapter("NetTxAdapter", rx_id);
  if (!rxa1->initialize())
  {
    return false;
  }
  rxa1->setSignalLevel(rx_id, rx_siglev);
  prev_src->registerSink(rxa1, true);
  prev_src = 0;

  txa1->sigTransmit.connect(mem_fun(*rxa1, &RxAdapter::setSquelchState));
  txa1->sendDtmfDigit.connect(rxa1->dtmfDigitDetected.make_slot());
  txa1->sendTone.connect(rxa1->toneDetected.make_slot());
  txa1->transmittedSignalStrength.connect(sigc::bind<0>(
        mem_fun(*this, &NetTrxAdapter::onTransmittedSignalStrength), rxa1));
  rxa1->ctcssDetectorFqChanged.connect(mem_fun(*txa1, &TxAdapter::setCtcssFq));

    
    // NetRx audio chain
  txa2 = new TxAdapter("NetTxAdapter");
  if (!txa2->initialize())
  {
    return false;
  }
  prev_src = txa2;
  
  AudioFifo *fifo2 = new AudioFifo(8000);
  prev_src->registerSink(fifo2, true);
  prev_src = fifo2;
  
  RxAdapter *rxa2 = new RxAdapter("NetRxAdapter");
  if (!rxa2->initialize())
  {
    return false;
  }
  prev_src->registerSink(rxa2, true);
  prev_src = 0;

  txa2->sigTransmit.connect(mem_fun(*rxa2, &RxAdapter::setSquelchState));
  txa2->sendDtmfDigit.connect(rxa2->dtmfDigitDetected.make_slot());
  txa2->sendTone.connect(rxa2->toneDetected.make_slot());
  txa2->transmittedSignalStrength.connect(sigc::bind<0>(
        mem_fun(*this, &NetTrxAdapter::onTransmittedSignalStrength), rxa2));
  rxa2->ctcssDetectorFqChanged.connect(mem_fun(*txa2, &TxAdapter::setCtcssFq));
  
    // Create a NetUplink connected to the TX and RX adapters
  ul = new NetUplink(cfg, net_uplink_name, rxa2, txa1);
  if (!ul->initialize())
  {
    cerr << "*** ERROR: Could not initialize uplink object for uplink "
         << net_uplink_name << "\n";
    delete ul;
    ul = 0;
    return false;
  }

  return true;
  
} /* NetTrxAdapter::initialize */




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

void NetTrxAdapter::onTransmittedSignalStrength(RxAdapter *rx, char rx_id,
                                                float siglev)
{
  rx->setSignalLevel(rx_id, siglev);
} /* NetTrxAdapter::onTransmittedSignalStrength */



/*
 * This file has not been truncated
 */

