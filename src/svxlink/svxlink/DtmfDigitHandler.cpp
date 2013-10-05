/**
@file	 DtmfDigitHandler.cpp
@brief   Handle incoming DTMF digits to form commands
@author  Tobias Blomberg / SM0SVX
@date	 2013-08-20

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DtmfDigitHandler.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

DtmfDigitHandler::DtmfDigitHandler(void)
  : anti_flutter(false), prev_digit('?')
{
  cmd_tmo_timer.expired.connect(
      hide(mem_fun(*this, &DtmfDigitHandler::cmdTimeout)));
  cmd_tmo_timer.setTimeout(10000);
  cmd_tmo_timer.setEnable(false);
} /* DtmfDigitHandler::DtmfDigitHandler */


DtmfDigitHandler::~DtmfDigitHandler(void)
{
  
} /* DtmfDigitHandler::~DtmfDigitHandler */


void DtmfDigitHandler::digitReceived(char digit)
{
  cmd_tmo_timer.reset();
  cmd_tmo_timer.setEnable(true);

  if ((digit == '#') || (anti_flutter && (digit == 'C')))
  {
    commandComplete();
    reset();
  }
  else if (digit == 'A')
  {
    anti_flutter = true;
    prev_digit = '?';
  }
  else if (digit == 'D')
  {
    received_digits = "D";
    prev_digit = '?';
  }
  else if (received_digits.size() < 20)
  {
    // FIXME: The "H"-trick may have outlived its purpose. Check this!
    if (digit == 'H') // Make it possible to type a hash mark in a macro
    {
      received_digits += '#';
    }
    else if (digit == 'B')
    {
      if (anti_flutter && (prev_digit != '?'))
      {
        received_digits += prev_digit;
        prev_digit = '?';
      }
    }
    else if (isdigit(digit)
        || ((digit == '*') && (received_digits != "*")))
    {
      if (anti_flutter)
      {
        if (digit != prev_digit)
        {
          received_digits += digit;
          prev_digit = digit;
        }
      }
      else
      {
        received_digits += digit;
      }
    }
  }
}

void DtmfDigitHandler::reset(void)
{
  cmd_tmo_timer.setEnable(false);
  received_digits = "";
  anti_flutter = false;
  prev_digit = '?';
}


void DtmfDigitHandler::forceCommandComplete()
{
  if (!received_digits.empty())
  {
    commandComplete();
    reset();
  }
}


void DtmfDigitHandler::cmdTimeout(void)
{
  received_digits = "";
  anti_flutter = false;
  prev_digit = '?';
}


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

