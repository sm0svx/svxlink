/**
@file	 SquelchSerial.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-02

A_detailed_description_for_this_file

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

/** @example SquelchSerial_demo.cpp
An example of how to use the SquelchSerial class
*/


#ifndef SQUELCH_SERIAL_INCLUDED
#define SQUELCH_SERIAL_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <string>


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2005-08-02

A_detailed_class_description

\include SquelchSerial_demo.cpp
*/
class SquelchSerial : public Squelch
{
  public:
    /**
     * @brief 	Default constuctor
     */
    SquelchSerial(void) {}
  
    /**
     * @brief 	Destructor
     */
    ~SquelchSerial(void) {}
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    bool initialize(Async::Config& cfg, const std::string& rx_name)
    {
      if (!Squelch::initialize(cfg, rx_name))
      {
      	return false;
      }
      
      std::string serial_port;
      if (!cfg.getValue(rx_name, "SERIAL_PORT", serial_port))
      {
	std::cerr << "*** ERROR: Config variable " << rx_name
	      	  << "/SERIAL_PORT not set\n";
	return false;
      }

      std::string serial_pin_str;
      if (!cfg.getValue(rx_name, "SERIAL_PIN", serial_pin_str))
      {
	std::cerr << "*** ERROR: Config variable " << rx_name
	      	  << "/SERIAL_PIN not set\n";
	return false;
      }
      std::string::iterator colon;
      colon = find(serial_pin_str.begin(), serial_pin_str.end(), ':');
      if ((colon == serial_pin_str.end()) ||
      	  (colon + 1 == serial_pin_str.end()))
      {
	std::cerr << "*** ERROR: Illegal format for config variable "
	      	  << rx_name << "/SERIAL_PIN. Should be PINNAME:LEVEL\n";
	return false;
      }
      std::string pin_str(serial_pin_str.begin(), colon);
      std::string pin_act_lvl_str(colon + 1, serial_pin_str.end());
      if (pin_str == "CTS")
      {
	sql_pin = Async::Serial::PIN_CTS;
      }
      else if (pin_str == "DSR")
      {
	sql_pin = Async::Serial::PIN_DSR;
      }
      else if (pin_str == "DCD")
      {
	sql_pin = Async::Serial::PIN_DCD;
      }
      else if (pin_str == "RI")
      {
	sql_pin = Async::Serial::PIN_RI;
      }
      else
      {
	std::cerr << "*** ERROR: Illegal pin name for config variable "
	      	  << rx_name << "/SERIAL_PIN. Should be CTS, DSR, DCD or RI.\n";
	return false;
      }
      if (pin_act_lvl_str == "SET")
      {
	sql_pin_act_lvl = true;
      }
      else if (pin_act_lvl_str == "CLEAR")
      {
	sql_pin_act_lvl = false;
      }
      else
      {
	std::cerr << "*** ERROR: Illegal pin level for config variable "
	      	  << rx_name << "/SERIAL_PIN. Should be SET or CLEAR.\n";
	return false;
      }

      serial = new Async::Serial(serial_port);
      if (!serial->open())
      {
	delete serial;
	serial = 0;
	return false;
      }
      
      return true;
    }
    
    
  protected:
    /**
     * @brief 	Process the incoming samples in the squelch detector
     * @param 	samples A buffer containing samples
     * @param 	count The number of samples in the buffer
     * @return	Return the number of processed samples
     */
    int processSamples(float *samples, int count)
    {
      bool is_set = false;
      if (!serial->getPin(sql_pin, is_set))
      {
	perror("getPin");
	return count;
      }
      bool is_activated = (is_set == sql_pin_act_lvl);

      //printf("Serial squelch %s...\n",
      	      //is_activated ? "ACTIVATED" : "DEACTIVATED");
      setOpen(is_activated);

      return count;
    }

    
  private:
    Async::Serial     	    *serial;
    Async::Serial::Pin      sql_pin;
    bool      	      	    sql_pin_act_lvl;
  
    SquelchSerial(const SquelchSerial&);
    SquelchSerial& operator=(const SquelchSerial&);
    
};  /* class SquelchSerial */


//} /* namespace */

#endif /* SQUELCH_SERIAL_INCLUDED */



/*
 * This file has not been truncated
 */

