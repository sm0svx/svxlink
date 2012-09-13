/**
@file	 SwFmsDecoder.h
@brief   This file contains a class that implements a sw Fms decoder
@author  Tobias Blomberg / SM0SVX & Thomas Sailer / HB9JNX &
         Christian Stussak (University of Halle) & Markus Grohmann  &
         Stephan Effertz & Adi Bier / DL1HRC
@date	 2012-08-10

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


#ifndef SW_FMS_DECODER_INCLUDED
#define SW_FMS_DECODER_INCLUDED


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

#include "FmsDecoder.h"


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
 * @brief   This class implements a software Fms decoder
 * @author  Tobias Blomberg, SM0SVX & Adi / DL1HRC
 * @date    2012-07-10
 *
 * This class implements a software Fms decoder
 */
class SwFmsDecoder : public FmsDecoder
{
  public:
    /**
     * @brief 	Constructor
     * @param 	cfg A previously initialised configuration object
     * @param 	name The name of the receiver configuration section
     */
    SwFmsDecoder(Async::Config &cfg, const std::string &name);

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

    struct demod_state {
      const struct demod_param *dem_par;
      struct l2_state_hdlc {
              int rxstate;
              unsigned int rxptr[256];
              unsigned int rxbitstream;
          } hdlc;
      struct l1_state_fms {
            unsigned int dcd_shreg;
            unsigned int sphase;
            unsigned int lasts;
            unsigned int subsamp;
          } fms;
    };

    // Fms is 48bit long, see
    // http://de.wikipedia.org/wiki/Funkmeldesystem
    // for further information

    struct fms_struct {
    		unsigned short
			bos[1],   // 0-3 	BOS-Dienstekennung (4 Bit)
			land[1],  // 4-7 	Länderkennung (4 Bit)
			ort[2],   // 8-15 	Ortskennung (8 Bit)
			kfz[4],   // 16-31 	Fahrzeugkennung (16 Bit)
			stat[1],  // 32-35 	Status (4 Bit)
			bst[1],   // 36 	Baustufenkennung (1 Bit)
			dir[1],   // 37 	Richtungskennung (1 Bit)
			tki[1],   // 38-39 	taktische Kurzinformation (2 Bit)
			crc[7];   // 40-46 	Redundanz
			short txtnr ;
    };
    // 47 	Schlussbit

    fms_struct fms;
    std::string message;

    float lp1_c[3],lp2_c[3],bp0_c[3],bp1_c[3];
	float lp1_b[4],lp2_b[4],bp0_b[4],bp1_b[4];

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
    bool debug;

    void demod(float *buffer, int length);
    int crc_check(struct demod_state *s, int offset);
    float mac(const float *a, const float *b, unsigned int size);
    float fsqr(float f);
    void rxbit(struct demod_state *s, int bit);

};  /* class SwFmsDecoder */

//} /* namespace */

#endif /* SW_FMS_DECODER_INCLUDED */

/*
 * This file has not been truncated
 */

