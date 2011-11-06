/**
@file	 DtmfEncoder.h
@brief   This file contains a class that implements a DTMF encoder.
@author  Tobias Blomberg / SM0SVX
@date	 2006-07-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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

A_detailed_class_description
*/
class DtmfEncoder : public Async::AudioSource, sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    DtmfEncoder(int sampling_rate);
  
    /**
     * @brief 	Destructor
     */
    ~DtmfEncoder(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void setToneLength(int length_ms);
    void setToneSpacing(int spacing_ms);
    void setToneAmplitude(int amp_db);
    void send(const std::string &str);
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
    int       	sampling_rate;
    int       	tone_length;
    int       	tone_spacing;
    float       tone_amp;
    std::string current_str;
    int       	low_tone;
    int       	high_tone;
    int       	pos;
    int       	length;
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

