/**
@file	 AfskDemodulator.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-09

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioProcessor.h>
#include <AsyncAudioClipper.h>
#include <AsyncAudioFilter.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AfskDemodulator.h"
#include "remez.h"



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
        cout << "Delay: " << delay << endl;

        buf = new float[delay];
        memset(buf, 0, sizeof(float) * delay);
      }

    protected:
      void processSamples(float *out, const float *in, int len)
      {
        //cout << "processSamples: len=" << len << endl;
        for (int i=0; i<len; ++i)
        {
          out[i] = in[i] * buf[head];
          buf[head] = in[i];
          head = (head+1) % delay;
        }
      }

    private:
      unsigned delay;
      float *buf;
      unsigned head;
  };
};


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

AfskDemodulator::AfskDemodulator(unsigned f0, unsigned f1, unsigned baudrate,
                                 unsigned sample_rate)
  : f0(f0), f1(f1), baudrate(baudrate)
{
  AudioSource *prev_src = 0;

    // Clip the signal to eliminate level differences
  /*
  AudioClipper *clipper = new AudioClipper(0.05);
  AudioSink::setHandler(clipper);
  prev_src = clipper;
  */

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
  /*
  cout << "ret=" << ret << endl;
  for (int i=0; i<numtaps; ++i)
  {
    cout << pre_filter_taps[i] << endl;
  }
  */
  stringstream ss;
  //ss << "BpBu3/" << (f0 - baudrate/2) << "-" << (f1 + baudrate/2);
  ss << fixed << setprecision(9);
  ss << "x";
  for (int i=0; i<numtaps; ++i)
  {
    ss << " " << pre_filter_taps[i];
  }
  cout << "Passband filter: " << ss.str() << endl;
  AudioFilter *passband_filter = new AudioFilter(ss.str(), sample_rate);
  AudioSink::setHandler(passband_filter);
  //prev_src->registerSink(passband_filter, true);
  prev_src = passband_filter;
  AudioFilter *passband_filter2 = new AudioFilter(ss.str(), sample_rate);
  prev_src->registerSink(passband_filter2, true);
  prev_src = passband_filter2;

    // Run the sample stream through the correlator
  Correlator *corr = new Correlator(f0, f1, baudrate, sample_rate);
  prev_src->registerSink(corr, true);
  prev_src = corr;

    // Low pass out the constant component from the correlator
  ss.str("");
  ss << "LpBu5/" << baudrate;
  cout << "Correlator filter: " << ss.str() << endl;
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

