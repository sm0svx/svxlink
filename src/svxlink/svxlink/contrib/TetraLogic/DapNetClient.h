/**
@file	 DapNetClient.h
@brief   Network connection manager for DapNet
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2021-02-07

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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


#ifndef DAPNET_CLIENT_INCLUDED
#define DAPNET_CLIENT_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <map>
#include <utility>
#include <string>
#include <iomanip>
#include <sys/time.h>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncConfig.h>


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
  class Timer;
};


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

using namespace sigc;


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
@brief	Network connection manager for remote transceivers
@author Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date   2021-02-07
*/
class DapNetClient : public Async::TcpClient<>
{
  public:
    typedef Async::TcpConnection::DisconnectReason DiscReason;
    /**
     * @brief 	
     * @param   
     * @param   
     * @return	
     *
     * 
     */
    DapNetClient(Async::Config& cfg, const std::string& name);
    
    /**
     * @brief 	Destructor
     */
    ~DapNetClient(void);

    
    /**
     * @brief 
     */
    bool initialize(void);
   
    /**
     * @brief A signal that is emitted when a message has been received
     * @param msg The received message
     */
    sigc::signal<void, std::string, std::string> dapnetMessageReceived;

    /**
     * @brief A signal that is emitted when a log message is send
     * @param msg The received message
     */
    sigc::signal<void, uint8_t, std::string> dapnetLogmessage;

    /**
     *
     *
     */
    bool sendDapMessage(std::string call, std::string message);
    
  protected:

    
  private:

    Async::Config             &cfg;
    std::string                name;
    Async::TcpClient<>        *dapcon;
    short                       debug;
    Async::Timer              *reconnect_timer;
    std::string                dapnet_server;
    int                        dapnet_port;
    std::string                dapnet_key;
    std::map<int, std::vector<std::string> >     ric2issi;
    std::map<int, std::string> ric2rubrics;
    std::string                dapmessage;
    std::string                callsign;
    typedef std::vector<std::string> StrList;
    std::string                dapnet_username;
    std::string                dapnet_password;
    std::string                dapnet_webhost;
    std::string                dapnet_webpath;
    int                        dapnet_webport;
    Async::TcpClient<>         *dapwebcon;
    std::string                txgroup;
    std::string                destcall;
    std::string                destmessage;
        
    DapNetClient(const DapNetClient&);
    DapNetClient& operator=(const DapNetClient&);
    
    void onDapnetConnected(void);
    void onDapnetDisconnected(Async::TcpConnection *con,
      	      	      	 Async::TcpConnection::DisconnectReason reason);
    int onDapnetDataReceived(Async::TcpConnection *con, void *data, int size);
    void onDapwebConnected(void);
    void onDapwebDisconnected(Async::TcpConnection *con,
      	      	      	 Async::TcpConnection::DisconnectReason reason);
    int onDapwebDataReceived(Async::TcpConnection *con, void *data, int size);
    char* encodeBase64(const char input_str[], int len_str);
    void handleTimeSync(std::string msg);
    void handleDapType4(std::string msg);
    void handleDapText(std::string msg);
    void dapOK(void);
    void dapTanswer(std::string msg);
    void handleDapMessage(std::string dapmessage);
    void reconnectDapnetServer(Async::Timer *t);
    int checkDapMessage(std::string mesg);
    bool rmatch(std::string tok, std::string pattern);
    std::string rot1code(std::string inmessage);
};  /* class DapNetClient */


//} /* namespace */

#endif /* DAPNET_CLIENT_INCLUDED */



/*
 * This file has not been truncated
 */

