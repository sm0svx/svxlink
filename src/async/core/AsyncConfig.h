/**
@file	 AsyncConfig.h
@brief   A class for reading "INI-foramtted" configuration files
@author  Tobias Blomberg
@date	 2004-03-17

This file contains a class that is used to read configuration files that is
in the famous MS Windows INI file format. An example of a configuration file
is shown below.

\include test.cfg

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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
#include <memory>
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
@brief	A class for reading INI-formatted configuration files
@author Tobias Blomberg
@date   2004-03-17

This class is used to read configuration files that is in the famous MS Windows
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
    Config(void) : file(NULL) {}
  
    /**
     * @brief 	Destructor
     */
    ~Config(void);
  
    /**
     * @brief 	Open the given config file
     * @param 	name The name of the configuration file to open
     * @return	Returns \em true on success or else \em false.
     *
     * This function will read the given configuration file into memory.
     * If this function return false and errno != 0, the errno variable may
     * give a hint what the problem was.
     */
    bool open(const std::string& name);
    
    /**
     * @brief 	Return the string value of the given configuration variable
     * @param 	section The name of the section where the configuration
     *	      	      	variable is located
     * @param 	tag   	The name of the configuration variable to get
     * @return	Returns String with content of the configuration variable.
     *                  If no variable is found an empty string is returned
     *
     * This function will return the string value corresponding to the given
     * configuration variable. If the configuration variable is unset, an
     * empty sting is returned.
     */
    const std::string &getValue(const std::string& section,
				 const std::string& tag) const;

    /**
     * @brief 	Get the string value of the given configuration variable
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	value 	   The value is returned in this argument. Any previous
     *	      	      	   contents is wiped
     * @return	Returns \em true on success or else \em false on failure
     *
     * This function is used to get the value for a configuration variable
     * of type "string".
     */
    bool getValue(const std::string& section, const std::string& tag,
      	      	  std::string& value) const;

    /**
     * @brief 	Get the value of the given configuration variable.
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	rsp 	   The value is returned in this argument.
     *	      	      	   Successful completion overwrites previous contents
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true on success or else \em false on failure.
     *
     * This function is used to get the value of a configuraiton variable.
     * It's a template function meaning that it can take any value type
     * that supports the operator>> function. Note that when the value is of
     * type string, the overloaded getValue is used rather than this function.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * still returns \em false if an illegal value is specified.
     */
    template <typename Rsp>
    bool getValue(const std::string& section, const std::string& tag,
		  Rsp &rsp, bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
	return missing_ok;
      }
      std::stringstream ssval(str_val);
      Rsp tmp;
      ssval >> tmp;
      if(!ssval.eof())
      {
        ssval >> std::ws;
      }
      if (ssval.fail() || !ssval.eof())
      {
	return false;
      }
      rsp = tmp;
      return true;
    } /* Config::getValue */

    /**
     * @brief 	Get the value of the given config variable into container
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	c 	   The value is returned in this argument.
     *	      	      	   Successful completion overwrites previous contents
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true on success or else \em false on failure.
     *
     * This function is used to get the value of a configuraiton variable.
     * The config variable is read into a container (e.g. vector, list etc).
     * It's a template function meaning that it can take any value type
     * that supports the operator>> function. 
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * still returns \em false if an illegal value is specified.
     */
    template <template <typename, typename> class Container,
              typename Value>
    bool getValue(const std::string& section, const std::string& tag,
		  Container<Value, std::allocator<Value> > &c,
                  bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
	return missing_ok;
      }
      if (str_val.empty())
      {
        c.clear();
        return true;
      }
      std::stringstream ssval(str_val);
      while (!ssval.eof())
      {
        Value tmp;
        ssval >> tmp;
        if(!ssval.eof())
        {
          ssval >> std::ws;
        }
        if (ssval.fail())
        {
          return false;
        }
        c.push_back(tmp);
      }
      return true;
    } /* Config::getValue */

    /**
     * @brief 	Get a range checked variable value
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get.
     * @param 	min   	   Smallest valid value.
     * @param 	max   	   Largest valid value.
     * @param 	rsp 	   The value is returned in this argument.
     *	      	      	   Successful completion overwites prevoius contents.
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true if value is within range otherwise \em false.
     *
     * This function is used to get the value of the given configuration
     * variable, checking if it is within the given range (min <= value <= max).
     * Requires operators >>, < and > to be defined in the value object.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * till returns \em false if an illegal value is specified.
     */
    template <typename Rsp>
    bool getValue(const std::string& section, const std::string& tag,
		  const Rsp& min, const Rsp& max, Rsp &rsp,
		  bool missing_ok = false) const
    {
      std::string str_val;
      if (!getValue(section, tag, str_val))
      {
	return missing_ok;
      }
      std::stringstream ssval(str_val);
      Rsp tmp;
      ssval >> tmp;
      if(!ssval.eof())
      {
        ssval >> std::ws;
      }
      if (ssval.fail() || !ssval.eof() || (tmp < min) || (tmp > max))
      {
	return false;
      }
      rsp = tmp;
      return true;
    } /* Config::getValue */

    /**
     * @brief   Return the name of all configuration sections
     * @return  Returns a list of all existing section names
     */
    std::list<std::string> listSections(void);

    /**
     * @brief 	Return the name of all the tags in the given section
     * @param 	section The name of the section where the configuration
     *	      	      	variables are located
     * @return	Returns the list of tags in the given section
     */
    std::list<std::string> listSection(const std::string& section);

    /**
     * @brief   Set the value of a configuration variable
     * @param 	section   The name of the section where the configuration
     *	      	      	  variable is located
     * @param 	tag   	  The name of the configuration variable to set.
     * @param   value     The value to set
     *
     * This function is used to set the value of a configuration variable.
     * If the given configuration section or variable does not exist, it
     * is created.
     * Note that this function will not write anything back to the
     * associated configuration file. It will only set the value in memory.
     */
    void setValue(const std::string& section, const std::string& tag,
      	      	  const std::string& value);
    
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

