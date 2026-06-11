/**
@file	 AsyncSQLiteConfigBackend.cpp
@brief   SQLite-based configuration backend implementation
@author  Rui Barreiros / CR7BPM
@date	 2025-09-19

This file contains the implementation of the SQLite-based configuration backend.

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

#include <sqlite3.h>
#include <iostream>
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

#include "AsyncSQLiteConfigBackend.h"

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

#ifdef HAS_SQLITE_SUPPORT
// Factory registration for SQLite backend
static ConfigBackendSpecificFactory<SQLiteConfigBackend> sqlite_factory;
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

SQLiteConfigBackend::SQLiteConfigBackend(void)
  : ConfigBackend(true, 300000), m_db(nullptr), m_last_check_time("1970-01-01 00:00:00")
{
} /* SQLiteConfigBackend::SQLiteConfigBackend */

SQLiteConfigBackend::~SQLiteConfigBackend(void)
{
  // Stop the poll thread before closing the database handle!!!  
  stopAutoPolling();
  close();
} /* SQLiteConfigBackend::~SQLiteConfigBackend */

bool SQLiteConfigBackend::open(const string& source)
{
  close();
  
  m_db_path = source;
  
  int rc = sqlite3_open(source.c_str(), &m_db);
  if (rc != SQLITE_OK)
  {
    cerr << "*** ERROR: Cannot open SQLite database '" << source 
         << "': " << sqlite3_errmsg(m_db) << endl;
    close();
    return false;
  }
  
  // Enable foreign keys
  if (!executeSQL("PRAGMA foreign_keys = ON"))
  {
    cerr << "*** ERROR: Failed to enable foreign keys" << endl;
    close();
    return false;
  }
  
  return true;
} /* SQLiteConfigBackend::open */

void SQLiteConfigBackend::close(void)
{
  if (m_db != nullptr)
  {
    sqlite3_close(m_db);
    m_db = nullptr;
  }
  m_db_path.clear();
} /* SQLiteConfigBackend::close */

bool SQLiteConfigBackend::isOpen(void) const
{
  return (m_db != nullptr);
} /* SQLiteConfigBackend::isOpen */

bool SQLiteConfigBackend::getValue(const std::string& section, const std::string& tag,
                                   std::string& value) const
{
  if (!isOpen())
  {
    return false;
  }
  
  std::string table_name = getFullTableName("config");
  std::string sql = "SELECT value FROM " + table_name + " WHERE section = ? AND tag = ?";
  sqlite3_stmt* stmt;
  
  int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    cerr << "*** ERROR: Failed to prepare SELECT statement: " << getLastError() << endl;
    return false;
  }
  
  sqlite3_bind_text(stmt, 1, section.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, tag.c_str(), -1, SQLITE_STATIC);
  
  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW)
  {
    const char* result = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (result != nullptr)
    {
      value = result;
    }
    else
    {
      value.clear();
    }
    sqlite3_finalize(stmt);
    return true;
  }
  else if (rc == SQLITE_DONE)
  {
    // No row found
    sqlite3_finalize(stmt);
    return false;
  }
  else
  {
    cerr << "*** ERROR: Failed to execute SELECT statement: " << getLastError() << endl;
    sqlite3_finalize(stmt);
    return false;
  }
} /* SQLiteConfigBackend::getValue */

bool SQLiteConfigBackend::setValue(const std::string& section, const std::string& tag,
                                   const std::string& value)
{
  if (!isOpen())
  {
    return false;
  }
  
  std::string table_name = getFullTableName("config");
  std::string sql = "INSERT OR REPLACE INTO " + table_name + " (section, tag, value, updated_at) "
                    "VALUES (?, ?, ?, CURRENT_TIMESTAMP)";
  sqlite3_stmt* stmt;
  
  int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    cerr << "*** ERROR: Failed to prepare INSERT statement: " << getLastError() << endl;
    return false;
  }
  
  sqlite3_bind_text(stmt, 1, section.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, tag.c_str(), -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 3, value.c_str(), -1, SQLITE_STATIC);
  
  /*
  cout << "*** DEBUG: Executing INSERT statement: " << sql << endl;
  cout << "*** DEBUG: Section: " << section << endl;
  cout << "*** DEBUG: Tag: " << tag << endl;
  cout << "*** DEBUG: Value: " << value << endl;
  */
  
  rc = sqlite3_step(stmt);
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_DONE)
  {
    cerr << "*** ERROR: Failed to execute INSERT statement: " << getLastError() << endl;
    return false;
  }
  
  notifyValueChanged(section, tag, value);
  return true;
} /* SQLiteConfigBackend::setValue */

