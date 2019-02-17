/**
@file   Ddr.cpp
@brief  A receiver class to handle digital drop receivers
@author Tobias Blomberg / SM0SVX
@date   2014-07-16

This file contains a class that handle local digital drop receivers.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2019 Tobias Blomberg / SM0SVX

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
#include <stdlib.h>
#include <sigc++/sigc++.h>

#include <cstring>
#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <complex>
#include <algorithm>
#include <deque>
#include <thread>
#include <mutex>
#include <condition_variable>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioSource.h>
#include <AsyncTcpClient.h>
#include <AsyncThreadSigCAsyncConnector.h>
#include <AsyncAudioThreadSource.h>
#include <AsyncThreadSigCSignal.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Ddr.h"
#include "WbRxRtlSdr.h"
#include "DdrFilterCoeffs.h"


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
  template <class T>
  class Decimator
  {
    public:
      Decimator(void) : dec_fact(0), p_Z(0), taps(0) {}

      Decimator(int dec_fact, const float *coeff, int taps)
        : dec_fact(dec_fact), p_Z(0), taps(taps)
      {
        setDecimatorParams(dec_fact, coeff, taps);
      }

      ~Decimator(void)
      {
        delete [] p_Z;
      }

      int decFact(void) const { return dec_fact; }

      void setDecimatorParams(int dec_fact, const float *coeff, int taps)
      {
        assert(taps >= dec_fact);

        set_coeff.assign(coeff, coeff + taps);
        this->dec_fact = dec_fact;
        this->coeff = set_coeff;
        this->taps = taps;

        delete [] p_Z;
        p_Z = new T[taps]{0};
      }

      void setGain(double gain_adjust)
      {
        coeff = set_coeff;
        for (vector<float>::iterator it=coeff.begin(); it!=coeff.end(); ++it)
        {
          *it *= pow(10.0, gain_adjust / 20.0);
        }
      }

      void decimate(vector<T> &out, const vector<T> &in)
      {
        int orig_count = in.size();

          // this implementation assumes in.size() is a multiple of factor_M
        assert(in.size() % dec_fact == 0);

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
          T sum(0);
          for (int tap = 0; tap < taps; tap++)
          {
            sum += coeff[tap] * p_Z[tap];
          }
          out.push_back(sum);     /* store sum */
          num_out++;
        }
        assert(num_out == orig_count / dec_fact);
      }

    private:
      int             dec_fact;
      T               *p_Z;
      int             taps;
      vector<float>   set_coeff;
      vector<float>   coeff;
  };

  template <class T>
  class DecimatorMS
  {
    public:
      virtual ~DecimatorMS(void) {}
      virtual void setGain(float new_gain) = 0;
      virtual int decFact(void) const = 0;
      virtual void decimate(vector<T> &out, const vector<T> &in) = 0;
  };

  template <class T>
  class DecimatorMS0 : public DecimatorMS<T>
  {
    public:
      DecimatorMS0(void) : gain(1.0f) {}
      virtual void setGain(float gain_db)
      {
        gain = pow(10.0, gain_db / 20.0);
      }
      virtual int decFact(void) const { return 1; }
      virtual void decimate(vector<T> &out, const vector<T> &in)
      {
        out.clear();
        out.reserve(in.size());
        for (size_t i=0; i<in.size(); ++i)
        {
          out.push_back(gain * in[i]);
        }
      }

    private:
      float gain;
  };

  template <class T>
  class DecimatorMS1 : public DecimatorMS<T>
  {
    public:
      DecimatorMS1(Decimator<T> &d1) : d1(d1) {}
      virtual void setGain(float gain_db) { d1.setGain(gain_db); }
      virtual int decFact(void) const { return d1.decFact(); }
      virtual void decimate(vector<T> &out, const vector<T> &in)
      {
        d1.decimate(out, in);
      }

    private:
      Decimator<T> &d1;
  };

  template <class T>
  class DecimatorMS2 : public DecimatorMS<T>
  {
    public:
      DecimatorMS2(Decimator<T> &d1, Decimator<T> &d2) : d1(d1), d2(d2) {}
      virtual void setGain(float gain_db) { d2.setGain(gain_db); }
      virtual int decFact(void) const { return d1.decFact() * d2.decFact(); }
      virtual void decimate(vector<T> &out, const vector<T> &in)
      {
        vector<T> dec_samp1;
        d1.decimate(dec_samp1, in);
        d2.decimate(out, dec_samp1);
      }

    private:
      Decimator<T> &d1, &d2;
  };

  template <class T>
  class DecimatorMS3 : public DecimatorMS<T>
  {
    public:
      DecimatorMS3(Decimator<T> &d1, Decimator<T> &d2, Decimator<T> &d3)
        : d1(d1), d2(d2), d3(d3) {}
      virtual void setGain(float gain_db) { d3.setGain(gain_db); }
      virtual int decFact(void) const
      {
        return d1.decFact() * d2.decFact() * d3.decFact();
      }
      virtual void decimate(vector<T> &out, const vector<T> &in)
      {
        vector<T> dec_samp1, dec_samp2;
        d1.decimate(dec_samp1, in);
        d2.decimate(dec_samp2, dec_samp1);
        d3.decimate(out, dec_samp2);
      }

    private:
      Decimator<T> &d1, &d2, &d3;
  };

  template <class T>
  class DecimatorMS4 : public DecimatorMS<T>
  {
    public:
      DecimatorMS4(Decimator<T> &d1, Decimator<T> &d2, Decimator<T> &d3,
                   Decimator<T> &d4)
        : d1(d1), d2(d2), d3(d3), d4(d4) {}
      virtual void setGain(float gain_db) { d4.setGain(gain_db); }
      virtual int decFact(void) const
      {
        return d1.decFact() * d2.decFact() * d3.decFact() * d4.decFact();
      }
      virtual void decimate(vector<T> &out, const vector<T> &in)
      {
        vector<T> dec_samp1, dec_samp2, dec_samp3;
        d1.decimate(dec_samp1, in);
        d2.decimate(dec_samp2, dec_samp1);
        d3.decimate(dec_samp3, dec_samp2);
        d4.decimate(out, dec_samp3);
      }

    private:
      Decimator<T> &d1, &d2, &d3, &d4;
  };

  template <class T>
  class DecimatorMS5 : public DecimatorMS<T>
  {
    public:
      DecimatorMS5(Decimator<T> &d1, Decimator<T> &d2, Decimator<T> &d3,
                   Decimator<T> &d4, Decimator<T> &d5)
        : d1(d1), d2(d2), d3(d3), d4(d4), d5(d5) {}
      virtual void setGain(float gain_db) { d5.setGain(gain_db); }
      virtual int decFact(void) const
      {
        return d1.decFact() * d2.decFact() * d3.decFact() *
               d4.decFact() * d5.decFact();
      }
      virtual void decimate(vector<T> &out, const vector<T> &in)
      {
        vector<T> dec_samp1, dec_samp2, dec_samp3, dec_samp4;
        d1.decimate(dec_samp1, in);
        d2.decimate(dec_samp2, dec_samp1);
        d3.decimate(dec_samp3, dec_samp2);
        d4.decimate(dec_samp4, dec_samp3);
        d5.decimate(out, dec_samp4);
      }

    private:
      Decimator<T> &d1, &d2, &d3, &d4, &d5;
  };


  class Translate
  {
    public:
      Translate(unsigned samp_rate, int offset)
        : samp_rate(samp_rate), n(0)
      {
        setOffset(offset);
      }

      void setOffset(int offset)
      {
        n = 0;
        exp_lut.clear();
        if (offset == 0)
        {
          return;
        }
        unsigned N = samp_rate / gcd(samp_rate, abs(offset));
        //cout << "### Translate: offset=" << offset << " N=" << N << endl;
        exp_lut.resize(N);
        for (unsigned i=0; i<N; ++i)
        {
          complex<float> e(0.0f, -2.0*M_PI*offset*i/samp_rate);
          exp_lut[i] = exp(e);
        }
      }

      void iq_received(vector<WbRxRtlSdr::Sample> &out,
                       const vector<WbRxRtlSdr::Sample> &in)
      {
        if (exp_lut.size() > 0)
        {
          out.clear();
          out.reserve(in.size());
          vector<WbRxRtlSdr::Sample>::const_iterator it;
          for (it = in.begin(); it != in.end(); ++it)
          {
            out.push_back(*it * exp_lut[n]);
            if (++n == exp_lut.size())
            {
              n = 0;
            }
          }
        }
        else
        {
          out = in;
        }
      }

    private:
      unsigned samp_rate;
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
  }; /* Translate */


  class AGC
  {
    public:
      AGC(float attack=1.0e1, float decay=1.0e-2, float max_gain=2.0e2,
          float reference=0.25f)
        : m_attack(attack), m_decay(decay), m_max_gain(max_gain),
          m_reference(reference), m_gain(1.0f)
      {

      }

      void setReference(float reference) { m_reference = reference; }
      void setDecay(float decay) { m_decay = decay; }
      void setAttack(float attack) { m_attack = attack; }

      void iq_received(vector<WbRxRtlSdr::Sample> &out,
                       const vector<WbRxRtlSdr::Sample> &in)
      {
        out.clear();
        out.reserve(in.size());
        float P = 0.0f;
        for (vector<WbRxRtlSdr::Sample>::const_iterator it = in.begin();
             it != in.end();
             ++it)
        {
          const WbRxRtlSdr::Sample &samp = *it;
          WbRxRtlSdr::Sample osamp = m_gain * samp;
          P = osamp.real() * osamp.real() + osamp.imag() * osamp.imag();
          out.push_back(osamp);

          float err = m_reference - P;
          float rate;
          if (err > 0.0f)
          {
            rate = m_decay * err;
          }
          else
          {
            rate = m_attack * err;
          }
          m_gain += rate;
          if (m_gain < 0.0f)
          {
            m_gain = 0.0f;
          }
          else if (m_gain > m_max_gain)
          {
            m_gain = m_max_gain;
          }
        }
        //cout << "### P=" << P << "  m_gain=" << m_gain << endl;
      }

    private:
      float   m_attack;
      float   m_decay;
      float   m_max_gain;
      float   m_reference;
      float   m_gain;

  }; /* AGC */


  class Demodulator : public Async::AudioThreadSource
  {
    public:
      virtual ~Demodulator(void) {}
      virtual void iq_received(const vector<WbRxRtlSdr::Sample>& samples) = 0;
  };


  class DemodulatorFm : public Demodulator
  {
    public:
      DemodulatorFm(unsigned samp_rate, double max_dev)
        : iold(1.0f), qold(1.0f),
          audio_dec(2, coeff_dec_audio_32k_16k, coeff_dec_audio_32k_16k_cnt),
          dec(0)
      {
        setDemodParams(samp_rate, max_dev);
      }

      ~DemodulatorFm(void)
      {
        delete dec;
        dec = 0;
      }

      void setDemodParams(unsigned samp_rate, double max_dev)
      {
        delete dec;
        dec = 0;

        if (samp_rate == 16000)
        {
          dec = new DecimatorMS0<float>;
        }
        else if (samp_rate == 32000)
        {
          dec = new DecimatorMS1<float>(audio_dec);
        }
        else if (samp_rate == 160000)
        {
          audio_dec_wb.setDecimatorParams(5, coeff_dec_160k_32k, 
                                          coeff_dec_160k_32k_cnt);
          dec = new DecimatorMS2<float>(audio_dec_wb, audio_dec);
        }
        else if (samp_rate == 192000)
        {
          audio_dec_wb.setDecimatorParams(6, coeff_dec_192k_32k, 
                                          coeff_dec_192k_32k_cnt);
          dec = new DecimatorMS2<float>(audio_dec_wb, audio_dec);
        }

        assert((dec != 0) &&
               "DemodulatorFm::setDemodParams: Unsupported sampling rate");

          // Adjust the gain so that the maximum deviation corresponds
          // to a peak audio amplitude of 1.0, minus headroom.
        double adj = static_cast<double>(samp_rate) / (2.0 * M_PI * max_dev);
        adj /= 2.0; // Default to 6dB headroom
        double adj_db = 20.0 * log10(adj);
        dec->setGain(adj_db);
      }

      void iq_received(const vector<WbRxRtlSdr::Sample>& samples)
      {
          // From article-sdr-is-qs.pdf: Watch your Is and Qs:
          //   FM = (Qn.In-1 - In.Qn-1)/(In.In-1 + Qn.Qn-1)
          //
          // A more indepth report:
          //   Implementation of FM demodulator algorithms on a
          //   high performance digital signal processor
        vector<float> audio;
        for (size_t idx=0; idx<samples.size(); ++idx)
        {
          complex<float> samp = samples[idx];

            // Normalize signal amplitude
          samp = samp / abs(samp);

            // Mixed demodulator (delay demodulator + phase adapter demodulator)
          float i = samp.real();
          float q = samp.imag();
          double demod = atan2(q*iold - i*qold, i*iold + q*qold);
          iold = i;
          qold = q;

          audio.push_back(demod);
        }
        vector<float> dec_audio;
        dec->decimate(dec_audio, audio);
        writeSamples(&dec_audio[0], dec_audio.size());
      }

    private:
      float iold;
      float qold;
      Decimator<float> audio_dec_wb;
      Decimator<float> audio_dec;
      DecimatorMS<float> *dec;
  };


  class DemodulatorAm : public Demodulator
  {
    public:
      DemodulatorAm(void)
      {
        agc.setAttack(1.0e-0);
        agc.setDecay(1.0e-2);
        agc.setReference(1);
      }

      void iq_received(const vector<WbRxRtlSdr::Sample>& samples)
      {
        vector<WbRxRtlSdr::Sample> gain_adjusted;
        agc.iq_received(gain_adjusted, samples);

        vector<float> audio;
        for (size_t idx=0; idx<gain_adjusted.size(); ++idx)
        {
          complex<float> samp = gain_adjusted[idx];
          float demod = abs(samp);
          audio.push_back(demod);
        }
        sinkWriteSamples(&audio[0], audio.size());
      }

    private:
      AGC              agc;
  };


