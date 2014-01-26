/**
@file	 PttSerialPin.h
@brief   A PTT hardware controller using a pin in a serial port
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

#ifndef PTT_SERIAL_PIN_INCLUDED
#define PTT_SERIAL_PIN_INCLUDED


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

#include <AsyncSerial.h>


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
@brief	A PTT hardware controller using a pin in a serial port
@author Tobias Blomberg / SM0SVX
@date   2014-01-26
*/
class PttSerialPin : public Ptt
{
  public:
    struct Factory : public PttFactory<PttSerialPin>
    {
      Factory(void) : PttFactory<PttSerialPin>("SerialPin") {}
    };

    /**
     * @brief 	Default constructor
     */
    PttSerialPin(void);
  
    /**
     * @brief 	Destructor
     */
    ~PttSerialPin(void);
  
    /**
     * @brief 	Initialize the PTT hardware
     * @param 	cfg An initialized config object
     * @param   name The name of the config section to read config from
     * @returns Returns \em true on success or else \em false
     */
    virtual bool initialize(Async::Config &cfg, const std::string name);

    /**
     * @brief 	Set the state of the PTT, TX on or off
     * @param 	tx_on Set to \em true to turn the transmitter on
     * @returns Returns \em true on success or else \em false
     */
    virtual bool setTxOn(bool tx_on);

  protected:
    
  private:
    Async::Serial       *serial;
    Async::Serial::Pin  ptt_pin1;
    bool                ptt_pin1_rev;
    Async::Serial::Pin  ptt_pin2;
    bool                ptt_pin2_rev;

    PttSerialPin(const PttSerialPin&);
    PttSerialPin& operator=(const PttSerialPin&);
    int parsePttPin(const std::string &name, const char *str,
                    Async::Serial::Pin &pin, bool &rev);
    bool setPins(const Async::Config &cfg, const std::string &name);
    
};  /* class PttSerialPin */


#endif /* PTT_SERIAL_PIN_INCLUDED */


/*
 * This file has not been truncated
 */
