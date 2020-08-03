/**
@file	 SigLevDetAfsk.h
@brief   A signal level detector using tones in the 5.5 to 6.5kHz band
@author  Tobias Blomberg / SM0SVX
@date	 2009-05-23

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


#ifndef SIG_LEV_DET_AFSK_INCLUDED
#define SIG_LEV_DET_AFSK_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>
#include <vector>
#include <deque>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTimer.h>


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

class HdlcDeframer;
class Rx;


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
@brief	A signal level detector using AFSK transport
@author Tobias Blomberg / SM0SVX
@date   2013-05-09

This is not a signal level detector on its own but rather a transport mechanism
to transfer signal level measurements from a remote receiver site that is
linked in using RF.
The signal level measurements are transported using AFSK modulation
superimposed on the audio signal.
*/
class SigLevDetAfsk : public SigLevDet
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "AFSK";

    /**
     * @brief 	Constuctor
     * @param	sample_rate The rate with which samples enter the detector
     */
    SigLevDetAfsk(void);

    /**
     * @brief 	Destructor
     */
    ~SigLevDetAfsk(void);

    /**
     * @brief	Set the interval for continuous updates
     * @param	interval_ms The update interval, in milliseconds, to use.
     *
     * This function will set up how often the signal level detector will
     * report the signal strength.
     */
    virtual void setContinuousUpdateInterval(int interval_ms);

    /**
     * @brief	Set the integration time to use
     * @param	time_ms The integration time in milliseconds
     *
     * This function will set up the integration time for the signal level
     * detector. That is, the detector will build a mean value of the
     * detected signal strengths over the given integration time.
     */
    virtual void setIntegrationTime(int time_ms);

    /**
     * @brief 	Read the latest measured signal level
     * @return	Returns the latest measured signal level
     */
    virtual float lastSiglev(void) const { return last_siglev; }

    /**
     * @brief   Read the integrated siglev value
     * @return  Returns the integrated siglev value
     */
    virtual float siglevIntegrated(void) const;

    /**
     * @brief   Reset the signal level detector
     */
    virtual void reset(void);

    virtual int writeSamples(const float *samples, int len);
    virtual void flushSamples(void);
    virtual void frameReceived(std::vector<uint8_t> frame);

  protected:

  private:
    int                 last_siglev;
    Async::Timer        timeout_timer;

    SigLevDetAfsk(const SigLevDetAfsk&);
    SigLevDetAfsk& operator=(const SigLevDetAfsk&);
    void timeout(Async::Timer *t);

};  /* class SigLevDetAfsk */


//} /* namespace */

#endif /* SIG_LEV_DET_AFSK_INCLUDED */



/*
 * This file has not been truncated
 */

