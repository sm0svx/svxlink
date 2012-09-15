/**
@file	SwMdcDecoder.cpp
@brief  This file contains a class that implements a sw Mdc decoder
@author Tobias Blomberg / SM0SVX & Thomas Sailer / HB9JNX &
        Matthew Kaufman (matthew@eeph.com) &
        Christian Stussak (University of Halle) & Adi Bier / DL1HRC
@date	2012-09-10

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

#include "SwMdcDecoder.h"


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

/* further information can be found here:
*  http://batboard.batlabs.com/viewtopic.php?p=176498&highlight=#176498
*  http://servv89pn0aj.sn.sourcedns.com/~gbpprorg/nfl/MDT_Stuff.txt
*  http://batboard.batlabs.com/viewtopic.php?f=1&t=36459&start=0
*  http://batlabs.com/mdc1200.html
*/

SwMdcDecoder::SwMdcDecoder(Config &cfg, const string &name)
  : MdcDecoder(cfg, name), fbuf_cnt(0), baudrate(1200), frequ_mark(1200),
    frequ_space(1800), subsamp(2), debug(false)
{
    string value;

    if (cfg.getValue(name, "DIGI_DEBUG", value))
    {
       debug = true;
    }

    cfg.getValue(name, "MDC_DEC_BAUDRATE", value);
    baudrate = atoi(value.c_str());

    if (baudrate != 600 && baudrate !=1200)
    {
        cout << "*** WARNING: wrong/no param \"MDC_DEC_BAUDRATE\" " <<
                "setting Mdc default baudrate to 1200Bd" << endl;
        baudrate = 1200;
    }

    corrlen      = (int)(2*INTERNAL_SAMPLE_RATE/baudrate);
    sphaseinc    = 0x10000u*baudrate*subsamp/(2*INTERNAL_SAMPLE_RATE);
    corr_mark_i  = new float[corrlen+1];
    corr_mark_q  = new float[corrlen+1];
    corr_space_i = new float[corrlen+1];
    corr_space_q = new float[corrlen+1];

    cout << "baudrate  " << baudrate << "    Corrlen "
         << corrlen << "  sphaseinc " << sphaseinc << endl;

} /* SwMdcDecoder::SwMdcDecoder */


bool SwMdcDecoder::initialize(void)
{

  if (!MdcDecoder::initialize())
  {
    cout << "*** ERROR: Starting SwMdcDecoder" << endl;
    return false;
  }

  float f;
  int i;

  state = (demod_state *) malloc(sizeof(demod_state));
  memset(&state->hdlc, 0, sizeof(state->hdlc));
  memset(&state->mdc, 0, sizeof(state->mdc));
  state->hdlc.rxstate = -1;
  state->mdc.last = 0;

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
     f = 0.54 - 0.4*cos(M_PI*i/(float)(corrlen-1));
     corr_mark_i[i] *= f;
     corr_mark_q[i] *= f;
     corr_space_i[i] *= f;
     corr_space_q[i] *= f;
  }
  cout << "Starting " << baudrate << "Bd SwMdcDecoder" <<endl;
  return true;
} /* SwMdcDecoder::initialize */


// is stl::queue() better???
int SwMdcDecoder::writeSamples(const float *samples, int count)
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

void SwMdcDecoder::demod(float *buffer, int length)
{

    float f;
	unsigned char curbit;

	if (state->mdc.subsamp) {

		int numfill = subsamp - state->mdc.subsamp;
		if (length < numfill) {
		  state->mdc.subsamp += length;
		  return;
		}
		buffer += numfill;
		length -= numfill;
		state->mdc.subsamp = 0;
	}

     // the non-coherent fsk detector
     // http://en.wikipedia.org/wiki/Frequency-shift_keying
    for (; length>0; length-=subsamp, buffer+=subsamp ) {
		f = fsqr(mac(buffer, corr_mark_i, corrlen)) +
			fsqr(mac(buffer, corr_mark_q, corrlen)) -
			fsqr(mac(buffer, corr_space_i, corrlen)) -
			fsqr(mac(buffer, corr_space_q, corrlen));
		state->mdc.dcd_shreg <<= 1;
		state->mdc.dcd_shreg |= (f < 0);

		/*
		 * check if transition
		 */
		if ((state->mdc.dcd_shreg ^ (state->mdc.dcd_shreg >> 1)) & 1) {
			if (state->mdc.sphase < (0x8000u-(sphaseinc/2)))
				state->mdc.sphase += sphaseinc/8;
			else
				state->mdc.sphase -= sphaseinc/8;
		}
		state->mdc.sphase += sphaseinc;
		if (state->mdc.sphase >= 0x10000u)
		{
           state->mdc.sphase &= 0xffffu;

           // the incoming bitstream has to be XORed first
           // that means: y[n] = x[n] ^ x[n-1]
	       curbit = ((state->mdc.lasts>>1) &0x1) ^ (state->mdc.dcd_shreg &0x1);

            // then conduct the following procedure:
            // if curbit == 1 -> change last bitstate, do not change the last bit
           if (curbit & 0x1)
           {
              state->mdc.last = !(state->mdc.last & 0x1);
           }
	       hdlc_rxbit(state, state->mdc.last);
	       state->mdc.lasts = state->mdc.dcd_shreg & 0x1;
		}
	}
	state->mdc.subsamp = length;

} /* SwMdcDecoder::demod */


