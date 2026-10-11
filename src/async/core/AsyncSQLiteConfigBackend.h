/**
@file	 AsyncSQLiteConfigBackend.h
@brief   SQLite-based configuration backend implementation
@author  Rui Barreiros / CR7BPM
@date	 2025-09-19

This file contains the SQLite-based configuration backend that stores
configuration data in an SQLite database.

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

#ifndef ASYNC_SQLITE_CONFIG_BACKEND_INCLUDED
#define ASYNC_SQLITE_CONFIG_BACKEND_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
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

struct sqlite3;

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
@brief	SQLite-based configuration backend
@author Rui Barreiros / CR7BPM
@date   2025-09-19

This class implements a configuration backend that stores configuration
data in an SQLite database. The database schema consists of a single table
with columns for section, tag, and value.

Database schema:
CREATE TABLE config (
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  section TEXT NOT NULL,
  tag TEXT NOT NULL,
  value TEXT NOT NULL,
  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,
  UNIQUE(section, tag)
);
*/
class SQLiteConfigBackend : public ConfigBackend
{
  public:
    static constexpr const char* OBJNAME = "sqlite";

    /**
     * @brief 	Default constructor
     */
    SQLiteConfigBackend(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~SQLiteConfigBackend(void);
  
    /**
     * @brief 	Open the SQLite database
     * @param 	source The path to the SQLite database file
     * @return	Returns \em true on success or else \em false.
     *
     * This function will open the SQLite database and create the necessary
     * tables if they don't exist. The source parameter should be the path
     * to the SQLite database file.
     */
    virtual bool open(const std::string& source) override;
    
    /**
     * @brief 	Close the SQLite database connection
     *
     * This function closes the database connection and cleans up resources.
     */
    virtual void close(void) override;
    
    /**
     * @brief 	Check if the database connection is open
     * @return	Returns \em true if connected, \em false otherwise
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
     * This function inserts or updates the configuration variable in the database.
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
     * @return  Returns "sqlite"
     */
    virtual std::string getBackendType(void) const override;

    /**
     * @brief   Get backend-specific information
     * @return  Returns the database file path
     */
    virtual std::string getBackendInfo(void) const override;

    /**
     * @brief   Check for external changes to the database
     * @return  true if changes were detected, false otherwise
     *
     * This method queries the database for any records that have been modified
     * since the last check and emits valueChanged signals for each change.
     */
    virtual bool checkForExternalChanges(void) override;

    /**
     * @brief Initialize database tables
     * @return true on success, false on failure
     */
    virtual bool initializeTables(void) override;

    /**
     * @brief Finalize database initialization after tables are populated
     * @return true on success, false on failure
     */
    virtual bool finalizeInitialization(void) override;

  protected:

  private:
    sqlite3*    m_db;
    std::string m_db_path;
    std::string m_last_check_time;

    bool createTables(void);
    bool executeSQL(const std::string& sql) const;
    std::string getLastError(void) const;
    void initializeLastCheckTime(void);

}; /* class SQLiteConfigBackend */

} /* namespace */

#endif /* ASYNC_SQLITE_CONFIG_BACKEND_INCLUDED */

/*
 * This file has not been truncated
 */
