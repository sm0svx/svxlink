/**
@file	SwFmsDecoder.cpp
@brief  This file contains a class that implements a sw Fms decoder
@author Tobias Blomberg / SM0SVX & Thomas Sailer / HB9JNX &
        Christian Stussak (University of Halle) & Markus Grohmann  &
        Stephan Effertz & Adi Bier / DL1HRC
@date	2012-08-10

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
GNU General Public License for more details->

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cstring>
#include <string>
#include <cmath>
#include <cstdlib>


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

#include "SwFmsDecoder.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/


/*
 * the CRC routines are stolen from the monitord
 * project by Stephan Effertz / Markus Grohmann
 * http://monitord.de
*/
const unsigned short crc_table[] = {
	48, 48, 48, 48, 48, 48, 48, 27,
	48, 48, 48, 18, 48,  5, 26, 48,
	48, 48, 48, 37, 48,  1, 17, 48,
	48,  8,  4, 48, 25, 48, 48, 48,
	48, 48, 48, 42, 48, 22, 36, 48,
	48, 30,  0, 48, 16, 48, 48, 13,
	48, 48,  7, 48,  3, 48, 48, 10,
	24, 48, 48, 32, 48, 48, 48, 48,
	48, 48, 48, 48, 48, 46, 41, 48,
	48, 40, 21, 48, 35, 48, 48, 45,
	48, 20, 29, 48, 48, 48, 48, 39,
	15, 48, 48, 44, 48, 34, 12, 48,
	48, 28, 48, 48,  6, 48, 48, 19,
	 2, 48, 48, 38, 48, 48,  9, 48,
	23, 48, 48, 43, 48, 14, 31, 48,
	48, 11, 48, 48, 48, 48, 48, 33,
};


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

SwFmsDecoder::SwFmsDecoder(Config &cfg, const string &name)
  : FmsDecoder(cfg, name), fbuf_cnt(0), baudrate(1200), frequ_mark(1200),
    frequ_space(1800), corrlen(26), sphaseinc(9830.4), subsamp(2),
    debug(false)
{

    string value;
    if (cfg.getValue(name, "DIGI_DEBUG", value))
    {
       debug = true;
    }

    corrlen      = (int)(2*INTERNAL_SAMPLE_RATE/baudrate);
    sphaseinc    = 0x10000u*baudrate*subsamp/(2*INTERNAL_SAMPLE_RATE);
    corr_mark_i  = new float[corrlen+1];
    corr_mark_q  = new float[corrlen+1];
    corr_space_i = new float[corrlen+1];
    corr_space_q = new float[corrlen+1];

} /* SwFmsDecoder::SwFmsDecoder */


bool SwFmsDecoder::initialize(void)
{
  if (!FmsDecoder::initialize())
  {
    cout << "*** ERROR: Starting SwFmsDecoder" << endl;
    return false;
  }

  float f;
  int i;

  state = (demod_state *) malloc(sizeof(demod_state));
  memset(&state->hdlc, 0, sizeof(state->hdlc));
  memset(&state->fms, 0, sizeof(state->fms));
  state->hdlc.rxstate = -1;

  for (f = 0, i = 0; i < corrlen; i++)
  {
	 corr_mark_i[i] = cos(f);
	 corr_mark_q[i] = sin(f);
	 f += 1.0*M_PI*frequ_mark/INTERNAL_SAMPLE_RATE;
  }
  for (f = 0, i = 0; i < corrlen; i++)
  {
	 corr_space_i[i] = cos(f);
	 corr_space_q[i] = sin(f);
	 f += 1.0*M_PI*frequ_space/INTERNAL_SAMPLE_RATE;
  }

  // the Hamming window
  for (f = 0, i = 0; i < corrlen; i++)
  {
     f = 0.54 - 0.46*cos(M_PI*i/(float)(corrlen-1));
     corr_mark_i[i] *= f;
     corr_mark_q[i] *= f;
     corr_space_i[i] *= f;
     corr_space_q[i] *= f;
  }

  cout << "Starting " << baudrate << "Bd SwFmsDecoder" <<endl;
  return true;

} /* SwFmsDecoder::initialize */


