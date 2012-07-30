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

#include <sstream>


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

/**
 * @brief   Set a value of any type supporting streaming from a string
 * @param   val Reference to the value to set
 * @param   str The sting to set the value from
 * @returns Returns \em true if the operation succeeded or \em false if not
 *
 * This function use the streaming operator to set the outgoing value from the
 * incoming string so any type that support the streaming operator will work.
 * All characters in the string must be consumed for the parser to indicate
 * success.
 */
template <typename ValueType>
static bool setValueFromString(ValueType &val, const std::string &str)
{
  std::istringstream ss(str);
  ss >> std::noskipws >> val >> std::ws;
  return !ss.fail() && ss.eof();
} /* setValueFromString */


/**
 * @brief   Set a value of any type supporting streaming from a string
 * @param   val Reference to the value to set
 * @param   str The sting to set the value from
 * @returns Returns \em true if the operation succeeded or \em false if not
 *
 * This function is a specialization for std::string of the more general
 * template function. It just copies the incoming string to the outgoing value.
 * If streaming were to be used for strings, white space would be
 * unconditionally ignored.
 */
template <>
#if defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__ < 403)
static
#endif
bool setValueFromString(std::string &val, const std::string &str)
{
  val = str;
  return true;
} /* setValueFromString */


/**
 * @brief   Split a string using a set of delimiters and put the tokens in a
 *          container
 * @param   L       The container to put the tokens into. Any container that
 *                  supports the clear, size and push_back methods can be used.
 * @param   seq     The string to split
 * @param   delims  The set of delimiters used to split the string. Be sure to
 *                  include any white space characters that you want to ignore.
 * @returns Returns the number of tokens stored in the container
 *
 * This is a template function that is used to split strings into tokens,
 * storing the tokens into a container. The container can be of any type that
 * supports the clear, size and push_back methods (e.g. vector, list, deque).
 *
 * The container value type may be any that supports setting a value by
 * streaming to it (e.g. std::string, int, float, char etc).
 *
 * If a parsed value is malformed, in a way so that it cannot be parsed into
 * the given container value type, it will be ignored.
 *
 * Empty values will not be stored.
 */
template <class Container>
static typename Container::size_type splitStr(Container &L,
                                              const std::string &seq,
                                              const std::string &delims)
{
  L.clear();

  std::string::size_type pos = 0;
  std::string::size_type len = seq.size();
  while (pos < len)
  {
      // remove any delimiters including optional (white)spaces
    while ((delims.find(seq[pos]) != std::string::npos) && (pos < len))
    {
      pos++;
    }

      // Save token data
    std::string str;
    while ((delims.find(seq[pos]) == std::string::npos) && (pos < len))
    {
      str += seq[pos++];
    }

      // Put valid str buffer into the supplied list. If the specified string
      // does not match the value type, it is skipped.
    if (!str.empty())
    {
      typename Container::value_type val;
      if (setValueFromString(val, str))
      {
        L.push_back(val);
      }
    }
  }

  return L.size();

} /* splitStr */


} /* namespace */

#endif /* COMMON_INCLUDED */



/*
 * This file has not been truncated
 */

