/**
@file	 SwSel5Decoder.h
@brief   This file contains a class that implements a sw Sel5 decoder
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2010-03-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2010  Tobias Blomberg / SM0SVX

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


#ifndef SW_SEL5_DECODER_INCLUDED
#define SW_SEL5_DECODER_INCLUDED


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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Sel5Decoder.h"


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
 * @brief   This class implements a software SEL5 decoder
 * @author  Tobias Blomberg, SM0SVX & Adi / DL1HRC
 * @date    2010-02-27
 *
 * This class implements a software SEL5 decoder
 * implemented using Goertzel's algorithm.
 */
class SwSel5Decoder : public Sel5Decoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    SwSel5Decoder(Async::Config &cfg, const std::string &name);

    /**
     * @brief   Destructor
     */
    virtual ~SwSel5Decoder(void);

    /**
     * @brief 	Initialize the Fms decoder
     * @returns Returns \em true if the initialization was successful or
     *          else \em false.
     *
     * Call this function to initialize the Fms decoder. It must be called
     * before using it.
     */
    virtual bool initialize(void);

    /**
     * @brief 	Write samples into the Fms decoder
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    virtual int writeSamples(const float *samples, int count);

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
    GoertzelState row_out[36];

    /* tone-digit table*/
    char *sel5_table;

    /*! Row tone signal level values. */
    float row_energy[16];

    /*! Remaining sample count in the current detection interval. */
    int samples_left;
    /*! The result of the last tone analysis. */
    uint8_t last_hit;
    /*! This is the last stable tone digit. */
    uint8_t last_stable;
    /*! The detection timer advances when the input is stable. */
    int stable_timer;
    /*! The active timer is reset when a new non-zero digit is detected. */
    int active_timer;
    /*! the detected Sel5 sequence */
    std::string dec_digits;
    /*! the length of the tone definition */
    int arr_len;

    void Sel5Receive(void);
    void Sel5PostProcess(uint8_t hit);
    void goertzelInit(GoertzelState *s, float freq, float bw, float offset);
    float goertzelResult(GoertzelState *s);
    int findMaxIndex(const float f[]);

};  /* class SwSel5Decoder */


//} /* namespace */

#endif /* SW_SEL5_DECODER_INCLUDED */



/*
 * This file has not been truncated
 */