// stl::queue() better???
int SwFmsDecoder::writeSamples(const float *samples, int count)
{
    int i=0;

    while (i++ < count)
    {
       if (fbuf_cnt>0)
       {
          // linear interpolation between two samples to
          // increase the number of samples -> better decoding
          fbuf[fbuf_cnt] = (fbuf[fbuf_cnt-1] + *samples)/2;
          fbuf_cnt++;
       }
       fbuf[fbuf_cnt++] = *samples++;
    }

    if (fbuf_cnt > corrlen)
    {
       demod(fbuf, fbuf_cnt-corrlen);
       memmove(fbuf, fbuf+fbuf_cnt-corrlen, corrlen*sizeof(fbuf[0]));
       fbuf_cnt = corrlen;
    }
    return count;
} /* SwDtmfDecoder::writeSamples */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void SwFmsDecoder::demod(float *buffer, int length)
{
	float f;
	unsigned char curbit;

	if (state->fms.subsamp)
	{
		int numfill = subsamp - state->fms.subsamp;

		if (length < numfill) {
		  state->fms.subsamp += length;
		  return;
		}
		buffer += numfill;
		length -= numfill;
		state->fms.subsamp = 0;
	}

     // the non-coherent fsk detector
     // http://en.wikipedia.org/wiki/Frequency-shift_keying
    for (; length>0; length-=subsamp, buffer+=subsamp )
    {
		f = fsqr(mac(buffer, corr_mark_i, corrlen)) +
			fsqr(mac(buffer, corr_mark_q, corrlen)) -
			fsqr(mac(buffer, corr_space_i, corrlen)) -
			fsqr(mac(buffer, corr_space_q, corrlen));
		state->fms.dcd_shreg <<= 1;
		state->fms.dcd_shreg |= (f > 0);

		/*
		 * check if transition
		 */
		if ((state->fms.dcd_shreg ^ (state->fms.dcd_shreg >> 1)) & 1)
		{
		   if (state->fms.sphase < (0x8000u-(sphaseinc/2)))
			   state->fms.sphase += sphaseinc/8;
		   else
			   state->fms.sphase -= sphaseinc/8;
		}

		state->fms.sphase += sphaseinc;

		if (state->fms.sphase >= 0x10000u)
		{
			state->fms.sphase &= 0xffffu;
			curbit = (state->fms.dcd_shreg & 1);
			rxbit(state, curbit);
		}
	}
	state->fms.subsamp = length;

} /* SwFmsDecoder::demod */


