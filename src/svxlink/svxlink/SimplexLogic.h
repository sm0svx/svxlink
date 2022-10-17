/**
@file	 SimplexLogic.h
@brief   Contains a simplex logic SvxLink core implementation
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-23

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


#ifndef SIMPLEX_LOGIC_INCLUDED
#define SIMPLEX_LOGIC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Logic.h"


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
@brief	This class implements a simplex logic core
@author Tobias Blomberg
@date   2004-03-23
*/
class SimplexLogic : public Logic
{
  public:
    /**
     * @brief 	Default constructor
     */
    SimplexLogic(void);

    /**
     * @brief 	Initialize the simplex logic core
     * @param 	cfgobj      A previously opened configuration
     * @param 	plugin_name The configuration section name of this logic
     * @return	Returns \em true if the initialization was successful or else
     *	      	\em false is returned.
     */
    virtual bool initialize(Async::Config &cfgobj,
                            const std::string &logic_name) override;

  protected:
    /**
     * @brief 	Destructor
     */
    virtual ~SimplexLogic(void) override {};

    virtual void squelchOpen(bool is_open);
    virtual void transmitterStateChange(bool is_transmitting);
    
  private:
    bool  mute_rx_on_tx;
    bool  mute_tx_on_rx;
    bool  rgr_sound_always;
    
};  /* class SimplexLogic */


//} /* namespace */

#endif /* SIMPLEX_LOGIC_INCLUDED */



/*
 * This file has not been truncated
 */