//#define USE_SSB_PHASE_DEMOD
#ifdef USE_SSB_PHASE_DEMOD
  class DemodulatorSsb : public Demodulator
  {
    public:
      DemodulatorSsb(unsigned samp_rate)
        : I(coeff_hilbert_cnt/2, 0),
          hilbert(1, coeff_hilbert, coeff_hilbert_cnt),
          use_lsb(false)
      {
      }

      void useLsb(bool use)
      {
        use_lsb = use;
      }

      void iq_received(const vector<WbRxRtlSdr::Sample>& samples)
      {
        vector<float> Q, Qh, audio;
        Q.reserve(samples.size());
        for (vector<WbRxRtlSdr::Sample>::const_iterator it = samples.begin();
             it != samples.end();
             ++it)
        {
          I.push_back(it->real());
          Q.push_back(it->imag());
        }
        hilbert.decimate(Qh, Q);
        audio.reserve(Qh.size());
        for (size_t idx=0; idx<Qh.size(); ++idx)
        {
          float demod;
          if (use_lsb)
          {
            demod = I[idx] + Qh[idx];
          }
          else
          {
            demod = I[idx] - Qh[idx];
          }
          audio.push_back(demod);
        }
        I.erase(I.begin(), I.begin() + Qh.size());
        sinkWriteSamples(&audio[0], audio.size());
      }

    private:
      deque<float>      I;
      Decimator<float>  hilbert;
      bool              use_lsb;
  };