void SwMdcDecoder::hdlc_rxbit(struct demod_state *s, int bit)
{

    short a=0;

  	s->hdlc.rxbitstream <<= 1;
	s->hdlc.rxbitstream |= !!bit;

     // scan for the Mdc sync frame
     // original start frame: 0x07092a446f
    unsigned int c = s->hdlc.rxbitstream ^ 0x092a446f;

	if (c == 0xffffffff || c == 0x00000000)
    {
       // the following bitstream contains the information
       // starts the receiver
	  s->hdlc.rxstate = 0;

       // reset the rx pointer
      *s->hdlc.rxptr = s->hdlc.rxptr[0];

       // invert all incoming bits if the incoming bitstream has
       // an inverted sequence
	  if (c == 0xffffffff)
	  {
	    s->hdlc.inv = true;
	  }
	  return;
	}

    /* Receive the 112 bit data block and undo the bit interleave. This
     * means that one must reorder the bits in the following sequence:
     * {0,7,14,21,28,35,....,1,8,15,22,29,36,...,2,9,16,23,30,37,...}
     * if the orignal sequence were received as {0,1,2,3,4,5,6,7,8,...}
     * incoming bit stream    new bitstream
     *            0 ------------> 0
     *            1 ------------> 7
     *            2 ------------> 14
     *            3 ------------> 21
     *            4 ------------> 28
     *            5 ------------> 35
     *            6 ------------> 42
     *             ...             */

    if (s->hdlc.rxstate >= 0)
    {
       if (s->hdlc.inv)
       {
          bit = !(bit & 0x1);
       }
       a = (s->hdlc.rxstate % 16) * 7;

       if (a == 0)
       {
          s->hdlc.b = (int)s->hdlc.rxstate / 16;
       }
       s->hdlc.rxptr[a + s->hdlc.b] = bit & 0x1;
       s->hdlc.rxstate++;
    }

     // last bit received?
    if (s->hdlc.rxstate > 111)
    {
        /* the first 16 bits containing state information
         * Fun Arg
         * 01 (80) PTT ID
         * 01 (00) POST ID
         * 03 (00) RADIO CHK ACK
         * 00 (20) EMERG ALRAM ACK
         * 00 (81) Emergency XTS3000
         * 11 (8A) REMOTE MONITOR
         * 22 (06) STATUS REQ
         * 23 (00) STATUS ACK
         * 2B (0C) RADIO ENABLE
         * 2B (00) RADIO DISABLE
         * 35 (89) CALL ALERT
         * 46 (XX) STS XX
         * 47 (XX) MSG XX
         * 63 (85) RADIO CHECK
         */
       int data[15];

       for (a=0; a<14; a++)
       {
          data[a] = 0;
          for (short cnt=0; cnt<8; cnt++)
          {
             data[a] |= ((s->hdlc.rxptr[a*8 + cnt] & 0x1) << cnt);
          }
       }

        // do crc check
       int rcrc = data[4] | (data[5] << 8);
       int ccrc = docrc(data, 4);

       if (rcrc != ccrc)
       {
         if (debug)
         {
           cout << "*** Mdc detector CRC error" << endl;
         }
       }
       else
       {
          // extract the interesting data
         s->hdlc.func = data[0];  // command op-code
         s->hdlc.arg  = data[1];  // the argument
         s->hdlc.unitID= data[2] << 8 | data[3]; // unit ID

         char message[20];
         sprintf(message,"%02x %02x %04x",
                    s->hdlc.func,s->hdlc.arg,s->hdlc.unitID);

         string msg = message;
         mdcDetected(msg);
         if (debug)
         {
           cout << "Mdc message received: " << msg << endl;
         }
       }

        // stops the receiver, clear all params and buffers
       s->hdlc.rxstate = -1;
       s->hdlc.func = 0;
       s->hdlc.arg = 0;
       s->hdlc.unitID = 0;
       s->hdlc.b = 0;
       s->hdlc.inv = false;
    }
} /* SwMdcDecoder::hdlc_rxbit */


float SwMdcDecoder::mac(const float *a, const float *b, unsigned int size)
{
	float sum = 0;
	unsigned int i;

	for (i = 0; i < size; i++)
	{
       sum += (*a++) * (*b++);
	}
	return sum;
} /* SwMdcDecoder::mac */


float SwMdcDecoder::fsqr(float f)
{
  return f*f;
} /* SwMdcDecoder::fsqr */


unsigned short SwMdcDecoder::docrc(int *p, int len)
{
	int i, j;
	unsigned short c;
	int bit;
	unsigned short crc = 0x0000;

	for (i=0; i<len; i++)
	{
		c = (short)*p++;

		c = flip(c, 8);

		for (j=0x80; j; j>>=1)
		{
			bit = crc & 0x8000;
			crc<<= 1;
			if (c & j)
				bit^= 0x8000;
			if (bit)
				crc^= 0x1021;
		}
	}

	crc = flip(crc, 16);
	crc ^= 0xffff;
	crc &= 0xFFFF;

	return(crc);
} /* SwMdcDecoder::docrc */


unsigned short SwMdcDecoder::flip(unsigned short crc, int bitnum)
{
	unsigned short crcout, i, j;

	j = 1;
	crcout = 0;

	for (i=1<<(bitnum-1); i; i>>=1)
	{
		if (crc & i)
			 crcout |= j;
		j<<= 1;
	}
	return (crcout);
} /* SwMdcDecoder::flip */


/*
 * This file has not been truncated
 */
