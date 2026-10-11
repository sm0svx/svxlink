/**
@file	 AsyncMySQLConfigBackend.cpp
@brief   MySQL/MariaDB-based configuration backend implementation
@author  Rui Barreiros / CR7BPM
@date	 2025-09-19

This file contains the implementation of the MySQL/MariaDB-based configuration backend.

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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <mysql/mysql.h>
#include <iostream>
#include <sstream>
#include <stdexcept>

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

#include "AsyncMySQLConfigBackend.h"

/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

#ifdef HAS_MYSQL_SUPPORT
// Factory registration for MySQL backend
static ConfigBackendSpecificFactory<MySQLConfigBackend> mysql_factory;
#endif

/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

MySQLConfigBackend::MySQLConfigBackend(void)
  : ConfigBackend(true, 300000), m_mysql(nullptr), m_mysql_poll(nullptr),
    m_last_check_time("1970-01-01 00:00:00")
{
  m_conn_params.port = 3306; // Default MySQL port
} /* MySQLConfigBackend::MySQLConfigBackend */

MySQLConfigBackend::~MySQLConfigBackend(void)
{
  // Stop the poll thread before closing the connection handle!!!
  stopAutoPolling();
  close();
} /* MySQLConfigBackend::~MySQLConfigBackend */

bool MySQLConfigBackend::open(const string& source)
{
  close();
  
  m_connection_string = source;
  
  if (!parseConnectionString(source, m_conn_params))
  {
    cerr << "*** ERROR: Invalid MySQL connection string format" << endl;
    return false;
  }
  
  if (!connectMysql(m_mysql))
  {
    cerr << "*** ERROR: Failed to connect to MySQL database: "
         << getLastError(m_mysql) << endl;
    close();
    return false;
  }

  // Note: Tables will be created later after table prefix is set
  // createTables() is now called from initializeDatabase()

  return true;
} /* MySQLConfigBackend::open */

void MySQLConfigBackend::close(void)
{
  closePollConnection();
  if (m_mysql != nullptr)
  {
    mysql_close(m_mysql);
    m_mysql = nullptr;
  }
  m_connection_string.clear();
} /* MySQLConfigBackend::close */

bool MySQLConfigBackend::openPollConnection(void)
{
  closePollConnection();

  if (!isOpen())
  {
    return false;
  }

  if (!connectMysql(m_mysql_poll))
  {
    cerr << "*** ERROR: Failed to open MySQL poll connection: "
         << getLastError(m_mysql_poll) << endl;
    closePollConnection();
    return false;
  }

  return true;
} /* MySQLConfigBackend::openPollConnection */

void MySQLConfigBackend::closePollConnection(void)
{
  if (m_mysql_poll != nullptr)
  {
    mysql_close(m_mysql_poll);
    m_mysql_poll = nullptr;
  }
} /* MySQLConfigBackend::closePollConnection */

bool MySQLConfigBackend::pollForExternalChanges(void)
{
  return checkForExternalChangesUsing(m_mysql_poll);
} /* MySQLConfigBackend::pollForExternalChanges */

bool MySQLConfigBackend::isOpen(void) const
{
  return (m_mysql != nullptr);
} /* MySQLConfigBackend::isOpen */

bool MySQLConfigBackend::getValue(const std::string& section, const std::string& tag,
                                  std::string& value) const
{
  if (!isOpen())
  {
    return false;
  }
  
  std::string table_name = getFullTableName("config");
  string escaped_section = escapeString(section);
  string escaped_tag = escapeString(tag);
  
  ostringstream query;
  query << "SELECT value FROM " << table_name << " WHERE section = '" << escaped_section 
        << "' AND tag = '" << escaped_tag << "'";
  
  if (mysql_query(m_mysql, query.str().c_str()) != 0)
  {
    cerr << "*** ERROR: Failed to execute SELECT query: " << getLastError() << endl;
    return false;
  }
  
  MYSQL_RES* result = mysql_store_result(m_mysql);
  if (result == nullptr)
  {
    cerr << "*** ERROR: Failed to get query result: " << getLastError() << endl;
    return false;
  }
  
  MYSQL_ROW row = mysql_fetch_row(result);
  if (row != nullptr && row[0] != nullptr)
  {
    value = row[0];
    mysql_free_result(result);
    return true;
  }
  
  mysql_free_result(result);
  return false;
} /* MySQLConfigBackend::getValue */

