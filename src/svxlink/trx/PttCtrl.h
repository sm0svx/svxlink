/**
@file	 PttCtrl.h
@brief   Implements a PTT controller
@author  Tobias Blomberg / SM0SVX
@date	 2008-07-17

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

#ifndef PTT_CTRL_INCLUDED
#define PTT_CTRL_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>
#include <AsyncAudioValve.h>
#include <AsyncAudioFifo.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Tx.h"


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

/**
@brief	A class that implements the PTT controller
@author Tobias Blomberg
@date   2008-07-17

This class handle the triggering of the PTT for LocalTx. It can operate in
three different modes: OFF, ON or AUTO. The first two are obvious. In AUTO
mode, the PTT is triggered as long as there are incoming audio.

This class also implements the TX delay feature.
*/
class PttCtrl : public Async::AudioSink, public Async::AudioSource,
      	      	public sigc::trackable
{
  public:
    /**
     * @brief 	Constructor
     * @param 	tx_delay  The number of milliseconds to delay audio after
     *	      	      	  PTT activation
     */
    explicit PttCtrl(int tx_delay);
    
    /**
     * @brief 	Destructor
     */
    ~PttCtrl(void);

    /**
     * @brief 	Set the TX delay
     * @param 	delay_ms  The number of milliseconds to delay audio after
     *	      	      	  PTT activation
     */
    void setTxDelay(int delay_ms) { tx_delay = delay_ms; }
    
    /**
     * @brief 	Set the PTT control mode
     * @param 	mode  Set to one of Tx::TX_OFF, Tx::TX_ON or Tx::TX_AUTO
     *
     * Use this function to set the TX mode. This decide how the PTT should
     * be handled. The ON and OFF modes are obvious. The AUTO mode will
     * trigger the PTT for as long as there are incoming audio.
     */
    void setTxCtrlMode(Tx::TxCtrlMode mode);
    
    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    int writeSamples(const float *samples, int count);

    /**
     * @brief   Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    void allSamplesFlushed(void);
    
    /**
     * @brief This signal is emitted when the PTT should be turned on or off
     * @param do_transmit Turn the transmitter on if \em true or else
     *	      	      	  turn it off
     */
    sigc::signal<void, bool> transmitterStateChange;

    sigc::signal<bool, bool> preTransmitterStateChange;
    
    
  private:
    Tx::TxCtrlMode      tx_ctrl_mode;
    bool      	        is_transmitting;
    Async::Timer     	*tx_delay_timer;
    int       	        tx_delay;
    Async::AudioFifo 	*fifo;
    Async::AudioValve	valve;
    
    void transmit(bool do_transmit);
    void txDelayExpired(Async::Timer *t);
    
}; /* class PttCtrl */


#endif /* PTT_CTRL_INCLUDED */


/*
 * This file has not been truncated
 */

