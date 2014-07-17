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

#define FILTER_COEFF(name, coeffs...) \
  static const float name[] = { \
    coeffs \
  }; \
  static const int name ## _cnt = sizeof(name) / sizeof(*name);


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

/**
 * fs=960000;
 * a=[1 1 0 0];
 * f=[0 10000/(fs/2) 96000/(fs/2) 1];
 * b=firpm(30,f,a);
 *
 * Lowpass filter of order 30 for first stage decimation from 960kHz to 192kHz
 * sampling frequency. Below -50dB over 96kHz.
 */
FILTER_COEFF(nbfm_iq_dec_coeff1,
-0.0028713422063345,
-0.0041769139545598,
-0.0062171808745743,
-0.0078031238748803,
-0.0081233271798609,
-0.0062743621398142,
-0.0014205292313401,
0.0070048711105898,
0.0191247988093941,
0.0344897867015570,
0.0520570673062483,
0.0702711557447386,
0.0872655777710594,
0.1011208091483218,
0.1101849356054080,
0.1133410547020278,
0.1101849356054080,
0.1011208091483218,
0.0872655777710594,
0.0702711557447386,
0.0520570673062483,
0.0344897867015570,
0.0191247988093941,
0.0070048711105898,
-0.0014205292313401,
-0.0062743621398142,
-0.0081233271798609,
-0.0078031238748803,
-0.0062171808745743,
-0.0041769139545598,
-0.0028713422063345
)

/**
 * fs=192000;
 * a=[1 1 0 0];
 * f=[0 10000/(fs/2) 24000/(fs/2) 1];
 * b=firpm(38,f,a);
 *
 * Lowpass filter of order 38 for second stage decimation from 192kHz to 48kHz
 * sampling frequency. Below -50dB over 24kHz.
 */
FILTER_COEFF(nbfm_iq_dec_coeff2,
-0.0022054057399946,
-0.0013759144555157,
-0.0003560235137129,
0.0019402788096753,
0.0050318463119633,
0.0077118103385981,
0.0083196771915043,
0.0053564859260711,
-0.0017223951190513,
-0.0118310814176461,
-0.0220269740210023,
-0.0279535379185214,
-0.0249640367446503,
-0.0096365156797809,
0.0187997839077061,
0.0576385308284084,
0.1008804142901618,
0.1404539524894698,
0.1682431134370760,
0.1782444890349868,
0.1682431134370760,
0.1404539524894698,
0.1008804142901618,
0.0576385308284084,
0.0187997839077061,
-0.0096365156797809,
-0.0249640367446503,
-0.0279535379185214,
-0.0220269740210023,
-0.0118310814176461,
-0.0017223951190513,
0.0053564859260711,
0.0083196771915043,
0.0077118103385981,
0.0050318463119633,
0.0019402788096753,
-0.0003560235137129,
-0.0013759144555157,
-0.0022054057399946
)

/**
 * fs=48000;
 * a=[1 1 0 0];
 * f=[0 10000/(fs/2) 12500/(fs/2) 1];
 * b=firpm(52,f,a);
 *
 * Lowpass filter of order 52 for channel filter to create a channel that start
 * falling off at 20kHz bandwidth and at -50dB over 25kHz bandwidth.
 */
FILTER_COEFF(nbfm_iq_ch_filt_coeff,
0.0008241697920589,
-0.0021070026154187,
-0.0018283827176036,
0.0016310471394501,
0.0028118758642382,
-0.0019396699761340,
-0.0046522841665745,
0.0017070490502376,
0.0070035913525649,
-0.0007830014477388,
-0.0098758879175436,
-0.0011783591096547,
0.0131725633282163,
0.0045849076900695,
-0.0167379346321622,
-0.0099840064006532,
0.0203645795200879,
0.0182368672779849,
-0.0238117005219107,
-0.0310369079128660,
0.0268255689484280,
0.0527102597060804,
-0.0291747144820595,
-0.0992601034613792,
0.0306674034773647,
0.3159828500981747,
0.4688223043097902,
0.3159828500981747,
0.0306674034773647,
-0.0992601034613792,
-0.0291747144820595,
0.0527102597060804,
0.0268255689484280,
-0.0310369079128660,
-0.0238117005219107,
0.0182368672779849,
0.0203645795200879,
-0.0099840064006532,
-0.0167379346321622,
0.0045849076900695,
0.0131725633282163,
-0.0011783591096547,
-0.0098758879175436,
-0.0007830014477388,
0.0070035913525649,
0.0017070490502376,
-0.0046522841665745,
-0.0019396699761340,
0.0028118758642382,
0.0016310471394501,
-0.0018283827176036,
-0.0021070026154187,
0.0008241697920589
)

