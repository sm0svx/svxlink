/**
@file	 NewSwDtmfDecoder.h
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


#ifndef NEW_SW_DTMF_DECODER_INCLUDED
#define NEW_SW_DTMF_DECODER_INCLUDED


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
#include "Goertzel.h"


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
 * @date    2015-02-22
 *
 * This class implements a software DTMF decoder
 * implemented using Goertzel's algorithm.
 */   
class NewSwDtmfDecoder : public DtmfDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    NewSwDtmfDecoder(Async::Config &cfg, const std::string &name);

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
    int handleSamples(const float *samples, int count);
    
    /**
     * @brief 	Return the active digit
     * @return	Return the active digit if any or a '?' if none.
     */
    char activeDigit(void) const
    {
      return last_digit_detected ? last_digit_detected : '?';
    }

  private:
    struct DtmfGoertzel : public Goertzel
    {
      float m_freq;
      float m_max_fqdiff;

      void initialize(float freq);
    };

    static const float DEFAULT_MAX_NORMAL_TWIST_DB = 8.0f;
    static const float DEFAULT_MAX_REV_TWIST_DB = 8.0f;
    static const size_t BLOCK_SIZE = INTERNAL_SAMPLE_RATE / 100; // 10ms
    static const float ENERGY_THRESH = 1e-3 * BLOCK_SIZE; // Min passband energy
    static const float REL_THRESH = 0.8; // Passband/tones relation threshold
    static const float MAX_FQ_ERROR = 0.025; // Max 2.5% frequency error

    /*! Maximum acceptable "normal" (lower bigger than higher) twist ratio */
    float twist_nrm_thresh;
    /*! Maximum acceptable "reverse" (higher bigger than lower) twist ratio */
    float twist_rev_thresh;

    std::vector<DtmfGoertzel> row;
    std::vector<DtmfGoertzel> col;
    float block[BLOCK_SIZE];
    size_t block_size;
    size_t block_pos;
    size_t det_cnt;
    size_t undet_cnt;
    char last_digit_detected;

    void processBlock(void);
    float phaseDiffToFq(float phase, float prev_phase);

};  /* class NewSwDtmfDecoder */


//} /* namespace */

#endif /* NEW_SW_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

