/**
@file	 Uplink.h
@brief   Contains the base class for implementing a remote trx uplink
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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


#ifndef UPLINK_INCLUDED
#define UPLINK_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <string>


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

class Rx;
class Tx;


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
@brief	The base class for implementing a remote receiver uplink
@author Tobias Blomberg / SM0SVX
@date   2006-04-14

This is the base class for implementing a remote trx uplink. That is,
the link to the SvxLink core.
*/
class Uplink : public sigc::trackable
{
  public:
    static Uplink *create(Async::Config &cfg, const std::string &name,
      	      	      	  Rx *rx, Tx *tx);
    
    /**
     * @brief 	Default constuctor
     */
    Uplink(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~Uplink(void);
  
    /**
     * @brief 	Initialize the uplink
     * @return	Return \em true on success or \em false on failure
     */
    virtual bool initialize(void) = 0;
    
    /**
     * @brief 	Set squelch state to open/closed
     * @param 	is_open Set to \em true if open or \em false if closed
     * @param 	signal_strength The RF signal strength
     */
    //virtual void squelchOpen(bool is_open) = 0;
    
    /**
     * @brief 	Pass on received DTMF digit
     * @param 	digit The received digit
     */
    //virtual void dtmfDigitDetected(char digit) = 0;
    
    /**
     * @brief 	Pass on detected tone
     * @param 	tone_fq The frequency of the received tone
     */
    //virtual void toneDetected(int tone_fq) = 0;
    
    /**
     * @brief 	Pass on received audio
     * @param 	samples The buffer containing the samples
     * @param 	count 	The number of samples in the buffer
     */
    //virtual int audioReceived(short *samples, int count) = 0;
        
    
  protected:
    
  private:
    Uplink(const Uplink&);
    Uplink& operator=(const Uplink&);
    
};  /* class Uplink */


//} /* namespace */

#endif /* UPLINK_INCLUDED */



/*
 * This file has not been truncated
 */

