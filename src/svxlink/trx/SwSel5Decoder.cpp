/**
@file	SwSel5Decoder.cpp
@brief  This file contains a class that implements a sw Sel5 decoder
@author Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC &
        Christian Stussak (University of Halle)
@date	2010-04-10

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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <algorithm>
#include <cstring>
#include <cmath>
#include <cstdlib>

#include <stdint.h>



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

#include "SwSel5Decoder.h"



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

// All values are squared magnitude ratios !
#define SEL5_RELATIVE_PEAK          20.0f  /* 13dB */

// The Goertzel algorithm is just a recursive way to evaluate the DFT at a
// single frequency. For maximum detection reliability, the bandwidth will
// be adapted to place the tone frequency near the center of the DFT.
// As a side effect, the detection bandwidth is slightly narrowed, which
// however is acceptable for the SEL5 decoder. Furthermore, a time slack
// between the tone detectors is unavoidable, since they use different
// block lengths.
#define SEL5_BANDWIDTH              35     /* 35Hz */
#define SEL5_BLOCK_LENGTH           (INTERNAL_SAMPLE_RATE / 1000)


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

SwSel5Decoder::SwSel5Decoder(Config &cfg, const string &name)
  : Sel5Decoder(cfg, name), sel5_table(0), samples_left(SEL5_BLOCK_LENGTH),
    last_hit(0), last_stable(0), stable_timer(0), active_timer(0), arr_len(0)
{
} /* SwSel5Decoder::SwSel5Decoder */


SwSel5Decoder::~SwSel5Decoder(void)
{
  delete [] sel5_table;
} /* SwSel5Decoder::~SwSel5Decoder */


