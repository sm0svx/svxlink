/**
@file	 AsyncFileConfigBackend.h
@brief   File-based configuration backend implementation
@author  Tobias Blomberg / SM0SVX, Rui Barreiros / CR7BPM
@date	 2025-09-19

This file contains the file-based configuration backend that reads INI-formatted
configuration files, similar to the original AsyncConfig implementation.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2026 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_FILE_CONFIG_BACKEND_INCLUDED
#define ASYNC_FILE_CONFIG_BACKEND_INCLUDED

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

#include "AsyncConfigBackend.h"

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
@brief	File-based configuration backend
@author Tobias Blomberg / SM0SVX
@date   2025-09-19

This class implements a configuration backend that reads INI-formatted
configuration files. It provides the same functionality as the original
AsyncConfig class but through the ConfigBackend interface.

*/
class FileConfigBackend : public ConfigBackend
{
  public:
    static constexpr const char* OBJNAME = "file";

    /**
     * @brief 	Default constructor
     */
    FileConfigBackend(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~FileConfigBackend(void);
  
    /**
     * @brief 	Open the given config file
     * @param 	source The path to the configuration file to open
     * @return	Returns \em true on success or else \em false.
     *
     * This function will read the given configuration file into memory.
     * If this function return false and errno != 0, the errno variable may
     * give a hint what the problem was.
     */
    virtual bool open(const std::string& source) override;
    
    /**
     * @brief 	Close the configuration file
     *
     * This function closes the file and cleans up resources.
     */
    virtual void close(void) override;
    
    /**
     * @brief 	Check if the file backend is open
     * @return	Returns \em true if file is loaded, \em false otherwise
     */
    virtual bool isOpen(void) const override;
    
    /**
     * @brief 	Get the string value of a configuration variable
     * @param 	section The name of the section where the configuration
     *	      	      	variable is located
     * @param 	tag   	The name of the configuration variable to get
     * @param 	value 	The value is returned in this argument
     * @return	Returns \em true on success or else \em false on failure
     */
    virtual bool getValue(const std::string& section, const std::string& tag,
                          std::string& value) const override;

    /**
     * @brief 	Set the value of a configuration variable
     * @param 	section   The name of the section where the configuration
     *	      	      	  variable is located
     * @param 	tag   	  The name of the configuration variable to set.
     * @param   value     The value to set
     * @return	Returns \em true on success or else \em false on failure
     *
     * Note: This function only modifies the in-memory representation.
     * It does not write back to the configuration file.
     */
    virtual bool setValue(const std::string& section, const std::string& tag,
                          const std::string& value) override;

    /**
     * @brief   Return the name of all configuration sections
     * @return  Returns a list of all existing section names
     */
    virtual std::list<std::string> listSections(void) const override;

    /**
     * @brief 	Return the name of all the tags in the given section
     * @param 	section The name of the section where the configuration
     *	      	      	variables are located
     * @return	Returns the list of tags in the given section
     */
    virtual std::list<std::string> listSection(const std::string& section) const override;

    /**
     * @brief   Get backend type identifier
     * @return  Returns "file"
     */
    virtual std::string getBackendType(void) const override;

    /**
     * @brief   Get backend-specific information
     * @return  Returns the file path
     */
    virtual std::string getBackendInfo(void) const override;

  protected:

  private:
    struct Value
    {
      std::string val;
    };
    typedef std::map<std::string, Value>  Values;
    typedef std::map<std::string, Values> Sections;

    Sections    m_sections;
    std::string m_filename;
    bool        m_is_open;

    bool parseCfgFile(FILE *file);
    char *trimSpaces(char *line);
    char *parseSection(char *line);
    char *parseDelimitedString(char *str, char begin_tok, char end_tok);
    bool parseValueLine(char *line, std::string& tag, std::string& value);
    char *parseValue(char *value);
    char *translateEscapedChars(char *val);

}; /* class FileConfigBackend */

} /* namespace */

#endif /* ASYNC_FILE_CONFIG_BACKEND_INCLUDED */

/*
 * This file has not been truncated
 */
