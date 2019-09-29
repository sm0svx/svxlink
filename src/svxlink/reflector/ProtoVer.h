/**
@file   ProtoVer.h
@brief  A couple of classes for doing protocol version manipulations
@author Tobias Blomberg / SM0SVX
@date   2019-07-25

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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

#ifndef PROTO_VER_INCLUDED
#define PROTO_VER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <limits>


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
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  A class for doing protocol version manipulations
@author Tobias Blomberg / SM0SVX
@date   2019-07-25

This class is used to represent a protocol version and to compare one version
to another.
*/
class ProtoVer
{
  public:
    static ProtoVer max(void)
    {
      return ProtoVer(
          std::numeric_limits<uint16_t>::max(),
          std::numeric_limits<uint16_t>::max());
    }

    /**
     * @brief   Default constructor
     */
    ProtoVer(void) : m_major_ver(0), m_minor_ver(0) {}
    ProtoVer(uint16_t major_ver, uint16_t minor_ver)
      : m_major_ver(major_ver), m_minor_ver(minor_ver) {}
    void set(uint16_t major_ver, uint16_t minor_ver)
    {
      m_major_ver = major_ver;
      m_minor_ver = minor_ver;
    }
    uint16_t majorVer(void) const { return m_major_ver; }
    uint16_t minorVer(void) const { return m_minor_ver; }
    bool isValid(void) const { return (m_major_ver > 0) || (m_minor_ver > 0); }
    bool operator ==(const ProtoVer& rhs) const
    {
      return (m_major_ver == rhs.m_major_ver) &&
             (m_minor_ver == rhs.m_minor_ver);
    }
    bool operator !=(const ProtoVer& rhs) const
    {
      return !(m_major_ver == rhs.m_major_ver);
    }
    bool operator <(const ProtoVer& rhs) const
    {
      return (m_major_ver < rhs.m_major_ver) ||
             ((m_major_ver == rhs.m_major_ver) &&
              (m_minor_ver < rhs.m_minor_ver));
    }
    bool operator >(const ProtoVer& rhs) const
    {
      return (m_major_ver > rhs.m_major_ver) ||
             ((m_major_ver == rhs.m_major_ver) &&
              (m_minor_ver > rhs.m_minor_ver));
    }
    bool operator <=(const ProtoVer& rhs) const
    {
      return (*this == rhs) || (*this < rhs);
    }
    bool operator >=(const ProtoVer& rhs) const
    {
      return (*this == rhs) || (*this > rhs);
    }

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */

  private:
    uint16_t m_major_ver;
    uint16_t m_minor_ver;

};  /* class ProtoVer */


/**
@brief  A class for doing protocol version manipulations on ranges
@author Tobias Blomberg / SM0SVX
@date   2019-07-25

This class is used to represent a protocol version range and to check if a
version is within that range.
*/
class ProtoVerRange
{
  public:
    ProtoVerRange(void) {}
    ProtoVerRange(const ProtoVer& min, const ProtoVer& max)
      : m_min(min), m_max(max)
    {
    }

    bool isValid(void) const { return m_min.isValid() && m_max.isValid(); }

    bool isWithinRange(const ProtoVer& ver) const
    {
      return (ver >= m_min) && (ver <= m_max);
    }

  private:
    ProtoVer m_min;
    ProtoVer m_max;
};  /* class ProtoVerRange */


//} /* namespace */

#endif /* PROTO_VER_INCLUDED */

/*
 * This file has not been truncated
 */
