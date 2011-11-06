/**
@file	 TrxHandler.h
@brief   This file contains the receiver handler class
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2004-2008 Tobias Blomberg / SM0SVX

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


#ifndef TRX_HANDLER_INCLUDED
#define TRX_HANDLER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>


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
@brief	This class handles the trx
@author Tobias Blomberg / SM0SVX
@date   2006-04-14
*/
class TrxHandler : public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    TrxHandler(Async::Config &cfg, const std::string &name);
  
    /**
     * @brief 	Destructor
     */
    ~TrxHandler(void);
  
    /**
     * @brief 	Initialize the trx handler
     * @return	Returns \em true on success or else \em false
     */
    bool initialize(void);
    
  protected:
    
  private:
    Async::Config &m_cfg;
    std::string	  m_name;
    Uplink    	  *m_uplink;
    Rx	      	  *m_rx;
    Tx	      	  *m_tx;
    
    TrxHandler(const TrxHandler&);
    TrxHandler& operator=(const TrxHandler&);
    void cleanup(void);
    
};  /* class TrxHandler */


//} /* namespace */

#endif /* TRX_HANDLER_INCLUDED */



/*
 * This file has not been truncated
 */

