/**
@file	 LocalRxBase.h
@brief   A base receiver class to handle local receivers
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

This file contains a class that handle local receivers. A local receiver is
a receiver that is directly connected to the sound card on the computer where
the SvxLink core is running. It can also be a DDR (Digital Drop Receiver).

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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


#ifndef LOCAL_RX_BASE_INCLUDED
#define LOCAL_RX_BASE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <stdint.h>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioValve.h>
#include <AsyncAudioDelayLine.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Rx.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
  class AudioSplitter;
  class AudioValve;
  class AudioFifo;
};

class Squelch;
class HdlcDeframer;


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class SigLevDet;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	A base class to handle local receivers
@author Tobias Blomberg
@date   2004-03-21

This class handle local receivers. A local receiver is a receiver that is
directly connected to the sound card on the computer where the SvxLink core
is running. It can also be a DDR (Digital Drop Receiver).
*/
class LocalRxBase : public Rx
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit LocalRxBase(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~LocalRxBase(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void);
    
    /**
     * @brief 	Set the mute state for this receiver
     * @param 	mute_state The mute state to set for this receiver
     */
    virtual void setMuteState(MuteState new_mute_state);
    
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
                                 int required_duration);

    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    virtual float signalStrength(void) const;

    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    char sqlRxId(void) const;

    /**
     * @brief 	Reset the receiver object to its default settings
     */
    virtual void reset(void);

    /**
     * @brief  A signal that is emitted when the CTCSS tone SNR has changed
     * @param  snr The current SNR
     *
     * This signal will be emitted as soon as a new SNR value for the CTCSS
     * tone has been calculated. The signal will only be emitted when
     * CTCSS_MODE is set to 2 or 3.
     */
    sigc::signal<void, float> ctcssSnrUpdated;
    
  protected:
    /**
     * @brief   Open the audio input source
     * @return  Returns \em true on success or else \em false
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the audio source object is initialized before calling
     * the LocalRxBase::initialize function.
     */
    virtual bool audioOpen(void) = 0;

    /**
     * @brief   Close the audio input source
     *
     * This function may be used during the initialization of LocalRxBase so
     * make sure that the audio source object is initialized before calling the
     * LocalRxBase::initialize function.
     */
    virtual void audioClose(void) = 0;

    /**
     * @brief   Get the sampling rate of the audio source
     * @return  Returns the sampling rate of the audio source
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the proper sampling rate can be returned before calling
     * the LocalRxBase::initialize function.
     */
    virtual int audioSampleRate(void) = 0;

    /**
     * @brief   Get the audio source object
     * @return  Returns an instantiated audio source object
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the audio source object is initialized before calling
     * the LocalRxBase::initialize function.
     */
    virtual Async::AudioSource *audioSource(void) = 0;
    
  private:
    Async::Config     	      	&cfg;
    MuteState      	      	mute_state;
    Squelch   	      	      	*squelch_det;
    SigLevDet 	      	        *siglevdet;
    Async::AudioSplitter      	*tone_dets;
    Async::AudioValve 	        *sql_valve;
    Async::AudioDelayLine     	*delay;
    int       	      	      	sql_tail_elim;
    float            	      	preamp_gain;
    Async::AudioValve 	      	*mute_valve;
    unsigned                    sql_hangtime;
    unsigned                    sql_extended_hangtime;
    unsigned                    sql_extended_hangtime_thresh;
    Async::AudioFifo            *input_fifo;
    int                         dtmf_muting_pre;
    HdlcDeframer *              ob_afsk_deframer;
    HdlcDeframer *              ib_afsk_deframer;
    bool                        audio_dev_keep_open;

    int audioRead(float *samples, int count);
    void dtmfDigitActivated(char digit);
    void dataFrameReceived(std::vector<uint8_t> frame);
    void dataFrameReceivedIb(std::vector<uint8_t> frame);
    void dtmfDigitDeactivated(char digit, int duration_ms);
    void sel5Detected(std::string sequence);
    void audioStreamStateChange(bool is_active, bool is_idle);
    void onSquelchOpen(bool is_open);
    void tone1750detected(bool detected);
    void onSignalLevelUpdated(float siglev);
    void setSqlHangtimeFromSiglev(float siglev);
    void rxReadyStateChanged(void);

};  /* class LocalRxBase */


//} /* namespace */

#endif /* LOCAL_RX_BASE_INCLUDED */



/*
 * This file has not been truncated
 */

