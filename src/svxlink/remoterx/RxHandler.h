/**
@file	 RxHandler.h
@brief   This file contains the receiver handler class
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

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


#ifndef RX_HANDLER_INCLUDED
#define RX_HANDLER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>


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

class Uplink;
class Rx;
  

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
@brief	This class handles the receivers
@author Tobias Blomberg / SM0SVX
@date   2006-04-14

This is the class that handle all the receivers that is to be monitored. It
also handle the network connection to the SvxLink core.
*/
class RxHandler : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    RxHandler(Async::Config &cfg);
  
    /**
     * @brief 	Destructor
     */
    ~RxHandler(void);
  
    /**
     * @brief 	Initialize the RX handler
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(void);
    
  protected:
    
  private:
    Async::Config &cfg;
    Uplink    	  *uplink;
    Rx	      	  *rx;
    
    RxHandler(const RxHandler&);
    RxHandler& operator=(const RxHandler&);
    
};  /* class RxHandler */


//} /* namespace */

#endif /* RX_HANDLER_INCLUDED */



/*
 * This file has not been truncated
 */

