/**
@file	 AfskDemodulator.cpp
@brief   A class to demodulate Audio Frequency Shift Keying
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

#include <cstring>
#include <cmath>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <deque>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioProcessor.h>
//#include <AsyncAudioClipper.h>
#include <AsyncAudioFilter.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AfskDemodulator.h"
//#include "remez.h"


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

namespace {
  class Correlator : public AudioProcessor
  {
    public:
      Correlator(float f0, float f1, unsigned baudrate,
          unsigned sample_rate=INTERNAL_SAMPLE_RATE)
        : delay(0), head(0)
      {
          // Calculate the optimum value for the delay
        unsigned samples_per_symbol = sample_rate / baudrate;
        double max_k_val = 0.0;
        for (unsigned k=0; k<samples_per_symbol; ++k)
        {
          double k_val = abs(cos(2.0 * M_PI * f0 * k / sample_rate)
              - cos(2.0 * M_PI * f1 * k / sample_rate));
          if (k_val > max_k_val)
          {
            max_k_val = k_val;
            delay = k;
          }
        }
        cout << "### Delay: " << delay << endl;

        buf = new float[delay];
        memset(buf, 0, sizeof(float) * delay);
      }

      ~Correlator(void)
      {
        delete [] buf;
      }

    protected:
      void processSamples(float *out, const float *in, int len)
      {
        for (int i=0; i<len; ++i)
        {
          out[i] = in[i] * buf[head];
          buf[head] = in[i];
          head = (head+1) % delay;
        }
      }

    private:
      unsigned  delay;
      float *   buf;
      unsigned  head;
  };

#if 0
  class RollingAverage
  {
    public:
      RollingAverage(unsigned order)
        : delay(order-1, 0), prev(0.0f)
      {
      }

      void process(float *dest, const float *src, size_t count)
      {
        for (size_t i=0; i<count; ++i)
        {
          float in = src[i];
          float out = in - delay.front() + prev;
          out /= delay.size();
          dest[i] = out;
          prev = out;
          delay.push_back(in);
          delay.pop_front();
        }
      }

    private:
      deque<float>  delay;
      float         prev;
  };
#endif


  class DcBlocker : public AudioProcessor
  {
    public:
      DcBlocker(size_t order)
        : order(order), delay(order, 0.0f), prev(0.0f)
      {
      }

      ~DcBlocker(void) {}

      /**
       * @brief Process incoming samples and put them into the output buffer
       * @param dest  Destination buffer
       * @param src   Source buffer
       * @param count Number of samples in the source buffer
       *
       * This function do the actual processing of the incoming samples. All
       * samples must be processed, otherwise they are lost and the output
       * buffer will contain garbage.
       */
      virtual void processSamples(float *dest, const float *src, int count)
      {
        for (int i=0; i<count; ++i)
        {
          float in = src[i];
          float out = (in - delay.back()) / order + prev;
          //dest[i] = delay[order/2]-out;
          dest[i] = in-out;
          prev = out;
          delay.pop_back();
          delay.push_front(in);
        }
      }

    private:
      const size_t  order;
      deque<float>  delay;
      float         prev;

  }; /* class DcBlocker */
}; /* Anonymous namespace */


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