list<string> SQLiteConfigBackend::listSections(void) const
{
  list<string> sections;
  
  if (!isOpen())
  {
    return sections;
  }
  
  std::string table_name = getFullTableName("config");
  std::string sql = "SELECT DISTINCT section FROM " + table_name + " ORDER BY section";
  sqlite3_stmt* stmt;
  
  int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    cerr << "*** ERROR: Failed to prepare SELECT DISTINCT statement: " << getLastError() << endl;
    return sections;
  }
  
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
  {
    const char* section = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (section != nullptr)
    {
      sections.push_back(section);
    }
  }
  
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_DONE)
  {
    cerr << "*** ERROR: Failed to execute SELECT DISTINCT statement: " << getLastError() << endl;
    sections.clear();
  }
  
  return sections;
} /* SQLiteConfigBackend::listSections */

list<string> SQLiteConfigBackend::listSection(const string& section) const
{
  list<string> tags;
  
  if (!isOpen())
  {
    return tags;
  }
  
  std::string table_name = getFullTableName("config");
  std::string sql = "SELECT tag FROM " + table_name + " WHERE section = ? ORDER BY tag";
  sqlite3_stmt* stmt;
  
  int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    cerr << "*** ERROR: Failed to prepare SELECT tags statement: " << getLastError() << endl;
    return tags;
  }
  
  sqlite3_bind_text(stmt, 1, section.c_str(), -1, SQLITE_STATIC);
  
  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
  {
    const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (tag != nullptr)
    {
      tags.push_back(tag);
    }
  }
  
  sqlite3_finalize(stmt);
  
  if (rc != SQLITE_DONE)
  {
    cerr << "*** ERROR: Failed to execute SELECT tags statement: " << getLastError() << endl;
    tags.clear();
  }
  
  return tags;
} /* SQLiteConfigBackend::listSection */

std::string SQLiteConfigBackend::getBackendType(void) const
{
  return "sqlite";
} /* SQLiteConfigBackend::getBackendType */

std::string SQLiteConfigBackend::getBackendInfo(void) const
{
  return m_db_path;
} /* SQLiteConfigBackend::getBackendInfo */

bool SQLiteConfigBackend::initializeTables(void)
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
} /* SQLiteConfigBackend::initializeTables */

bool SQLiteConfigBackend::finalizeInitialization(void)
{
  if (!isOpen())
  {
    cerr << "*** ERROR: Cannot finalize initialization - database not open" << endl;
    return false;
  }
  
  initializeLastCheckTime();
  
  return true;
} /* SQLiteConfigBackend::finalizeInitialization */

bool SQLiteConfigBackend::checkForExternalChanges(void)
{
  if (!isOpen())
  {
    return false;
  }

  // Query for all records updated since last check
  std::string table_name = getFullTableName("config");
  std::string sql = "SELECT section, tag, value, updated_at FROM " + table_name + " WHERE updated_at > ? ORDER BY updated_at";
  sqlite3_stmt* stmt;

  int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    cerr << "*** ERROR: Failed to prepare change detection query: " << getLastError() << endl;
    return false;
  }

  sqlite3_bind_text(stmt, 1, m_last_check_time.c_str(), -1, SQLITE_STATIC);

  // Debug output
  char* expanded_sql = sqlite3_expanded_sql(stmt);
  if (expanded_sql != nullptr)
  {
    //std::cout << "[DEBUG] Change detection SQL: " << expanded_sql << std::endl;
    sqlite3_free(expanded_sql);
  }
  //std::cout << "[DEBUG] Last check time: " << m_last_check_time << std::endl;

  bool changes_detected = false;
  std::string latest_timestamp = m_last_check_time;

  while ((rc = sqlite3_step(stmt)) == SQLITE_ROW)
  {
    const char* section = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    const char* tag = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
    const char* value = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
    const char* updated_at = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));

    if (section && tag && value && updated_at)
    {
      //std::cout << "[DEBUG] Change detected: [" << section << "]" << tag 
      //          << " = '" << value << "' (updated_at: " << updated_at << ")" << std::endl;
      notifyValueChanged(std::string(section), std::string(tag), std::string(value));
      latest_timestamp = std::string(updated_at);
      changes_detected = true;
    }
  }

  sqlite3_finalize(stmt);

  // Update last check time to the latest timestamp seen
  if (changes_detected)
  {
    //std::cout << "[DEBUG] Updated last_check_time: " << m_last_check_time 
    //          << " -> " << latest_timestamp << std::endl;
    m_last_check_time = latest_timestamp;
  }
  else
  {
    //std::cout << "[DEBUG] No external changes detected" << std::endl;
  }

  return changes_detected;
} /* SQLiteConfigBackend::checkForExternalChanges */

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