#else

  class DemodulatorSsb : public Demodulator
  {
    public:
      DemodulatorSsb(unsigned samp_rate)
        : trans(samp_rate, -2000)
      {
      }

      void useLsb(bool lsb)
      {
        trans.setOffset(lsb ? 2000 : -2000);
      }

      void iq_received(const vector<WbRxRtlSdr::Sample>& samples)
      {
        vector<WbRxRtlSdr::Sample> gain_adjusted;
        agc.iq_received(gain_adjusted, samples);

        vector<WbRxRtlSdr::Sample> translated;
        trans.iq_received(translated, gain_adjusted);

        vector<float> audio;
        audio.reserve(gain_adjusted.size());
        for (vector<WbRxRtlSdr::Sample>::const_iterator it = translated.begin();
             it != translated.end();
             ++it)
        {
          float demod = it->real();
          audio.push_back(demod);
        }
        sinkWriteSamples(&audio[0], audio.size());
      }

    private:
      Translate         trans;
      AGC               agc;
  };
#endif


  class DemodulatorCw : public Demodulator
  {
    public:
      DemodulatorCw(unsigned samp_rate)
        : trans(samp_rate, 600)
      {
        agc.setAttack(1.0e+2);
        agc.setDecay(4.0e-2);
        agc.setReference(0.05);
      }

      void iq_received(const vector<WbRxRtlSdr::Sample>& samples)
      {
        vector<WbRxRtlSdr::Sample> gain_adjusted;
        agc.iq_received(gain_adjusted, samples);

        vector<WbRxRtlSdr::Sample> translated;
        trans.iq_received(translated, gain_adjusted);
        vector<float> audio;
        audio.reserve(translated.size());
        for (vector<WbRxRtlSdr::Sample>::const_iterator it = translated.begin();
             it != translated.end();
             ++it)
        {
          float demod = it->real();
          audio.push_back(demod);
        }
        sinkWriteSamples(&audio[0], audio.size());
      }

    private:
      Translate         trans;
      AGC               agc;
  };


  class Channelizer
  {
    public:
      typedef enum
      {
        BW_WIDE, BW_20K, BW_10K, BW_6K, BW_3K, BW_500
      } Bandwidth;

      virtual ~Channelizer(void) {}
      virtual void setBw(Bandwidth bw) = 0;
      virtual unsigned chSampRate(void) const = 0;
      virtual void iq_received(vector<WbRxRtlSdr::Sample> &out,
                               const vector<WbRxRtlSdr::Sample> &in) = 0;

      sigc::signal<void, const std::vector<RtlTcp::Sample>&> preDemod;
  };

  class Channelizer960 : public Channelizer
  {
    public:
      Channelizer960(void)
        : dec_960k_192k(5, coeff_dec_960k_192k, coeff_dec_960k_192k_cnt),
          dec_192k_64k( 3, coeff_dec_192k_64k,  coeff_dec_192k_64k_cnt ),
          dec_64k_32k(  2, coeff_dec_64k_32k,   coeff_dec_64k_32k_cnt  ),
          dec_192k_48k( 4, coeff_dec_192k_48k,  coeff_dec_192k_48k_cnt ),
          dec_48k_16k(  3, coeff_dec_48k_16k,   coeff_dec_48k_16k_cnt  ),
          ch_filt(      1, coeff_25k_channel,   coeff_25k_channel_cnt  ),
          ch_filt_narr( 1, coeff_12k5_channel,  coeff_12k5_channel_cnt ),
          ch_filt_6k(   1, coeff_nbam_channel,  coeff_nbam_channel_cnt ),
          ch_filt_3k(   1, coeff_ssb_channel,   coeff_ssb_channel_cnt  ),
          ch_filt_500(  1, coeff_cw_channel,    coeff_cw_channel_cnt   ),
          dec(0)
      {
        setBw(BW_20K);
      }
      virtual ~Channelizer960(void)
      {
        delete dec;
        dec = 0;
      }

      virtual void setBw(Bandwidth bw)
      {
        delete dec;
        dec = 0;
        switch (bw)
        {
          case BW_WIDE:
            dec = new DecimatorMS1<complex<float> >(dec_960k_192k);
            return;
          case BW_20K:
            dec = new DecimatorMS4<complex<float> >(dec_960k_192k,
                                                    dec_192k_64k,
                                                    dec_64k_32k, 
                                                    ch_filt);
            return;
          case BW_10K:
            dec = new DecimatorMS4<complex<float> >(dec_960k_192k,
                                                    dec_192k_48k,
                                                    dec_48k_16k,
                                                    ch_filt_narr);
            return;
          case BW_6K:
            dec = new DecimatorMS4<complex<float> >(dec_960k_192k,
                                                    dec_192k_48k,
                                                    dec_48k_16k,
                                                    ch_filt_6k);
            return;
          case BW_3K:
            dec = new DecimatorMS4<complex<float> >(dec_960k_192k,
                                                    dec_192k_48k,
                                                    dec_48k_16k,
                                                    ch_filt_3k);
            return;
          case BW_500:
            dec = new DecimatorMS4<complex<float> >(dec_960k_192k,
                                                    dec_192k_48k,
                                                    dec_48k_16k,
                                                    ch_filt_500);
            return;
        }
        assert(!"Channelizer::setBw: Unknown bandwidth");
      }

      virtual unsigned chSampRate(void) const
      {
        return 960000 / dec->decFact();
      }

      virtual void iq_received(vector<WbRxRtlSdr::Sample> &out,
                               const vector<WbRxRtlSdr::Sample> &in)
      {
        dec->decimate(out, in);
        preDemod(out);
      }

    private:
      Decimator<complex<float> >    dec_960k_192k;
      Decimator<complex<float> >    dec_192k_64k;
      Decimator<complex<float> >    dec_64k_32k;
      Decimator<complex<float> >    dec_192k_48k;
      Decimator<complex<float> >    dec_48k_16k;
      Decimator<complex<float> >    ch_filt;
      Decimator<complex<float> >    ch_filt_narr;
      Decimator<complex<float> >    ch_filt_6k;
      Decimator<complex<float> >    ch_filt_3k;
      Decimator<complex<float> >    ch_filt_500;
      DecimatorMS<complex<float> >  *dec;
  };

  class Channelizer2400 : public Channelizer
  {
    public:
      Channelizer2400(void)
        : dec_2400k_800k(3, coeff_dec_2400k_800k, coeff_dec_2400k_800k_cnt),
          dec_800k_160k (5, coeff_dec_800k_160k,  coeff_dec_800k_160k_cnt ),
          dec_160k_32k  (5, coeff_dec_160k_32k,   coeff_dec_160k_32k_cnt  ),
          dec_32k_16k   (2, coeff_dec_32k_16k,    coeff_dec_32k_16k_cnt   ),
          ch_filt       (1, coeff_25k_channel,    coeff_25k_channel_cnt   ),
          ch_filt_narr  (1, coeff_12k5_channel,   coeff_12k5_channel_cnt  ),
          ch_filt_6k    (1, coeff_nbam_channel,   coeff_nbam_channel_cnt  ),
          ch_filt_3k    (1, coeff_ssb_channel,    coeff_ssb_channel_cnt   ),
          ch_filt_500   (1, coeff_cw_channel,     coeff_cw_channel_cnt    ),
          dec(0)
      {
        setBw(BW_20K);
      }
      virtual ~Channelizer2400(void)
      {
        delete dec;
        dec = 0;
      }

      virtual void setBw(Bandwidth bw)
      {
        delete dec;
        dec = 0;

        switch (bw)
        {
          case BW_WIDE:
            dec = new DecimatorMS2<complex<float> >(dec_2400k_800k,
                                                    dec_800k_160k);
            return;
          case BW_20K:
            dec = new DecimatorMS4<complex<float> >(dec_2400k_800k,
                                                    dec_800k_160k,
                                                    dec_160k_32k, 
                                                    ch_filt);
            return;
          case BW_10K:
            dec = new DecimatorMS5<complex<float> >(dec_2400k_800k,
                                                    dec_800k_160k,
                                                    dec_160k_32k, 
                                                    dec_32k_16k,
                                                    ch_filt_narr);
            return;
          case BW_6K:
            dec = new DecimatorMS5<complex<float> >(dec_2400k_800k,
                                                    dec_800k_160k,
                                                    dec_160k_32k, 
                                                    dec_32k_16k,
                                                    ch_filt_6k);
            return;
          case BW_3K:
            dec = new DecimatorMS5<complex<float> >(dec_2400k_800k,
                                                    dec_800k_160k,
                                                    dec_160k_32k,
                                                    dec_32k_16k,
                                                    ch_filt_3k);
            return;
          case BW_500:
            dec = new DecimatorMS5<complex<float> >(dec_2400k_800k,
                                                    dec_800k_160k,
                                                    dec_160k_32k,
                                                    dec_32k_16k,
                                                    ch_filt_500);
            return;
        }
        assert(!"Channelizer::setBw: Unknown bandwidth");
      }

      virtual unsigned chSampRate(void) const
      {
        return 2400000 / dec->decFact();
      }

      virtual void iq_received(vector<WbRxRtlSdr::Sample> &out,
                               const vector<WbRxRtlSdr::Sample> &in)
      {
        dec->decimate(out, in);
        preDemod(out);
      }

    private:
      Decimator<complex<float> >    dec_2400k_800k;
      Decimator<complex<float> >    dec_800k_160k;
      Decimator<complex<float> >    dec_160k_32k;
      Decimator<complex<float> >    dec_32k_16k;
      Decimator<complex<float> >    ch_filt;
      Decimator<complex<float> >    ch_filt_narr;
      Decimator<complex<float> >    ch_filt_6k;
      Decimator<complex<float> >    ch_filt_3k;
      Decimator<complex<float> >    ch_filt_500;
      DecimatorMS<complex<float> >  *dec;
  };

}; /* anonymous namespace */


