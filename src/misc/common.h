/**
@file	 common.h
@brief   Contains some commonly used global functions
@author  Tobias Blomberg / SM0SVX
@date	 2010-04-05

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

#include <utility>
#include <sstream>
#include <istream>
#include <ostream>
#include <cstring>
#include <cassert>


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
bool setValueFromString(ValueType &val, const std::string &str)
{
  std::istringstream ss(str);
  ss >> std::noskipws >> val;
  if(!ss.eof())
  {
    ss >> std::ws;
  }
  return !ss.fail() && ss.eof();
} /* setValueFromString */


/**
 * @brief   Set a value of any type supporting streaming from a string
 * @param   val Reference to the value to set
 * @param   str The sting to set the value from
 * @returns Returns \em true if the operation succeeded or \em false if not
 *
 * This function overload the more general template function when the value
 * to be set is an std::string. It just copies the incoming string to the
 * outgoing value. If streaming were to be used for strings, white space
 * would be unconditionally ignored.
 */
bool setValueFromString(std::string &val, const std::string &str);


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
 * supports the clear, size and insert methods (e.g. vector, list, deque,
 * set etc). Insertions will be done at the end of the container.
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
        L.insert(L.end(), val);
      }
    }
  }

  return L.size();

} /* splitStr */


/**
 * @brief A pair type with separator which can be streamed
 * @param a The first type in the pair
 * @param b The second type in the pair
 * @param sep The character used to separate the two values (default colon)
 *
 * This class was created to make it easy to stream a pair type in or out.
 * For example, if we read the string "100.5:1000" from an input stream and
 * we declare the variable "SepPair<float, unsigned> my_pair", we can write
 * "is >> my_pair". Then we have my_pair.first == 100.5 and
 * my_pair.second == 1000.
 */
template <typename a, typename b, char sep=':'>
class SepPair : public std::pair<a, b>
{
  public:
    friend std::istream &operator>>(std::istream &is, SepPair &cp)
    {
      std::string value;
      is >> value;
      std::string::size_type seppos = value.find(sep);
      if (seppos == std::string::npos)
      {
        is.setstate(std::ios_base::failbit);
        return is;
      }
      std::string first(value.substr(0, seppos));
      std::string second(value.substr(seppos+1));
      if (!setValueFromString(cp.first, first) ||
          !setValueFromString(cp.second, second))
      {
        is.setstate(std::ios_base::failbit);
      }
      return is;
    }

    friend std::ostream &operator<<(std::ostream &output, SepPair &cp)
    {
      output << cp.first << sep << cp.second;
      return output;
    }
};


static inline const char* strError(int errnum)
{
  static thread_local struct
  {
    char errstr[256];

      // XSI version of strerror_r
    const char* operator()(int ret)
    {
      assert(ret == 0);
      return errstr;
    }

      // GNU version of strerror_r
    const char* operator()(char* ret)
    {
      return ret;
    }
  } wrap;
  return wrap(strerror_r(errno, wrap.errstr, sizeof(wrap.errstr)));
} /* strError */


} /* namespace */

#endif /* COMMON_INCLUDED */



/*
 * This file has not been truncated
 */

