/**
@file    SigLevDetConst.h
@brief   A constant level signal level "detector"
@author  Tobias Blomberg / SM0SVX
@date    2019-11-04

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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

#ifndef SIG_LEV_DET_CONST_INCLUDED
#define SIG_LEV_DET_CONST_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDet.h"


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
@brief	A constant level signal level "detector"
@author Tobias Blomberg / SM0SVX
@date   2019-11-04

This signal level detector can be used to indicate a constant signal level. It
doesn't detect anything. It just read a configuration variable which set the
constant signal level.
*/
class SigLevDetConst : public SigLevDet
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "CONST";

    /**
     * @brief   Default constuctor
     */
    SigLevDetConst(void)
      : m_sample_rate(0), m_update_cnt(0),
        m_update_cnt_reset(0), m_siglev(0.0f) {}

    /**
     * @brief   Destructor
     */
    ~SigLevDetConst(void) {}

    /**
     * @brief   Initialize the signal detector
     * @param   cfg An initialized config object
     * @param   name The name of the config section to read config from
     * @param   sample_rate The rate with which samples enter the detector
     * @return  Return \em true on success, or \em false on failure
     */
    virtual bool initialize(Async::Config &cfg, const std::string& name,
                            int sample_rate)
    {
      m_sample_rate = sample_rate;
      if (!cfg.getValue(name, "SIGLEV_CONST", m_siglev, true))
      {
        return false;
      }
      return SigLevDet::initialize(cfg, name, sample_rate);
    }

    /**
     * @brief   Set the interval for continuous updates
     * @param   interval_ms The update interval, in milliseconds, to use.
     *
     * This function will set up how often the signal level detector will
     * report the signal strength.
     */
    virtual void setContinuousUpdateInterval(int interval_ms)
    {
      m_update_cnt_reset = interval_ms * m_sample_rate / 1000;
      m_update_cnt = m_update_cnt_reset;
      m_update_cnt = 0;
    }

    /**
     * @brief   Set the integration time to use
     * @param   time_ms The integration time in milliseconds
     *
     * This function will set up the integration time for the signal level
     * detector. That is, the detector will build a mean value of the
     * detected signal strengths over the given integration time.
     */
    virtual void setIntegrationTime(int time_ms) {}

    /**
     * @brief   Read the latest measured signal level
     * @return  Returns the latest measured signal level
     */
    virtual float lastSiglev(void) const { return m_siglev; }

    /**
     * @brief   Read the integrated siglev value
     * @return  Returns the integrated siglev value
     */
    virtual float siglevIntegrated(void) const { return m_siglev; }

    /**
     * @brief   Reset the signal level detector
     */
    virtual void reset(void)
    {
      m_update_cnt_reset = 0;
      m_update_cnt = 0;
    }

  protected:
    virtual int writeSamples(const float *samples, int count)
    {
      if (count <= 0)
      {
        return 0;
      }
      if (m_update_cnt_reset > 0)
      {
        m_update_cnt += count;
        if (m_update_cnt >= m_update_cnt_reset)
        {
          signalLevelUpdated(m_siglev);
          m_update_cnt = m_update_cnt % m_update_cnt_reset;
        }
      }
      return count;
    }
    virtual void flushSamples(void) {}

  private:
    int   m_sample_rate;
    int   m_update_cnt;
    int   m_update_cnt_reset;
    float m_siglev;

    SigLevDetConst(const SigLevDetConst&);
    SigLevDetConst& operator=(const SigLevDetConst&);

};  /* class SigLevDetConst */


//} /* namespace */

#endif /* SIG_LEV_DET_CONST_INCLUDED */



/*
 * This file has not been truncated
 */

