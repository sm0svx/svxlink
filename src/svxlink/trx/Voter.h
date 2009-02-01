/**
@file	 Voter.h
@brief   This file contains a class that implement a receiver voter
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-18

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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


#ifndef VOTER_INCLUDED
#define VOTER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <list>


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

#include "Rx.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Timer;
  class AudioSelector;
};


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

class SatRx;
  

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
@brief	An Rx class that implement a receiver voter
@author Tobias Blomberg
@date   2005-04-18

This class implements a receiver voter. A voter is a device that choose the
best receiver from a pool of receivers tuned to the same frequency. This
make it possible to cover a larger geographical area with a radio system.
*/
class Voter : public Rx
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	cfg   The Config object to read configuration data from
     * @param 	name  The name of the receiver configuration section
     */
    Voter(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    ~Voter(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    bool initialize(void);
    
    /**
     * @brief   Set the verbosity level of the receiver
     * @param   verbose Set to \em false to keep the rx from printing things
     */
    void setVerbose(bool verbose) { m_verbose = verbose; }

    /**
     * @brief 	Mute the receiver
     * @param 	do_mute Set to \em true to mute or \em false to unmute
     */
    void mute(bool do_mute);
    
    /**
     * @brief 	Call this function to add a tone detector to the RX
     * @param 	fq The tone frequency to detect
     * @param 	bw The bandwidth of the detector
     * @param 	thresh The detection threshold in dB SNR
     * @param 	required_duration The required time in milliseconds that
     *	      	the tone must be active for activity to be reported.
     * @return	Return \em true if the Rx is capable of tone detection or
     *	      	\em false if it's not.
     */
    bool addToneDetector(float fq, int bw, float thresh, int required_duration);
    
    /**
     * @brief 	Read the current signal strength
     * @return	Returns the signal strength
     */
    float signalStrength(void) const;
    
    /**
     * @brief 	Find out RX ID of last receiver with squelch activity
     * @returns Returns the RX ID
     */
    int sqlRxId(void) const;
    
    /**
     * @brief 	Reset the receiver object to its default settings
     */
    void reset(void);
    
  protected:
    
  private:
    Async::Config     	  &cfg;
    std::list<SatRx *>	  rxs;
    SatRx	      	  *active_rx;
    bool      	      	  is_muted;
    bool	      	  m_verbose;
    SatRx	      	  *best_rx;
    double    	      	  best_rx_siglev;
    Async::Timer      	  *best_rx_timer;
    int       	      	  voting_delay;
    int       	      	  sql_rx_id;
    Async::AudioSelector  *selector;
    int                   buffer_length;
    
    void satSquelchOpen(bool is_open, SatRx *rx);
    void chooseBestRx(Async::Timer *t);

};  /* class Voter */


//} /* namespace */

#endif /* VOTER_INCLUDED */



/*
 * This file has not been truncated
 */

