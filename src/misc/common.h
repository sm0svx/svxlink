/**
@file	 common.h
@brief   Contains some commonly used global functions
@author  Tobias Blomberg / SM0SVX
@date	 2010-04-05

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

#ifndef COMMON_INCLUDED
#define COMMON_INCLUDED


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

namespace SvxLink
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
 * Global functions
 *
 ****************************************************************************/

template <class Container>
int splitStr(Container &L, const std::string &seq, const std::string &delims)
{
  L.clear();

  std::string str;
  std::string::size_type pos = 0;
  std::string::size_type len = seq.size();
  while (pos < len)
  {
      // Init/clear the token buffer
    str = "";

      // remove any delimiters including optional (white)spaces
    while ((delims.find(seq[pos]) != std::string::npos) && (pos < len))
    {
      pos++;
    }

      // leave if @eos
    if (pos == len)
    {
      return L.size();
    }

      // Save token data
    while ((delims.find(seq[pos]) == std::string::npos) && (pos < len))
    {
      str += seq[pos++];
    }

      // put valid str buffer into the supplied list
    if (!str.empty())
    {
      L.push_back(str);
    }
  }

  return L.size();

} /* splitStr */


} /* namespace */

#endif /* COMMON_INCLUDED */



/*
 * This file has not been truncated
 */