FILTER_COEFF(wbfm_iq_dec_coeff,
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
)

FILTER_COEFF(audio_dec_coeff,
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
)


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
          ch_filt(1, nbfm_iq_ch_filt_coeff, nbfm_iq_ch_filt_coeff_cnt),
          sql_open(false)
      {
        //Decimator<float> audio_dec(20, audio_dec_coeff, audio_dec_coeff_cnt);
        outfile.open("out.bin", ios::out | ios::binary);
      }

      ~FmDemod()
      {
        outfile.close();
      }

      void iq_received(vector<WbRxRtlTcp::Sample> samples)
      {
        //cout << "### Received " << samples.size() << " samples\n";
#if 0
        outfile.write(reinterpret_cast<char*>(&samples[0]),
            samples.size() * sizeof(samples[0]));
#endif

        vector<WbRxRtlTcp::Sample> dec_samp1, dec_samp2, ch_samp;
        iq_dec1.decimate(dec_samp1, samples);
        iq_dec2.decimate(dec_samp2, dec_samp1);
        ch_filt.decimate(ch_samp, dec_samp2);
        //dec_samp = samples;
#if 0
        outfile.write(reinterpret_cast<char*>(&ch_samp[0]),
            ch_samp.size() * sizeof(ch_samp[0]));
#endif

          // From article-sdr-is-qs.pdf: Watch your Is and Qs:
          // FM = (Qn.In-1 - In.Qn-1)/(In.In-1 + Qn.Qn-1)
        vector<float> audio;
        //double sumE = 0.0;
        for (size_t idx=0; idx<ch_samp.size(); ++idx)
        {
          //sumE += pow(abs(ch_samp[idx]), 2.0);

            // Normalize signal amplitude
          ch_samp[idx] = ch_samp[idx] / abs(ch_samp[idx]);

          float i = ch_samp[idx].real();
          float q = ch_samp[idx].imag();
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
          float demod = arg(ch_samp[idx] * conj(prev_samp));
          prev_samp = ch_samp[idx];
#endif

          audio.push_back(demod);
          //int16_t samp = static_cast<int16_t>(demod * 4096);
          //outfile.write(reinterpret_cast<char*>(&samp), sizeof(samp));

          iold = i;
          qold = q;
        }
        //double meanE = sumE / ch_samp.size();
        //if (meanE > 5.0E-6)
        //if (meanE > 0.01)
        {
          //cout << "meanE=" << 10*log10(meanE/1.0E-3) << endl;
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
      Decimator<complex<float> > ch_filt;
      bool sql_open;
      ofstream outfile;

  };


  class Translate
  {
    public:
      Translate(float samp_rate, float offset)
        : n(0)
      {
        unsigned N = gcd(samp_rate, abs(offset));
        cout << "### Translate: offset=" << offset << " N=" << N << endl;
        exp_lut.resize(N);
        for (int i=0; i<N; ++i)
        {
          complex<float> e(0.0f, -2.0*M_PI*offset*i/samp_rate);
          exp_lut[i] = exp(e);
        }
      }

      void iq_received(vector<WbRxRtlTcp::Sample> &out,
                       const vector<WbRxRtlTcp::Sample> &in)
      {
        vector<WbRxRtlTcp::Sample>::const_iterator it;
        for (it = in.begin(); it != in.end(); ++it)
        {
          out.push_back(*it * exp_lut[n++]);
          if (n == exp_lut.size())
          {
            n = 0;
          }
        }
      }

    private:
      vector<complex<float> > exp_lut;
      unsigned n;

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
  };

}; /* anonymous namespace */


class Ddr::Channel
{
  public:
    Channel(AudioSink &audio_sink, int fq_offset)
      : fm_demod(audio_sink), trans(960000, fq_offset)
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
  : LocalRxBase(cfg, name), cfg(cfg), audio_pipe(0), channel(0), rtl(0)
{
} /* Ddr::Ddr */


Ddr::~Ddr(void)
{
  delete channel;
  delete rtl;
  delete audio_pipe;
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
  
  rtl = WbRxRtlTcp::instance(cfg, wbrx);
  if (rtl == 0)
  {
    cout << "*** ERROR: Could not create WBRX " << wbrx
         << " specified in receiver " << name() << endl;
    return false;
  }

  channel = new Channel(*audio_pipe, fq-rtl->centerFq());
  rtl->iqReceived.connect(mem_fun(*channel, &Channel::iq_received));

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