#if 0
namespace {
  const double pre_filter_taps[] =
  {
    -0.0497607774343533,
    -0.0414626205397930,
    0.0147754767686696,
    -0.0050460885570081,
    0.0126147216425678,
    -0.0264073840416123,
    0.0296003565900331,
    -0.0130149931857980,
    -0.0144000295612727,
    0.0295301867452591,
    -0.0152325941654806,
    -0.0201650467073737,
    0.0449705722700260,
    -0.0314722795301329,
    -0.0147694650389599,
    0.0541293567854939,
    -0.0469861720933350,
    -0.0064907618162468,
    0.0607226391419224,
    -0.0637607134140635,
    0.0074537437539216,
    0.0603289683874208,
    -0.0769560352643945,
    0.0239509524606725,
    0.0532261698461783,
    -0.0844641222378282,
    0.0403188191263031,
    0.0403188191263031,
    -0.0844641222378282,
    0.0532261698461783,
    0.0239509524606725,
    -0.0769560352643945,
    0.0603289683874208,
    0.0074537437539216,
    -0.0637607134140635,
    0.0607226391419224,
    -0.0064907618162468,
    -0.0469861720933350,
    0.0541293567854939,
    -0.0147694650389599,
    -0.0314722795301329,
    0.0449705722700260,
    -0.0201650467073737,
    -0.0152325941654806,
    0.0295301867452591,
    -0.0144000295612727,
    -0.0130149931857980,
    0.0296003565900331,
    -0.0264073840416123,
    0.0126147216425678,
    -0.0050460885570081,
    0.0147754767686696,
    -0.0414626205397930,
    -0.0497607774343533
  };
  const int numtaps = sizeof(pre_filter_taps) / sizeof(*pre_filter_taps);
};
#endif


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

AfskDemodulator::AfskDemodulator(unsigned f0, unsigned f1, unsigned baudrate,
                                 unsigned sample_rate)
  : f0(f0), f1(f1), baudrate(baudrate)
{
  AudioSource *prev_src = 0;

  //assert(f0 == 5415);
  //assert(f1 == 5585);
  //assert(baudrate == 300);
  assert(sample_rate == 16000);

    // Clip the signal to eliminate level differences
  /*
  AudioClipper *clipper = new AudioClipper(0.05);
  AudioSink::setHandler(clipper);
  prev_src = clipper;
  */

  DcBlocker *dc_blocker = new DcBlocker(sample_rate / 10);
  AudioSink::setHandler(dc_blocker);
  prev_src = dc_blocker;


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
#endif
  /*
  cout << "ret=" << ret << endl;
  for (int i=0; i<numtaps; ++i)
  {
    cout << pre_filter_taps[i] << endl;
  }
  */
#if 0
  if ((f0 == 5415) && (f1 == 5585) && (baudrate == 300))
  {
    stringstream ss;
    //ss << "BpBu3/" << (f0 - baudrate/2) << "-" << (f1 + baudrate/2);
    ss << fixed << setprecision(9);
    ss << "x";
    for (int i=0; i<numtaps; ++i)
    {
      ss << " " << pre_filter_taps[i];
    }
    cout << "### AFSK RX Passband filter: " << ss.str() << endl;
    AudioFilter *passband_filter = new AudioFilter(ss.str(), sample_rate);
    //AudioSink::setHandler(passband_filter);
    prev_src->registerSink(passband_filter, true);
    prev_src = passband_filter;
    AudioFilter *passband_filter2 = new AudioFilter(ss.str(), sample_rate);
    prev_src->registerSink(passband_filter2, true);
    prev_src = passband_filter2;
  }
#endif

    // Run the sample stream through the correlator
  Correlator *corr = new Correlator(f0, f1, baudrate, sample_rate);
  prev_src->registerSink(corr, true);
  prev_src = corr;

    // Low pass out the constant component from the correlator
  stringstream ss("");
  ss << "LpBu5/" << baudrate;
  cout << "### Correlator filter: " << ss.str() << endl;
  AudioFilter *corr_filter = new AudioFilter(ss.str(), sample_rate);
  corr_filter->setOutputGain(10);
  prev_src->registerSink(corr_filter, true);
  prev_src = corr_filter;

  AudioSource::setHandler(prev_src);
} /* AfskDemodulator::AfskDemodulator */


AfskDemodulator::~AfskDemodulator(void)
{
  AudioSink *handler = AudioSink::handler();
  AudioSink::clearHandler();
  delete handler;
} /* AfskDemodulator::~AfskDemodulator */



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



/*
 * This file has not been truncated
 */