bool MySQLConfigBackend::setValue(const std::string& section, const std::string& tag,
                                  const std::string& value)
{
  if (!isOpen())
  {
    return false;
  }
  
  std::string table_name = getFullTableName("config");
  string escaped_section = escapeString(section);
  string escaped_tag = escapeString(tag);
  string escaped_value = escapeString(value);
  
  ostringstream query;
  query << "INSERT INTO " << table_name << " (section, tag, value) VALUES ('"
        << escaped_section << "', '" << escaped_tag << "', '" << escaped_value
        << "') ON DUPLICATE KEY UPDATE value = '" << escaped_value 
        << "', updated_at = CURRENT_TIMESTAMP";
  
  if (mysql_query(m_mysql, query.str().c_str()) != 0)
  {
    cerr << "*** ERROR: Failed to execute INSERT/UPDATE query: " << getLastError() << endl;
    return false;
  }
  
  notifyValueChanged(section, tag, value);
  return true;
} /* MySQLConfigBackend::setValue */

list<string> MySQLConfigBackend::listSections(void) const
{
  list<string> sections;
  
  if (!isOpen())
  {
    return sections;
  }
  
  std::string table_name = getFullTableName("config");
  std::ostringstream query;
  query << "SELECT DISTINCT section FROM " << table_name << " ORDER BY section";
  
  if (mysql_query(m_mysql, query.str().c_str()) != 0)
  {
    cerr << "*** ERROR: Failed to execute SELECT DISTINCT query: " << getLastError() << endl;
    return sections;
  }
  
  MYSQL_RES* result = mysql_store_result(m_mysql);
  if (result == nullptr)
  {
    cerr << "*** ERROR: Failed to get query result: " << getLastError() << endl;
    return sections;
  }
  
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr)
  {
    if (row[0] != nullptr)
    {
      sections.push_back(row[0]);
    }
  }
  
  mysql_free_result(result);
  return sections;
} /* MySQLConfigBackend::listSections */

list<string> MySQLConfigBackend::listSection(const string& section) const
{
  list<string> tags;
  
  if (!isOpen())
  {
    return tags;
  }
  
  std::string table_name = getFullTableName("config");
  string escaped_section = escapeString(section);
  
  ostringstream query;
  query << "SELECT tag FROM " << table_name << " WHERE section = '" << escaped_section 
        << "' ORDER BY tag";
  
  if (mysql_query(m_mysql, query.str().c_str()) != 0)
  {
    cerr << "*** ERROR: Failed to execute SELECT tags query: " << getLastError() << endl;
    return tags;
  }
  
  MYSQL_RES* result = mysql_store_result(m_mysql);
  if (result == nullptr)
  {
    cerr << "*** ERROR: Failed to get query result: " << getLastError() << endl;
    return tags;
  }
  
  MYSQL_ROW row;
  while ((row = mysql_fetch_row(result)) != nullptr)
  {
    if (row[0] != nullptr)
    {
      tags.push_back(row[0]);
    }
  }
  
  mysql_free_result(result);
  return tags;
} /* MySQLConfigBackend::listSection */

std::string MySQLConfigBackend::getBackendType(void) const
{
  return "mysql";
} /* MySQLConfigBackend::getBackendType */

std::string MySQLConfigBackend::getBackendInfo(void) const
{
  if (!isOpen())
  {
    return "Not connected";
  }
  
  ostringstream info;
  info << "host=" << m_conn_params.host 
       << ";port=" << m_conn_params.port
       << ";user=" << m_conn_params.user
       << ";database=" << m_conn_params.database;
  
  return info.str();
} /* MySQLConfigBackend::getBackendInfo */

