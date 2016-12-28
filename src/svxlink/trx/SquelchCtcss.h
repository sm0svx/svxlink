/**
@file	 SquelchCtcss.h
@brief   A CTCSS squelch detector
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-02

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2011 Tobias Blomberg / SM0SVX

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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioFilter.h>


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
@brief	A CTCSS squelch detector
@author Tobias Blomberg / SM0SVX
@date   2005-08-02

This squelch detector use a tone detector to detect the presence of a CTCSS
squelch tone. The actual tone detector is implemented outside of this class.
*/
class SquelchCtcss : public Squelch
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit SquelchCtcss(void) : det(0), filter(0), sink(0) {}

    /**
     * @brief 	Destructor
     */
    virtual ~SquelchCtcss(void)
    {
      delete det;
      delete filter;
    }

    /**
     * @brief 	Initialize the squelch detector
     * @param 	cfg A previsously initialized config object
     * @param 	rx_name The name of the RX (config section name)
     * @return	Returns \em true on success or else \em false
     */
    virtual bool initialize(Async::Config& cfg, const std::string& rx_name)
    {
      float ctcss_fq = 0.0f;
      cfg.getValue(rx_name, "CTCSS_FQ", ctcss_fq);
      if (ctcss_fq <= 0)
      {
	std::cerr << "*** ERROR: Config variable " << rx_name
      	     << "/CTCSS_FQ (" << ctcss_fq
      	     << ") not set or is set to an illegal value.\n";
	return false;
      }

      int ctcss_mode = 0;
      cfg.getValue(rx_name, "CTCSS_MODE", ctcss_mode);
      
      float ctcss_snr_offset = 0.0f;
      cfg.getValue(rx_name, "CTCSS_SNR_OFFSET", ctcss_snr_offset);
      
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

      if (cfg.getValue(rx_name, "CTCSS_OPEN_THRESH", open_thresh))
      {
	close_thresh = open_thresh;
	cfg.getValue(rx_name, "CTCSS_CLOSE_THRESH", close_thresh);
      }

      open_thresh += ctcss_snr_offset;
      close_thresh += ctcss_snr_offset;
      
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
      
#if 0
      std::cout << "### mode=" << ctcss_mode
		<< " open=" << open_thresh
		<< " close=" << close_thresh
		<< " low=" << bpf_low
		<< " high=" << bpf_high
		<< std::endl;
#endif
		
      det = new ToneDetector(ctcss_fq, 8.0f);
      det->activated.connect(mem_fun(*this, &SquelchCtcss::setSignalDetected));
      det->snrUpdated.connect(snrUpdated.make_slot());
      sink = det;

      std::stringstream filter_spec;
      filter_spec << "BpBu8/" << bpf_low << "-" << bpf_high;
	  
      switch (ctcss_mode)
      {
        case 1:
          //std::cout << "### CTCSS mode: Neighbour bins\n";
	  det->setDetectPeakThresh(open_thresh);
	  det->setUndetectPeakThresh(close_thresh);
          break;

        case 3:
          //std::cout << "### CTCSS mode: Estimated SNR + Phase\n";
          //det->setDetectUseWindowing(false);
          det->setDetectBw(16.0f);
          det->setDetectPeakThresh(0.0f);
          //det->setDetectPeakToTotPwrThresh(0.6f);
          det->setDetectSnrThresh(open_thresh, bpf_high - bpf_low);
          det->setDetectStableCountThresh(1);
          det->setDetectPhaseBwThresh(2.0f, 2.0f);

          //det->setUndetectBw(8.0f);
          det->setUndetectUseWindowing(false);
          det->setUndetectPeakThresh(0.0f);
          //det->setUndetectPeakToTotPwrThresh(0.3f);
          det->setUndetectSnrThresh(close_thresh, bpf_high - bpf_low);
          det->setUndetectStableCountThresh(2);
          //det->setUndetectPhaseBwThresh(4.0f, 16.0f);

            // Set up CTCSS band pass filter
          filter = new Async::AudioFilter(filter_spec.str());
          filter->registerSink(det);
          sink = filter;
          break;

        case 2:
        default:
          //std::cout << "### CTCSS mode: Estimated SNR\n";
          //det->setDetectBw(6.0f);
          det->setDetectUseWindowing(false);
          det->setDetectPeakThresh(0.0f);
          //det->setDetectPeakToTotPwrThresh(0.6f);
          det->setDetectSnrThresh(open_thresh, bpf_high - bpf_low);
          det->setDetectStableCountThresh(1);

          //det->setUndetectBw(8.0f);
          det->setUndetectUseWindowing(false);
          det->setUndetectPeakThresh(0.0f);
          //det->setUndetectPeakToTotPwrThresh(0.3f);
          det->setUndetectSnrThresh(close_thresh, bpf_high - bpf_low);
          det->setUndetectStableCountThresh(2);

            // Set up CTCSS band pass filter
          filter = new Async::AudioFilter(filter_spec.str());
          filter->registerSink(det);
          sink = filter;
          break;
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
      det->reset();
      Squelch::reset();
    }

    /**
     * @brief   Set the time the squelch should hang open after squelch close
     * @param   hang The number of milliseconds to hang
     */
    virtual void setHangtime(int hang)
    {
      det->setUndetectDelay(hang);
    }

    /**
      * @brief   Set the time a squelch open should be delayed
      * @param   delay The delay in milliseconds
      */
    virtual void setDelay(int delay)
    {
      det->setDetectDelay(delay);
    }

    /**
     * @brief  A signal that is emitted when the tone SNR has been recalculated
     * @param  snr The current SNR
     *
     * This signal will be emitted as soon as a new SNR value for the CTCSS
     * tone has been calculated. The signal will only be emitted when
     * CTCSS_MODE is set to 2 or 3.
     */
    sigc::signal<void, float> snrUpdated;

  protected:
    /**
     * @brief 	Process the incoming samples in the squelch detector
     * @param 	samples A buffer containing samples
     * @param 	count The number of samples in the buffer
     * @return	Return the number of processed samples
     */
    int processSamples(const float *samples, int count)
    {
      return sink->writeSamples(samples, count);
    }

  private:
    ToneDetector	*det;
    Async::AudioFilter	*filter;
    Async::AudioSink	*sink;

    SquelchCtcss(const SquelchCtcss&);
    SquelchCtcss& operator=(const SquelchCtcss&);

};  /* class SquelchCtcss */


//} /* namespace */

#endif /* SQUELCH_CTCSS_INCLUDED */



/*
 * This file has not been truncated
 */

