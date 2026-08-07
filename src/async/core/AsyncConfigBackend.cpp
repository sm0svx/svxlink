/**
 * @file   AsyncConfigBackend.cpp
 * @brief  Implementation of ConfigBackend base class and factory functions
 * @author Rui Barreiros / CR7BPM
 * @date   2025-09-19

This file contains the base class implementation for configuration backends that
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

#include "AsyncConfigBackend.h"
#include "AsyncConfigSource.h"
#include "AsyncFdWatch.h"
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

namespace Async
{

ConfigBackend::ConfigBackend(bool enable_notifications, unsigned int auto_poll_interval_ms)
  : m_enable_change_notifications(enable_notifications),
    m_default_poll_interval(auto_poll_interval_ms),
    m_current_poll_interval(0),
    m_fd_watch(nullptr),
    m_poll_running(false)
{
  m_wakeup_pipe[0] = m_wakeup_pipe[1] = -1;
} /* ConfigBackend::ConfigBackend */

ConfigBackend::~ConfigBackend(void)
{
  stopAutoPolling();
} /* ConfigBackend::~ConfigBackend */

void ConfigBackend::setTablePrefix(const std::string& prefix)
{
  m_table_prefix = prefix;
} /* ConfigBackend::setTablePrefix */

std::string ConfigBackend::getFullTableName(const std::string& base_name) const
{
  return m_table_prefix + base_name;
} /* ConfigBackend::getFullTableName */

void ConfigBackend::enableChangeNotifications(bool enable)
{
  m_enable_change_notifications = enable;
} /* ConfigBackend::enableChangeNotifications */

bool ConfigBackend::changeNotificationsEnabled(void) const
{
  return m_enable_change_notifications;
} /* ConfigBackend::changeNotificationsEnabled */

bool ConfigBackend::checkForExternalChanges(void)
{
  // Database backends should override this
  return false;
} /* ConfigBackend::checkForExternalChanges */

bool ConfigBackend::pollForExternalChanges(void)
{
  return checkForExternalChanges();
} /* ConfigBackend::pollForExternalChanges */

bool ConfigBackend::openPollConnection(void)
{
  return true;
} /* ConfigBackend::openPollConnection */

void ConfigBackend::closePollConnection(void)
{
} /* ConfigBackend::closePollConnection */

void ConfigBackend::startAutoPolling(unsigned int interval_ms)
{
  if (interval_ms == 0)
  {
    stopAutoPolling();
    return;
  }

  stopAutoPolling();

  if (::pipe(m_wakeup_pipe) == -1)
  {
    std::cerr << "*** ERROR: ConfigBackend: pipe() failed: "
              << strerror(errno) << std::endl;
    return;
  }
  // Make the read end non-blocking so drain reads never stall
  int flags = ::fcntl(m_wakeup_pipe[0], F_GETFL, 0);
  ::fcntl(m_wakeup_pipe[0], F_SETFL, flags | O_NONBLOCK);

  m_fd_watch = new Async::FdWatch(m_wakeup_pipe[0], Async::FdWatch::FD_WATCH_RD);
  m_fd_watch->activity.connect(sigc::mem_fun(*this, &ConfigBackend::onWakeupPipe));

  if (!openPollConnection())
  {
    std::cerr << "*** ERROR: ConfigBackend: Failed to open poll connection"
              << std::endl;
    delete m_fd_watch;
    m_fd_watch = nullptr;
    ::close(m_wakeup_pipe[0]);
    ::close(m_wakeup_pipe[1]);
    m_wakeup_pipe[0] = m_wakeup_pipe[1] = -1;
    return;
  }

  std::cout << "Starting async config polling (interval "
            << interval_ms << " ms)" << std::endl;

  m_current_poll_interval = interval_ms;
  m_poll_running = true;
  m_poll_thread = std::thread(&ConfigBackend::pollThreadFunc, this, interval_ms);
} /* ConfigBackend::startAutoPolling */

