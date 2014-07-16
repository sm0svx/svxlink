/**
@file	 Ddr.cpp
@brief   A receiver class to handle digital drop receivers
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

This file contains a class that handle local digital drop receivers.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sigc++/sigc++.h>

#include <cstring>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <complex>
#include <fstream>
#include <algorithm>
#include <iterator>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioPassthrough.h>
#include <AsyncTcpClient.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Ddr.h"
#include "WbRxRtlTcp.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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

static const int nbfm_iq_dec_coeff1_cnt = 11;
static const float nbfm_iq_dec_coeff1[nbfm_iq_dec_coeff1_cnt] = {
-0.0042774311138192,
0.0203215628564991,
0.0662460869736418,
0.1305633864584946,
0.1880313441401774,
0.2111689145317699,
0.1880313441401774,
0.1305633864584946,
0.0662460869736418,
0.0203215628564991,
-0.0042774311138192
};

static const int nbfm_iq_dec_coeff2_cnt = 31;
static const float nbfm_iq_dec_coeff2[nbfm_iq_dec_coeff2_cnt] = {
-0.0638521136019694,
-0.0010208850725564,
0.0116531523021925,
0.0263863783640881,
0.0351438143433804,
0.0313791760854124,
0.0136787364296511,
-0.0124236208278904,
-0.0354874256756020,
-0.0420853937348059,
-0.0222900781714972,
0.0255080232882617,
0.0926463175001950,
0.1622142633364627,
0.2145119015320957,
0.2339193865994095,
0.2145119015320957,
0.1622142633364627,
0.0926463175001950,
0.0255080232882617,
-0.0222900781714972,
-0.0420853937348059,
-0.0354874256756020,
-0.0124236208278904,
0.0136787364296511,
0.0313791760854124,
0.0351438143433804,
0.0263863783640881,
0.0116531523021925,
-0.0010208850725564,
-0.0638521136019694
};

static const int wbfm_iq_dec_coeff_cnt = 41;
static const float wbfm_iq_dec_coeff[wbfm_iq_dec_coeff_cnt] = {
0.0194679922087664,
-0.0005977805219536,
-0.0061383768321768,
-0.0125104713279058,
-0.0160872817947965,
-0.0138809700545037,
-0.0052351956520739,
0.0072710975817097,
0.0183426114801711,
0.0220231641013175,
0.0144027836146915,
-0.0036725021706061,
-0.0260775629116080,
-0.0425493300881981,
-0.0422802568440652,
-0.0182490019178980,
0.0293433807630875,
0.0920107509228422,
0.1549430768095743,
0.2014808299049397,
0.2186376583168589,
0.2014808299049397,
0.1549430768095743,
0.0920107509228422,
0.0293433807630875,
-0.0182490019178980,
-0.0422802568440652,
-0.0425493300881981,
-0.0260775629116080,
-0.0036725021706061,
0.0144027836146915,
0.0220231641013175,
0.0183426114801711,
0.0072710975817097,
-0.0052351956520739,
-0.0138809700545037,
-0.0160872817947965,
-0.0125104713279058,
-0.0061383768321768,
-0.0005977805219536,
0.0194679922087664
};

static const int audio_dec_coeff_cnt = 21;
static const float audio_dec_coeff[audio_dec_coeff_cnt] = {
0.0018440287120654,
0.0101759828785910,
0.0099946080524450,
-0.0016951247788318,
-0.0254220310953511,
-0.0423576044725107,
-0.0248714560096571,
0.0431946872471736,
0.1473022777557578,
0.2432684435456421,
0.2823051451694801,
0.2432684435456421,
0.1473022777557578,
0.0431946872471736,
-0.0248714560096571,
-0.0423576044725107,
-0.0254220310953511,
-0.0016951247788318,
0.0099946080524450,
0.0101759828785910,
0.0018440287120654
};


namespace {
  template <class T>
  class Decimator
  {
    public:
      Decimator(int dec_fact, const float *coeff, int taps)
        : dec_fact(dec_fact), taps(taps)
      {
        this->coeff.assign(coeff, coeff+taps);

        p_Z = new T[taps];
        memset(p_Z, 0, taps * sizeof(*p_Z));
      }

      void decimate(vector<T> &out, const vector<T> &in)
      {
        int orig_count = in.size();

        /*
        cout << "### in.size()=" << in.size()
             << " dec_fact=" << dec_fact
             << endl;
        */

          // this implementation assumes in.size() is a multiple of factor_M
        assert(in.size() % dec_fact == 0);
        assert(taps >= dec_fact);

        int num_out = 0;
        typename vector<T>::const_iterator src = in.begin();
        out.clear();
        out.reserve(in.size() / dec_fact);
        while (src != in.end())
        {
            // shift Z delay line up to make room for next samples
          memmove(p_Z + dec_fact, p_Z, (taps - dec_fact) * sizeof(T));

            // copy next samples from input buffer to bottom of Z delay line
          for (int tap = dec_fact - 1; tap >= 0; tap--)
          {
            assert(src != in.end());
            p_Z[tap] = *src++;
          }

            // calculate FIR sum
          T sum;
          memset(&sum, 0, sizeof(T));
          for (int tap = 0; tap < taps; tap++)
          {
            sum += coeff[tap] * p_Z[tap];
          }
          out.push_back(sum);     /* store sum */
          num_out++;
        }

        //printf("out.size()=%d  in.size()=%d  dec_fact=%d\n", out.size(), in.size(), dec_fact);
        assert(num_out == orig_count / dec_fact);
      }

    private:
      const int       dec_fact;
      T               *p_Z;
      int             taps;
      vector<float>   coeff;
  };


  class FmDemod
  {
    public:
      FmDemod(AudioSink &audio_sink)
        : iold(1.0f), qold(1.0f), audio_sink(audio_sink),
          iq_dec1(5, nbfm_iq_dec_coeff1, nbfm_iq_dec_coeff1_cnt),
          iq_dec2(4, nbfm_iq_dec_coeff2, nbfm_iq_dec_coeff2_cnt),
          sql_open(false)
      {
        //Decimator<float> audio_dec(20, audio_dec_coeff, audio_dec_coeff_cnt);
      }

      void iq_received(vector<WbRxRtlTcp::Sample> samples)
      {
        //cout << "### Received " << samples.size() << " samples\n";
#if 0
        outfile.write(reinterpret_cast<char*>(&samples[0]),
            samples.size() * sizeof(samples[0]));
#endif

        vector<WbRxRtlTcp::Sample> dec_samp1, dec_samp;
        iq_dec1.decimate(dec_samp1, samples);
        iq_dec2.decimate(dec_samp, dec_samp1);
        //dec_samp = samples;

          // Från article-sdr-is-qs.pdf: Watch your Is and Qs:
          // FM = (Qn.In-1 - In.Qn-1)/(In.In-1 + Qn.Qn-1)
        vector<float> audio;
        double sumE = 0.0;
        for (size_t idx=0; idx<dec_samp.size(); ++idx)
        {
          sumE += pow(abs(dec_samp[idx]), 2.0);

            // Normalize signal amplitude
          dec_samp[idx] = dec_samp[idx] / abs(dec_samp[idx]);

          float i = dec_samp[idx].real();
          float q = dec_samp[idx].imag();
          //outfile.write(reinterpret_cast<char*>(&i), sizeof(float));
          //outfile.write(reinterpret_cast<char*>(&q), sizeof(float));

#if 1
          float demod = (q*iold - i*qold)/(i*iold + q*qold);
          demod = atanf(demod);
#endif
          /*
          demod /= 5;
          */

            // Complex baseband delay demodulator
#if 0
          float demod = arg(dec_samp[idx] * conj(prev_samp));
          prev_samp = dec_samp[idx];
#endif

          audio.push_back(demod);
          //int16_t samp = static_cast<int16_t>(demod * 4096);
          //outfile.write(reinterpret_cast<char*>(&samp), sizeof(samp));

          iold = i;
          qold = q;
        }
        double meanE = sumE / dec_samp.size();
        //if (meanE > 5.0E-6)
        //if (meanE > 0.01)
        {
          //cout << "meanE=" << meanE << endl;
          sql_open = true;
          vector<float> dec_audio;
          //audio_dec.decimate(dec_audio, audio);
          dec_audio = audio;
          audio_sink.writeSamples(&dec_audio[0], dec_audio.size());
        }
        /*
        else if (sql_open)
        {
          audio_sink.flushSamples();
          sql_open = false;
        }
        */
      }

    private:
      float iold;
      float qold;
      WbRxRtlTcp::Sample prev_samp;
      AudioSink &audio_sink;
      Decimator<complex<float> > iq_dec1;
      Decimator<complex<float> > iq_dec2;
      bool sql_open;

  };


  class Translate
  {
    public:
      Translate(float offset)
        : offset(offset), n(0)
      {
        
      }

      void iq_received(vector<WbRxRtlTcp::Sample> &out,
                       const vector<WbRxRtlTcp::Sample> &in)
      {
        vector<WbRxRtlTcp::Sample>::const_iterator it;
        for (it = in.begin(); it != in.end(); ++it)
        {
          complex<float> e(0.0f, -2.0*M_PI*offset*n/FS);
          if (++n >= FS)
          {
            n = 0;
          }
          out.push_back(*it * exp(e));
        }
      }

    private:
      static const unsigned FS = 960000;

      float offset;
      unsigned n;

  };

  class Channel
  {
    public:
      Channel(AudioSink &audio_sink, int fq_offset)
        : fm_demod(audio_sink), trans(fq_offset)
      {
      }

      void iq_received(vector<WbRxRtlTcp::Sample> samples)
      {
        vector<WbRxRtlTcp::Sample> translated;
        trans.iq_received(translated, samples);
        fm_demod.iq_received(translated);
      };

    private:
      FmDemod fm_demod;
      Translate trans;
  }; /* Channel */
}; /* anonymous namespace */



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