bool SwSel5Decoder::initialize(void)
{
  if (!Sel5Decoder::initialize())
  {
    return false;
  }

  float tones[16];
  /*  the tones for each mode
   *                   0        1         2        3        4        5
   *                   6        7         8        9        A        B
   *                   C        D         E        F
  */
  float zvei1[] = { 2400.0f, 1060.0f, 1160.0f, 1270.0f, 1400.0f, 1530.0f,
                    1670.0f, 1830.0f, 2000.0f, 2200.0f, 2800.0f,  810.0f,
                     970.0f,  885.0f, 2600.0f,  680.0f
                   };
  float zvei2[] = { 2400.0f, 1060.0f, 1160.0f, 1270.0f, 1400.0f, 1530.0f,
                     1670.0f, 1830.0f, 2000.0f, 2200.0f, 885.0f,   810.0f,
                      740.0f,  680.0f,  970.0f, 2600.0f
                   };
  float zvei3[] = { 2200.0f, 970.0f, 1060.0f, 1160.0f, 1270.0f, 1400.0f,
                     1530.0f,1670.0f, 1830.0f, 2000.0f,  885.0f,  810.0f,
                      740.0f, 680.0f, 2400.0f, 2600.0f
                   };
  float ccir[]  = { 1981.0f, 1124.0f, 1197.0f, 1275.0f, 1358.0f, 1446.0f,
                     1540.0f, 1640.0f, 1747.0f, 1860.0f, 2400.0f,  930.0f,
                     2247.0f,  991.0f, 2110.0f, 1055.0f
                   };
  float pccir[] = { 1981.0f, 1124.0f, 1197.0f, 1275.0f, 1358.0f, 1446.0f,
                     1540.0f, 1640.0f, 1747.0f, 1860.0f, 1050.0f,  930.0f,
                     2247.0f,  991.0f, 2110.0f, 1055.0f
                   };
  float eea[]   = { 1981.0f, 1124.0f, 1197.0f, 1275.0f, 1358.0f, 1446.0f,
                    1540.0f, 1640.0f, 1747.0f, 1860.0f, 1055.0f,  930.0f,
                    2400.0f,  991.0f, 2110.0f, 2047.0f
                   };
  float eia[]   = {  600.0f,  741.0f,  882.0f, 1023.0f, 1164.0f, 1305.0f,
                    1446.0f, 1587.0f, 1728.0f, 1869.0f, 2151.0f, 2433.0f,
                    2010.0f, 2292.0f,  459.0f, 1091.0f
                   };
  float dzvei[] = { 2200.0f,  970.0f, 1060.0f, 1160.0f, 1270.0f, 1400.0f,
                    1530.0f, 1670.0f, 1830.0f, 2000.0f,  825.0f,  740.0f,
                    2600.0f,  885.0f, 2400.0f,  680.0f
                   };
  float pzvei[] = { 2400.0f, 1060.0f, 1160.0f, 1270.0f, 1400.0f, 1530.0f,
                    1670.0f, 1830.0f, 2000.0f, 2200.0f,  970.0f,  810.0f,
                    2800.0f,  885.0f, 2600.0f,  680.0f
                   };
  float pdzvei[]= { 2200.0f, 970.0f, 1060.0f, 1160.0f, 1270.0f, 1400.0f,
                     1530.0f,1670.0f, 1830.0f, 2000.0f,  825.0f,  886.0f,
                     2600.0f, 856.0f, 2400.0f
                   };
  float vdew[] =  { 2280.0f,  370.0f,  450.0f,  550.0f,  675.0f,  825.0f,
                     1010.0f, 1240.0f, 1520.0f, 1860.0f, 2000.0f, 2100.0f,
                     2200.0f, 2300.0f, 2400.0f
                   };
  float ccitt[] = {  400.0f,  679.0f,  770.0f,  852.0f,  941.0f, 1209.0f,
                     1335.0f, 1477.0f, 1633.0f, 1800.0f, 1900.0f, 2000.0f,
                     2100.0f, 2220.0f, 2300.0f
                   };
  float euro[] =  {  979.8f,  903.1f,  832.5f,  764.4f,  707.4f,  652.0f,
                     601.0f,  554.0f,  510.7f,  470.8f,  433.9f,  400.0f,
                     368.7f,  393.9f, 1062.9f,  313.3f
                   };
  float natel[] = { 1633.0f,  631.0f,  697.0f,  770.0f,  852.0f,  941.0f,
                     1040.0f, 1209.0f, 1336.0f, 1477.0f, 1995.0f,  571.0f,
                     2205.0f, 2437.0f, 1805.0f, 2694.0f
                   };
  float modat[] = {  637.5f,  787.5f,  937.5f, 1087.5f, 1237.5f, 1387.5f,
                     1537.5f, 1687.5f, 1837.5f, 1987.5f, 487.5f
                   };
  float autoa[] = { 1962.0f,  764.0f,  848.0f,  942.0f, 1047.0f, 1163.0f,
                     1292.0f, 1436.0f, 1595.0f, 1770.0f, 2430.0f,
                     2188.0f
                   };

  string value;
  string tonedef = "0123456789";

  cfg().getValue(name(), "SEL5_TYPE", value);
  if (value == "ZVEI1")
  {
     memcpy(tones, zvei1, sizeof(zvei1));
     tonedef += "ABCDEF";
  }
  else if (value == "ZVEI2")
  {
     memcpy(tones, zvei2, sizeof(zvei2));
     tonedef += "ABCDEF";
  }
  else if (value == "ZVEI3")
  {
     memcpy(tones, zvei3, sizeof(zvei3));
     tonedef += "ABCDEF";
  }
  else if (value == "PZVEI")
  {
     memcpy(tones, pzvei, sizeof(pzvei));
     tonedef += "ABCDEF";
  }
  else if (value == "DZVEI")
  {
     memcpy(tones, dzvei, sizeof(dzvei));
     tonedef += "ABCDEF";
  }
  else if (value == "PDZVEI")
  {
     memcpy(tones, pdzvei, sizeof(pdzvei));
     tonedef += "ABCDE";
  }
  else if (value == "EEA")
  {
     memcpy(tones, eea, sizeof(eea));
     tonedef += "ABCDEF";
  }
  else if (value == "EIA")
  {
     memcpy(tones, eia, sizeof(eia));
     tonedef += "ABCDEF";
  }
  else if (value == "VDEW")
  {
     memcpy(tones, vdew, sizeof(vdew));
     tonedef += "ABCDE";
  }
  else if (value == "CCIR" || value == "CCIR1" ||
            value == "CCIR2")
  {
     memcpy(tones, ccir, sizeof(ccir));
     tonedef += "ABCDEF";
  }
  else if (value == "PCCIR")
  {
     memcpy(tones, pccir, sizeof(pccir));
     tonedef += "ABCDE";
  }
  else if (value == "CCITT")
  {
     memcpy(tones, ccitt, sizeof(ccitt));
     tonedef += "ABCDE";
  }
  else if (value == "NATEL")
  {
     memcpy(tones, natel, sizeof(natel));
     tonedef += "ABCDEF";
  }
  else if (value == "EURO")
  {
     memcpy(tones, euro, sizeof(euro));
     tonedef += "ABCDEF";
  }
  else if (value == "MODAT")
  {
     memcpy(tones, modat, sizeof(modat));
     tonedef += "E";
  }
  else if (value == "AUTOA")
  {
     memcpy(tones, autoa, sizeof(autoa));
     tonedef += "BE";
  }
  else
  {
     cout << "*** WARNING: No/wrong Sel5 type defined, using default\n";
     value = "ZVEI1";
     memcpy(tones, zvei1, sizeof(zvei1));
     tonedef += "ABCDEF";
  }

  arr_len = tonedef.length() - 1;
  sel5_table = new char[arr_len + 1];
  strcpy(sel5_table, tonedef.c_str());

  cout << "Starting " << value << " decoder" << endl;

  memset(row_energy, 0, sizeof(row_energy));

  /* Init row detectors */
  for (int a=0; a<=arr_len; a++)
  {
     goertzelInit(&row_out[a],    tones[a], SEL5_BANDWIDTH, 0.0f);
     goertzelInit(&row_out[a+20], tones[a], SEL5_BANDWIDTH, 0.5f);
  }

  return true;

} /* SwSel5Decoder::initialize */