void ConfigBackend::stopAutoPolling(void)
{
  if (!m_poll_running.load())
  {
    return;
  }

  {
    std::lock_guard<std::mutex> lock(m_poll_mutex);
    m_poll_running = false;
  }
  m_poll_cv.notify_all();

  if (m_poll_thread.joinable())
  {
    m_poll_thread.join();
  }

  delete m_fd_watch;
  m_fd_watch = nullptr;

  if (m_wakeup_pipe[0] != -1)
  {
    ::close(m_wakeup_pipe[0]);
    ::close(m_wakeup_pipe[1]);
    m_wakeup_pipe[0] = m_wakeup_pipe[1] = -1;
  }

  closePollConnection();

  // Discard any queued changes that never made it to the event loop
  {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    while (!m_pending_changes.empty())
      m_pending_changes.pop();
  }

  m_current_poll_interval = 0;
  std::cout << "Stopped async config polling" << std::endl;
} /* ConfigBackend::stopAutoPolling */

bool ConfigBackend::isAutoPolling(void) const
{
  return m_poll_running.load();
} /* ConfigBackend::isAutoPolling */

unsigned int ConfigBackend::getPollingInterval(void) const
{
  return m_current_poll_interval;
} /* ConfigBackend::getPollingInterval */

void ConfigBackend::pollThreadFunc(unsigned int interval_ms)
{
  std::unique_lock<std::mutex> lock(m_poll_mutex);
  while (m_poll_running.load())
  {
    auto status = m_poll_cv.wait_for(lock,
        std::chrono::milliseconds(interval_ms));

    if (!m_poll_running.load())
    {
      break;
    }

    if (status == std::cv_status::timeout)
    {
      lock.unlock();
      pollForExternalChanges();
      lock.lock();
    }
  }
} /* ConfigBackend::pollThreadFunc */

void ConfigBackend::onWakeupPipe(Async::FdWatch*)
{
  // Drain all notification bytes written by the poll thread
  char buf[64];
  while (::read(m_wakeup_pipe[0], buf, sizeof(buf)) > 0) {}

  // Swap-out the queue so we hold the lock for the shortest possible time
  std::queue<std::tuple<std::string,std::string,std::string>> batch;
  {
    std::lock_guard<std::mutex> lock(m_queue_mutex);
    std::swap(batch, m_pending_changes);
  }

  // Emit all queued valueChanged signals on the event-loop thread
  while (!batch.empty())
  {
    auto& [section, tag, value] = batch.front();
    valueChanged(section, tag, value);
    batch.pop();
  }
} /* ConfigBackend::onWakeupPipe */

void ConfigBackend::notifyValueChanged(const std::string& section,
                                      const std::string& tag,
                                      const std::string& value)
{
  if (!m_enable_change_notifications.load())
  {
    return;
  }

  std::cout << "Configuration changed: [" << section << "]/" << tag
            << std::endl;

  if (m_wakeup_pipe[1] != -1)
  {
    // Called from the poll thread: queue the change and wake the event loop
    {
      std::lock_guard<std::mutex> lock(m_queue_mutex);
      m_pending_changes.emplace(section, tag, value);
    }
    char byte = 1;
    if (::write(m_wakeup_pipe[1], &byte, sizeof(byte)) == -1 && errno != EAGAIN)
    {
      std::cerr << "*** WARNING: ConfigBackend: wakeup pipe write failed: "
                << strerror(errno) << std::endl;
    }
  }
  else
  {
    // No polling thread active: emit directly on the calling (event-loop) thread
    valueChanged(section, tag, value);
  }
} /* ConfigBackend::notifyValueChanged */

// Factory convenience functions

ConfigBackendPtr createConfigBackend(const std::string& url)
{
  auto parsed = ConfigSource::parse(url);
  if (!parsed)
  {
    std::cerr << "*** ERROR: Invalid configuration source URL" << std::endl;
    return nullptr;
  }

  return createConfigBackendByType(parsed->backend_type_name, parsed->connection_info);
} /* createConfigBackend */

ConfigBackendPtr createConfigBackendByType(const std::string& backend_type,
                                          const std::string& connection_info)
{
  ConfigBackendPtr backend(ConfigBackendFactory::createNamedObject(backend_type));
  if (!backend)
  {
    std::cerr << "*** ERROR: Failed to create backend of type '" << backend_type 
              << "'. Available: " << ConfigBackendFactory::validFactories() << std::endl;
    return nullptr;
  }
  
  if (!backend->open(connection_info))
  {
    std::cerr << "*** ERROR: Failed to open '" << backend_type
              << "' configuration backend" << std::endl;
    return nullptr;
  }
  
  return backend;
} /* createConfigBackendByType */

} /* namespace Async */