Ddr::Ddr(Config &cfg, const std::string& name)
  : LocalRxBase(cfg, name), cfg(cfg)
{
} /* Ddr::Ddr */


Ddr::~Ddr(void)
{
} /* Ddr::~Ddr */


bool Ddr::initialize(void)
{
  double fq = 0.0;
  if (!cfg.getValue(name(), "FQ", fq))
  {
    cerr << "*** ERROR: Config variable " << name() << "/FQ not set\n";
    return false;
  }
  
  string wbrx;
  if (!cfg.getValue(name(), "WBRX", wbrx))
  {
    cerr << "*** ERROR: Config variable " << name()
         << "/WBRX not set\n";
    return false;
  }

  audio_pipe = new AudioPassthrough;
  
  unsigned int fc = 433400000;
  WbRxRtlTcp *rtl = new WbRxRtlTcp;
  rtl->setSampleRate(960000);
  rtl->setFqCorr(60);
  //rtl->setFqCorr(36);
  rtl->setCenterFq(fc);
  //rtl->enableDigitalAgc(false);

  Channel *test = new Channel(*audio_pipe, 433450000-fc);
  rtl->iqReceived.connect(mem_fun(*test, &Channel::iq_received));
  //Channel ru1("alsa:plughw:0", 0, 434625000-fc);
  //rtl->iqReceived.connect(mem_fun(ru1, &Channel::iq_received));
  //Channel *ru5 = new Channel(*audio_pipe, 434725000-fc);
  //rtl->iqReceived.connect(mem_fun(*ru5, &Channel::iq_received));
  //Channel ru13("alsa:plughw:0", 1, 434925000-fc);
  //rtl->iqReceived.connect(mem_fun(ru13, &Channel::iq_received));
    // Create the audio source object

  if (!LocalRxBase::initialize())
  {
    return false;
  }

  return true;
  
} /* Ddr:initialize */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

bool Ddr::audioOpen(void)
{
  return true;
} /* Ddr::audioOpen */


void Ddr::audioClose(void)
{
} /* Ddr::audioClose */


int Ddr::audioSampleRate(void)
{
  return 48000;
} /* Ddr::audioSampleRate */


Async::AudioSource *Ddr::audioSource(void)
{
  return audio_pipe;
} /* Ddr::audioSource */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