void SwFmsDecoder::rxbit(struct demod_state *s, int bit)
{
	bool crc = true;
	s->hdlc.rxbitstream <<= 1;
	s->hdlc.rxbitstream |= !!bit;

     // the 0xfff1a signals the beginning of the fms frame
	if ((s->hdlc.rxbitstream & 0xfffff) == 0xfff1a && s->hdlc.rxstate == -1)
	{
	    // set rx to "on"
       s->hdlc.rxstate = 0;
       return;
	}

     // if s->hdlc.rxstate > -1 -> RX mode
    if (s->hdlc.rxstate > -1)
    {
       s->hdlc.rxptr[s->hdlc.rxstate] = !!bit;

       if (s->hdlc.rxstate++ > 47)
       {
            // an error occurred
          if (crc_check(s,0))
          {
             crc = false;
             int i=0;

             if (debug)
             {
                cout << "*** Fms decoder CRC error, trying to fix it\n";
             }

             do {
                 // toggle just one bit, check if crc is ok
                s->hdlc.rxptr[i] = !s->hdlc.rxptr[i];

                if (!crc_check(s,0))
                {
                    // caught it ;-)
                   if (debug)
                   {
                      cout << "*** Fms decoder CRC ok now\n";
                   }
                   crc = true;
                   break;
                }
                  // re-toggle one bit, another one is erroneous
                s->hdlc.rxptr[i] = !s->hdlc.rxptr[i];
                i++;
             } while (i < 47);
          }

           // crc os ok, gib out received data
          if (crc)
          {
            fms.bos[0]  = s->hdlc.rxptr[3] << 3 | s->hdlc.rxptr[2] << 2 |
                          s->hdlc.rxptr[1] << 1 | s->hdlc.rxptr[0];
		    fms.land[0] = s->hdlc.rxptr[7] << 3 | s->hdlc.rxptr[6] << 2 |
		                  s->hdlc.rxptr[5] << 1 | s->hdlc.rxptr[4];
		    fms.ort[0]  = s->hdlc.rxptr[11] << 3 | s->hdlc.rxptr[10] << 2 |
		                  s->hdlc.rxptr[9] << 1 | s->hdlc.rxptr[8];
		    fms.ort[1]  = s->hdlc.rxptr[15] << 3 | s->hdlc.rxptr[14] << 2 |
		                  s->hdlc.rxptr[13] << 1 | s->hdlc.rxptr[12];
		    fms.kfz[0]  = s->hdlc.rxptr[19] << 3 | s->hdlc.rxptr[18] << 2 |
		                  s->hdlc.rxptr[17] << 1 | s->hdlc.rxptr[16];
		    fms.kfz[1]  = s->hdlc.rxptr[23] << 3 | s->hdlc.rxptr[22] << 2 |
		                  s->hdlc.rxptr[21] << 1 | s->hdlc.rxptr[20];
		    fms.kfz[2]  = s->hdlc.rxptr[27] << 3 | s->hdlc.rxptr[26] << 2 |
		                  s->hdlc.rxptr[25] << 1 | s->hdlc.rxptr[24];
		    fms.kfz[3]  = s->hdlc.rxptr[31] << 3 | s->hdlc.rxptr[30] << 2 |
                          s->hdlc.rxptr[29] << 1 | s->hdlc.rxptr[28];
		    fms.stat[0] = s->hdlc.rxptr[35] << 3 | s->hdlc.rxptr[34] << 2 |
		                  s->hdlc.rxptr[33] << 1 | s->hdlc.rxptr[32];
		    fms.bst[0]  = s->hdlc.rxptr[36];
		    fms.dir[0]  = s->hdlc.rxptr[37];
		    fms.tki[0]  = s->hdlc.rxptr[38] << 1 | s->hdlc.rxptr[39];

		     // CRC check?
		    for (int i = 0; i < 7; i++)
		    {
		       fms.crc[i] = s->hdlc.rxptr[40 + i];
		    }

            stringstream ss;
            ss << fms.bos[0] << " " << fms.land[0] << " " << fms.ort[0]
               << fms.ort[1] << " " << fms.kfz[0] << fms.kfz[1]
               << fms.kfz[2] << fms.kfz[3] << " " << fms.stat[0] << " "
               << fms.bst[0] << " " << fms.dir[0] << " " << fms.tki[0];

            fmsDetected(ss.str());
            if (debug)
            {
               cout << ss.str() << endl;
            }
          }
          else
          {
             if (debug)
             {
               cout << "*** Fms decoder CRC error" << endl;
             }
          }
           // set rx to "off"
          s->hdlc.rxstate = -1;
          return;
       }
    }
} /* SwFmsDecoder::rxbit */


int SwFmsDecoder::crc_check(struct demod_state *s, int offset)
{

	#define	CODE	47

	unsigned char	g[] = {1,0,0,0,1,0,1};
	unsigned char	r[] = {0,0,0,0,0,0,0,0};
	char *p;
	unsigned int	i = 0, j, bit;

	//	CRC-check begins
	for (i = 0; i < CODE; i++)
	{
		bit = s->hdlc.rxptr[i + offset] ^ r[0];
		p = (char*) r;
		for (j = 0; j < 7; j++) *p++ = (bit & g[j]) ^ r[j + 1];
	}

	//	check the rest-polynome
	bit = 0;
	for (i = 0; i < 7; i++)
	{
		bit <<= 1;
		bit |= r[i];
	}

	//	ok, if no rest (*bit=0)
	if (bit)
	{
		bit = crc_table[bit];
		if (bit < 48) s->hdlc.rxptr[(bit)++] ^= 1;
			else return bit;
	}

	return 0;
} /* SwFmsDecoder::crc_check */


float SwFmsDecoder::mac(const float *a, const float *b, unsigned int size)
{
	float sum = 0;
	unsigned int i;

	for (i = 0; i < size; i++)
	{
       sum += (*a++) * (*b++);
	}
	return sum;

} /* SwFmsDecoder::mac */


float SwFmsDecoder::fsqr(float f)
{
  return f*f;
} /* SwFmsDecoder::fsqr */

/*
 * This file has not been truncated
 */
