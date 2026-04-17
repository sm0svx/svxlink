/**
@file	 RemoteUserAuth.cpp
@brief   Remote User Authentication
@author  Rui Barreiros / CR7BPM
@date	 2026-01-07

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
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

#include <iostream>
#include <sstream>
#include <cstring>
#include <memory>
#include <json/json.h>
#include <curl/curl.h>


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

#include "RemoteUserAuth.h"


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

// Constants, this could later on be configurable...
static const int  MAX_RETRIES            = 2;
static const long CONNECT_TIMEOUT_SECS   = 5;
static const long LOW_SPEED_TIME_SECS    = 10;
static const long LOW_SPEED_LIMIT_BPS    = 100;
static const long TIMEOUT_SECS           = 10;


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/


/****************************************************************************
 *
 * Local functions
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
 * Public static functions
 *
 ****************************************************************************/

bool RemoteUserAuth::curlGlobalInit(void)
{
  CURLcode res = curl_global_init(CURL_GLOBAL_DEFAULT);
  if (res != CURLE_OK)
  {
    cerr << "*** ERROR: curl_global_init() failed: "
         << curl_easy_strerror(res) << endl;
    return false;
  }
  cout << "RemoteUserAuth: global curl initialized" << endl;
  return true;
} /* RemoteUserAuth::curlGlobalInit */


void RemoteUserAuth::curlGlobalCleanup(void)
{
  curl_global_cleanup();
  cout << "RemoteUserAuth: global curl cleaned up" << endl;
} /* RemoteUserAuth::curlGlobalCleanup */


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/

RemoteUserAuth::RemoteUserAuth(void)
  : m_force_valid_ssl(true), m_timeout_timer(0, Timer::TYPE_ONESHOT, false),
    m_total_requests(0), m_failed_requests(0), m_retried_requests(0)
{
  m_multi_handle = curl_multi_init();
  if (!m_multi_handle)
  {
    cerr << "*** ERROR: curl_multi_init() failed. RemoteUserAuth will not function." << endl;
    return;
  }
  
  // Set up socket action API callbacks
  curl_multi_setopt(m_multi_handle, CURLMOPT_SOCKETFUNCTION, 
                    &RemoteUserAuth::socketCallback);
  curl_multi_setopt(m_multi_handle, CURLMOPT_SOCKETDATA, this);
  curl_multi_setopt(m_multi_handle, CURLMOPT_TIMERFUNCTION,
                    &RemoteUserAuth::timerCallback);
  curl_multi_setopt(m_multi_handle, CURLMOPT_TIMERDATA, this);
  
  m_timeout_timer.expired.connect(sigc::mem_fun(*this, &RemoteUserAuth::onCurlTimer));
} /* RemoteUserAuth::RemoteUserAuth */


RemoteUserAuth::~RemoteUserAuth(void)
{
  m_timeout_timer.setEnable(false);
  
  // Clean all pending requests
  for (auto& [curl, req] : m_request_map)
  {
    curl_multi_remove_handle(m_multi_handle, curl);
    curl_easy_cleanup(curl);
    curl_slist_free_all(req->headers);
  }
  m_request_map.clear();

  // Clean all watches
  m_watch_map.clear();

  if (m_multi_handle)
  {
    curl_multi_cleanup(m_multi_handle);
    m_multi_handle = nullptr;
  }
} /* RemoteUserAuth::~RemoteUserAuth */


void RemoteUserAuth::setParams(const string& auth_url, const string& auth_token,
                               bool force_valid_ssl)
{
  m_auth_url = auth_url;
  m_auth_token = auth_token;
  m_force_valid_ssl = force_valid_ssl;
} /* RemoteUserAuth::setParams */


void RemoteUserAuth::checkUser(const string& username, const string& digest,
                               const string& challenge,
                               sigc::slot<void(bool, string)> callback)
{
  if (!m_multi_handle)
  {
    cerr << "*** ERROR[" << username << "]: Remote auth failed - libcurl not initialized" << endl;
    callback(false, "Internal error: libcurl not initialized");
    return;
  }
  m_total_requests++;
  
  CURL* curl = curl_easy_init();
  if (!curl)
  {
    m_failed_requests++;
    callback(false, "Failed to initialize curl");
    return;
  }

  auto req = std::make_unique<Request>();
  req->curl = curl;
  req->callback = callback;
  req->headers = NULL;
  req->retry_count = 0;
  req->start_time = std::chrono::steady_clock::now();
  req->username = username;

  Json::Value root;
  root["username"] = username;
  root["digest"] = digest;
  root["challenge"] = challenge;
  
  Json::StreamWriterBuilder wbuilder;
  req->post_data = Json::writeString(wbuilder, root);

  string auth_header = "Authorization: Bearer " + m_auth_token;
  req->headers = curl_slist_append(req->headers, "Content-Type: application/json");
  req->headers = curl_slist_append(req->headers, auth_header.c_str());

  curl_easy_setopt(curl, CURLOPT_URL, m_auth_url.c_str());
  curl_easy_setopt(curl, CURLOPT_POSTFIELDS, req->post_data.c_str());
  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, req->headers);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &RemoteUserAuth::writeCallback);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, req.get());
  curl_easy_setopt(curl, CURLOPT_PRIVATE, req.get());
  
  // timeout settings
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, TIMEOUT_SECS);
  curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, CONNECT_TIMEOUT_SECS);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME_SECS);
  curl_easy_setopt(curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT_BPS);
  curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1L);

  if (!m_force_valid_ssl)
  {
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
  }

  cout << username << ": Starting remote authentication request..." << endl;

  // Transfer ownership to map
  m_request_map[curl] = std::move(req);
  
  curl_multi_add_handle(m_multi_handle, curl);

  // Kick off the multi interface
  int running_handles;
  curl_multi_socket_action(m_multi_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
} /* RemoteUserAuth::checkUser */


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

