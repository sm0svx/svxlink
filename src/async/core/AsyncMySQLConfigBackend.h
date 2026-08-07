/**
@file	 AsyncMySQLConfigBackend.h
@brief   MySQL/MariaDB-based configuration backend implementation
@author  Rui Barreiros / CR7BPM
@date	 2025-09-19

This file contains the MySQL/MariaDB-based configuration backend that stores
configuration data in a MySQL or MariaDB database.

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

#ifndef ASYNC_MYSQL_CONFIG_BACKEND_INCLUDED
#define ASYNC_MYSQL_CONFIG_BACKEND_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <list>
#include <mutex>

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

struct MYSQL;

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
@brief	MySQL/MariaDB-based configuration backend
@author Rui Barreiros / CR7BPM
@date   2025-09-19

This class implements a configuration backend that stores configuration
data in a MySQL or MariaDB database. The database schema consists of a single
table with columns for section, tag, and value.

Connection string format:
"host=hostname;port=3306;user=username;password=password;database=dbname"

Database schema:
CREATE TABLE config (
  id INT AUTO_INCREMENT PRIMARY KEY,
  section VARCHAR(255) NOT NULL,
  tag VARCHAR(255) NOT NULL,
  value TEXT NOT NULL,
  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
  UNIQUE KEY unique_config (section, tag)
);
*/
class MySQLConfigBackend : public ConfigBackend
{
  public:
    static constexpr const char* OBJNAME = "mysql";

    /**
     * @brief 	Default constructor
     */
    MySQLConfigBackend(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~MySQLConfigBackend(void);
  
    /**
     * @brief 	Connect to the MySQL database
     * @param 	source The connection string
     * @return	Returns \em true on success or else \em false.
     *
     * This function will connect to the MySQL database and create the necessary
     * tables if they don't exist. The source parameter should be a connection
     * string in the format:
     * "host=hostname;port=3306;user=username;password=password;database=dbname"
     */
    virtual bool open(const std::string& source) override;
    
    /**
     * @brief 	Close the MySQL database connection
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
     * @return  Returns "mysql"
     */
    virtual std::string getBackendType(void) const override;

    /**
     * @brief   Get backend-specific information
     * @return  Returns the connection information (without password)
     */
    virtual std::string getBackendInfo(void) const override;

    /**
     * @brief   Check for external changes to the database
     * @return  true if changes were detected, false otherwise
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
    virtual bool pollForExternalChanges(void) override;
    virtual bool openPollConnection(void) override;
    virtual void closePollConnection(void) override;

  private:
    struct ConnectionParams
    {
      std::string host;
      unsigned int port;
      std::string user;
      std::string password;
      std::string database;
    };

    MYSQL*           m_mysql;
    MYSQL*           m_mysql_poll;
    ConnectionParams m_conn_params;
    std::string      m_connection_string;
    std::string      m_last_check_time;
    mutable std::mutex m_last_check_mutex;

    bool parseConnectionString(const std::string& conn_str, ConnectionParams& params);
    bool createTables(void);
    bool connectMysql(MYSQL*& mysql);
    bool checkForExternalChangesUsing(MYSQL* mysql);
    std::string escapeString(const std::string& str) const;
    std::string escapeString(const std::string& str, MYSQL* mysql) const;
    std::string getLastError(void) const;
    std::string getLastError(MYSQL* mysql) const;
    void initializeLastCheckTime(void);

}; /* class MySQLConfigBackend */

} /* namespace */

#endif /* ASYNC_MYSQL_CONFIG_BACKEND_INCLUDED */

/*
 * This file has not been truncated
 */
