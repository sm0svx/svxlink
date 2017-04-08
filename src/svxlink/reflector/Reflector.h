/**
@file	 Reflector.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

A_detailed_description_for_this_file

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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

/** @example Template_demo.cpp
An example of how to use the Reflector class
*/


#ifndef REFLECTOR_INCLUDED
#define REFLECTOR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <sys/time.h>
#include <vector>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpServer.h>
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

namespace Async
{
  class UdpSocket;
  class Config;
};

class ReflectorClient;
class ReflectorMsg;
class ReflectorUdpMsg;


/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2017-02-11

A_detailed_class_description
*/
class Reflector : public sigc::trackable
{
  public:
    /**
     * @brief 	Constructor
     */
    Reflector(void);

    /**
     * @brief 	Destructor
     */
    ~Reflector(void);

    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    bool initialize(Async::Config &cfg);

    void nodeList(std::vector<std::string>& nodes) const;
    void broadcastMsgExcept(const ReflectorMsg& msg, ReflectorClient *client=0);
    void sendUdpDatagram(ReflectorClient *client, const void *buf, size_t count);

  protected:

  private:
    typedef std::map<uint32_t, ReflectorClient*> ReflectorClientMap;
    typedef std::map<Async::TcpConnection*,
                     ReflectorClient*> ReflectorClientConMap;

    Async::TcpServer<>*   srv;
    Async::UdpSocket*     udp_sock;
    ReflectorClientMap    client_map;
    std::string           m_auth_key;
    ReflectorClient*      m_talker;
    Async::Timer          m_talker_timeout_timer;
    struct timeval        m_last_talker_timestamp;
    ReflectorClientConMap m_client_con_map;
    unsigned              m_sql_timeout;
    unsigned              m_sql_timeout_cnt;
    unsigned              m_sql_timeout_blocktime;

    Reflector(const Reflector&);
    Reflector& operator=(const Reflector&);
    void clientConnected(Async::TcpConnection *con);
    void clientDisconnected(Async::TcpConnection *con,
                            Async::TcpConnection::DisconnectReason reason);
    void udpDatagramReceived(const Async::IpAddress& addr, uint16_t port,
                             void *buf, int count);
    //void sendUdpMsg(ReflectorClient *client, const ReflectorUdpMsg& msg);
    void broadcastUdpMsgExcept(const ReflectorClient *client,
                               const ReflectorUdpMsg& msg);
    void checkTalkerTimeout(Async::Timer *t);
    void setTalker(ReflectorClient *client);

};  /* class Reflector */


//} /* namespace */

#endif /* REFLECTOR_INCLUDED */



/*
 * This file has not been truncated
 */
