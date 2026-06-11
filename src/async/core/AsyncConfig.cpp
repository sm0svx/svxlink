/**
@file	 AsyncConfig.cpp
@brief   A class for configuration handling
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-17

This file contains a class that is used to supply and save configuration data
to a backend which can be a file, a database, etc. The backend can be
implemented by extending AsyncConfigBackend.

\include test.cfg

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2026 Tobias Blomberg / SM0SVX

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

#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <dirent.h>
#include <sys/stat.h>

#include <iostream>
#include <fstream>
#include <cassert>
#include <cstring>
#include <cstdlib>
#include <map>
#include <vector>
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

#include "AsyncConfig.h"
#include "AsyncConfigBackend.h"
#include "AsyncConfigSource.h"



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

#ifndef SVX_SYSCONF_INSTALL_DIR
#define SVX_SYSCONF_INSTALL_DIR "/etc"
#endif


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


Config::~Config(void)
{
} /* Config::~Config */


bool Config::open(const string& default_config_name)
{
  return openWithFallback("", "", default_config_name);
} /* Config::open */

bool Config::openFromDbConfig(const string& db_conf_path)
{
  if (!openFromDbConfigInternal(db_conf_path, "svxlink.conf", "svxlink_"))
  {
    cerr << "*** FATAL ERROR: Cannot initialize configuration from "
         << db_conf_path << endl;
    exit(1);
  }
  loadCfgDir();
  return true;
} /* Config::openFromDbConfig */

bool Config::openFromDbConfigInternal(const std::string& db_conf_path,
                                       const std::string& default_config_name,
                                       const std::string& default_table_prefix)
{
  DbConf conf;
  if (!parseDbConfFile(db_conf_path, conf))
  {
    cerr << "*** ERROR: Could not parse database configuration file: "
         << db_conf_path << endl;
    return false;
  }

  applyTablePrefix(conf, default_table_prefix);
  m_main_config_file = (conf.type == "file") ? conf.source : db_conf_path;

  ConfigBackendPtr backend = createAndConfigureBackend(conf, default_config_name);
  if (!backend)
  {
    cerr << "*** ERROR: Failed to create backend from: " << db_conf_path << endl;
    return false;
  }

  setBackend(std::move(backend));
  return true;
} /* Config::openFromDbConfigInternal */

bool Config::openDirect(const string& source)
{
  // Create the appropriate backend using the factory
  m_backend = createConfigBackend(source);
  if (m_backend == nullptr)
  {
    cerr << "*** ERROR: Failed to create configuration backend" << endl;
    return false;
  }

  // Store the main config file path for CFG_DIR resolution
  if (source.find("file://") == 0)
  {
    m_main_config_file = source.substr(7); // Remove "file://" prefix
  }
  else
  {
    m_main_config_file = ""; // Database backend - no single file
  }

  // For database backends ensure the schema exists before any use.
  // FileConfigBackend::initializeTables() is a no-op so this is safe for all
  // backend types.
  if (!m_backend->initializeTables())
  {
    cerr << "*** ERROR: Failed to initialize backend tables for: " << source << endl;
    m_backend.reset();
    return false;
  }

  finalizeBackendSetup();
  return true;
} /* Config::openDirect */


void Config::loadCfgDir(void)
{
  if (getBackendType() != "file") return;

  string cfg_dir;
  if (!getValue("GLOBAL", "CFG_DIR", cfg_dir)) return;

  if (cfg_dir[0] != '/')
  {
    auto slash_pos = m_main_config_file.rfind('/');
    cfg_dir = (slash_pos != string::npos)
                  ? m_main_config_file.substr(0, slash_pos) + "/" + cfg_dir
                  : "./" + cfg_dir;
  }

  DIR *dir = opendir(cfg_dir.c_str());
  if (dir == nullptr)
  {
    cerr << "*** ERROR: Could not read from directory specified by "
         << "configuration variable GLOBAL/CFG_DIR=" << cfg_dir << endl;
    exit(1);
  }

  struct dirent *dirent;
  while ((dirent = readdir(dir)) != nullptr)
  {
    char *dot = strrchr(dirent->d_name, '.');
    if (dot == nullptr || dirent->d_name[0] == '.' || strcmp(dot, ".conf") != 0)
      continue;
    string cfg_filename = cfg_dir + "/" + dirent->d_name;
    cout << "Loading additional configuration: " << cfg_filename << endl;
    if (!openDirect("file://" + cfg_filename))
    {
      cerr << "*** ERROR: Could not open configuration file: "
           << cfg_filename << endl;
      exit(1);
    }
  }

  if (closedir(dir) == -1)
  {
    cerr << "*** ERROR: Error closing directory specified by "
         << "configuration variable GLOBAL/CFG_DIR=" << cfg_dir << endl;
    exit(1);
  }
} /* Config::loadCfgDir */