bool MySQLConfigBackend::initializeTables(void)
{
  if (!isOpen())
  {
    cerr << "*** ERROR: Cannot initialize tables - database not open" << endl;
    return false;
  }
  
  // Create tables if they don't exist
  if (!createTables())
  {
    cerr << "*** ERROR: Failed to create database tables" << endl;
    return false;
  }
  
  return true;
} /* MySQLConfigBackend::initializeTables */

bool MySQLConfigBackend::finalizeInitialization(void)
{
  if (!isOpen())
  {
    cerr << "*** ERROR: Cannot finalize initialization - database not open" << endl;
    return false;
  }
  
  initializeLastCheckTime();
  
  return true;
} /* MySQLConfigBackend::finalizeInitialization */

bool MySQLConfigBackend::checkForExternalChanges(void)
{
  return checkForExternalChangesUsing(m_mysql);
} /* MySQLConfigBackend::checkForExternalChanges */

bool MySQLConfigBackend::checkForExternalChangesUsing(MYSQL* mysql)
{
  if (!isOpen() || mysql == nullptr)
  {
    return false;
  }

  std::string last_check_time;
  {
    std::lock_guard<std::mutex> lock(m_last_check_mutex);
    last_check_time = m_last_check_time;
  }

  // Query for all records updated since last check
  std::string table_name = getFullTableName("config");
  string escaped_last_check = escapeString(last_check_time, mysql);
  ostringstream query;
  query << "SELECT section, tag, value, updated_at FROM " << table_name << " "
        << "WHERE updated_at > '" << escaped_last_check << "' "
        << "ORDER BY updated_at";

  if (mysql_query(mysql, query.str().c_str()) != 0)
  {
    cerr << "*** ERROR: Failed to execute change detection query: "
         << getLastError(mysql) << endl;
    return false;
  }

  MYSQL_RES* result = mysql_store_result(mysql);
  if (!result)
  {
    cerr << "*** ERROR: Failed to store result: " << getLastError(mysql) << endl;
    return false;
  }

  bool changes_detected = false;
  std::string latest_timestamp = last_check_time;
  MYSQL_ROW row;

  while ((row = mysql_fetch_row(result)))
  {
    if (row[0] && row[1] && row[2] && row[3])
    {
      notifyValueChanged(std::string(row[0]), std::string(row[1]), std::string(row[2]));
      latest_timestamp = std::string(row[3]);
      changes_detected = true;
    }
  }

  mysql_free_result(result);

  // Update last check time to the latest timestamp seen
  if (changes_detected)
  {
    std::lock_guard<std::mutex> lock(m_last_check_mutex);
    m_last_check_time = latest_timestamp;
  }

  return changes_detected;
} /* MySQLConfigBackend::checkForExternalChangesUsing */

/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

bool MySQLConfigBackend::connectMysql(MYSQL*& mysql)
{
  mysql = mysql_init(nullptr);
  if (mysql == nullptr)
  {
    return false;
  }

  unsigned int timeout = 10;
  mysql_options(mysql, MYSQL_OPT_CONNECT_TIMEOUT, &timeout);

  bool reconnect = true;
  mysql_options(mysql, MYSQL_OPT_RECONNECT, &reconnect);

  if (mysql_real_connect(mysql,
                         m_conn_params.host.c_str(),
                         m_conn_params.user.c_str(),
                         m_conn_params.password.c_str(),
                         m_conn_params.database.c_str(),
                         m_conn_params.port,
                         nullptr, 0) == nullptr)
  {
    mysql_close(mysql);
    mysql = nullptr;
    return false;
  }

  return true;
} /* MySQLConfigBackend::connectMysql */

