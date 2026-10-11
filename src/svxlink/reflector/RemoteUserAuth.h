/**
@file	 RemoteUserAuth.h
@brief   Remote user authentication
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

#ifndef REMOTE_USER_AUTH_INCLUDED
#define REMOTE_USER_AUTH_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <map>
#include <memory>
#include <chrono>
#include <curl/curl.h>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>
#include <AsyncTimer.h>


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



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief   Handles remote user authentication via a web API
@author  Rui Barreiros / CR7BPM
@date    2026-01-02

This class performs asynchronous HTTP POST requests to a remote authentication
server. It uses libcurl's multi-interface to avoid blocking the main event
loop.
If remote authentication is enabled, it's always the first to be checked. If the
authentication fails, it will fallback to local config authentication.

### Configuration Options
The following options should be set in the `[REMOTE_USER_AUTH]` section of `svxreflector.conf`:
- `USER_AUTH_ENABLE`: Set to 1 to enable remote authentication.
- `USER_AUTH_URL`: The URL of the authentication endpoint (e.g., `https://hostname/api/v1/auth/user`).
- `USER_AUTH_TOKEN`: A Bearer token used for authenticating with the remote API.
- `USER_AUTH_FORCE_VALID_SSL`: Set to 0 to allow insecure/self-signed SSL certificates.

Example:

[REMOTE_USER_AUTH]
USER_AUTH_ENABLE=1
USER_AUTH_URL=https://hostname/api/v1/auth/user
USER_AUTH_TOKEN=_JWT_TOKEN_
USER_AUTH_FORCE_VALID_SSL=1

### API Communication

**Request (POST)**:
The body is a JSON object containing the username, HMAC digest, and original challenge.
```json
{
  "username": "SM0ABC",
  "digest": "a1b2c3d4e5f6...",
  "challenge": "f8e7d6c5b4a3..."
}
```
An `Authorization: Bearer <token>` header is also included.

**Response (Expected JSON)**:
The server MUST return a JSON object with at least `success` and `message` fields.
```json
{
  "success": true,
  "message": "Authentication successful"
}
```
If `success` is false, the `message` will be logged/reported to the client.

The absense of success or anything other than true, will be treated as a login failure.

*/
class RemoteUserAuth : public sigc::trackable
{
  public:
    /**
     * @brief   Initialize curl globally (call once at application startup)
     *
     *          This could actually be called in the main() function, it
     *          should only be called once per application! But since Reflector
     *          class is ran only once, it's ok!
     *
     * @return  True on success, false on failure
     */
    static bool curlGlobalInit(void);


    /**
     * @brief   Cleanup curl globally (call once at application shutdown)
     *
     *          This could actually be called in the main() function, like
     *          curlGlobalInit, it should only be called once per application!
     *          But since Reflector class is ran only once, it's ok!
     */
    static void curlGlobalCleanup(void);

    /**
     * @brief   Constructor
     */
    RemoteUserAuth(void);

    /**
     * @brief   Destructor
     */
    ~RemoteUserAuth(void);

    /**
     * @brief   Set parameters for the remote authentication
     * @param   auth_url The URL of the authentication endpoint
     * @param   auth_token The Bearer token for authentication
     * @param   force_valid_ssl Whether to enforce valid SSL certificates
     */
    void setParams(const std::string& auth_url, const std::string& auth_token,
                   bool force_valid_ssl);

    /**
     * @brief   Check if a user is authenticated on the remote server
     * @param   username The username to check
     * @param   digest Hex encoded HMAC digest
     * @param   challenge Hex encoded challenge
     * @param   callback The callback to call when the check is complete
     */
    void checkUser(const std::string& username, const std::string& digest,
                   const std::string& challenge,
                   sigc::slot<void(bool, std::string)> callback);

    /**
     * @brief   Get total number of requests processed
     * @return  Total request count
     */
    uint64_t totalRequests(void) const { return m_total_requests; }

    /**
     * @brief   Get number of failed requests
     * @return  Failed request count
     */
    uint64_t failedRequests(void) const { return m_failed_requests; }

    /**
     * @brief   Get number of retried requests
     * @return  Retried request count
     */
    uint64_t retriedRequests(void) const { return m_retried_requests; }

  private:
    struct WatchSet
    {
      WatchSet() : rd_enabled(false), wr_enabled(false) {}
      Async::FdWatch rd;
      Async::FdWatch wr;
      bool rd_enabled;
      bool wr_enabled;
    };

    struct Request
    {
      CURL* curl;
      curl_slist* headers;
      std::string post_data;
      std::string response_data;
      sigc::slot<void(bool, std::string)> callback;
      int retry_count;
      std::chrono::steady_clock::time_point start_time;
      std::string username;  // For logging
    };

    std::string m_auth_url;
    std::string m_auth_token;
    bool        m_force_valid_ssl;
    CURLM*      m_multi_handle;
    Async::Timer m_timeout_timer;
    std::map<int, std::unique_ptr<WatchSet>> m_watch_map;
    std::map<CURL*, std::unique_ptr<Request>> m_request_map;
    
    // Useful statistics, for logging and debugging
    uint64_t m_total_requests;
    uint64_t m_failed_requests;
    uint64_t m_retried_requests;

    void onCurlTimer(Async::Timer *timer);
    void onSocketActivity(Async::FdWatch *watch);
    void checkMultiInfo(void);
    void retryRequest(Request* req);
    
    // Socket action API callbacks
    static int socketCallback(CURL* easy, curl_socket_t s, int what,
                             void* userp, void* socketp);
    static int timerCallback(CURLM* multi, long timeout_ms, void* userp);
    
    int handleSocketAction(CURL* easy, curl_socket_t s, int what, void* socketp);
    int handleTimerUpdate(long timeout_ms);
    void performSocketAction(curl_socket_t s, int event_bitmask);
    
    static size_t writeCallback(void *contents, size_t size, size_t nmemb,
                                void *userp);

};  /* class RemoteUserAuth */


#endif /* REMOTE_USER_AUTH_INCLUDED */
