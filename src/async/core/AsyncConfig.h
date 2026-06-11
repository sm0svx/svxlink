/**
@file	 AsyncConfig.h
@brief   A class for configuration handling
@author  Tobias Blomberg
@date	 2004-03-17

This file contains a class that is used to supply and save configuration data
to a backend which can be a file, a database, etc. The backend can be
implemented by extending AsyncConfigBackend.

\include test.cfg

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
#include <sigc++/sigc++.h>

#include <string>
#include <map>
#include <list>
#include <memory>
#include <sstream>
#include <locale>
#include <vector>
#include <cassert>


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
    Config(void) {}

    /**
     * @brief 	Copy constructor (deleted - Config is not copyable)
     */
    Config(const Config&) = delete;

    /**
     * @brief 	Assignment operator (deleted - Config is not assignable)
     */
    Config& operator=(const Config&) = delete;
  
    /**
     * @brief 	Destructor
     */
    ~Config(void);
  
    /**
     * @brief   Open configuration searching standard locations
     * @param   default_config_name Config filename to search for (default: "svxlink.conf")
     * @return  Returns \em true on success or else \em false.
     *
     * Convenience wrapper around openWithFallback() with empty CLI overrides.
     * Searches standard locations for db.conf first, then for default_config_name.
     * Use getLastError() for error details on failure.
     *
     * For direct file opening use openDirect("file:///path/to/file.conf") instead.
     */
    bool open(const std::string& default_config_name = "svxlink.conf");

    /**
     * @brief 	Open configuration using specific db.conf file
     * @param 	db_conf_path The full path to the db.conf file to use
     * @return	Returns \em true on success or else \em false.
     *
     * This function directly uses the specified db.conf file for backend
     * selection instead of searching in standard locations.
     */
    bool openFromDbConfig(const std::string& db_conf_path);

    /**
     * @brief 	Open configuration with explicit source 
     * @param 	source The configuration source (file path, database URL, etc.)
     * @return	Returns \em true on success or else \em false.
     *
     * This directly opens a configuration source.
     * For new applications, prefer the parameterless open() method that
     * uses db.conf for backend selection.
     */
    bool openDirect(const std::string& source);

    /**
     * @brief 	Get the main configuration file path
     * @return	Returns the path to the main configuration file, or empty if using database backend
     *
     * This method returns the path to the main configuration file that was loaded.
     * For file backend, this is the path to the main .conf file.
     * For database backends, this returns an empty string since there's no single file.
     */
    std::string getMainConfigFile(void) const;

    /**
     * @brief   Get the configuration backend type
     * @return  Returns the backend type string ("file", "sqlite", "mysql", "postgresql")
     *
     * This method returns the type of configuration backend currently in use.
     */
    std::string getBackendType(void) const;

    /**
     * @brief   Get direct access to the configuration backend
     * @return  Pointer to the ConfigBackend or nullptr if not open
     *
     * This provides direct access to the backend for advanced operations
     * like enabling change notifications or starting auto-polling.
     */
    ConfigBackend* getBackend(void);

    /**
     * @brief   Reload the configuration from its source
     *
     * This method forces a reload of all configuration values from the backend.
     * For database backends, it calls checkForExternalChanges() first.
     * After reloading, it triggers all subscribeValue callbacks for values that changed.
     */
    void reload(void);

    /**
     * @brief   Import configuration from an installed example config file
     * @param   config_filename Filename to search for (e.g., "svxlink.conf")
     * @return  \em true if the file was found and imported, \em false otherwise
     *
     * Searches the standard locations (~/.svxlink/, /etc/svxlink/,
     * SVX_SYSCONF_INSTALL_DIR) for @a config_filename, opens it as a file
     * backend, and copies every section/key/value (including any CFG_DIR
     * sub-files) into the currently active backend.
     *
     * Intended as a one-time database initialisation helper. The caller
     * should verify that the active backend is not a file backend and that
     * it is empty before calling this method.
     */
    bool importFromConfigFile(const std::string& config_filename);

    /**
     * @brief   Smart configuration initialization with fallback
     * @param   cmdline_config    Path from --config CLI option (empty if not given)
     * @param   cmdline_dbconfig  Path from --dbconfig CLI option (empty if not given)
     * @param   default_config_name Default config filename (e.g., "svxlink.conf")
     * @return  \em true on success, \em false on failure
     *
     * Implements the full configuration initialization cascade:
     * 1. --config given  → openDirect() with that file
     * 2. --dbconfig given → open backend described by that db.conf
     * 3. Search standard locations for db.conf, use if found
     * 4. Search standard locations for default_config_name, use if found
     * 5. Fail — call getLastError() for details
     *
     * On success, use getMainConfigFile() and getBackendType() for diagnostics.
     *
     * Example:
     * @code
     * Async::Config cfg;
     * if (!cfg.openWithFallback(config_arg, dbconfig_arg, "svxlink.conf")) {
     *   cerr << "*** ERROR: " << cfg.getLastError() << endl;
     *   exit(1);
     * }
     * cout << "Loaded from: " << cfg.getMainConfigFile() << endl;
     * @endcode
     */
    bool openWithFallback(const std::string& cmdline_config,
                          const std::string& cmdline_dbconfig,
                          const std::string& default_config_name);

    /**
     * @brief   Get the last error message from a failed open call
     * @return  Human-readable error description, or empty string if no error
     */
    std::string getLastError(void) const { return m_last_error; }
    
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
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true on success or else \em false on failure
     *
     * This function is used to get the value for a configuration variable
     * of type "string".
     */
    bool getValue(const std::string& section, const std::string& tag,
                  std::string& value, bool missing_ok = false) const;

    /**
     * @brief 	Get the char value of the given configuration variable
     * @param 	section    The name of the section where the configuration
     *	      	      	   variable is located
     * @param 	tag   	   The name of the configuration variable to get
     * @param 	value 	   The value is returned in this argument. Any previous
     *	      	      	   contents is wiped
     * @param	missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return	Returns \em true on success or else \em false on failure
     *
     * This function is used to get the value for a configuration variable
     * of type "char". It is an error if the size of the value is anything but
     * 1 byte.
     */
    bool getValue(const std::string& section, const std::string& tag,
                  char& value, bool missing_ok = false) const;
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
      ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
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
     * @brief 	Get the value of the given config variable into keyed container
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
     * The config variable is read into a keyed container (e.g. set, multiset
     * etc).
     * It's a template function meaning that it can take any key type
     * that supports the operator>> function.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to
     * \em true, this function returns \em true for a missing variable but
     * still returns \em false if an illegal value is specified.
     */
    template <template <typename, typename, typename> class Container,
              typename Key>
    bool getValue(const std::string& section, const std::string& tag,
                  Container<Key, std::less<Key>, std::allocator<Key> > &c,
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
      ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
      while (!ssval.eof())
      {
        Key tmp;
        ssval >> tmp;
        if(!ssval.eof())
        {
          ssval >> std::ws;
        }
        if (ssval.fail())
        {
          return false;
        }
        c.insert(tmp);
      }
      return true;
    } /* Config::getValue */

    /**
     * @brief   Get value of given config variable into associative container
     * @param   section    The name of the section where the configuration
     *                     variable is located
     * @param   tag        The name of the configuration variable to get
     * @param   c          The value is returned in this argument.
     *                     Successful completion overwrites previous contents
     * @param   sep        The character used to separate key and value
     * @param   missing_ok If set to \em true, return \em true if the
     *                     configuration variable is missing
     * @return  Returns \em true on success or else \em false on failure.
     *
     * This function is used to get the value of a configuraiton variable.  The
     * config variable is read into an associative container (e.g. std::map or
     * std::multimap).  It's a template function meaning that it can take any
     * key and value type that supports the operator>> function.
     * Normally a missing configuration variable is seen as an error and the
     * function returns \em false. If the missing_ok parameter is set to \em
     * true, this function returns \em true for a missing variable but still
     * returns \em false if an illegal value is specified.
     */
    template <template <typename, typename, typename, typename> class Container,
              class Key, class T, class Compare=std::less<Key>,
              class Allocator=std::allocator<std::pair<const Key, T>>>
    bool getValue(const std::string& section, const std::string& tag,
                  Container<Key, T, Compare, Allocator>& c,
                  char sep = ':', bool missing_ok = false) const
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
      ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
      while (!ssval.eof())
      {
        std::string entry;
        ssval >> entry;
        std::string::size_type seppos = entry.find(sep);
        if (seppos == std::string::npos)
        {
          return false;
        }
        std::string keystr(entry.substr(0, seppos));
        std::string valuestr(entry.substr(seppos+1));
        Key key;
        T value;
        if (!setValueFromString(key, keystr) ||
           !setValueFromString(value, valuestr))
        {
          return false;
        }
        if(!ssval.eof())
        {
          ssval >> std::ws;
        }
        if (ssval.fail())
        {
          return false;
        }
        c.insert(std::pair<Key, T>(key, value));
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
     * @brief   Opaque subscription ID (internal use / unsubscribeValue).
     */
    using SubId = std::size_t;

    /**
     * @brief   Subscription handle returned by subscribeValue / subscribeOptionalValue
     *
     * Holds a live subscription.  When destroyed (or reset() is called) the
     * subscription is automatically removed from the Config so that the stored
     * std::function — and therefore any code in a plugin library that was
     * captured by the callback — is destroyed while that library is still
     * loaded.  This prevents the use-after-dlclose crash that occurs when
     * Config::~Config tries to destroy subscriber callbacks whose _M_manager
     * lives in an already-unloaded shared library.
     *
     * Store one Subscription per subscribeValue / subscribeOptionalValue call
     * as a member variable of the subscribing class.  The Subscription is
     * move-only; it cannot be copied!
     */
    class Subscription
    {
    public:
      Subscription() = default;

      ~Subscription() { reset(); }

      Subscription(Subscription&& other) noexcept
        : m_cfg(other.m_cfg),
          m_section(std::move(other.m_section)),
          m_tag(std::move(other.m_tag)),
          m_id(other.m_id)
      {
        other.m_cfg = nullptr;
      }

      Subscription& operator=(Subscription&& other) noexcept
      {
        if (this != &other)
        {
          reset();
          m_cfg     = other.m_cfg;
          m_section = std::move(other.m_section);
          m_tag     = std::move(other.m_tag);
          m_id      = other.m_id;
          other.m_cfg = nullptr;
        }
        return *this;
      }

      Subscription(const Subscription&)            = delete;
      Subscription& operator=(const Subscription&) = delete;

      /** Cancel the subscription immediately. */
      void reset()
      {
        if (m_cfg != nullptr)
        {
          m_cfg->unsubscribeValue(m_section, m_tag, m_id);
          m_cfg = nullptr;
        }
      }

      bool valid() const { return m_cfg != nullptr; }

    private:
      friend class Config;

      Config*     m_cfg    = nullptr;
      std::string m_section;
      std::string m_tag;
      SubId       m_id{};

      Subscription(Config* cfg, std::string sec, std::string tag, SubId id)
        : m_cfg(cfg),
          m_section(std::move(sec)),
          m_tag(std::move(tag)),
          m_id(id)
      {
      }
    }; /* class Subscription */

    /**
     * @brief Subscribe to the given configuration variable (char*)
     * @param section The name of the section where the configuration
     *                variable is located
     * @param tag     The name of the configuration variable to get
     * @param def     Default value if the config var does not exist
     * @param func    The function to call when the config var changes
     *
     * This function is used to subscribe to the changes of the specified
     * configuration variable. The given function will be called when the value
     * changes. If the configuration variable is not set, it will be set to the
     * given default value.
     *
     * This version of the function is called when the default value is a C
     * string (char*).
     */
    template <typename F=std::function<void(const char*)>>
    Subscription subscribeValue(const std::string& section, const std::string& tag,
                                const char* def, F func)
    {
      return subscribeValue(section, tag, std::string(def),
          [=](const std::string& str_val) -> void
          {
            func(str_val.c_str());
          });
    } /* subscribeValue */

    /**
     * @brief Subscribe to the given configuration variable
     * @param section The name of the section where the configuration
     *                variable is located
     * @param tag     The name of the configuration variable to get
     * @param def     Default value if the config var does not exist
     * @param func    The function to call when the config var changes
     *
     * This function is used to subscribe to the changes of the specified
     * configuration variable. The given function will be called when the value
     * changes. If the configuration variable is not set, it will be set to the
     * given default value.
     *
     * This version of the function is called when the default value is of a
     * non-container type (e.g. std::string, int, bool etc).
     */
    template <typename Rsp, typename F=std::function<void(const Rsp&)>>
    Subscription subscribeValue(const std::string& section, const std::string& tag,
                                const Rsp& def, F func)
    {
      Value& v = getValueP(section, tag, def);
      SubId id = v.next_id++;
      v.subs[id] = [=](const std::string& str_val) -> void
          {
            std::stringstream ssval(str_val);
            ssval.imbue(std::locale(ssval.getloc(), new empty_ctype));
            Rsp tmp;
            ssval >> tmp;
            func(tmp);
          };
      v.subs[id](v.val);
      return Subscription(this, section, tag, id);
    } /* subscribeValue */

    /**
     * @brief Subscribe to an optional configuration variable (no auto-create)
     * @param section The name of the section where the configuration
     *                variable is located
     * @param tag     The name of the configuration variable to get
     * @param func    The function to call when the config var changes
     *
     * This function is used to subscribe to the changes of the specified
     * configuration variable. Unlike subscribeValue, this function does NOT
     * create the variable if it doesn't exist.
     *
     * - If the variable exists: the callback is called immediately with the value
     * - If the variable doesn't exist: no callback is made initially
     * - When the variable is added later: the callback is triggered
     *
     * This is useful for optional configuration values that should not be
     * auto-created when missing.
     */
    template <typename F=std::function<void(const std::string&)>>
    Subscription subscribeOptionalValue(const std::string& section, const std::string& tag, F func)
    {
      Value& v = m_sections[section][tag];
      SubId id = v.next_id++;
      v.subs[id] = [=](const std::string& str_val) -> void { func(str_val); };
      if (!v.val.empty())
      {
        v.subs[id](v.val);
      }
      return Subscription(this, section, tag, id);
    } /* subscribeOptionalValue */

    /**
     * @brief Subscribe to the given configuration variable (sequence)
     * @param section The name of the section where the configuration
     *                variable is located
     * @param tag     The name of the configuration variable to get
     * @param def     Default value if the config var does not exist
     * @param func    The function to call when the config var changes
     *
     * This function is used to subscribe to the changes of the specified
     * configuration variable. The given function will be called when the value
     * changes. If the configuration variable is not set, it will be set to the
     * given default value.
     *
     * This version of the function is called when the default value is a
     * sequence container (e.g. std::vector, std::list etc).
     */
    template <template <typename, typename> class Container,
              typename Rsp, typename F=std::function<void(const Rsp&)>>
    Subscription subscribeValue(const std::string& section, const std::string& tag,
                                const Container<Rsp, std::allocator<Rsp>>& def, F func)
    {
      Value& v = getValueP(section, tag, def);
      SubId id = v.next_id++;
      v.subs[id] = [=](const std::string& str_val) -> void
          {
            std::stringstream ssval(str_val);
            ssval.imbue(std::locale(ssval.getloc(), new csv_whitespace));
            Container<Rsp, std::allocator<Rsp>> c;
            while (!ssval.eof())
            {
              Rsp tmp;
              ssval >> tmp;
              if(!ssval.eof())
              {
                ssval >> std::ws;
              }
              if (ssval.fail())
              {
                return;
              }
              c.push_back(tmp);
            }
            func(std::move(c));
          };
      v.subs[id](v.val);
      return Subscription(this, section, tag, id);
    } /* Config::subscribeValue */

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
     *
     * The valueUpdated signal will be emitted so that subscribers can get
     * notified when the value of a configuration variable is changed.
     */
    void setValue(const std::string& section, const std::string& tag,
      	      	  const std::string& value);

    /**
     * @brief   Set the value of a configuration variable (generic type)
     * @param   section   The name of the section where the configuration
     *                    variable is located
     * @param   tag       The name of the configuration variable to set.
     * @param   value     The value to set
     *
     * This function is used to set the value of a configuration variable.
     * The type of the value may be any type that support streaming to string.
     * If the given configuration section or variable does not exist, it
     * is created.
     * Note that this function will not write anything back to the
     * associated configuration file. It will only set the value in memory.
     *
     * The valueUpdated signal will be emitted so that subscribers can get
     * notified when the value of a configuration variable is changed.
     */
    template <typename Rsp>
    void setValue(const std::string& section, const std::string& tag,
                  const Rsp& value)
    {
      std::ostringstream ss;
      ss << value;
      setValue(section, tag, ss.str());
    }

    /**
     * @brief   Set the value of a configuration variable (sequence container)
     * @param   section   The name of the section where the configuration
     *                    variable is located
     * @param   tag       The name of the configuration variable to set.
     * @param   c         The sequence to set
     *
     * This function is used to set the value of a configuration variable that
     * holds a sequence container (e.g. std::vector, std::list etc).
     * The type of the elements of the container may be any type that support
     * streaming to string.
     * If the given configuration section or variable does not exist, it
     * is created.
     * Note that this function will not write anything back to the
     * associated configuration file. It will only set the value in memory.
     *
     * The valueUpdated signal will be emitted so that subscribers can get
     * notified when the value of a configuration variable is changed.
     */
    template <template <typename, typename> class Container,
              typename Rsp>
    void setValue(const std::string& section, const std::string& tag,
                  const Container<Rsp, std::allocator<Rsp>>& c)
    {
      std::ostringstream ss;
      bool first_val = true;
      for (const auto& val : c)
      {
        if (!first_val)
        {
          ss << ",";
        }
        first_val = false;
        ss << val;
      }
      setValue(section, tag, ss.str());
    } /* setValue */

    /**
     * @brief   A signal that is emitted when a config value is updated
     * @param   section The config section of the update
     * @param   tag     The tag (variable name) of the update
     *
     * This signal is emitted whenever a configuration variable is changed
     * by calling the setValue function. It will only be emitted if the value
     * actually changes.
     */
    sigc::signal<void(const std::string&, const std::string&, const std::string&)> valueUpdated;

    /**
     * @brief   Remove a previously registered subscription
     * @param   section The configuration section name
     * @param   tag     The configuration tag name
     * @param   id      The subscription ID returned by subscribeValue / subscribeOptionalValue
     *
     * If the id is no longer valid (already removed or never registered), this
     * is a no-op.  Safe to call from within a subscribed callback.
     */
    void unsubscribeValue(const std::string& section, const std::string& tag, SubId id)
    {
      auto sec_it = m_sections.find(section);
      if (sec_it != m_sections.end())
      {
        auto val_it = sec_it->second.find(tag);
        if (val_it != sec_it->second.end())
        {
          val_it->second.subs.erase(id);
        }
      }
    }

  private:
    using Subscriber = std::function<void(const std::string&)>;
    struct Value
    {
      std::string                 val;
      SubId                       next_id = 0;
      std::map<SubId, Subscriber> subs;
    };
    typedef std::map<std::string, Value>  Values;
    typedef std::map<std::string, Values> Sections;

      // Really wanted to use classic_table() but it returns nullptr on Alpine
    static const std::ctype<char>::mask* empty_table()
    {
      static const auto table_size = std::ctype<char>::table_size;
      static std::ctype<char>::mask v[table_size];
      std::fill(&v[0], &v[table_size], 0);
      return &v[0];
    }

    struct empty_ctype : std::ctype<char>
    {
      static const mask* make_table(void) { return empty_table(); }
      empty_ctype(std::size_t refs=0) : ctype(make_table(), false, refs) {}
    };

    struct csv_whitespace : std::ctype<char>
    {
      static const mask* make_table()
      {
        auto tbl = empty_table();
        static std::vector<mask> v(tbl, tbl + table_size);
        v[' '] |= space;
        v[','] |= space;
        return &v[0];
      }
      csv_whitespace(std::size_t refs=0) : ctype(make_table(), false, refs) {}
    };

    ConfigBackendPtr m_backend;
    Sections         m_sections;       // In-memory for subscriptions
    std::string      m_main_config_file; // Path to main config file (for CFG_DIR resolution)
    std::string      m_last_error;     // Last error from a failed open call

    void loadFromBackend(void);
    void syncToBackend(const std::string& section, const std::string& tag);
    void onBackendValueChanged(const std::string& section, const std::string& tag, const std::string& value);
    void connectBackendSignals(void);
    void finalizeBackendSetup(void);
    void setBackend(ConfigBackendPtr backend);
    void loadCfgDir(void);
    bool openFromDbConfigInternal(const std::string& db_conf_path,
                                  const std::string& default_config_name,
                                  const std::string& default_table_prefix);

    struct DbConf
    {
      std::string type;
      std::string source;
      std::string table_prefix;
      bool        enable_change_notifications;
      unsigned int poll_interval_seconds;
      DbConf() : enable_change_notifications(false), poll_interval_seconds(0) {}
    };

    static std::string findConfigFile(const std::string& config_dir,
                                      const std::string& filename);
    bool parseDbConfFile(const std::string& path, DbConf& conf);
    void applyTablePrefix(DbConf& conf, const std::string& default_prefix);
    ConfigBackendPtr createAndConfigureBackend(const DbConf& conf,
                                              const std::string& default_config_file);
    bool initializeDatabase(ConfigBackend* backend,
                            const std::string& default_config_file);
    bool populateFromExistingFiles(ConfigBackend* backend,
                                   const std::string& config_file);

    template <class T>
    bool setValueFromString(T& val, const std::string &str) const
    {
      std::istringstream ss(str);
      ss >> std::noskipws >> val;
      if(!ss.eof())
      {
        ss >> std::ws;
      }
      return !ss.fail() && ss.eof();
    }

    template <typename T>
    Value& getValueP(const std::string& section, const std::string& tag,
                     const T& def)
    {
      Values::iterator val_it = m_sections[section].find(tag);
      if (val_it == m_sections[section].end())
      {
        setValue(section, tag, def);
      }

      return m_sections[section][tag];
    } /* getValueP */

}; /* class Config */


} /* namespace */

#endif /* ASYNC_CONFIG_INCLUDED */



/*
 * This file has not been truncated
 */

