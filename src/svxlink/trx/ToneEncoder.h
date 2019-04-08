/**
@file	 ToneEncoder.h
@brief   A tone encoder that can be used SvxLink wide
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2019-04-08

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2019  Tobias Blomberg / SM0SVX

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


#ifndef TONE_ENCODER_INCLUDED
#define TONE_ENCODER_INCLUDED


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
@brief	A tone encoder that can be used SvxLink wide
@author Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date   2019-04-08
*/

class ToneEncoder : public Async::AudioSource
{
  public:
    /**
     * @brief Constructor
     * @param tone_hz The frequency in Hz of the tone that should be detected
     * @param level The audio level (0-100%)
     */
    ToneEncoder(float tone_hz, int level);

    /**
     * @brief   Destructor
     */
    ~ToneEncoder(void);

    /**
     * @brief  Return the detection frequency
     * @param  tone_hz The tone frequency in Hertz
     */
    void setTone(float tone_hz);

    /**
     * @brief  Set the detection delay
     * @param  level Set the audio level from 0-100%
     */ 
    void setLevel(int level);

    /**
     * @brief  Set the detection delay in processing blocks
     * @param  enable Enable/disable the tone generator
     */
    void setEnable(bool enabled);
    
    void allSamplesFlushed(void);
    
    void resumeOutput(void);
    
    
  private:
 
    float               m_tone;
    float               m_level;
    bool                is_enabled;
    int                 pos;
  
    /**
     * @brief Write samples into the audio source
     */
    virtual void writeSamples(void);

  
};  /* class ToneEncoder */


//} /* namespace */

#endif /* TONE_ENCODER_INCLUDED */



/*
 * This file has not been truncated
 */

