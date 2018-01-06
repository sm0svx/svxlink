/**
@file	 Fcs.h
@brief   Calculate the HDLC CRC16 Frame Check Sequence
@author  Tobias Blomberg / SM0SVX
@date	 2013-05-09

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

#ifndef FCS_INCLUDED
#define FCS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdint.h>
#include <vector>


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
 * Public functions
 *
 ****************************************************************************/

/**
 * @brief   Calculate the frame check sequence for the given frame buffer
 * @param   buf The buffer containing the data bytes
 * @return  Return the 16 bit frame check sequence
 */
uint16_t fcsCalc(const std::vector<uint8_t> buf);

/**
 * @brief   Check if the buffer contain a valid data stream
 * @param   buf The buffer containing the data bytes and the transmitted FCS
 * @return  Returns \em true on success or \em false on failure
 * */
bool fcsOk(const std::vector<uint8_t> buf);


//} /* namespace */

#endif /* FCS_INCLUDED */



/*
 * This file has not been truncated
 */

