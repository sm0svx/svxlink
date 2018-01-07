/**
@file	 AfskModulator.cpp
@brief   Audio Frequency Shift Keying modulator
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#include <cmath>
#include <iostream>
#include <sstream>
#include <cstring>
#include <iomanip>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncSigCAudioSource.h>
#include <AsyncAudioFilter.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

//#include "remez.h"
#include "AfskModulator.h"


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

namespace {
  /**
   * @brief Find the greatest common divisor for two numbers
   * @param dividend The larger number
   * @param divisor The lesser number
   *
   * This function will return the greatest common divisor of the two given
   * numbers. This implementation requires that the dividend is larger than
   * the divisor.
   */
  unsigned gcd(unsigned dividend, unsigned divisor)
  {
    unsigned reminder = dividend % divisor;
    if (reminder == 0)
    {
      return divisor;
    }
    return gcd(divisor, reminder);
  }

#if 0
  /**
   * @brief Find the multiplier that make the given fractional an integer
   * @param numerator The numer to be divided
   * @param denominator The number to divide with
   * @return  Returns the mutiplier
   *
   * This function will return a multiplier constructed so that if
   * multiplied with the given fractional, the result will be an
   * integer number.
   */
  unsigned AfskModulator::findMultiplier(unsigned numerator, unsigned denominator)
  {
    unsigned quotient = denominator / numerator;
    unsigned rest = denominator - quotient * numerator;
    cout << "numerator=" << numerator << " denominator=" << denominator
         << " quotient=" << quotient << " rest=" << rest << endl;
    if (rest == 0)
    {
      return quotient;
    }
    unsigned N = findMultiplier(rest, numerator);
    return N * quotient + N * rest / numerator;
  } /* AfskModulator::findMultiplier */
#endif
};


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

AfskModulator::AfskModulator(unsigned f0, unsigned f1, unsigned baudrate,
                             float level, unsigned sample_rate)
  : baudrate(baudrate), sample_rate(sample_rate), phi(0), bitclock(0),
    buf_pos(BUFSIZE), sigc_src(0), fade_len(0), fade_pos(0), fade_dir(0)
{
#if 0
  unsigned N0 = findMultiplier(f0, sample_rate);
  cout << "N0=" << N0 << endl;
  unsigned N1 = findMultiplier(f1, sample_rate);
  cout << "N1=" << N1 << endl;
  if (N0 > N1)
  {
    N = N1 * findMultiplier(N1, N0);
  }
  else
  {
    N = N0 * findMultiplier(N0, N1);
  }
#endif
  unsigned N0 = sample_rate / gcd(sample_rate, f0);
  unsigned N1 = sample_rate / gcd(sample_rate, f1);
  if (N0 > N1)
  {
    N = N1 * N0 / gcd(N0, N1);
  }
  else
  {
    N = N0 * N1 / gcd(N1, N0);
  }

  sin_lookup = new float[N];
  float sin_ampl = powf(10.0f, level/20.0f);
  for (unsigned n=0; n<N; ++n)
  {
    sin_lookup[n] =  sin_ampl* sinf(2.0*M_PI*n/N);
  }

  k0 = N * f0 / sample_rate;
  k1 = N * f1 / sample_rate;

  fade_len = FADE_SYMBOLS * sample_rate / baudrate;
  exp_lookup = new float[fade_len];
  for (unsigned i=0; i<fade_len; ++i)
  {
    exp_lookup[i] =
      expf(logf(FADE_START_VAL) * (fade_len - 1 - i) / (fade_len - 1));
  }

  cout << "### N0=" << N0 << endl;
  cout << "### N1=" << N1 << endl;
  cout << "### N=" << N << endl;
  cout << "### k0=" << k0 << " k1=" << k1 << endl;

  sigc_src = new SigCAudioSource;
  sigc_src->sigResumeOutput.connect(
      mem_fun(*this, &AfskModulator::onResumeOutput));
  sigc_src->sigAllSamplesFlushed.connect(
      mem_fun(*this, &AfskModulator::onAllSamplesFlushed));
  AudioSource *prev_src = sigc_src;

  /*
  stringstream ss;
  ss << "BpBu3/" << (f0 - baudrate/2) << "-" << (f1 + baudrate/2);
  cout << "FSK modulator passband filter: " << ss.str() << endl;
  AudioFilter *filter = new AudioFilter(ss.str());
  prev_src->registerSink(filter);
  prev_src = filter;
  */

#if 0
    // Apply a bandpass filter to filter out the AFSK signal.
    // The constructed filter is a FIR filter with linear phase.
    // The Parks-Macclellan algorithm is used to design the filter.
    // The length of the filter is chosen to be one symbol long to not cause
    // inter symbol interference (ISI).
  const int numtaps = sample_rate / baudrate;
  const int numband = 3;
  const double bands[] = {
    0.0,
    (f0-baudrate/2.0-200.0)/sample_rate,
    (f0-baudrate/2.0)/sample_rate,
    (f1+baudrate/2.0)/sample_rate,
    (f1+baudrate/2.0+200.0)/sample_rate,
    0.5
  };
  const double des[] = {0.0, 0.0, 1.0, 1.0, 0.0, 0.0};
  const double weight[] = {1.0, 1.0, 1.0};
  const int type = BANDPASS;
  const int griddensity = 16;
  double pre_filter_taps[numtaps];
  int ret = remez(pre_filter_taps, numtaps, numband, bands, des, weight, type,
                  griddensity);
  assert(ret == 0);
  stringstream ss;
  ss << fixed << setprecision(9);
  ss << "x";
  for (int i=0; i<numtaps; ++i)
  {
    ss << " " << pre_filter_taps[i];
  }
  cout << "AFSK TX passband filter: " << ss.str() << endl;
  AudioFilter *passband_filter = new AudioFilter(ss.str(), sample_rate);
  prev_src->registerSink(passband_filter, true);
  prev_src = passband_filter;
  AudioFilter *passband_filter2 = new AudioFilter(ss.str(), sample_rate);
  prev_src->registerSink(passband_filter2, true);
  prev_src = passband_filter2;
  AudioFilter *passband_filter3 = new AudioFilter(ss.str(), sample_rate);
  prev_src->registerSink(passband_filter3, true);
  prev_src = passband_filter3;
  AudioFilter *passband_filter4 = new AudioFilter(ss.str(), sample_rate);
  prev_src->registerSink(passband_filter4, true);
  prev_src = passband_filter4;
#endif

  AudioSource::setHandler(prev_src);
  prev_src = 0;

} /* AfskModulator::AfskModulator */