class Ddr::Channel : public sigc::trackable, public Async::AudioSource
{
  public:
    Channel(const std::string& name, int fq_offset, unsigned sample_rate)
      : m_name(name), m_sample_rate(sample_rate), m_channelizer(0),
        m_fm_demod(32000, 5000.0), m_ssb_demod(16000), m_cw_demod(16000),
        m_demod(0), m_trans(sample_rate, fq_offset), m_enabled(false),
        m_ch_offset(0), m_fq_offset(fq_offset)
    {
    }

    ~Channel(void)
    {
      disable();
      delete m_channelizer;
    }

    bool initialize(void)
    {
      if (m_sample_rate == 2400000)
      {
        m_channelizer = new Channelizer2400;
      }
      else if (m_sample_rate == 960000)
      {
        m_channelizer = new Channelizer960;
      }
      else
      {
        cout << "*** ERROR: Unsupported tuner sampling rate " << m_sample_rate
             << ". Legal values are: 960000 and 2400000\n";
        return false;
      }
      setModulation(Modulation::MOD_FM);
      m_channelizer->preDemod.connect(preDemod.make_slot());
      return true;
    }

    void setFqOffset(int fq_offset)
    {
      std::lock_guard<std::mutex> lk(m_process_mu);
      m_fq_offset = fq_offset;
      m_trans.setOffset(fq_offset - m_ch_offset);
    }

