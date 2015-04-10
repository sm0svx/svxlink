/**
@file	 Dh1dmSwDtmfDecoder.h
@brief   This file contains a class that implements a sw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2003-04-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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


#ifndef DH1DM_SW_DTMF_DECODER_INCLUDED
#define DH1DM_SW_DTMF_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <stdint.h>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

//#include <AsyncAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DtmfDecoder.h"


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
 * @brief   This class implements a software DTMF decoder
 * @author  Tobias Blomberg, SM0SVX
 * @date    2007-05-01
 *
 * This class implements a software DTMF decoder
 * implemented using Goertzel's algorithm.
 */   
class Dh1dmSwDtmfDecoder : public DtmfDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    Dh1dmSwDtmfDecoder(Async::Config &cfg, const std::string &name);

    /**
     * @brief 	Initialize the DTMF decoder
     * @returns Returns \em true if the initialization was successful or
     *          else \em false.
     *
     * Call this function to initialize the DTMF decoder. It must be called
     * before using it.
     */
    virtual bool initialize(void);
    
    /**
     * @brief 	Write samples into the DTMF decoder
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    virtual int writeSamples(const float *samples, int count);

    /**
     * @brief 	Tell the DTMF decoder to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     */
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }
    
    /**
     * @brief 	Return the active digit
     * @return	Return the active digit if any or a '?' if none.
     */
    char activeDigit(void) const
    {
      return last_stable ? last_stable : '?';
    }

    /**
     * @brief   The detection time for this detector
     * @returns Returns the detection time in milliseconds
     *
     * This function will return the time in milliseconds that it will take
     * for the detector to detect a DTMF digit. That is, the time from the
     * moment when the tone is activated until the digitActivated signal is
     * emitted.
     * The time can for example be used in a DTMF muting function.
     */
    virtual int detectionTime(void) const { return 75; }

  private:

    // Tone detection descriptor
    typedef struct
    {
      float v2;
      float v3;
      float fac;
      float scale_factor;
      std::vector<float> window_table;
      std::vector<float>::const_iterator win;
      int samples_left;
      int block_length;
    } GoertzelState;

    /*! Tone detector working states for the row tones. */
    GoertzelState row_out[8];
    /*! Tone detector working states for the column tones. */
    GoertzelState col_out[8];
    /*! Row tone signal level values. */
    float row_energy[4];
    /*! Column tone signal level values. */
    float col_energy[4];
    /*! Remaining sample count in the current detection interval. */
    int samples_left;
    /*! The result of the last tone analysis. */
    uint8_t last_hit;
    /*! This is the last stable DTMF digit. */
    uint8_t last_stable;
    /*! The detection timer advances when the input is stable. */
    unsigned stable_timer;
    /*! The active timer is reset when a new non-zero digit is detected. */
    int active_timer;
    /*! Maximum acceptable "normal" (lower bigger than higher) twist ratio */
    float normal_twist;
    /*! Maximum acceptable "reverse" (higher bigger than lower) twist ratio */
    float reverse_twist;

    void dtmfReceive(void);
    void dtmfPostProcess(uint8_t hit);
    void goertzelInit(GoertzelState *s, float freq, float offset);
    float goertzelResult(GoertzelState *s);
    int findMaxIndex(const float f[]);

};  /* class Dh1dmSwDtmfDecoder */


//} /* namespace */

#endif /* DH1DM_SW_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