bool SQLiteConfigBackend::createTables(void)
{
  // Get the full table name with prefix
  std::string table_name = getFullTableName("config");
  
  std::string create_config_table = 
    "CREATE TABLE IF NOT EXISTS " + table_name + " ("
    "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
    "  section TEXT NOT NULL,"
    "  tag TEXT NOT NULL,"
    "  value TEXT NOT NULL,"
    "  created_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "  updated_at DATETIME DEFAULT CURRENT_TIMESTAMP,"
    "  UNIQUE(section, tag)"
    ")";
  
  if (!executeSQL(create_config_table))
  {
    cerr << "*** ERROR: Failed to create config table: " << table_name << endl;
    return false;
  }
  
  // Create index for faster lookups
  std::string create_index = 
    "CREATE INDEX IF NOT EXISTS idx_" + table_name + "_section_tag ON " + table_name + "(section, tag)";
  
  if (!executeSQL(create_index))
  {
    cerr << "*** ERROR: Failed to create index" << endl;
    return false;
  }
  
  // Create trigger to automatically update updated_at on any UPDATE
  std::string create_trigger = 
    "CREATE TRIGGER IF NOT EXISTS update_" + table_name + "_timestamp "
    "AFTER UPDATE ON " + table_name + " "
    "FOR EACH ROW "
    "BEGIN "
    "  UPDATE " + table_name + " SET updated_at = CURRENT_TIMESTAMP WHERE id = NEW.id; "
    "END";
  
  if (!executeSQL(create_trigger))
  {
    cerr << "*** ERROR: Failed to create update trigger" << endl;
    return false;
  }
  
  return true;
} /* SQLiteConfigBackend::createTables */

bool SQLiteConfigBackend::executeSQL(const std::string& sql) const
{
  if (!isOpen())
  {
    return false;
  }
  
  char* error_msg = nullptr;
  int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &error_msg);
  
  if (rc != SQLITE_OK)
  {
    cerr << "*** ERROR: SQL execution failed: " << error_msg << endl;
    sqlite3_free(error_msg);
    return false;
  }
  
  return true;
} /* SQLiteConfigBackend::executeSQL */

std::string SQLiteConfigBackend::getLastError(void) const
{
  if (m_db != nullptr)
  {
    return sqlite3_errmsg(m_db);
  }
  return "Database not open";
} /* SQLiteConfigBackend::getLastError */

void SQLiteConfigBackend::initializeLastCheckTime(void)
{
  if (!isOpen())
  {
    return;
  }

  // Query for the most recent updated_at timestamp in the database
  std::string table_name = getFullTableName("config");
  std::string sql = "SELECT MAX(updated_at) FROM " + table_name;
  sqlite3_stmt* stmt;

  int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
  if (rc != SQLITE_OK)
  {
    std::cerr << "*** WARNING: Failed to query max updated_at: " << getLastError() << std::endl;
    return;
  }

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW)
  {
    const char* max_timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
    if (max_timestamp != nullptr)
    {
      m_last_check_time = std::string(max_timestamp);
      //std::cout << "[DEBUG] Initialized last_check_time to: " << m_last_check_time << std::endl;
    }
    else
    {
      // Database is empty or all updated_at are NULL
      m_last_check_time = "1970-01-01 00:00:00";
      //std::cout << "[DEBUG] Database empty, initialized last_check_time to epoch" << std::endl;
    }
  }
  else
  {
    // Query failed, use default
    m_last_check_time = "1970-01-01 00:00:00";
    //std::cout << "[DEBUG] Query failed, initialized last_check_time to epoch" << std::endl;
  }

  sqlite3_finalize(stmt);
} /* SQLiteConfigBackend::initializeLastCheckTime */

/*
 * This file has not been truncated
 */