void Config::setBackend(ConfigBackendPtr backend)
{
  m_backend = std::move(backend);
  finalizeBackendSetup();
} /* Config::setBackend */


std::string Config::findConfigFile(const std::string& config_dir,
                                    const std::string& filename)
{
  vector<string> search_paths;

  if (!config_dir.empty())
  {
    search_paths.push_back(config_dir + "/" + filename);
  }

  const char* home = getenv("HOME");
  if (home != nullptr)
  {
    search_paths.push_back(string(home) + "/.svxlink/" + filename);
  }

  search_paths.push_back("/etc/svxlink/" + filename);
  search_paths.push_back(string(SVX_SYSCONF_INSTALL_DIR) + "/" + filename);

  for (const string& path : search_paths)
  {
    struct stat st;
    if (stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode) &&
        access(path.c_str(), R_OK) == 0)
    {
      return path;
    }
  }

  return "";
} /* Config::findConfigFile */


bool Config::parseDbConfFile(const std::string& file_path, DbConf& conf)
{
  ifstream file(file_path);
  if (!file.is_open())
  {
    return false;
  }

  cout << "Reading database configuration from: " << file_path << endl;

  string line;
  bool in_database_section = false;

  while (getline(file, line))
  {
    size_t start = line.find_first_not_of(" \t\r\n");
    if (start == string::npos) continue;
    size_t end = line.find_last_not_of(" \t\r\n");
    line = line.substr(start, end - start + 1);

    if (line[0] == '#' || line[0] == ';') continue;

    if (line[0] == '[' && line.back() == ']')
    {
      in_database_section = (line.substr(1, line.size() - 2) == "DATABASE");
      continue;
    }

    if (in_database_section)
    {
      size_t eq_pos = line.find('=');
      if (eq_pos != string::npos)
      {
        string key = line.substr(0, eq_pos);
        string value = line.substr(eq_pos + 1);
        key.erase(0, key.find_first_not_of(" \t"));
        key.erase(key.find_last_not_of(" \t") + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        value.erase(value.find_last_not_of(" \t") + 1);

        if      (key == "TYPE")   conf.type   = value;
        else if (key == "SOURCE") conf.source = value;
        else if (key == "TABLE_PREFIX") conf.table_prefix = value;
        else if (key == "ENABLE_CHANGE_NOTIFICATIONS")
          conf.enable_change_notifications =
              (value == "1" || value == "true" || value == "TRUE" ||
               value == "yes"  || value == "YES");
        else if (key == "POLL_INTERVAL")
        {
          try
          {
            conf.poll_interval_seconds = static_cast<unsigned int>(stoul(value));
          }
          catch (const std::exception&)
          {
            cerr << "*** ERROR: Invalid POLL_INTERVAL in db.conf: " << value << endl;
            return false;
          }
        }
      }
    }
  }

  if (conf.type.empty() || conf.source.empty())
  {
    cerr << "*** ERROR: Invalid db.conf - missing TYPE or SOURCE in [DATABASE] section" << endl;
    return false;
  }

  cout << "Database configuration: TYPE=" << conf.type;
  if (!conf.table_prefix.empty())
    cout << ", TABLE_PREFIX=" << conf.table_prefix;
  cout << endl;

  return true;
} /* Config::parseDbConfFile */


void Config::applyTablePrefix(DbConf& conf, const std::string& default_prefix)
{
  if (default_prefix.empty()) return;

  if (conf.table_prefix.empty())
  {
    conf.table_prefix = default_prefix;
    cout << "Using automatic table prefix: " << conf.table_prefix << endl;
  }
  else
  {
    conf.table_prefix = conf.table_prefix + default_prefix;
    cout << "Using table prefix: " << conf.table_prefix
         << " (from db.conf + binary name)" << endl;
  }
} /* Config::applyTablePrefix */


ConfigBackendPtr Config::createAndConfigureBackend(const DbConf& conf,
                                                    const std::string& default_config_file)
{
  if (!ConfigSource::isBackendAvailable(conf.type))
  {
    cerr << "*** ERROR: Backend '" << conf.type
         << "' is not available (not compiled in)" << endl;
    cerr << "Available backends: " << ConfigBackendFactory::validFactories() << endl;
    return nullptr;
  }

  ConfigBackendPtr backend = createConfigBackendByType(conf.type, conf.source);
  if (!backend)
  {
    cerr << "*** ERROR: Failed to create " << conf.type << " backend" << endl;
    return nullptr;
  }

  if (!conf.table_prefix.empty())
    backend->setTablePrefix(conf.table_prefix);

  if (backend->getBackendType() != "file")
  {
    if (!initializeDatabase(backend.get(), default_config_file))
    {
      cerr << "*** ERROR: Failed to initialize database backend" << endl;
      return nullptr;
    }
  }

  backend->enableChangeNotifications(conf.enable_change_notifications);
  if (conf.enable_change_notifications && conf.poll_interval_seconds > 0)
  {
    backend->startAutoPolling(conf.poll_interval_seconds * 1000);
    cout << "Auto-polling enabled with interval: "
         << conf.poll_interval_seconds << " seconds" << endl;
  }

  cout << "Successfully initialized " << backend->getBackendType()
       << " configuration backend: " << backend->getBackendInfo() << endl;

  return backend;
} /* Config::createAndConfigureBackend */


bool Config::initializeDatabase(ConfigBackend* backend,
                                 const std::string& default_config_file)
{
  if (!backend) return false;

  if (!backend->initializeTables())
  {
    cerr << "*** ERROR: Failed to initialize database tables" << endl;
    return false;
  }

  list<string> sections = backend->listSections();
  if (!sections.empty())
  {
    cout << "Database already initialized with " << sections.size()
         << " sections" << endl;
    if (!backend->finalizeInitialization())
      cerr << "*** WARNING: Failed to finalize database initialization" << endl;
    return true;
  }

  cout << "Database is empty, initializing..." << endl;

  bool was_enabled = backend->changeNotificationsEnabled();
  if (was_enabled) backend->enableChangeNotifications(false);

  if (populateFromExistingFiles(backend, default_config_file))
  {
    cout << "Database initialized from existing configuration files" << endl;
  }
  else
  {
    cout << "*** WARNING: Database is empty and no existing configuration files found. "
            "The application may need to seed defaults." << endl;
  }

  if (!backend->finalizeInitialization())
    cerr << "*** WARNING: Failed to finalize database initialization" << endl;

  if (was_enabled) backend->enableChangeNotifications(true);

  sections = backend->listSections();
  if (sections.empty())
  {
    cerr << "*** ERROR: Failed to initialize database" << endl;
    return false;
  }

  cout << "Database initialized successfully with " << sections.size()
       << " sections" << endl;
  return true;
} /* Config::initializeDatabase */


bool Config::populateFromExistingFiles(ConfigBackend* backend,
                                        const std::string& config_file_name)
{
  if (!backend) return false;

  string config_file = findConfigFile("", config_file_name);
  if (config_file.empty()) return false;

  cout << "Found existing configuration file: " << config_file << endl;
  cout << "Loading existing configuration to populate database..." << endl;

  ConfigBackendPtr file_backend = createConfigBackend("file://" + config_file);
  if (!file_backend)
  {
    cerr << "*** WARNING: Could not create file backend for " << config_file << endl;
    return false;
  }

  list<string> sections = file_backend->listSections();
  for (const string& section : sections)
  {
    list<string> tags = file_backend->listSection(section);
    for (const string& tag : tags)
    {
      string value;
      if (file_backend->getValue(section, tag, value))
      {
        if (!backend->setValue(section, tag, value))
          cerr << "*** WARNING: Failed to set " << section << "/" << tag
               << " in database" << endl;
      }
    }
  }

  string cfg_dir;
  if (file_backend->getValue("GLOBAL", "CFG_DIR", cfg_dir))
  {
    if (cfg_dir[0] != '/')
    {
      auto slash_pos = config_file.rfind('/');
      cfg_dir = (slash_pos != string::npos)
                    ? config_file.substr(0, slash_pos + 1) + cfg_dir
                    : "./" + cfg_dir;
    }

    cout << "Processing CFG_DIR: " << cfg_dir << endl;

    DIR* dir = opendir(cfg_dir.c_str());
    if (dir != nullptr)
    {
      struct dirent* de;
      while ((de = readdir(dir)) != nullptr)
      {
        char* dot = strrchr(de->d_name, '.');
        if (dot == nullptr || de->d_name[0] == '.' || strcmp(dot, ".conf") != 0)
          continue;

        string cfg_file_path = cfg_dir + "/" + de->d_name;
        cout << "Loading additional config file: " << cfg_file_path << endl;

        ConfigBackendPtr extra = createConfigBackend("file://" + cfg_file_path);
        if (extra)
        {
          list<string> add_sections = extra->listSections();
          for (const string& section : add_sections)
          {
            list<string> add_tags = extra->listSection(section);
            for (const string& tag : add_tags)
            {
              string value;
              if (extra->getValue(section, tag, value))
              {
                if (!backend->setValue(section, tag, value))
                  cerr << "*** WARNING: Failed to set " << section << "/" << tag
                       << " from " << cfg_file_path << endl;
              }
            }
          }
        }
        else
        {
          cerr << "*** WARNING: Could not load additional config file: "
               << cfg_file_path << endl;
        }
      }
      closedir(dir);
    }
    else
    {
      cerr << "*** WARNING: Could not open CFG_DIR: " << cfg_dir << endl;
    }
  }

  return true;
} /* Config::populateFromExistingFiles */


bool Config::importFromConfigFile(const std::string& config_filename)
{
  return populateFromExistingFiles(m_backend.get(), config_filename);
} /* Config::importFromConfigFile */


std::string Config::getMainConfigFile(void) const
{
  return m_main_config_file;
} /* Config::getMainConfigFile */

std::string Config::getBackendType(void) const
{
  if (m_backend != nullptr)
  {
    return m_backend->getBackendType();
  }
  return "";
} /* Config::getBackendType */


bool Config::getValue(const std::string& section, const std::string& tag,
                      std::string& value, bool missing_ok) const
{
  // First try to get from in-memory (for subscriptions)
  Sections::const_iterator sec_it = m_sections.find(section);
  if (sec_it != m_sections.end())
  {
    Values::const_iterator val_it = sec_it->second.find(tag);
    if (val_it != sec_it->second.end())
    {
      value = val_it->second.val;
      return true;
    }
  }

  // If not in memory, try to get from backend
  if (m_backend != nullptr && m_backend->isOpen())
  {
    if (m_backend->getValue(section, tag, value))
    {
      return true;
    }
  }

  return missing_ok;
} /* Config::getValue */


bool Config::getValue(const std::string& section, const std::string& tag,
                      char& value, bool missing_ok) const
{
  string str_value;
  if (!getValue(section, tag, str_value, missing_ok))
  {
    return missing_ok;
  }

  if (str_value.size() != 1)
  {
    return false;
  }

  value = str_value[0];
  return true;
} /* Config::getValue */


const string &Config::getValue(const string& section, const string& tag) const
{
  static string cached_value;
  
  if (getValue(section, tag, cached_value, true))
  {
    return cached_value;
  }
  
  static const string empty_string;
  return empty_string;
} /* Config::getValue */


list<string> Config::listSections(void)
{
  if (m_backend != nullptr && m_backend->isOpen())
  {
    return m_backend->listSections();
  }
  
  list<string> section_list;
  for (Sections::const_iterator it = m_sections.begin(); it != m_sections.end(); ++it)
  {
    section_list.push_back(it->first);
  }
  return section_list;
} /* Config::listSections */


list<string> Config::listSection(const string& section)
{
  if (m_backend != nullptr && m_backend->isOpen())
  {
    return m_backend->listSection(section);
  }
  
  list<string> tags;
  
  if (m_sections.count(section) == 0)
  {
    return tags;
  }
  
  const Values& values = m_sections.at(section);
  for (Values::const_iterator it = values.begin(); it != values.end(); ++it)
  {
    tags.push_back(it->first);
  }
  
  return tags;
} /* Config::listSection */


void Config::setValue(const std::string& section, const std::string& tag,
                      const std::string& value)
{
  // Update in-memory 
  Values &values = m_sections[section];
  bool value_changed = (value != values[tag].val);
  
  if (value_changed)
  {
    values[tag].val = value;
    
    // Sync to backend
    syncToBackend(section, tag);
    
    // Emit signals and call subscribers
    valueUpdated(section, tag, value);
    for (auto& [id, func] : values[tag].subs)
    {
      func(value);
    }
  }
} /* Config::setValue */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void Config::loadFromBackend(void)
{
  if (m_backend == nullptr || !m_backend->isOpen())
  {
    return;
  }

  // Load all sections and their tags/values into memory
  list<string> sections = m_backend->listSections();
  for (const string& section : sections)
  {
    list<string> tags = m_backend->listSection(section);
    for (const string& tag : tags)
    {
      string value;
      if (m_backend->getValue(section, tag, value))
      {
        m_sections[section][tag].val = value;
      }
    }
  }
} /* Config::loadFromBackend */

void Config::syncToBackend(const std::string& section, const std::string& tag)
{
  if (m_backend == nullptr || !m_backend->isOpen())
  {
    return;
  }

  // Temporarily disable notifications so we don't fire twice!
  bool was_enabled = m_backend->changeNotificationsEnabled();
  if (was_enabled)
  {
    m_backend->enableChangeNotifications(false);
  }

  // Get the value from in-memory
  Sections::const_iterator sec_it = m_sections.find(section);
  if (sec_it != m_sections.end())
  {
    Values::const_iterator val_it = sec_it->second.find(tag);
    if (val_it != sec_it->second.end())
    {
      if (!m_backend->setValue(section, tag, val_it->second.val))
      {
        cerr << "*** WARNING: Failed to sync configuration change to backend: "
             << section << "/" << tag << endl;
      }
    }
  }

  // Restore notifications if they were enabled
  if (was_enabled)
  {
    m_backend->enableChangeNotifications(true);
  }
} /* Config::syncToBackend */

ConfigBackend* Config::getBackend(void)
{
  return m_backend.get();
} /* Config::getBackend */

void Config::reload(void)
{
  if (m_backend == nullptr || !m_backend->isOpen())
  {
    return;
  }

  // For database backends, check for external changes first
  if (m_backend->getBackendType() != "file")
  {
    m_backend->checkForExternalChanges();
  }

  // Reload all sections and tags
  auto sections = m_backend->listSections();
  for (const auto& section : sections)
  {
    auto tags = m_backend->listSection(section);
    for (const auto& tag : tags)
    {
      std::string new_value;
      if (m_backend->getValue(section, tag, new_value))
      {
        // Update memory; auto-creates entry for newly-seen sections/tags
        auto& cached = m_sections[section][tag];
        if (cached.val != new_value)
        {
          cached.val = new_value;
          for (auto& [id, sub] : cached.subs)
          {
            sub(new_value);
          }
          valueUpdated(section, tag, new_value);
        }
      }
    }
  }
} /* Config::reload */

void Config::onBackendValueChanged(const std::string& section,
                                    const std::string& tag,
                                    const std::string& value)
{
  //std::cout << "[DEBUG Config] onBackendValueChanged called: [" << section << "]" << tag 
  //          << " = '" << value << "'" << std::endl;

  // Update in-memory
  m_sections[section][tag].val = value;

  // Trigger subscriptions
  auto sec_it = m_sections.find(section);
  if (sec_it != m_sections.end())
  {
    auto val_it = sec_it->second.find(tag);
    if (val_it != sec_it->second.end())
    {
      //std::cout << "[DEBUG Config] Found " << val_it->second.subs.size() 
      //          << " subscription(s) for [" << section << "]" << tag << std::endl;
      for (auto& [id, sub] : val_it->second.subs)
      {
        sub(value);
      }
    }
    else
    {
      //std::cout << "[DEBUG Config] Tag not found in memory: [" << section << "]" << tag << std::endl;
    }
  }
  else
  {
    //std::cout << "[DEBUG Config] Section not found in memory: [" << section << "]" << std::endl;
  }

  // Emit valueUpdated signal
  valueUpdated(section, tag, value);
} /* Config::onBackendValueChanged */

void Config::connectBackendSignals(void)
{
  if (m_backend != nullptr)
  {
    m_backend->valueChanged.connect(
        sigc::mem_fun(*this, &Config::onBackendValueChanged));
  }
} /* Config::connectBackendSignals */

void Config::finalizeBackendSetup(void)
{
  if (m_backend == nullptr)
  {
    return;
  }

  // Disable notifications during initial config load to avoid spurious signals
  bool was_enabled = m_backend->changeNotificationsEnabled();
  if (was_enabled)
  {
    m_backend->enableChangeNotifications(false);
  }

  // Load all configuration data into memory for subscription support
  loadFromBackend();

  // Re-enable notifications before connecting signals
  if (was_enabled)
  {
    m_backend->enableChangeNotifications(true);
  }

  // Connect backend signals for external change detection
  connectBackendSignals();
} /* Config::finalizeBackendSetup */

bool Config::openWithFallback(const std::string& cmdline_config,
                               const std::string& cmdline_dbconfig,
                               const std::string& default_config_name)
{
  m_last_error.clear();

  // Derive table prefix from config name: svxlink.conf → svxlink_
  std::string default_table_prefix;
  size_t dot_pos = default_config_name.find('.');
  if (dot_pos != std::string::npos)
    default_table_prefix = default_config_name.substr(0, dot_pos) + "_";

  // First check for --config option
  if (!cmdline_config.empty())
  {
    if (!openDirect("file://" + cmdline_config))
    {
      m_last_error = "Failed to open configuration file: " + cmdline_config;
      return false;
    }
    loadCfgDir();
    return true;
  }

  // Next check for --dbconfig option
  if (!cmdline_dbconfig.empty())
  {
    if (!openFromDbConfigInternal(cmdline_dbconfig, default_config_name,
                                   default_table_prefix))
    {
      m_last_error = "Failed to open database configuration: " + cmdline_dbconfig;
      return false;
    }
    loadCfgDir();
    return true;
  }

  // search standard locations for db.conf
  string db_conf_path = findConfigFile("", "db.conf");
  if (!db_conf_path.empty())
  {
    if (!openFromDbConfigInternal(db_conf_path, default_config_name,
                                   default_table_prefix))
    {
      m_last_error = "Found db.conf at " + db_conf_path + " but failed to load it";
      return false;
    }
    loadCfgDir();
    return true;
  }

  // search standard locations for default config file
  string config_path = findConfigFile("", default_config_name);
  if (!config_path.empty())
  {
    if (!openDirect("file://" + config_path))
    {
      m_last_error = "Found configuration at " + config_path + " but failed to load it";
      return false;
    }
    loadCfgDir();
    return true;
  }

  m_last_error = "No configuration found. Searched for:\n"
                 "  - db.conf in: ~/.svxlink/, /etc/svxlink/, "
                 + std::string(SVX_SYSCONF_INSTALL_DIR) + "\n"
                 "  - " + default_config_name
                 + " in: ~/.svxlink/, /etc/svxlink/, "
                 + std::string(SVX_SYSCONF_INSTALL_DIR);
  return false;
} /* Config::openWithFallback */


/*
 * This file has not been truncated
 */