void RemoteUserAuth::onCurlTimer(Timer *timer)
{
  if (!m_multi_handle)
  {
    return;
  }
  int running_handles;
  curl_multi_socket_action(m_multi_handle, CURL_SOCKET_TIMEOUT, 0, &running_handles);
  checkMultiInfo();
} /* RemoteUserAuth::onCurlTimer */


void RemoteUserAuth::onSocketActivity(FdWatch *watch)
{
  int event_bitmask = 0;
  if (watch->type() & FdWatch::FD_WATCH_RD)
  {
    event_bitmask |= CURL_CSELECT_IN;
  }
  if (watch->type() & FdWatch::FD_WATCH_WR)
  {
    event_bitmask |= CURL_CSELECT_OUT;
  }
  
  performSocketAction(watch->fd(), event_bitmask);
} /* RemoteUserAuth::onSocketActivity */


void RemoteUserAuth::performSocketAction(curl_socket_t s, int event_bitmask)
{
  int running_handles;
  curl_multi_socket_action(m_multi_handle, s, event_bitmask, &running_handles);
  checkMultiInfo();
} /* RemoteUserAuth::performSocketAction */


// called by curl when socket state changes, static
int RemoteUserAuth::socketCallback(CURL* easy, curl_socket_t s, int what,
                                   void* userp, void* socketp)
{
  RemoteUserAuth* self = static_cast<RemoteUserAuth*>(userp);
  return self->handleSocketAction(easy, s, what, socketp);
} /* RemoteUserAuth::socketCallback */


// handle socket action from curl
int RemoteUserAuth::handleSocketAction(CURL* easy, curl_socket_t s, int what,
                                       void* socketp)
{
  if (!m_multi_handle)
  {
    return -1;
  }
  if (what == CURL_POLL_REMOVE)
  {
    // Remove the watch for this socket
    auto it = m_watch_map.find(s);
    if (it != m_watch_map.end())
    {
      m_watch_map.erase(it);
    }
  }
  else
  {
    // Create or update watch for this socket
    WatchSet* ws = nullptr;
    auto it = m_watch_map.find(s);
    if (it == m_watch_map.end())
    {
      auto new_ws = std::make_unique<WatchSet>();
      ws = new_ws.get();
      m_watch_map[s] = std::move(new_ws);
    }
    else
    {
      ws = it->second.get();
    }

    // Update read watch
    bool need_read = (what & CURL_POLL_IN) != 0;
    if (need_read && !ws->rd.isEnabled())
    {
      ws->rd.setFd(s, FdWatch::FD_WATCH_RD);
      ws->rd.activity.connect(sigc::mem_fun(*this, &RemoteUserAuth::onSocketActivity));
      ws->rd.setEnabled(true);
      ws->rd_enabled = true;
    }
    else if (!need_read && ws->rd.isEnabled())
    {
      ws->rd.setEnabled(false);
      ws->rd_enabled = false;
    }

    // Update write watch
    bool need_write = (what & CURL_POLL_OUT) != 0;
    if (need_write && !ws->wr.isEnabled())
    {
      ws->wr.setFd(s, FdWatch::FD_WATCH_WR);
      ws->wr.activity.connect(sigc::mem_fun(*this, &RemoteUserAuth::onSocketActivity));
      ws->wr.setEnabled(true);
      ws->wr_enabled = true;
    }
    else if (!need_write && ws->wr.isEnabled())
    {
      ws->wr.setEnabled(false);
      ws->wr_enabled = false;
    }
  }
  
  return 0;
} /* RemoteUserAuth::handleSocketAction */


// called by curl when timeout needs updating, static
int RemoteUserAuth::timerCallback(CURLM* multi, long timeout_ms, void* userp)
{
  RemoteUserAuth* self = static_cast<RemoteUserAuth*>(userp);
  return self->handleTimerUpdate(timeout_ms);
} /* RemoteUserAuth::timerCallback */


