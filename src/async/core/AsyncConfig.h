/**
@file	 AsyncConfig.h
@brief   A class for reading configuration files that is on the famous
      	 INI file format
@author  Tobias Blomberg
@date	 2004-03-17

This file contains a class that is used to read configuration files that is
on the famous MS Windows INI file format. An example of a configuration file
is shown below.

\include test.cfg

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
@brief	A class for reading configuration files that is on the famous
      	INI file format
@author Tobias Blomberg
@date   2004-03-17

This class is used to read configuration files that is on the famous MS Windows
INI file format. An example of a configuration file and how to use the class
is shown below.

\include test.cfg

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
     * @brief 	Open the given config file
     * @param 	name The name of the configuration file to open
     * @return	Returns \em true on success or else \em false.
     */
    bool open(const std::string& name);
    
    /**
     * @brief 	Get the value of the given configuration variable
     * @param 	section The name of the section where the configuration
     *	      	      	variable is located
     * @param 	tag   	The name of the configuration variable to get
     * @param 	value 	The value is returned in this argument. Any previous
     *	      	      	contents is wiped
     * @return	Returns \em true on success or else \em false on failure
     */
    bool getValue(const std::string& section, const std::string& tag,
      	      	  std::string& value);
    
    /**
     * @brief 	Return the name of all the tags in the given section
     * @param 	section The name of the section where the configuration
     *	      	      	variables are located
     * @return	Returns the list of tags in the given section
     */
    std::list<std::string> listSection(const std::string& section);
    
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

