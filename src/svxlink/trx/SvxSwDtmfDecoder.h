/**
@file	 SvxSwDtmfDecoder.h
@brief   This file contains a class that implements a sw DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2015-02-22

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


#ifndef SVX_SW_DTMF_DECODER_INCLUDED
#define SVX_SW_DTMF_DECODER_INCLUDED


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

#include <CppStdCompat.h>


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
 * This class implements a software DTMF decoder implemented using Goertzel's
 * algorithm.
 */   
class SvxSwDtmfDecoder : public DtmfDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    SvxSwDtmfDecoder(Async::Config &cfg, const std::string &name);

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
    virtual char activeDigit(void) const
    {
      return (det_state == STATE_DETECTED) ? last_digit_active : '?';
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
    virtual int detectionTime(void) const { return 40; }

  private:
    struct DtmfGoertzel : public Goertzel
    {
      float m_freq;
      float m_max_fqdiff;

      void initialize(float freq);
    };
    typedef enum
    {
      STATE_IDLE, STATE_DET_DELAY, STATE_DETECTED
    } DetState;

    static CONSTEXPR float DEFAULT_MAX_NORMAL_TWIST_DB = 8.5f;
    static CONSTEXPR float DEFAULT_MAX_REV_TWIST_DB = 6.0f;
    static CONSTEXPR size_t DET_CNT_HI_WEIGHT = 12;
    static CONSTEXPR size_t DET_CNT_MED_WEIGHT = 4;
    static CONSTEXPR size_t DET_CNT_LO_WEIGHT = 1;
    static CONSTEXPR size_t DEFAULT_MIN_DET_CNT = 2*DET_CNT_HI_WEIGHT;
    static CONSTEXPR size_t DEFAULT_MIN_UNDET_CNT = 3;
    static CONSTEXPR size_t BLOCK_SIZE = 20*INTERNAL_SAMPLE_RATE/1000; // 20ms
    static CONSTEXPR size_t STEP_SIZE = 10*INTERNAL_SAMPLE_RATE/1000; // 10ms
    static CONSTEXPR float ENERGY_THRESH = 1e-6*BLOCK_SIZE; // Min pb energy
    static CONSTEXPR float REL_THRESH_LO = 0.5; // Tone/pb pwr low thresh
    static CONSTEXPR float REL_THRESH_MED = 0.73; // Tone/pb pwr medium thresh
    static CONSTEXPR float REL_THRESH_HI = 0.9; // Tone/pb pwr high thresh
    //static CONSTEXPR float MAX_FQ_ERROR = 0.025; // Max 2.5% frequency error
    static CONSTEXPR float WIN_ENB = 1.37f; // FFT window equivalent noise bw
    static CONSTEXPR float MAX_OT_REL = 0.2f; // Overtone at least ~7dB below
    static CONSTEXPR float MAX_SEC_REL = 0.13f; // Second strongest > ~9dB below
    static CONSTEXPR float MAX_IM_REL = 0.1f; // Intermod prod > 10dB below

    float twist_nrm_thresh;
    float twist_rev_thresh;
    std::vector<DtmfGoertzel> row;
    std::vector<DtmfGoertzel> col;
    float block[BLOCK_SIZE];
    size_t block_size;
    size_t block_pos;
    size_t det_cnt;
    size_t undet_cnt;
    char last_digit_active;
    size_t min_det_cnt;
    size_t min_undet_cnt;
    DetState det_state;
    size_t det_cnt_weight;
    int duration;
    float win[BLOCK_SIZE];
    size_t undet_thresh;
    bool debug;
    float win_pwr_comp;


    void processBlock(void);

};  /* class SvxSwDtmfDecoder */


//} /* namespace */

#endif /* SVX_SW_DTMF_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