AfskModulator::~AfskModulator(void)
{
  AudioSource::clearHandler();
  delete sigc_src;
} /* AfskModulator::~AfskModulator */



void AfskModulator::sendBits(const vector<bool> &bits)
{
  if (bits.empty())
  {
    return;
  }

  /*
  for (size_t i=0; i<bits.size(); ++i)
  {
    cout << (bits[i] ? '1' : '0');
  }
  cout << endl;
  */
  if (bitbuf.empty() && (fade_pos < fade_len-1))
  {
    fade_dir = 1;
    bitbuf.resize(FADE_SYMBOLS);
    fill_n(bitbuf.begin(), FADE_SYMBOLS, bits.front());
  }
  bitbuf.insert(bitbuf.end(), bits.begin(), bits.end());
  writeToSink();
} /* AfskModulator::sendBits */


void AfskModulator::onResumeOutput(void)
{
  writeToSink();
} /* AfskModulator::resumeOutput */


void AfskModulator::onAllSamplesFlushed(void)
{
} /* AfskModulator::allSamplesFlushed */



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

void AfskModulator::writeToSink(void)
{
  for (;;)
  {
    if (buf_pos >= BUFSIZE)
    {
      if (bitbuf.empty())
      {
        phi = 0;
        //fade_pos = 0;
        //fade_dir = 0;
        sigc_src->flushSamples();
        return;
      }
      for (size_t i=0; i<BUFSIZE; ++i)
      {
        buf[i] = sin_lookup[phi];
        if (fade_pos < fade_len-1)
        {
          //cout << fade_pos << endl;
          buf[i] *= exp_lookup[fade_pos];
        }
        if (fade_dir != 0)
        {
          fade_pos += fade_dir;
          if ((fade_pos == 0) || (fade_pos == fade_len-1))
          {
            fade_dir = 0;
          }
        }
        phi += bitbuf.front() ? k1 : k0;
        if (phi >= N)
        {
          phi -= N;
        }

        bitclock += baudrate;
        if (bitclock >= sample_rate)
        {
          bitclock -= sample_rate;
          bitbuf.pop_front();
          if (bitbuf.empty())
          {
            if (fade_pos == fade_len-1)
            {
              fade_dir = -1;
              for (unsigned sym=0; sym<FADE_SYMBOLS; ++sym)
              {
                bitbuf.push_back(bitbuf.back());
              }
            }
            else
            {
              i += 1;
              memset(buf+i, 0, sizeof(*buf)*(BUFSIZE-i));
              break;
            }
          }
        }
      }
      buf_pos = 0;
    }

    int to_write = BUFSIZE-buf_pos;
    int written = sigc_src->writeSamples(buf+buf_pos, to_write);
    buf_pos += written;
    if (written == 0)
    {
      break;
    }
  }
} /* AfskModulator::writeToSink */



/*
 * This file has not been truncated
 */

