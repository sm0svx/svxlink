/**
@file	 Modulation.h
@brief   Transceiver modulation representation
@author  Tobias Blomberg / SM0SVX
@date	 2018-01-07

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2018 Tobias Blomberg / SM0SVX

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

#ifndef MODULATION_INCLUDED
#define MODULATION_INCLUDED


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

namespace Modulation
{


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
 * @brief   Modulation types
 */
typedef enum
{
  MOD_UNKNOWN,  //!< Unknown/unset
  MOD_FM,       //!< FM. Standard two-way radio 20kHz channel.
  MOD_NBFM,     //!< FM narrow. Narrow band two-way radio 10kHz channel.
  MOD_WBFM,     //!< Wide band FM. Standard FM broadcasting 180kHz channel.
  MOD_AM,       //!< AM. Standard two-way radio 10kHz channel.
  MOD_NBAM,     //!< AM narrow. Narrow band two-way radio 6kHz channel.
  MOD_USB,      //!< Upper Sideband 3kHz channel
  MOD_LSB,      //!< Lower Sideband 3kHz channel
  MOD_CW,       //!< Morse
  MOD_WBCW      //!< Morse 3kHz channel
} Type;


/**
 * @brief 	A_brief_member_function_description
 * @param 	param1 Description_of_param1
 * @return	Return_value_of_this_member_function
 */
Type fromString(const std::string& modstr);

const char *toString(Type mod);


} /* namespace */

#endif /* MODULATION_INCLUDED */


/*
 * This file has not been truncated
 */
