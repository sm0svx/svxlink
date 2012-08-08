/**
@file	 SwAfskDecoder.h
@brief   This file contains a class that implements a sw Afsk decoder
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2012-07-20

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2012  Tobias Blomberg / SM0SVX

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


#ifndef SW_AFSK_DECODER_INCLUDED
#define SW_AFSK_DECODER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

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

#include "AfskDecoder.h"


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
 * @brief   This class implements a software Afsk decoder
 * @author  Tobias Blomberg, SM0SVX & Adi / DL1HRC
 * @date    2012-07-10
 *
 * This class implements a software Afsk decoder
 */
class SwAfskDecoder : public AfskDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    SwAfskDecoder(Async::Config &cfg, const std::string &name);

    /**
     * @brief 	Initialize the Afsk decoder
     * @returns Returns \em true if the initialization was successful or
     *          else \em false.
     *
     * Call this function to initialize the Afsk decoder. It must be called
     * before using it.
     */
    virtual bool initialize(void);

    /**
     * @brief 	Write samples into the Afsk decoder
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     */
    virtual int writeSamples(const float *samples, int count);

  private:

    struct demod_state {
      const struct demod_param *dem_par;
      struct l2_state_hdlc {
              unsigned char rxbuf[1024];
              unsigned char *rxptr;
              unsigned int rxstate;
              unsigned int rxbitstream;
              unsigned int rxbitbuf;
          } hdlc;
      struct l1_state_afsk {
            unsigned int dcd_shreg;
            unsigned int sphase;
            unsigned int lasts;
            unsigned int subsamp;
          } afsk;
    };

    float *corr_mark_i;
    float *corr_mark_q;
    float *corr_space_i;
    float *corr_space_q;

    struct demod_state *state;
    int fbuf_cnt;
    int baudrate;
    int frequ_mark;
    int frequ_space;
    int corrlen;
    float sphaseinc;
    int subsamp;
    float fbuf[1060];

    void demod(float *buffer, int length);
    int check_crc_ccitt(const unsigned char *bp, unsigned int cnt);
    float mac(const float *a, const float *b, unsigned int size);
    float fsqr(float f);
    void hdlc_rxbit(struct demod_state *s, int bit);
    void ax25_disp_packet(unsigned char *bp, unsigned int len);

};  /* class SwAfskDecoder */

//} /* namespace */

#endif /* SW_AFSK_DECODER_INCLUDED */

/*
 * This file has not been truncated
 */

