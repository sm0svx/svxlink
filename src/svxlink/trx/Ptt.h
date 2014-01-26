/**
@file	 Ptt.h
@brief   Base class for PTT hw control
@author  Tobias Blomberg / SM0SVX
@date	 2014-01-26

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#ifndef PTT_INCLUDED
#define PTT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <map>


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

#include "Factory.h"


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
@brief	Base class for PTT hw control
@author Tobias Blomberg / SM0SVX
@date   2014-01-26

This is the base class for implementing a controller for PTT hardware.
*/
class Ptt
{
  public:
    /**
     * @brief 	Default constructor
     */
    Ptt(void) {}
  
    /**
     * @brief 	Destructor
     */
    virtual ~Ptt(void) {}
  
    /**
     * @brief 	Initialize the PTT hardware
     * @param 	cfg An initialized config object
     * @param   name The name of the config section to read config from
     * @returns Returns \em true on success or else \em false
     */
    virtual bool initialize(Async::Config &cfg, const std::string name) = 0;

    /**
     * @brief 	Set the state of the PTT, TX on or off
     * @param 	tx_on Set to \em true to turn the transmitter on
     * @returns Returns \em true on success or else \em false
     */
    virtual bool setTxOn(bool tx_on) = 0;
    
  protected:
    
  private:
    Ptt(const Ptt&);
    Ptt& operator=(const Ptt&);
    
};  /* class Ptt */


/**
@brief	Base class for a PTT hw controller factory
@author Tobias Blomberg / SM0SVX
@date   2014-01-26

This is the base class for a Ptt hardware controller factory. However, this
is not the class to inherit from when implementing a Ptt hardware controller
factory. Use the PttFactory class for that.
This class is essentially used only to access the createNamedPtt function.
*/
struct PttFactoryBase : public FactoryBase<Ptt>
{
  static Ptt *createNamedPtt(Async::Config& cfg, const std::string& name);
  PttFactoryBase(const std::string &name) : FactoryBase<Ptt>(name) {}
};  /* class PttFactoryBase */


/**
@brief	Base class for implementing a PTT hw controller factory
@author Tobias Blomberg / SM0SVX
@date   2014-01-26

This class should be inherited from when implementing a new PTT hardware
controller factory.
*/
template <class T>
struct PttFactory : public Factory<PttFactoryBase, T>
{
  PttFactory(const std::string &name) : Factory<PttFactoryBase, T>(name) {}
}; /* class PttFactory */


//} /* namespace */

#endif /* PTT_INCLUDED */



/*
 * This file has not been truncated
 */
