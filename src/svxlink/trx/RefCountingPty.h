/**
@file	 RefCountingPty.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2018-01-14

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

#ifndef REF_COUNTING_PTY_INCLUDED
#define REF_COUNTING_PTY_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <map>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncPty.h>


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2018-01-14

A_detailed_class_description
*/
class RefCountingPty : public Async::Pty
{
  public:
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    static RefCountingPty *instance(const std::string& name)
    {
      RefCountingPty *pty = 0;
      PtyMap::iterator it = ptys().find(name);
      if (it == ptys().end())
      {
        pty = new RefCountingPty(name);
        if (!pty->open())
        {
          delete pty;
          pty = 0;
        }
        ptys()[name] = pty;
      }
      else
      {
        pty = it->second;
        pty->m_refs += 1;
      }
      return pty;
    }

    const std::string& name(void) const { return m_name; }

    void destroy(void)
    {
      assert(m_refs > 0);
      if (--m_refs == 0)
      {
        close();
        ptys().erase(m_name);
        delete this;
      }
    }

  private:
    typedef std::map<std::string, RefCountingPty*> PtyMap;

    std::string m_name;
    unsigned    m_refs;

    static PtyMap& ptys(void)
    {
      static PtyMap pty_map;
      return pty_map;
    }

    RefCountingPty(const std::string& name) : Pty(name), m_refs(1) {}
    ~RefCountingPty(void) {}
    RefCountingPty(const RefCountingPty&);
    RefCountingPty& operator=(const RefCountingPty&);

};  /* class RefCountingPty */


//} /* namespace */

#endif /* REF_COUNTING_PTY_INCLUDED */



/*
 * This file has not been truncated
 */
