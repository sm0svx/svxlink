/**
@file	 DtmfEncoder.h
@brief   This file contains a class that implements a DTMF encoder.
@author  Tobias Blomberg / SM0SVX
@date	 2006-07-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2015  Tobias Blomberg / SM0SVX

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


#ifndef DTMF_ENCODER_INCLUDED
#define DTMF_ENCODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <deque>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSource.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



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
@brief	This class implements a DTMF encoder
@author Tobias Blomberg / SM0SVX
@date   2006-07-09

This class implement a DTMF encoder which can be used to generate DTMF audio
from a sequence of DTMF digits.
*/
class DtmfEncoder : public Async::AudioSource, sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    DtmfEncoder(int sampling_rate=INTERNAL_SAMPLE_RATE);
  
    /**
     * @brief 	Destructor
     */
    ~DtmfEncoder(void);
  
    /**
     * @brief 	Set the duration for each digit
     * @param 	duration_ms The duration of the tone in milliseconds
     */
    void setDigitDuration(int duration_ms);

    int digitDuration(void) const
    {
      return 1000 * tone_length / sampling_rate;
    }

    /**
     * @brief   Set the spacing between each digit
     * @param   spacing_ms The spacing in milliseconds
     */
    void setDigitSpacing(int spacing_ms);

    int digitSpacing(void) const { return 1000 * tone_spacing / sampling_rate; }

    /**
     * @brief   Set the power of the generated DTMF digits
     * @param   power_db The power to set
     *
     * The tone power is set in dB. A value of 0dB will generate a DTMF digit
     * with the same power as a full scale sine wave. Since DTMF contain two
     * sine waves, each tone will have a power of -3dB.
     */
    void setDigitPower(int power_db);

    /**
     * @brief   Return the currently set tone power
     * @returns Return the tone power in dB (@see setTonePower)
     */
    int digitPower(void) const;

    /**
     * @brief   Send the specified digits
     * @param   str A string of digits (0-9, A-D, *, #)
     * @param   duration The duration of the digit
     */
    void send(const std::string &str, unsigned duration=0);

    /**
     * @brief   Check if we are currently transmitting digits
     * @returns Return \em true if digits are being transmitted
     */
    bool isSending(void) const { return is_sending_digits; }
    
    /**
     * @brief Resume audio output to the sink
     * 
     * This function must be reimplemented by the inheriting class. It
     * will be called when the registered audio sink is ready to accept
     * more samples.
     * This function is normally only called from a connected sink object.
     */
    void resumeOutput(void);
    
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function must be implemented by the inheriting class. It
     * will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    void allSamplesFlushed(void);
    
    /*
     * @brief A signal that is emitted when all digits have been sent.
     */
    sigc::signal<void> allDigitsSent;


  protected:
    
  private:
    struct SendQueueItem
    {
      char      digit;
      unsigned  duration;

      SendQueueItem(char digit, unsigned duration)
        : digit(digit), duration(duration)
      {
      }
    };
    typedef std::deque<SendQueueItem> SendQueue;

    unsigned    sampling_rate;
    unsigned    tone_length;
    unsigned    tone_spacing;
    float       tone_amp;
    //std::string current_str;
    SendQueue   send_queue;
    unsigned    low_tone;
    unsigned    high_tone;
    unsigned    pos;
    unsigned    length;
    bool      	is_playing;
    bool      	is_sending_digits;

    DtmfEncoder(const DtmfEncoder&);
    DtmfEncoder& operator=(const DtmfEncoder&);
    void playNextDigit(void);
    void writeAudio(void);
    
};  /* class DtmfEncoder */


//} /* namespace */

#endif /* DTMF_ENCODER_INCLUDED */



/*
 * This file has not been truncated
 */

