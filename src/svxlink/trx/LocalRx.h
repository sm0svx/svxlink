/**
@file	 LocalRx.h
@brief   A receiver class to handle local receivers
@author  Tobias Blomberg
@date	 2004-03-21

This file contains a class that handle local receivers. A local receiver is
a receiver that is directly connected to the sound card on the computer where
the SvxLink core is running.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008 Tobias Blomberg / SM0SVX

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


#ifndef LOCAL_RX_INCLUDED
#define LOCAL_RX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>


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
  class AudioIO;
  class AudioSplitter;
  class AudioValve;
};

class Squelch;


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
@brief	A class to handle local receivers
@author Tobias Blomberg
@date   2004-03-21

This class handle local receivers. A local receiver is a receiver that is
directly connected to the sound card on the computer where the SvxLink core
is running.
*/
class LocalRx : public Rx
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit LocalRx(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~LocalRx(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    bool initialize(void);
    
    /**
     * @brief 	Mute the receiver
     * @param 	do_mute Set to \em true to mute or \em false to unmute
     */
    void mute(bool do_mute);
    
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
    bool addToneDetector(float fq, int bw, float thresh, int required_duration);

    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    float signalStrength(void) const;
    
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    void reset(void);
    

  protected:
    
  private:
    Async::Config     	      	&cfg;
    Async::AudioIO    	      	*audio_io;
    bool      	      	      	is_muted;
    Squelch   	      	      	*squelch_det;
    SigLevDet 	      	        *siglevdet;
    Async::AudioSplitter      	*tone_dets;
    Async::AudioValve 	        *sql_valve;
    Async::AudioDelayLine     	*delay;
    bool      	      	      	mute_dtmf;
    int       	      	      	sql_tail_elim;
    int       	      	      	preamp_gain;
    
    int audioRead(float *samples, int count);
    void dtmfDigitActivated(char digit);
    void dtmfDigitDeactivated(char digit, int duration_ms);
    void sel5Detected(std::string sequence);
    void audioStreamStateChange(bool is_active, bool is_idle);
    void onSquelchOpen(bool is_open);
    SigLevDet *createSigLevDet(const std::string &name, int sample_rate);

};  /* class LocalRx */


//} /* namespace */

#endif /* LOCAL_RX_INCLUDED */



/*
 * This file has not been truncated
 */