    void setModulation(Modulation::Type mod)
    {
      std::lock_guard<std::mutex> lk(m_process_mu);
      m_demod = 0;
      m_ch_offset = 0;
      switch (mod)
      {
        case Modulation::MOD_FM:
          m_channelizer->setBw(Channelizer::BW_20K);
          m_fm_demod.setDemodParams(m_channelizer->chSampRate(), 5000);
          m_demod = &m_fm_demod;
          break;
        case Modulation::MOD_NBFM:
          m_channelizer->setBw(Channelizer::BW_10K);
          m_fm_demod.setDemodParams(m_channelizer->chSampRate(), 2500);
          m_demod = &m_fm_demod;
          break;
        case Modulation::MOD_WBFM:
          m_channelizer->setBw(Channelizer::BW_WIDE);
          m_fm_demod.setDemodParams(m_channelizer->chSampRate(), 75000);
          m_demod = &m_fm_demod;
          break;
        case Modulation::MOD_AM:
          m_channelizer->setBw(Channelizer::BW_10K);
          m_demod = &m_am_demod;
          break;
        case Modulation::MOD_NBAM:
          m_channelizer->setBw(Channelizer::BW_6K);
          m_demod = &m_am_demod;
          break;
        case Modulation::MOD_USB:
#ifdef USE_SSB_PHASE_DEMOD
          m_channelizer->setBw(Channelizer::BW_6K);
#else
          m_channelizer->setBw(Channelizer::BW_3K);
          m_ch_offset = -2000;
#endif
          m_ssb_demod.useLsb(false);
          m_demod = &m_ssb_demod;
          break;
        case Modulation::MOD_LSB:
#ifdef USE_SSB_PHASE_DEMOD
          m_channelizer->setBw(Channelizer::BW_6K);
#else
          m_channelizer->setBw(Channelizer::BW_3K);
          m_ch_offset = 2000;
#endif
          m_ssb_demod.useLsb(true);
          m_demod = &m_ssb_demod;
          break;
        case Modulation::MOD_CW:
          m_channelizer->setBw(Channelizer::BW_500);
          m_demod = &m_cw_demod;
          break;
        case Modulation::MOD_WBCW:
          m_channelizer->setBw(Channelizer::BW_3K);
          m_demod = &m_cw_demod;
          break;
        case Modulation::MOD_UNKNOWN:
          break;
      }
      m_trans.setOffset(m_fq_offset - m_ch_offset);
      assert((m_demod != 0) && "Channel::setModulation: Unknown modulation");
      setHandler(m_demod);
    }

