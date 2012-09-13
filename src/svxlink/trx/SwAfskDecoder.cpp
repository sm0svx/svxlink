/**
@file	SwAfskDecoder.cpp
@brief  This file contains a class that implements a sw Afsk decoder
@author Tobias Blomberg / SM0SVX & Thomas Sailer / HB9JNX &
        Christian Stussak (University of Halle) & Adi Bier / DL1HRC
@date	2012-07-20

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

#include "SwAfskDecoder.h"


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
 * the CRC routines are stolen from WAMPES/multimon
 * by Dieter Deyke
*/
static const unsigned short crc_ccitt_table[256] = {
        0x0000, 0x1189, 0x2312, 0x329b, 0x4624, 0x57ad, 0x6536, 0x74bf,
        0x8c48, 0x9dc1, 0xaf5a, 0xbed3, 0xca6c, 0xdbe5, 0xe97e, 0xf8f7,
        0x1081, 0x0108, 0x3393, 0x221a, 0x56a5, 0x472c, 0x75b7, 0x643e,
        0x9cc9, 0x8d40, 0xbfdb, 0xae52, 0xdaed, 0xcb64, 0xf9ff, 0xe876,
        0x2102, 0x308b, 0x0210, 0x1399, 0x6726, 0x76af, 0x4434, 0x55bd,
        0xad4a, 0xbcc3, 0x8e58, 0x9fd1, 0xeb6e, 0xfae7, 0xc87c, 0xd9f5,
        0x3183, 0x200a, 0x1291, 0x0318, 0x77a7, 0x662e, 0x54b5, 0x453c,
        0xbdcb, 0xac42, 0x9ed9, 0x8f50, 0xfbef, 0xea66, 0xd8fd, 0xc974,
        0x4204, 0x538d, 0x6116, 0x709f, 0x0420, 0x15a9, 0x2732, 0x36bb,
        0xce4c, 0xdfc5, 0xed5e, 0xfcd7, 0x8868, 0x99e1, 0xab7a, 0xbaf3,
        0x5285, 0x430c, 0x7197, 0x601e, 0x14a1, 0x0528, 0x37b3, 0x263a,
        0xdecd, 0xcf44, 0xfddf, 0xec56, 0x98e9, 0x8960, 0xbbfb, 0xaa72,
        0x6306, 0x728f, 0x4014, 0x519d, 0x2522, 0x34ab, 0x0630, 0x17b9,
        0xef4e, 0xfec7, 0xcc5c, 0xddd5, 0xa96a, 0xb8e3, 0x8a78, 0x9bf1,
        0x7387, 0x620e, 0x5095, 0x411c, 0x35a3, 0x242a, 0x16b1, 0x0738,
        0xffcf, 0xee46, 0xdcdd, 0xcd54, 0xb9eb, 0xa862, 0x9af9, 0x8b70,
        0x8408, 0x9581, 0xa71a, 0xb693, 0xc22c, 0xd3a5, 0xe13e, 0xf0b7,
        0x0840, 0x19c9, 0x2b52, 0x3adb, 0x4e64, 0x5fed, 0x6d76, 0x7cff,
        0x9489, 0x8500, 0xb79b, 0xa612, 0xd2ad, 0xc324, 0xf1bf, 0xe036,
        0x18c1, 0x0948, 0x3bd3, 0x2a5a, 0x5ee5, 0x4f6c, 0x7df7, 0x6c7e,
        0xa50a, 0xb483, 0x8618, 0x9791, 0xe32e, 0xf2a7, 0xc03c, 0xd1b5,
        0x2942, 0x38cb, 0x0a50, 0x1bd9, 0x6f66, 0x7eef, 0x4c74, 0x5dfd,
        0xb58b, 0xa402, 0x9699, 0x8710, 0xf3af, 0xe226, 0xd0bd, 0xc134,
        0x39c3, 0x284a, 0x1ad1, 0x0b58, 0x7fe7, 0x6e6e, 0x5cf5, 0x4d7c,
        0xc60c, 0xd785, 0xe51e, 0xf497, 0x8028, 0x91a1, 0xa33a, 0xb2b3,
        0x4a44, 0x5bcd, 0x6956, 0x78df, 0x0c60, 0x1de9, 0x2f72, 0x3efb,
        0xd68d, 0xc704, 0xf59f, 0xe416, 0x90a9, 0x8120, 0xb3bb, 0xa232,
        0x5ac5, 0x4b4c, 0x79d7, 0x685e, 0x1ce1, 0x0d68, 0x3ff3, 0x2e7a,
        0xe70e, 0xf687, 0xc41c, 0xd595, 0xa12a, 0xb0a3, 0x8238, 0x93b1,
        0x6b46, 0x7acf, 0x4854, 0x59dd, 0x2d62, 0x3ceb, 0x0e70, 0x1ff9,
        0xf78f, 0xe606, 0xd49d, 0xc514, 0xb1ab, 0xa022, 0x92b9, 0x8330,
        0x7bc7, 0x6a4e, 0x58d5, 0x495c, 0x3de3, 0x2c6a, 0x1ef1, 0x0f78
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

SwAfskDecoder::SwAfskDecoder(Config &cfg, const string &name)
  : AfskDecoder(cfg, name), fbuf_cnt(0), baudrate(1200), frequ_mark(1200),
    frequ_space(2200), corrlen(26), sphaseinc(9830.4), subsamp(2), debug(false)
{
    string value;

    if (cfg.getValue(name, "DIGI_DEBUG", value))
    {
       debug = true;
    }

    if (cfg.getValue(name, "AFSK_DEC_BAUDRATE", value))
    {
       baudrate = atoi(value.c_str());
       if (baudrate == 2400)
       {
          frequ_mark = 3970;
          frequ_space= 2165;
          subsamp = 1;
       }
    }
    else {
       cout << "*** WARNING: no param \"AFSK_DEC_BAUDRATE\" " <<
               "setting Afsk default baudrate 1200Bd" << endl;
    }

    corrlen      = (int)(2*INTERNAL_SAMPLE_RATE/baudrate);
    sphaseinc    = 0x10000u*baudrate*subsamp/(2*INTERNAL_SAMPLE_RATE);
    corr_mark_i  = new float[corrlen+1];
    corr_mark_q  = new float[corrlen+1];
    corr_space_i = new float[corrlen+1];
    corr_space_q = new float[corrlen+1];

} /* SwAfskDecoder::SwAfskDecoder */


bool SwAfskDecoder::initialize(void)
{
  if (!AfskDecoder::initialize())
  {
    cout << "*** ERROR: Starting SwAfskDecoder" << endl;
    return false;
  }

  float f;
  int i;

  state = (demod_state *) malloc(sizeof(demod_state));
  memset(&state->hdlc, 0, sizeof(state->hdlc));
  memset(&state->afsk, 0, sizeof(state->afsk));

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
  cout << "Starting " << baudrate << "Bd SwAfskDecoder" <<endl;
  return true;

} /* SwAfskDecoder::initialize */


// stl::queue() better???
int SwAfskDecoder::writeSamples(const float *samples, int count)
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

void SwAfskDecoder::demod(float *buffer, int length)
{
	float f;
	unsigned char curbit;

	if (state->afsk.subsamp) {

		int numfill = subsamp - state->afsk.subsamp;
		if (length < numfill) {
		  state->afsk.subsamp += length;
		  return;
		}
		buffer += numfill;
		length -= numfill;
		state->afsk.subsamp = 0;
	}

     // the non-coherent fsk detector
     // http://en.wikipedia.org/wiki/Frequency-shift_keying
    for (; length>0; length-=subsamp, buffer+=subsamp ) {
		f = fsqr(mac(buffer, corr_mark_i, corrlen)) +
			fsqr(mac(buffer, corr_mark_q, corrlen)) -
			fsqr(mac(buffer, corr_space_i, corrlen)) -
			fsqr(mac(buffer, corr_space_q, corrlen));
		state->afsk.dcd_shreg <<= 1;
		state->afsk.dcd_shreg |= (f > 0);

		/*
		 * check if transition
		 */
		if ((state->afsk.dcd_shreg ^ (state->afsk.dcd_shreg >> 1)) & 1) {
			if (state->afsk.sphase < (0x8000u-(sphaseinc/2)))
				state->afsk.sphase += sphaseinc/8;
			else
				state->afsk.sphase -= sphaseinc/8;
		}
		state->afsk.sphase += sphaseinc;
		if (state->afsk.sphase >= 0x10000u) {
			state->afsk.sphase &= 0xffffu;
			state->afsk.lasts <<= 1;
			state->afsk.lasts |= state->afsk.dcd_shreg & 1;
			curbit = (state->afsk.lasts ^
				  (state->afsk.lasts >> 1) ^ 1) & 1;
			hdlc_rxbit(state, curbit);
		}
	}
	state->afsk.subsamp = length;

} /* SwAfskDecoder::afsk_demod */



void SwAfskDecoder::hdlc_rxbit(struct demod_state *s, int bit)
{
	s->hdlc.rxbitstream <<= 1;
	s->hdlc.rxbitstream |= !!bit;

     // the 0x7e signals the beginning and the end of an ax25 frame
     // == HDLC_FLAG
	if ((s->hdlc.rxbitstream & 0xff) == 0x7e)
	{
	   if (s->hdlc.rxstate && (s->hdlc.rxptr - s->hdlc.rxbuf) > 2)
	   {
		 ax25_disp_packet(s->hdlc.rxbuf,s->hdlc.rxptr-s->hdlc.rxbuf);
	   }
       s->hdlc.rxstate = 1;
       s->hdlc.rxptr = s->hdlc.rxbuf;
       s->hdlc.rxbitbuf = 0x80;
       //afsk_mute(true);
       return;
	}

     // 0x7f == HDLC_RESET
	if ((s->hdlc.rxbitstream & 0x7f) == 0x7f)
	{
		s->hdlc.rxstate = 0;
		afsk_mute(false);
		return;
	}

	if (!s->hdlc.rxstate)
	{
	    afsk_mute(false);
	    return;
	}

	if ((s->hdlc.rxbitstream & 0x3f) == 0x3e) /* stuffed bit */
	{
	    return;
	}

	if (s->hdlc.rxbitstream & 1)
	{
	    s->hdlc.rxbitbuf |= 0x100;
	}

	if (s->hdlc.rxbitbuf & 1)
	{
	   if (s->hdlc.rxptr >= s->hdlc.rxbuf+sizeof(s->hdlc.rxbuf))
	   {
		  s->hdlc.rxstate = 0;
		  if (debug)
		  {
		    cout << "*** Error: SwAfskDecoder: packet size too large" << endl;
		  }
          return;
	   }
	   *s->hdlc.rxptr++ = s->hdlc.rxbitbuf >> 1;
       s->hdlc.rxbitbuf = 0x80;
       return;
	}
    s->hdlc.rxbitbuf >>= 1;

} /* SwAfskDecoder::hdlc_rxbit */



void SwAfskDecoder::ax25_disp_packet(unsigned char *bp, unsigned int len)
{
   string dest_call, src_call, path, payload;
   unsigned char i;

   if (!bp || len < 10)
   {
      return;
   }

   if (!check_crc_ccitt(bp, len))
   {
	 if (debug)
	 {
	   cout << "*** WARNING: SwAfskDecoder: wrong crc" << endl;
	 }
	 return;
   }

   len -= 2;
   if (!(bp[1]&1))
   {
       /*
        * normal header
       */
       if (len < 15)
       {
         return;
       }

	   for(i = 7; i < 13; i++)
  	   {
          if ((bp[i] &0xfe) != 0x40)
          {
              // decode the source call
              src_call += bp[i] >> 1;
          }
	   }

	   // gets the source call's ssid
	   if (((bp[13] >> 1) & 0xf) > 0)
	   {
              src_call += "-";
              src_call += 0x30 + ((bp[13] >> 1) & 0xf);
	   }

       for(i = 0; i < 6; i++)
       {
          if ((bp[i] &0xfe) != 0x40)
          {
             // decode the destination call
             dest_call += bp[i]>>1;
          }
       }

       // gets the destination call's ssid
       if (((bp[6] >> 1) & 0xf) > 0)
       {
         dest_call += "-";
         dest_call += 0x30 + ((bp[6] >> 1) & 0xf);
       }

       bp += 14;
       len -= 14;

       int path_cnt = 1;

        // create the path
       while ((!(bp[-1] & 1)) && (len >= 7))
       {
          for(i = 0; i < 6; i++)
          {
             if ((bp[i] &0xfe) != 0x40)
             {
                path += bp[i] >> 1;
             }
          }

          // check if the path contains "WIDE1-1,WIDEX-X" where X<=3
          // otherwise the frame will be ignored
          if ((((bp[6] >> 1) & 0xf) > path_cnt) || path_cnt > 3)
          {
             cout << "*** WARNING: ignoring wrong APRS path, should be " <<
                  "\"WIDE1-1\", \"WIDE1-1,WIDE2-2\" or \"WIDE1-1,WIDE3-3\"" <<
                  "\nRef: New n-N Paradigm, IARU conference 2008" << endl;
             return;
          }

          path += "-";
          path += 0x30 + ((bp[6] >> 1) & 0xf); // SSID
          path_cnt += 2; // will be 3 after 1st loop
          bp += 7;
          len -= 7;

          if ((!(bp[-1] & 1)) && (len >= 7))
          {
             path += ",";
          }
        }
         // qAR - Packet is placed on APRS-IS by an IGate from RF.
         // The callSSID following the qAR it the callSSID of the IGate.
        path += ",qAR";
      }
      else
      {
         // e.g. flexnet header
        return;
      }

      if (len<2)
      {
        return;
      }

      len -= 2;
      bp += 2;

      // handle the payload
      while (len)
      {
         i = *bp++;
         len--;
         if ((i > 0) && (i < 128))
         {
            payload += i;
         }
         else
         {
             // this shouldn't happen but
             // we make the most of it ;-)
            payload += ".";
         }
      }

      // provide the detected data to the aprs network
      string aprs_message = src_call + ">" + dest_call + "," + path;
      afskDetected(aprs_message, payload);
      if (debug)
      {
         cout << " Aprs message received: " << aprs_message << endl;
      }

} /* SwAfskDecoder::ax25_disp_packet */


int SwAfskDecoder::check_crc_ccitt(const unsigned char *bp, unsigned int cnt)
{
        unsigned int crc = 0xffff;

        for (; cnt > 0; cnt--)
        {
           crc = (crc >> 8) ^ crc_ccitt_table[(crc ^ *bp++) & 0xff];
        }
        return (crc & 0xffff) == 0xf0b8;

} /* SwAfskDecoder::check_crc_ccitt */


float SwAfskDecoder::mac(const float *a, const float *b, unsigned int size)
{
	float sum = 0;
	unsigned int i;

	for (i = 0; i < size; i++)
	{
       sum += (*a++) * (*b++);
	}
	return sum;

} /* SwAfskDecoder::mac */


float SwAfskDecoder::fsqr(float f)
{
  return f*f;
} /* SwAfskDecoder::fsqr */

/*
 * This file has not been truncated
 */
