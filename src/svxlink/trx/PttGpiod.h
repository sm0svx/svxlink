/**
@file    PttGpiod.h
@brief   A PTT hardware controller using a pin in a GPIO port
@author  Tobias Blomberg / SM0SVX
@date    2021-08-13

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

#ifndef PTT_GPIOD_INCLUDED
#define PTT_GPIOD_INCLUDED


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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Ptt.h"


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
@brief  A PTT hardware controller using a pin in a GPIO port
@author Tobias Blomberg / SM0SVX
@date   2021-08-12
*/
class PttGpiod : public Ptt
{
  public:
    struct Factory : public PttFactory<PttGpiod>
    {
      Factory(void) : PttFactory<PttGpiod>("GPIOD") {}
    };

    /**
     * @brief   Default constructor
     */
    PttGpiod(void);

    /**
     * @brief   Destructor
     */
    ~PttGpiod(void);

    /**
     * @brief   Initialize the PTT hardware
     * @param   cfg An initialized config object
     * @param   name The name of the config section to read config from
     * @returns Returns \em true on success or else \em false
     */
    virtual bool initialize(Async::Config &cfg, const std::string name);

    /**
     * @brief   Set the state of the PTT, TX on or off
     * @param   tx_on Set to \em true to turn the transmitter on
     * @returns Returns \em true on success or else \em false
     */
    virtual bool setTxOn(bool tx_on);

  private:
    struct gpiod_chip*  m_chip  = nullptr;
    struct gpiod_line*  m_line  = nullptr;

};  /* class PttGpiod */


#endif /* PTT_GPIOD_INCLUDED */


/*
 * This file has not been truncated
 */