    unsigned chSampRate(void) const
    {
      std::lock_guard<std::mutex> lk(m_process_mu);
      return m_channelizer->chSampRate();
    }

    void iq_received(const vector<WbRxRtlSdr::Sample>& samples)
    {
      if (m_enabled)
      {
        {
          std::lock_guard<std::mutex> lk(m_iq_buf_mu);
          m_iq_buf.insert(m_iq_buf.end(), samples.begin(), samples.end());
        }
        m_cond.notify_one();
      }
    };

    void operator()(void)
    {
#ifdef _GNU_SOURCE
      pthread_setname_np(pthread_self(), m_name.c_str());
#endif
      for (;;)
      {
        std::vector<WbRxRtlSdr::Sample> samples, translated, channelized;
        {
          std::unique_lock<std::mutex> lk(m_iq_buf_mu);
          m_cond.wait(lk, [this]{ return !m_iq_buf.empty() || !m_enabled; });
          if (!m_enabled) return;
          samples.swap(m_iq_buf);
        }
        {
          std::lock_guard<std::mutex> lk(m_process_mu);
          m_trans.iq_received(translated, samples);
          m_channelizer->iq_received(channelized, translated);
          m_demod->iq_received(channelized);
        }
      }
    }

    void enable(void)
    {
      if (!m_th.joinable())
      {
        m_enabled = true;
        m_th = std::thread(std::ref(*this));
      }
    }