int SwSel5Decoder::writeSamples(const float *buf, int len)
{
    int k;
    for (int i = 0; i < len; i++)
    {
        float v1;
        float famp = *(buf++);

        for (k=0; k<=arr_len; k++)
        {
          v1 = row_out[k].v2;
          row_out[k].v2 = row_out[k].v3;
          row_out[k].v3 = row_out[k].fac * row_out[k].v2 - v1 +
			  famp * *(row_out[k].win++);
        }

        /* Row result calculators */
        for (k=0; k<=arr_len; k++)
        {
          if (--row_out[k].samples_left == 0)
              row_energy[k] = goertzelResult(&row_out[k]);

          if (--row_out[k+20].samples_left == 0)
            row_energy[k] = goertzelResult(&row_out[k+20]);
        }

         /* Now we are at the end of the detection block */
        if (--samples_left == 0)
            Sel5Receive();
    }

    return len;

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

void SwSel5Decoder::Sel5Receive(void)
{

    /* Find the peak row and the peak column */
    int best_row = findMaxIndex(row_energy);

    uint8_t hit = 0;
    /* Valid index test */
    if (best_row >= 0)
    {
        /* Got a hit */
        hit = sel5_table[best_row];
    }

    /* Call the post-processing function. */
    Sel5PostProcess(hit);

    /* Reset the sample counter. */
    samples_left = SEL5_BLOCK_LENGTH;

} /* SwSel5Decoder::Sel5Receive */


void SwSel5Decoder::Sel5PostProcess(uint8_t hit)
{

  /* This function is called when a complete block has been received. */
  if (hit)
  {
//    cout << "hit: " << hit << endl;

    if (last_hit != hit) active_timer = 0;

    /* we need some successfully detects to ensure that a tone has really been
       detected */
    if (active_timer++ > 10 && last_stable != hit)
    {

       /* 'E' the the repeat tone  */
       if (hit == 'E') dec_digits += last_stable;
         else dec_digits += hit;

       /* the last successfully detected digit */
       last_stable = hit;
    }

    /* save the last hit */
    last_hit = hit;
    stable_timer = 0;
  }

  /* detecting end of sequence */
  if (!hit && stable_timer++ > 120)
  {
    /* we need at least 4 digits */
    if (dec_digits.length() > 3)
    {
       sequenceDetected(dec_digits);
//       cout << "Sel5 sequence detected: " << dec_digits << endl;
    }
    active_timer = 0;
    stable_timer = 0;
    dec_digits = "";
    last_hit = 0;
  }

} /* SwSel5Decoder::Sel5PostProcess */


void SwSel5Decoder::goertzelInit(GoertzelState *s, float freq, float bw, float offset)
{
    /* Adjust the block length to minimize the DFT error. */
    s->block_length = lrintf(ceilf(freq / bw) * INTERNAL_SAMPLE_RATE / freq);
    /* Scale output values to achieve same levels at different block lengths. */
    s->scale_factor = 1.0e6f / (s->block_length * s->block_length);
    /* Init detector frequency. */
    s->fac = 2.0f * cosf(2.0f * M_PI * freq / INTERNAL_SAMPLE_RATE);
    /* Reset the tone detector state. */
    s->v2 = s->v3 = 0.0f;
    s->samples_left = static_cast<int>(s->block_length * (1.0f - offset));
    /* Hamming window */
    for (int i = 0; i < s->block_length; i++)
    {
        s->window_table.push_back(
           0.54 - 0.46 * cosf(2.0f * M_PI * i / (s->block_length - 1)));
    }
    /* Point to the first table entry */
    s->win = s->window_table.begin();

} /* SwSel5Decoder::goertzelInit */


float SwSel5Decoder::goertzelResult(GoertzelState *s)
{
    float v1, res;

    /* Push a zero through the process to finish things off. */
    v1 = s->v2;
    s->v2 = s->v3;
    s->v3 = s->fac*s->v2 - v1;
    /* Now calculate the non-recursive side of the filter. */
    /* The result here is not scaled down to allow for the magnification
       effect of the filter (the usual DFT magnification effect). */
    res = (s->v3*s->v3 + s->v2*s->v2 - s->v2*s->v3*s->fac) * s->scale_factor;
    /* Reset the tone detector state. */
    s->v2 = s->v3 = 0.0f;
    s->samples_left = s->block_length;
    s->win = s->window_table.begin();
    /* Return the calculated signal level. */
    return res;

} /* SwSel5Decoder::goertzelResult */


int SwSel5Decoder::findMaxIndex(const float f[])
{
    float threshold = 1.0f;
    int idx = -1;
    int i;

    /* Peak search */
    for (i = 0; i < arr_len; i++)
    {
        if (f[i] > threshold)
        {
            threshold = f[i];
            idx = i;
        }
    }
    if (idx < 0)
        return -1;

    /* Peak test */
    threshold *= 1.0f / SEL5_RELATIVE_PEAK;

    for (i = 0; i < arr_len; i++)
    {
        if (idx != i && f[i] > threshold)
            return -1;
    }
    return idx;

} /* SwSel5Decoder::findMaxIndex */


/*- End of file ------------------------------------------------------------*/
