/**
@file	 AsyncConfig.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg
@date	 2004-03-17

A_detailed_description_for_this_file

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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

/** @example AsyncConfig_demo.cpp
An example of how to use the Config class
*/


#ifndef ASYNC_CONFIG_INCLUDED
#define ASYNC_CONFIG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>

#include <string>
#include <map>
#include <list>


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

namespace Async
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
@brief	A_brief_class_description
@author Tobias Blomberg
@date   2003-04-

A_detailed_class_description

\include AsyncConfig_demo.cpp
*/
class Config
{
  public:
    /**
     * @brief 	Default constuctor
     */
    Config(void);
  
    /**
     * @brief 	Destructor
     */
    ~Config(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    
    bool open(const std::string& name);
    
    bool getValue(const std::string& section, const std::string& tag,
      	      	  std::string& value);
    
    //bool setValue(const std::string& section, const std::string& tag,
    //  	      	  const std::string& value);
    
  protected:
    
  private:
    typedef std::map<std::string, std::string>	Values;
    typedef std::map<std::string, Values>   	Sections;
    
    FILE      *file;
    Sections  sections;
    
    bool parseCfgFile(void);
    char *trimSpaces(char *line);
    char *parseSection(char *line);
    char *parseDelimitedString(char *str, char begin_tok, char end_tok);
    bool parseValueLine(char *line, std::string& tag, std::string& value);
    char *parseValue(char *value);
    char *translateEscapedChars(char *val);

};  /* class Config */


} /* namespace */

#endif /* ASYNC_CONFIG_INCLUDED */



/*
 * This file has not been truncated
 */

