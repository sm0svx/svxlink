/**
@file	 AsyncConfigBackend.h
@brief   Abstract base class for configuration backends
@author  Rui Barreiros / CR7BPM
@date	 2025-09-19

This file contains the base class declarationfor configuration backends that
can load configuration data from various sources like files, databases, etc.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute  it and/or modify
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

#ifndef ASYNC_CONFIG_BACKEND_INCLUDED
#define ASYNC_CONFIG_BACKEND_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <map>
#include <list>
#include <memory>
#include <ctime>
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <queue>
#include <thread>
#include <tuple>
#include <sigc++/sigc++.h>

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFactory.h>

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

namespace Async
{
  class FdWatch;
}

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

class ConfigBackend : public sigc::trackable
{
  public:
    /**
     * @brief   Constructor with notification settings
     * @param   enable_notifications  Enable change notifications (default: false)
     * @param   auto_poll_interval_ms Auto-polling interval in milliseconds (0 = disabled)
     *
     * If enable_notifications is true and auto_poll_interval_ms > 0, auto-polling
     * will start immediately.
     */
    ConfigBackend(bool enable_notifications = false,
                  unsigned int auto_poll_interval_ms = 0);
  
    /**
     * @brief 	Destructor
     */
    virtual ~ConfigBackend(void);
  
    /**
     * @brief 	Open/connect to the configuration source
     * @param 	source The configuration source (file path, connection string, etc.)
     * @return	Returns \em true on success or else \em false.
     *
     * This function will establish a connection to the configuration source.
     * For file backends, this opens the file. For database backends, this
     * establishes a database connection.
     */
    virtual bool open(const std::string& source) = 0;
    
    /**
     * @brief 	Close/disconnect from the configuration source
     *
     * This function closes the connection to the configuration source and
     * performs any necessary cleanup.
     */
    virtual void close(void) = 0;
    
    /**
     * @brief 	Check if the backend is connected/open
     * @return	Returns \em true if connected, \em false otherwise
     */
    virtual bool isOpen(void) const = 0;
    
    /**
     * @brief 	Get the string value of a configuration variable
     * @param 	section The name of the section where the configuration
     *	      	      	variable is located
     * @param 	tag   	The name of the configuration variable to get
     * @param 	value 	The value is returned in this argument
     * @return	Returns \em true on success or else \em false on failure
     *
     * This function retrieves the value for a configuration variable.
     */
    virtual bool getValue(const std::string& section, const std::string& tag,
                          std::string& value) const = 0;

    /**
     * @brief 	Set the value of a configuration variable
     * @param 	section   The name of the section where the configuration
     *	      	      	  variable is located
     * @param 	tag   	  The name of the configuration variable to set.
     * @param   value     The value to set
     * @return	Returns \em true on success or else \em false on failure
     *
     * This function sets the value of a configuration variable.
     * Not all backends may support writing (e.g., read-only file backends).
     */
    virtual bool setValue(const std::string& section, const std::string& tag,
                          const std::string& value) = 0;

    /**
     * @brief   Return the name of all configuration sections
     * @return  Returns a list of all existing section names
     */
    virtual std::list<std::string> listSections(void) const = 0;

    /**
     * @brief 	Return the name of all the tags in the given section
     * @param 	section The name of the section where the configuration
     *	      	      	variables are located
     * @return	Returns the list of tags in the given section
     */
    virtual std::list<std::string> listSection(const std::string& section) const = 0;

    /**
     * @brief   Get backend type identifier
     * @return  Returns a string identifying the backend type
     */
    virtual std::string getBackendType(void) const = 0;

    /**
     * @brief   Get backend-specific information
     * @return  Returns a string with backend-specific details
     */
    virtual std::string getBackendInfo(void) const = 0;

    /**
     * @brief   Set the table prefix for database backends
     * @param   prefix The prefix to use for table names (e.g., "svxlink_", "prod_svxreflector_")
     *
     * This method allows setting a custom table prefix for database backends.
     * The prefix will be prepended to the base table name ("config") to create
     * isolated table names for different applications or environments.
     * 
     * For file backends, this has no effect.
     */
    virtual void setTablePrefix(const std::string& prefix);

    /**
     * @brief   Get the current table prefix
     * @return  The current table prefix
     */
    const std::string& getTablePrefix(void) const { return m_table_prefix; }

    /**
     * @brief   Initialize database tables (for database backends)
     * @return  true on success, false on failure
     * @note    This should be called after setTablePrefix() and before any data access
     */
    virtual bool initializeTables(void) { return true; }

    /**
     * @brief   Finalize database initialization after tables are populated
     * @return  true on success, false on failure
     * @note    This should be called after tables are created and populated with data.
     *          Backends can use this to initialize change tracking timestamps, etc.
     */
    virtual bool finalizeInitialization(void) { return true; }

    /**
     * @brief   Enable or disable change notifications
     * @param   enable true to enable, false to disable
     */
    void enableChangeNotifications(bool enable);

    /**
     * @brief   Check if change notifications are enabled
     * @return  true if enabled, false otherwise
     */
    bool changeNotificationsEnabled(void) const;

    /**
     * @brief   Check for external changes (e.g., direct database updates)
     * @return  true if changes were detected, false otherwise
     *
     * This method is called by the auto-polling timer (if enabled) or can be
     * called manually. Database backends that require polling to check on
     * database changes need to override this method.
     */
    virtual bool checkForExternalChanges(void);

    /**
     * @brief   Start automatic polling for external changes
     * @param   interval_ms Polling interval in milliseconds
     *
     * Starts a timer that periodically calls checkForExternalChanges().
     * Stops any existing polling timer first.
     */
    void startAutoPolling(unsigned int interval_ms);

    /**
     * @brief   Stop automatic polling
     */
    void stopAutoPolling(void);

    /**
     * @brief   Check if auto-polling is active
     * @return  true if polling is active, false otherwise
     */
    bool isAutoPolling(void) const;

    /**
     * @brief   Get the current polling interval
     * @return  The polling interval in milliseconds, or 0 if not polling
     */
    unsigned int getPollingInterval(void) const;

    /**
     * @brief   Signal emitted when a configuration value changes
     * @param   section The section name
     * @param   tag The configuration tag name
     * @param   value The new value
     *
     * This signal can be emitted on notifyValueChanged() calls
     * which signal that a value changed.
     * 
     * @see checkForExternalChanges()
     */
    sigc::signal<void(const std::string&, const std::string&, const std::string&)> valueChanged;

  protected:
    /**
     * @brief   Notify listeners of a value change
     * @param   section The section name
     * @param   tag The configuration tag name
     * @param   value The new value
     *
     * Call this method from setValue() and checkForExternalChanges() implementations
     * to emit the valueChanged signal.
     * 
     * Respects the m_enable_change_notifications flag.
     */
    void notifyValueChanged(const std::string& section,
                           const std::string& tag,
                           const std::string& value);

    /**
     * @brief   Check for external changes from the auto-poll thread
     * @return  true if changes were detected, false otherwise
     *
     * Called periodically by the background poll thread. Database backends
     * that use a dedicated poll connection should override this method.
     * The default implementation delegates to checkForExternalChanges().
     */
    virtual bool pollForExternalChanges(void);

    /**
     * @brief   Open a dedicated database connection for the poll thread
     * @return  true on success, false on failure
     *
     * Called by startAutoPolling() before the poll thread is started.
     */
    virtual bool openPollConnection(void);

    /**
     * @brief   Close the dedicated poll-thread database connection
     *
     * Called by stopAutoPolling() after the poll thread has exited.
     */
    virtual void closePollConnection(void);

  protected:
    /**
     * @brief   Get the full table name with prefix
     * @param   base_name The base table name (e.g., "config")
     * @return  The full table name with prefix applied
     *
     * Helper method for database backends to construct full table names.
     * Returns prefix + base_name (e.g., "svxlink_config", "prod_reflector_config")
     */
    std::string getFullTableName(const std::string& base_name) const;

  private:
    std::string               m_table_prefix;
    std::atomic<bool>         m_enable_change_notifications;
    unsigned int              m_default_poll_interval;
    unsigned int              m_current_poll_interval;

    // Non-blocking poll infrastructure
    int                       m_wakeup_pipe[2];   // [0]=read, [1]=write
    Async::FdWatch*           m_fd_watch;
    std::mutex                m_queue_mutex;
    std::queue<std::tuple<std::string,std::string,std::string>> m_pending_changes;
    std::thread               m_poll_thread;
    std::mutex                m_poll_mutex;
    std::condition_variable   m_poll_cv;
    std::atomic<bool>         m_poll_running;

    void onWakeupPipe(Async::FdWatch* w);
    void pollThreadFunc(unsigned int interval_ms);

    ConfigBackend(const ConfigBackend&) = delete;
    ConfigBackend& operator=(const ConfigBackend&) = delete;

}; /* class ConfigBackend */