bool MySQLConfigBackend::parseConnectionString(const std::string& conn_str, ConnectionParams& params)
{
  // Parse connection string format: "host=hostname;port=3306;user=username;password=password;database=dbname"
  istringstream iss(conn_str);
  string token;
  
  while (getline(iss, token, ';'))
  {
    size_t eq_pos = token.find('=');
    if (eq_pos == string::npos)
    {
      continue;
    }
    
    string key = token.substr(0, eq_pos);
    string value = token.substr(eq_pos + 1);
    
    if (key == "host")
    {
      params.host = value;
    }
    else if (key == "port")
    {
      try
      {
        params.port = static_cast<unsigned int>(stoul(value));
      }
      catch (const std::exception&)
      {
        cerr << "*** ERROR: Invalid port in MySQL connection string" << endl;
        return false;
      }
    }
    else if (key == "user")
    {
      params.user = value;
    }
    else if (key == "password")
    {
      params.password = value;
    }
    else if (key == "database")
    {
      params.database = value;
    }
  }
  
  // Validate required parameters
  if (params.host.empty() || params.user.empty() || params.database.empty())
  {
    return false;
  }
  
  return true;
} /* MySQLConfigBackend::parseConnectionString */

bool MySQLConfigBackend::createTables(void)
{
  std::string table_name = getFullTableName("config");
  
  std::ostringstream create_table_sql;
  create_table_sql << "CREATE TABLE IF NOT EXISTS " << table_name << " ("
    "  id INT AUTO_INCREMENT PRIMARY KEY,"
    "  section VARCHAR(255) NOT NULL,"
    "  tag VARCHAR(255) NOT NULL,"
    "  value TEXT NOT NULL,"
    "  created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
    "  updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,"
    "  UNIQUE KEY unique_" << table_name << " (section, tag),"
    "  INDEX idx_" << table_name << "_section (section)"
    ") ENGINE=InnoDB DEFAULT CHARSET=utf8";
  
  if (mysql_query(m_mysql, create_table_sql.str().c_str()) != 0)
  {
    cerr << "*** ERROR: Failed to create config table: " << getLastError() << endl;
    return false;
  }
  
  return true;
} /* MySQLConfigBackend::createTables */

std::string MySQLConfigBackend::escapeString(const std::string& str) const
{
  return escapeString(str, m_mysql);
} /* MySQLConfigBackend::escapeString */

std::string MySQLConfigBackend::escapeString(const std::string& str, MYSQL* mysql) const
{
  if (mysql == nullptr || str.empty())
  {
    return str;
  }

  char* escaped = new char[str.length() * 2 + 1];
  unsigned long escaped_length =
      mysql_real_escape_string(mysql, escaped, str.c_str(), str.length());

  string result(escaped, escaped_length);
  delete[] escaped;

  return result;
} /* MySQLConfigBackend::escapeString */

std::string MySQLConfigBackend::getLastError(void) const
{
  return getLastError(m_mysql);
} /* MySQLConfigBackend::getLastError */

std::string MySQLConfigBackend::getLastError(MYSQL* mysql) const
{
  if (mysql != nullptr)
  {
    return mysql_error(mysql);
  }
  return "MySQL connection not initialized";
} /* MySQLConfigBackend::getLastError */

void MySQLConfigBackend::initializeLastCheckTime(void)
{
  if (!isOpen())
  {
    return;
  }

  // Query for the most recent updated_at timestamp in the database
  std::string table_name = getFullTableName("config");
  std::ostringstream query;
  query << "SELECT MAX(updated_at) FROM " << table_name;

  if (mysql_query(m_mysql, query.str().c_str()) != 0)
  {
    cerr << "*** WARNING: Failed to query max updated_at: " << getLastError() << endl;
    return;
  }

  MYSQL_RES* result = mysql_store_result(m_mysql);
  if (result == nullptr)
  {
    cerr << "*** WARNING: Failed to store result: " << getLastError() << endl;
    return;
  }

  MYSQL_ROW row = mysql_fetch_row(result);
  {
    std::lock_guard<std::mutex> lock(m_last_check_mutex);
    if (row != nullptr && row[0] != nullptr)
    {
      m_last_check_time = std::string(row[0]);
    }
    else
    {
      // Database is empty or all updated_at are NULL
      m_last_check_time = "1970-01-01 00:00:00";
    }
  }

  mysql_free_result(result);
} /* MySQLConfigBackend::initializeLastCheckTime */

/*
 * This file has not been truncated
 */
