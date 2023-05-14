/**
@file	 SquelchCtcss.h
@brief   A CTCSS squelch detector
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-02

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

#ifndef SQUELCH_CTCSS_INCLUDED
#define SQUELCH_CTCSS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <iostream>
#include <iomanip>
#include <vector>
#include <cmath>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioFilter.h>
#include <AsyncAudioSplitter.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ToneDetector.h"
#include "Squelch.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  A CTCSS squelch detector
@author Tobias Blomberg / SM0SVX
@date   2005-08-02

This squelch detector use tone detectors to detect the presence of one or more
CTCSS squelch tones. The actual tone detector is implemented outside of this
class.
*/
class SquelchCtcss : public Squelch
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "CTCSS";

    /**
     * @brief 	Default constuctor
     */
    explicit SquelchCtcss(void) {}

    /**
     * @brief 	Destructor
     */
    virtual ~SquelchCtcss(void)
    {
      delete m_splitter;
    }

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    virtual bool initialize(Async::Config& cfg, const std::string& rx_name)
    {
      typedef std::vector<float> FqList;
      FqList ctcss_fqs;
      cfg.getValue(rx_name, "CTCSS_FQ", ctcss_fqs);
      if (ctcss_fqs.empty())
      {
        std::cerr << "*** ERROR: Config variable " << rx_name
                  << "/CTCSS_FQ not set or is set to an illegal value.\n";
        return false;
      }

      int ctcss_mode = 0;
      cfg.getValue(rx_name, "CTCSS_MODE", ctcss_mode);

      float ctcss_snr_offset = 0.0f;
      cfg.getValue(rx_name, "CTCSS_SNR_OFFSET", ctcss_snr_offset);
      cfg.getValue(rx_name, "CTCSS_SNR_OFFSETS", m_ctcss_snr_offsets);

      float open_thresh = 15.0f;
      float close_thresh = 9.0f;
      if (cfg.getValue(rx_name, "CTCSS_THRESH", open_thresh))
      {
	close_thresh = open_thresh;
	std::cerr << "*** WARNING: Config variable " << rx_name
		  << "/CTCSS_THRESH is deprecated. Use the "
		  << "CTCSS_OPEN_THRESH and CTCSS_CLOSE_THRESH "
		  << "config variables instead.\n";
      }

      cfg.getValue(rx_name, "CTCSS_OPEN_THRESH", open_thresh);
      cfg.getValue(rx_name, "CTCSS_CLOSE_THRESH", close_thresh);
      if (close_thresh > open_thresh)
      {
        close_thresh = open_thresh;
      }

      std::map<float, float> open_threshs;
      std::map<float, float> close_threshs;
      for (auto ctcss_fq : ctcss_fqs)
      {
        if (m_ctcss_snr_offsets.count(ctcss_fq) == 0)
        {
          m_ctcss_snr_offsets[ctcss_fq] = ctcss_snr_offset;
        }
        //std::cout << "### f=" << ctcss_fq
        //          << " offset=" << m_ctcss_snr_offsets[ctcss_fq]
        //          << std::endl;
        open_threshs[ctcss_fq] = open_thresh + m_ctcss_snr_offsets[ctcss_fq];
        close_threshs[ctcss_fq] = close_thresh + m_ctcss_snr_offsets[ctcss_fq];
      }

      unsigned bpf_low = 60;
      cfg.getValue(rx_name, "CTCSS_BPF_LOW", bpf_low);
      if ((bpf_low < 50) || (bpf_low > 300))
      {
	std::cerr << "*** ERROR: "
		  << rx_name << "/CTCSS_BPF_LOW out of range ("
		  << bpf_low << "). Valid range is 50 to 300\n";
	return false;
      }
      
      unsigned bpf_high = 270;
      cfg.getValue(rx_name, "CTCSS_BPF_HIGH", bpf_high);
      if ((bpf_high < 50) || (bpf_high > 300))
      {
	std::cerr << "*** ERROR: "
		  << rx_name << "/CTCSS_BPF_HIGH out of range ("
		  << bpf_high << "). Valid range is 50 to 300\n";
	return false;
      }
      
      if (bpf_low >= bpf_high)
      {
	std::cerr << "*** ERROR: CTCSS_BPF_LOW (" << bpf_low << ") cannot be "
		  << "larger than or equal to CTCSS_BPF_HIGH ("
		  << bpf_high << ") for receiver " << rx_name << ".\n";
	return false;
      }

      cfg.getValue(rx_name, "CTCSS_EMIT_TONE_DETECTED", m_emit_tone_detected);

      m_splitter = new Async::AudioSplitter;

      for (FqList::const_iterator it = ctcss_fqs.begin();
           it != ctcss_fqs.end(); ++it)
      {
        float ctcss_fq = *it;

        ToneDetector *det = new ToneDetector(ctcss_fq, 8.0f);
        det->activated.connect(sigc::bind(
            sigc::mem_fun(*this, &SquelchCtcss::checkSignalDetected), det));
        det->snrUpdated.connect(sigc::bind(snrUpdated.make_slot(), ctcss_fq));
        Async::AudioSink *sink = det;

        m_dets.push_back(det);

        std::stringstream filter_spec;
        filter_spec << "BpBu8/" << bpf_low << "-" << bpf_high;

        switch (ctcss_mode)
        {
          case 1:
          {
            //std::cout << "### CTCSS mode: Neighbour bins\n";
            det->setDetectPeakThresh(open_threshs[ctcss_fq]);
            det->setUndetectPeakThresh(close_threshs[ctcss_fq]);
            break;
          }

          case 3:
          {
            //std::cout << "### CTCSS mode: Estimated SNR + Phase\n";
            //det->setDetectUseWindowing(false);
            det->setDetectBw(16.0f);
            det->setDetectPeakThresh(0.0f);
            //det->setDetectPeakToTotPwrThresh(0.6f);
            det->setDetectSnrThresh(open_threshs[ctcss_fq], bpf_high - bpf_low);
            det->setDetectStableCountThresh(1);
            det->setDetectPhaseBwThresh(2.0f, 2.0f);

            //det->setUndetectBw(8.0f);
            det->setUndetectUseWindowing(false);
            det->setUndetectPeakThresh(0.0f);
            //det->setUndetectPeakToTotPwrThresh(0.3f);
            det->setUndetectSnrThresh(close_threshs[ctcss_fq], bpf_high - bpf_low);
            det->setUndetectStableCountThresh(2);
            //det->setUndetectPhaseBwThresh(4.0f, 16.0f);

              // Set up CTCSS band pass filter
            Async::AudioFilter *filter =
              new Async::AudioFilter(filter_spec.str());
            filter->registerSink(det, true);
            sink = filter;
            break;
          }

          case 2:
          {
            //std::cout << "### CTCSS mode: Estimated SNR\n";
            //det->setDetectBw(6.0f);
            det->setDetectUseWindowing(false);
            det->setDetectPeakThresh(0.0f);
            //det->setDetectPeakToTotPwrThresh(0.6f);
            det->setDetectSnrThresh(open_threshs[ctcss_fq], bpf_high - bpf_low);
            det->setDetectStableCountThresh(1);

            //det->setUndetectBw(8.0f);
            det->setUndetectUseWindowing(false);
            det->setUndetectPeakThresh(0.0f);
            //det->setUndetectPeakToTotPwrThresh(0.3f);
            det->setUndetectSnrThresh(close_threshs[ctcss_fq], bpf_high - bpf_low);
            det->setUndetectStableCountThresh(2);

              // Set up CTCSS band pass filter
            Async::AudioFilter *filter =
              new Async::AudioFilter(filter_spec.str());
            filter->registerSink(det, true);
            sink = filter;
            break;
          }

          default:
          case 4:
          {
            static const float OVERLAP_PERCENT    = 75.0f;
            static const float TONE_FQ_TOLERANCE  = 0.75f;
            static const bool  USE_WINDOWING      = false;

           //std::ostringstream ss;
           //ss << "### CTCSS " << std::setw(5) << std::setprecision(1)
           //   << std::fixed << det->toneFq() << " mode 4: " << OVERLAP_PERCENT
           //   << "% overlap + Estimated SNR + tone frequency "
           //   << std::setprecision(2) << TONE_FQ_TOLERANCE << "% tolerance";
           //std::cout << ss.str() << std::endl;

            det->setDetectBw(16.0f);
            det->setDetectOverlapPercent(OVERLAP_PERCENT);
            det->setDetectDelay(100);
            det->setDetectToneFrequencyTolerancePercent(TONE_FQ_TOLERANCE);
            det->setDetectUseWindowing(USE_WINDOWING);
            det->setDetectPeakThresh(0.0f);
            det->setDetectSnrThresh(open_threshs[ctcss_fq], bpf_high - bpf_low);

            det->setUndetectBw(8.0f);
            det->setUndetectOverlapPercent(OVERLAP_PERCENT);
            det->setUndetectDelay(100);
            det->setUndetectUseWindowing(USE_WINDOWING);
            det->setUndetectPeakThresh(0.0f);
            det->setUndetectSnrThresh(close_threshs[ctcss_fq], bpf_high - bpf_low);

              // Set up CTCSS band pass filter
            Async::AudioFilter *filter =
              new Async::AudioFilter(filter_spec.str());
            filter->registerSink(det, true);
            sink = filter;
            break;
          }
        }

        m_splitter->addSink(sink, true);
      }

      cfg.getValue(rx_name, "CTCSS_DEBUG", m_debug);
      if (m_debug)
      {
        m_dbg_timer = std::unique_ptr<Async::Timer>(
            new Async::Timer(100, Async::Timer::TYPE_PERIODIC));
        m_dbg_timer->expired.connect(
            sigc::hide(sigc::mem_fun(*this, &SquelchCtcss::printDebug)));
      }

      return Squelch::initialize(cfg, rx_name);
    }

    /**
     * @brief 	Reset the squelch detector
     *
     *  Reset the squelch so that the detection process starts from
     *	the beginning again.
     */
    virtual void reset(void)
    {
      for (DetList::iterator it = m_dets.begin(); it != m_dets.end(); ++it)
      {
        (*it)->reset();
      }
      m_active_det = 0;
      Squelch::reset();
    }

    /**
      * @brief   Set the time a squelch open should be delayed
      * @param   delay The delay in milliseconds
      */
    virtual void setDelay(int delay)
    {
      for (DetList::iterator it = m_dets.begin(); it != m_dets.end(); ++it)
      {
        (*it)->setDetectDelay(delay);
      }
    }

    /**
     * @brief  A signal that is emitted when the tone SNR has been recalculated
     * @param  snr The current SNR
     * @param  fq  The CTCSS frequency
     *
     * This signal will be emitted as soon as a new SNR value for the CTCSS
     * tone has been calculated. The signal will only be emitted when
     * CTCSS_MODE is set to 2 or 3.
     */
    sigc::signal<void, float, float> snrUpdated;

  protected:
    /**
     * @brief 	Process the incoming samples in the squelch detector
     * @param 	samples A buffer containing samples
     * @param 	count The number of samples in the buffer
     * @return	Return the number of processed samples
     */
    int processSamples(const float *samples, int count)
    {
      return m_splitter->writeSamples(samples, count);
    }

    /**
     * @brief   Set the time the squelch should hang open after squelch close
     * @param   hang The number of milliseconds to hang
     */
    virtual void setCurrentHangtime(int hang)
    {
      for (DetList::iterator it = m_dets.begin(); it != m_dets.end(); ++it)
      {
        (*it)->setUndetectDelay(hang);
      }
    }

  private:
    typedef std::vector<ToneDetector*> DetList;

    DetList                       m_dets;
    Async::AudioSplitter*         m_splitter            = nullptr;
    ToneDetector*                 m_active_det          = nullptr;
    std::map<float, float>        m_ctcss_snr_offsets;
    bool                          m_debug               = false;
    std::unique_ptr<Async::Timer> m_dbg_timer           = nullptr;
    bool                          m_emit_tone_detected  = true;

    SquelchCtcss(const SquelchCtcss&);
    SquelchCtcss& operator=(const SquelchCtcss&);

    void checkSignalDetected(bool is_detected, ToneDetector *det)
    {
      if (m_debug)
      {
        printDebug();
      }
      std::ostringstream ss;
      ss << std::setprecision(1) << std::fixed << det->toneFq();
      if (det->toneFqEstimate() > 0.0f)
      {
        float fq_err = det->toneFqEstimate() - det->toneFq();
        fq_err = 100.0 * fq_err / det->toneFq();
        ss << std::showpos << fq_err;
      }
      ss << ":" << static_cast<int>(
            std::roundf(det->lastSnr() - m_ctcss_snr_offsets[det->toneFq()]));
      if (is_detected)
      {
        if (m_active_det == 0)
        {
          m_active_det = det;
          setSignalDetected(true, ss.str());
          if (m_emit_tone_detected)
          {
            toneDetected(det->toneFq());
          }
        }
      }
      else
      {
        if (m_active_det == det)
        {
          m_active_det = 0;
          //for (const auto& d : m_dets)
          //{
          //  if (d->isActivated())
          //  {
          //    //std::cout << "### ANOTHER TONE ALREADY ACTIVE" << std::endl;
          //    m_active_det = d;
          //    //setSignalDetected(false, ss.str());
          //    ss.str(std::string());
          //    ss << std::setprecision(1) << std::fixed << d->toneFq()
          //       << ":" << static_cast<int>(std::roundf(d->lastSnr()));
          //    setSignalDetected(true, ss.str());
          //    return;
          //  }
          //}
          setSignalDetected(false, ss.str());
        }
      }
    }

    void printDebug(void)
    {
      std::ostringstream os;
      os << rxName() << ":";
      for (auto det : m_dets)
      {
        float snr = det->lastSnr() - m_ctcss_snr_offsets[det->toneFq()];
        char stat = det->isActivated() ? '*' : ':';
        os << std::showpos << std::setfill(' ')
           << std::setw(4) << static_cast<int>(roundf(snr))
           << stat << std::fixed << std::setprecision(1) << std::noshowpos
           << det->toneFq();
        if (det->toneFqEstimate() > 0.0)
        {
          float fq_err = det->toneFqEstimate() - det->toneFq();
          os << std::showpos << std::setfill('_') << std::setw(5) << fq_err;
        }
      }
      std::cout << os.str() << std::endl;
    }
};  /* class SquelchCtcss */


//} /* namespace */

#endif /* SQUELCH_CTCSS_INCLUDED */



/*
 * This file has not been truncated
 */

