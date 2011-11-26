/**
@file	 SquelchCtcss.h
@brief   A CTCSS squelch detector
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-02

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2010 Tobias Blomberg / SM0SVX

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
      std::string value;
      float ctcss_fq = 0;
      if (cfg.getValue(rx_name, "CTCSS_FQ", value))
      {
	ctcss_fq = atof(value.c_str());
      }
      if (ctcss_fq <= 0)
      {
	std::cerr << "*** ERROR: Config variable " << rx_name
      	     << "/CTCSS_FQ not set or is set to an illegal value\n";
	return false;
      }

      float ctcss_thresh = 10;
      if (cfg.getValue(rx_name, "CTCSS_THRESH", value))
      {
	ctcss_thresh = atof(value.c_str());
      }

      int ctcss_mode = 1;
      cfg.getValue(rx_name, "CTCSS_MODE", ctcss_mode);
      
      float snr_open_thresh = 16;
      cfg.getValue(rx_name, "CTCSS_SNR_OPEN_THRESH", snr_open_thresh);

      float snr_close_thresh = 10;
      cfg.getValue(rx_name, "CTCSS_SNR_CLOSE_THRESH", snr_close_thresh);

      det = new ToneDetector(ctcss_fq, 8.0f);
      det->setPeakThresh(ctcss_thresh);
      det->activated.connect(mem_fun(*this, &SquelchCtcss::setSignalDetected));
      sink = det;

      switch (ctcss_mode)
      {
        case 2:
          std::cout << "### CTCSS mode: Estimated SNR\n";
          //det->setDetectBw(6.0f);
          det->setDetectUseWindowing(false);
          det->setDetectPeakThresh(0.0f);
          //det->setDetectPeakToTotPwrThresh(0.6f);
          det->setDetectSnrThresh(snr_open_thresh, 210.0f);
          det->setDetectStableCountThresh(1);

          //det->setUndetectBw(8.0f);
          det->setUndetectUseWindowing(false);
          det->setUndetectPeakThresh(0.0f);
          //det->setUndetectPeakToTotPwrThresh(0.3f);
          det->setUndetectSnrThresh(snr_close_thresh, 210.0f);
          det->setUndetectStableCountThresh(2);

            // Set up CTCSS band pass filter
          filter = new Async::AudioFilter("BpBu8/60-270");
          filter->registerSink(det);
          sink = filter;
          break;

        case 3:
          std::cout << "### CTCSS mode: Estimated SNR + Phase\n";
          //det->setDetectUseWindowing(false);
          det->setDetectBw(16.0f);
          det->setDetectPeakThresh(0.0f);
          //det->setDetectPeakToTotPwrThresh(0.6f);
          det->setDetectSnrThresh(snr_open_thresh, 210.0f);
          det->setDetectStableCountThresh(1);
          det->setDetectPhaseBwThresh(2.0f, 2.0f);

          //det->setUndetectBw(8.0f);
          det->setUndetectUseWindowing(false);
          det->setUndetectPeakThresh(0.0f);
          //det->setUndetectPeakToTotPwrThresh(0.3f);
          det->setUndetectSnrThresh(snr_close_thresh, 210.0f);
          det->setUndetectStableCountThresh(2);
          //det->setUndetectPhaseBwThresh(4.0f, 16.0f);

            // Set up CTCSS band pass filter
          filter = new Async::AudioFilter("BpBu8/60-270");
          filter->registerSink(det);
          sink = filter;
          break;

        default:
          std::cout << "### CTCSS mode: Neighbour bins\n";
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