/**
 * @brief Smart pointer type for ConfigBackend
 */
using ConfigBackendPtr = std::unique_ptr<ConfigBackend>;

/****************************************************************************
 *
 * AsyncFactory Pattern Support
 *
 ****************************************************************************/

/**
 * @brief Factory for creating ConfigBackend instances
 *
 * Example registration:
 * @code
 * static ConfigBackendSpecificFactory<MySQLConfigBackend> mysql_factory;
 * @endcode
 */
using ConfigBackendFactory = Factory<ConfigBackend>;

/**
 * @brief Specific factory for a ConfigBackend implementation
 * @tparam T The concrete backend class (must inherit from ConfigBackend)
 *
 * Automatically registers the backend with the factory.
 */
template<class T>
struct ConfigBackendSpecificFactory : public SpecificFactory<ConfigBackend, T>
{
  ConfigBackendSpecificFactory(void)
    : SpecificFactory<ConfigBackend, T>(T::OBJNAME) {}
};

/****************************************************************************
 *
 * Convenience Functions
 *
 ****************************************************************************/

/**
 * @brief Create a ConfigBackend from a URL
 * @param url Configuration source URL (e.g., "mysql://user:pass@host/db")
 * @return ConfigBackendPtr on success, nullptr on failure
 *
 * Parses the URL, detects the backend type, and creates the appropriate backend.
 *
 * Example:
 * @code
 * auto backend = Async::createConfigBackend("sqlite:///var/lib/svxlink/db.sqlite");
 * if (backend && backend->open(...)) {
 *   // Use backend
 * }
 * @endcode
 */
ConfigBackendPtr createConfigBackend(const std::string& url);

/**
 * @brief Create a ConfigBackend by explicit type
 * @param backend_type Backend type name (e.g., "mysql", "sqlite")
 * @param connection_info Connection information (file path or connection string)
 * @return ConfigBackendPtr on success, nullptr on failure
 *
 * Example:
 * @code
 * auto backend = Async::createConfigBackendByType("mysql", "host,3306,db,user,pass");
 * @endcode
 */
ConfigBackendPtr createConfigBackendByType(const std::string& backend_type,
                                          const std::string& connection_info);

} /* namespace */

#endif /* ASYNC_CONFIG_BACKEND_INCLUDED */

/*
 * This file has not been truncated
 */
