/**
@file	 NetTrxAdapter.cpp
@brief   Make it possible to connect a remote NetTx/NetRx as a Rx/Tx
@author  Tobias Blomberg / SM0SVX
@date	 2008-04-15

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
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
    RxAdapter(const string &name)
      : Rx(cfg, name)
    {
      mute_valve.setOpen(false);
      mute_valve.setBlockWhenClosed(false);
      AudioSink::setHandler(&mute_valve);
      AudioSource::setHandler(&mute_valve);
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
     * @brief 	Set the mute state for this receiver
     * @param 	mute_state The mute state to set for this receiver
     */
    virtual void setMuteState(MuteState new_mute_state)
    {
      //cout << name() << ": RxAdapter::mute: do_mute=" << do_mute << endl;
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
      return false;
    }
    
    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    virtual float signalStrength(void) const { return 1; }
    
    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    virtual int sqlRxId(void) const { return 0; }
    
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
    }
    
   
  private:
    AudioValve  mute_valve;
    Config    	cfg;
    
    
}; /* class RxAdapter */


class TxAdapter : public Tx, public AudioSource
{
  public:
    TxAdapter(void)
      : tx_ctrl_mode(Tx::TX_OFF), is_transmitting(false), is_idle(true)
    {
    }

    ~TxAdapter(void) {}

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
     * @brief 	Check if the transmitter is transmitting
     * @return	Return \em true if transmitting or else \em false
     */
    virtual bool isTransmitting(void) const
    {
      return is_transmitting;
    }
    
    /**
     * @brief 	Enable/disable CTCSS on TX
     * @param 	enable	Set to \em true to enable or \em false to disable CTCSS
     */
    virtual void enableCtcss(bool enable) { }
    
    /**
     * @brief 	Send a string of DTMF digits
     * @param 	digits	The digits to send
     */
    virtual void sendDtmf(const std::string& digits)
    {
      for (unsigned i=0; i<digits.size(); ++i)
      {
      	sendDtmfDigit(digits[i], 100);
      }
    }
    


    int writeSamples(const float *samples, int count)
    {
      //cout << "TxAdapter::writeSamples\n";
      is_idle = false;
      if ((tx_ctrl_mode == Tx::TX_AUTO) && !is_transmitting)
      {
      	transmit(true);
      }
      
      return sinkWriteSamples(samples, count);
    }
    
    void flushSamples(void)
    {
      //cout << "TxAdapter::flushSamples\n";
      sinkFlushSamples();
    }
    
    void resumeOutput()
    {
      sourceResumeOutput();
    }
    
    void allSamplesFlushed(void)
    {
      //cout << "TxAdapter::allSamplesFlushed\n";
      is_idle = true;
      sourceAllSamplesFlushed();
      if ((tx_ctrl_mode == Tx::TX_AUTO) && is_transmitting)
      {
      	transmit(false);
      }
    }
    
    signal<void, bool> sigTransmit;
    signal<void, char, int> sendDtmfDigit;
    
    
    
  private:
    Tx::TxCtrlMode  tx_ctrl_mode;
    bool      	    is_transmitting;
    bool      	    is_idle;

    void transmit(bool do_transmit)
    {
      //cout << "TxAdapter::transmit: do_transmit=" << do_transmit << endl;
      if (do_transmit == is_transmitting)
      {
      	return;
      }
      
      is_transmitting = do_transmit;
      sigTransmit(do_transmit);
      transmitterStateChange(do_transmit);
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
  AudioSource *prev_src = 0;

    // NetTx audio chain
  txa1 = new TxAdapter;
  if (!txa1->initialize())
  {
    return false;
  }
  prev_src = txa1;
  
  AudioFifo *fifo1 = new AudioFifo(8000);
  prev_src->registerSink(fifo1, true);
  prev_src = fifo1;
  
  rxa1 = new RxAdapter("NetTxAdapter");
  if (!rxa1->initialize())
  {
    return false;
  }
  prev_src->registerSink(rxa1, true);
  prev_src = 0;

    
    // NetRx audio chain
  txa2 = new TxAdapter;
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


  txa1->sigTransmit.connect(mem_fun(*rxa1, &RxAdapter::setSquelchState));
  txa1->sendDtmfDigit.connect(rxa1->dtmfDigitDetected.make_slot());
  txa2->sigTransmit.connect(mem_fun(*rxa2, &RxAdapter::setSquelchState));
  txa2->sendDtmfDigit.connect(rxa2->dtmfDigitDetected.make_slot());
  

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



/*
 * This file has not been truncated
 */

