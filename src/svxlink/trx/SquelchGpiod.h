/**
@file	 SquelchGpiod.h
@brief   A squelch detector that read squelch state from a GPIO port pin
@author  Tobias Blomberg / SM0SVX
@date	 2021-08-13

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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

#ifndef SQUELCH_GPIOD_INCLUDED
#define SQUELCH_GPIOD_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <gpiod.h>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

//#include <AsyncFdWatch.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

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
@brief  A squelch detector that read squelch state from a GPIO port
@author Tobias Blomberg / Tobias
@date   2021-08-13

This squelch detector read the squelch indicator signal from a GPIO input pin
using the gpiod library.
*/
class SquelchGpiod : public Squelch
{
  public:
      /// The name of this class when used by the object factory
    static constexpr const char* OBJNAME = "GPIOD";

    /**
     * @brief   Default constuctor
     */
    SquelchGpiod(void);

    /**
     * @brief   Destructor
     */
    ~SquelchGpiod(void);

    /**
     * @brief   Initialize the squelch detector
     * @param   cfg A previsously initialized config object
     * @param   rx_name The name of the RX (config section name)
     * @return  Returns \em true on success or else \em false
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name);

  private:
    Async::Timer        m_timer;
    //Async::FdWatch      m_watch;
    struct gpiod_chip*  m_chip  = nullptr;
    struct gpiod_line*  m_line  = nullptr;

    //void readGpioValueData(void);

};  /* class SquelchGpiod */


//} /* namespace */

#endif /* SQUELCH_GPIOD_INCLUDED */



/*
 * This file has not been truncated
 */