// handle timer update from curl
int RemoteUserAuth::handleTimerUpdate(long timeout_ms)
{
  if (!m_multi_handle)
  {
    return -1;
  }
  m_timeout_timer.setEnable(false);
  
  if (timeout_ms < 0)
  {
    // No timeout needed
    return 0;
  }
  
  if (timeout_ms == 0)
  {
    // Schedule immediately (next event loop iteration) to break recursion
    // if this was called from within a curl callback (e.g. add_handle)
    timeout_ms = 0;
  }
  else
  {
    // Set timer for the specified duration
  }
  
  m_timeout_timer.setTimeout(timeout_ms);
  m_timeout_timer.setEnable(true);
  
  return 0;
} /* RemoteUserAuth::handleTimerUpdate */


void RemoteUserAuth::checkMultiInfo(void)
{
  int msgs_left;
  CURLMsg* msg;
  while ((msg = curl_multi_info_read(m_multi_handle, &msgs_left)))
  {
    if (msg->msg == CURLMSG_DONE)
    {
      CURL* curl = msg->easy_handle;
      auto it = m_request_map.find(curl);
      if (it == m_request_map.end())
      {
        continue;  // Request not found
      }
      
      Request* req = it->second.get();
      bool success = false;
      string message = "Unknown error";
      
      // Calculate request duration
      auto end_time = std::chrono::steady_clock::now();
      auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(
        end_time - req->start_time);
      
      CURLcode result = msg->data.result;
      
      // Check if we should retry
      bool should_retry = false;
      if (result != CURLE_OK && req->retry_count < MAX_RETRIES)
      {
        if (result == CURLE_COULDNT_CONNECT ||
            result == CURLE_OPERATION_TIMEDOUT ||
            result == CURLE_COULDNT_RESOLVE_HOST)
        {
          should_retry = true;
        }
      }
      
      if (should_retry)
      {
        cout << req->username << ": Request failed (" 
             << curl_easy_strerror(result)
             << "), retrying (attempt " << (req->retry_count + 2) << "/"
             << (MAX_RETRIES + 1) << ")..." << endl;
        
        req->retry_count++;
        m_retried_requests++;
        
        // Retry the request - move ownership temporarily
        auto req_unique = std::move(it->second);
        m_request_map.erase(it);
        
        curl_multi_remove_handle(m_multi_handle, curl);
        curl_multi_add_handle(m_multi_handle, curl);
        
        // Put it back
        m_request_map[curl] = std::move(req_unique);
        
        int running_handles;
        curl_multi_socket_action(m_multi_handle, CURL_SOCKET_TIMEOUT, 0, 
                                &running_handles);
        continue;
      }
      
      if (result == CURLE_OK)
      {
        // Get HTTP response code
        long http_code = 0;
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
        
        cout << req->username << ": Remote auth response received (HTTP "
             << http_code << ", " << duration.count() << "ms)" << endl;
        
        if (http_code >= 200 && http_code < 300)
        {
          Json::Value root;
          Json::CharReaderBuilder rbuilder;
          string errs;
          istringstream iss(req->response_data);
          
          if (Json::parseFromStream(rbuilder, iss, &root, &errs))
          {
            // field validation! or this will crash on wrong json!
            if (root.isMember("success") && root["success"].isBool())
            {
              success = root["success"].asBool();
            }
            else
            {
              cout << "*** WARNING[" << req->username 
                   << "]: 'success' field missing or invalid in JSON response"
                   << endl;
              success = false;
            }
            
            if (root.isMember("message") && root["message"].isString())
            {
              message = root["message"].asString();
            }
            else
            {
              message = success ? "Authentication successful" 
                                : "Authentication failed (no message)";
            }
          }
          else
          {
            message = "Failed to parse JSON response: " + errs;
            cout << "*** WARNING[" << req->username << "]: " << message << endl;
          }
        }
        else
        {
          message = string("HTTP error: ") + std::to_string(http_code);
          cout << "*** WARNING[" << req->username << "]: " << message << endl;
        }
      }
      else
      {
        message = string("CURL error: ") + curl_easy_strerror(result);
        cout << "*** WARNING[" << req->username << "]: " << message 
             << " (after " << duration.count() << "ms)" << endl;
        m_failed_requests++;
      }

      // may throw, but that's ok with unique_ptr....
      try
      {
        req->callback(success, message);
      }
      catch (const std::exception& e)
      {
        cerr << "*** ERROR: Exception in RemoteUserAuth callback: " 
             << e.what() << endl;
      }
      catch (...)
      {
        cerr << "*** ERROR: Unknown exception in RemoteUserAuth callback" << endl;
      }

      // unique_ptr handles deletion
      curl_multi_remove_handle(m_multi_handle, curl);
      curl_easy_cleanup(curl);
      curl_slist_free_all(req->headers);
      m_request_map.erase(it);
    }
  }
} /* RemoteUserAuth::checkMultiInfo */


size_t RemoteUserAuth::writeCallback(void *contents, size_t size, size_t nmemb,
                                     void *userp)
{
  size_t realsize = size * nmemb;
  Request* req = static_cast<Request*>(userp);
  req->response_data.append(static_cast<char*>(contents), realsize);
  return realsize;
} /* RemoteUserAuth::writeCallback */