    void disable(void)
    {
      if (m_th.joinable())
      {
        {
          std::lock_guard<std::mutex> lk(m_iq_buf_mu);
          m_enabled = false;
        }
        m_cond.notify_one();
        m_th.join();
      }
    }

    bool isEnabled(void) const { return m_enabled; }

    ThreadSigCSignal<void, const std::vector<RtlTcp::Sample>> preDemod;

  private:
    const std::string               m_name;
    unsigned                        m_sample_rate;
    Channelizer *                   m_channelizer;
    DemodulatorFm                   m_fm_demod;
    DemodulatorAm                   m_am_demod;
    DemodulatorSsb                  m_ssb_demod;
    DemodulatorCw                   m_cw_demod;
    Demodulator *                   m_demod;
    Translate                       m_trans;
    bool                            m_enabled;
    int                             m_ch_offset;
    int                             m_fq_offset;
    std::vector<WbRxRtlSdr::Sample> m_iq_buf;
    std::mutex                      m_iq_buf_mu;
    std::condition_variable         m_cond;
    std::thread                     m_th;
    mutable std::mutex              m_process_mu;
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

Ddr::DdrMap Ddr::ddr_map;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

Ddr *Ddr::find(const std::string &name)
{
  DdrMap::iterator it = ddr_map.find(name);
  if (it != ddr_map.end())
  {
    return (*it).second;
  }
  return 0;
} /* Ddr::find */


Ddr::Ddr(Config &cfg, const std::string& name)
  : LocalRxBase(cfg, name), cfg(cfg), channel(0), rtl(0),
    fq(0)
{
} /* Ddr::Ddr */


Ddr::~Ddr(void)
{
  if (rtl != 0)
  {
    rtl->unregisterDdr(this);
    rtl = 0;
  }

  DdrMap::iterator it = ddr_map.find(name());
  if (it != ddr_map.end())
  {
    ddr_map.erase(it);
  }

  delete channel;
} /* Ddr::~Ddr */


bool Ddr::initialize(void)
{
  DdrMap::iterator it = ddr_map.find(name());
  if (it != ddr_map.end())
  {
    cout << "*** ERROR: The name for a Digital Drop Receiver (DDR) must be "
         << "unique. There already is a receiver named \"" << name()
         << "\".\n";
    return false;
  }
  ddr_map[name()] = this;

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

  rtl = WbRxRtlSdr::instance(cfg, wbrx);
  if (rtl == 0)
  {
    cout << "*** ERROR: Could not create WBRX " << wbrx
         << " specified in receiver " << name() << endl;
    return false;
  }
  rtl->registerDdr(this);

  channel = new Channel(name(), fq-rtl->centerFq(), rtl->sampleRate());
  if (!channel->initialize())
  {
    cout << "*** ERROR: Could not initialize channel object for receiver "
         << name() << endl;
    delete channel;
    channel = 0;
    return false;
  }
  channel->preDemod.connect(preDemod.make_slot());
  rtl->iqReceived.connect(sigc::mem_fun(*channel, &Channel::iq_received));
  rtl->readyStateChanged.connect(readyStateChanged.make_slot());

  string modstr("FM");
  cfg.getValue(name(), "MODULATION", modstr);
  Modulation::Type mod = Modulation::fromString(modstr);
  if (mod != Modulation::MOD_UNKNOWN)
  {
    channel->setModulation(mod);
  }
  else
  {
    cout << "*** ERROR: Unknown modulation " << modstr
         << " specified in receiver " << name() << endl;
    delete channel;
    channel = 0;
    return false;
  }

  if (!LocalRxBase::initialize())
  {
    delete channel;
    channel = 0;
    return false;
  }

  tunerFqChanged(rtl->centerFq());

  channel->enable();

  return true;
} /* Ddr:initialize */


void Ddr::tunerFqChanged(uint32_t center_fq)
{
  updateFqOffset();
} /* Ddr::tunerFqChanged */


unsigned Ddr::preDemodSampleRate(void) const
{
  return channel->chSampRate();
} /* Ddr::preDemodSampleRate */


bool Ddr::isReady(void) const
{
  return (rtl != 0) && rtl->isReady();
} /* Ddr::isReady */


void Ddr::setFq(unsigned fq)
{
  this->fq = fq;
  rtl->updateDdrFq(this);
  updateFqOffset();
} /* Ddr::setFq */


void Ddr::setModulation(Modulation::Type mod)
{
  channel->setModulation(mod);
} /* Ddr::setModulation */



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
  return 16000;
} /* Ddr::audioSampleRate */


Async::AudioSource *Ddr::audioSource(void)
{
  return channel;
} /* Ddr::audioSource */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Ddr::updateFqOffset(void)
{
  if ((channel == 0) || (rtl == 0))
  {
    return;
  }

  double new_offset = fq - rtl->centerFq();
  if (abs(new_offset) > (rtl->sampleRate() / 2)-12500)
  {
    if (channel->isEnabled())
    {
      cout << "*** WARNING: Could not fit DDR \"" << name() 
           << "\" with frequency " << fq << "Hz into tuner " 
           << rtl->name() << endl;
      channel->disable();
    }
    return;
  }
  channel->setFqOffset(new_offset);
  channel->enable();
} /* Ddr::updateFqOffset */



/*
 * This file has not been truncated
 */

