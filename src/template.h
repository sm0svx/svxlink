/**
@file   MyNamespaceTemplate.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2025-

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
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

/** @example MyNamespaceTemplate_demo.cpp
An example of how to use the MyNamespace::Template class
*/

#ifndef TEMPLATE_INCLUDED
#define TEMPLATE_INCLUDED


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

namespace MyNamespace
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
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2025-

A_detailed_class_description

\include MyNamespaceTemplate_demo.cpp
*/
class Template
{
  public:
    /**
     * @brief   Default constructor
     */
    Template(void);

    /**
     * @brief   Disallow copy construction
     */
    Template(const Template&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    Template& operator=(const Template&) = delete;

    /**
     * @brief   Destructor
     */
    ~Template(void);

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */

  protected:

  private:

};  /* class Template */


} /* namespace MyNamespace */

#endif /* TEMPLATE_INCLUDED */

/*
 * This file has not been truncated
 */
