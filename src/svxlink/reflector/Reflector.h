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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpConnection.h>


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
  class TcpServer;
  class UdpSocket;
  class Config;
};

class ReflectorClient;
class Msg;


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

  protected:

  private:
    typedef std::map<uint32_t, ReflectorClient*> ReflectorClientMap;

    Async::TcpServer*   srv;
    Async::UdpSocket*   udp_sock;
    ReflectorClientMap  client_map;

    Reflector(const Reflector&);
    Reflector& operator=(const Reflector&);
    void clientConnected(Async::TcpConnection *con);
    void clientDisconnected(Async::TcpConnection *con,
                            Async::TcpConnection::DisconnectReason reason);
    void udpDatagramReceived(const Async::IpAddress& addr, uint16_t port,
                             void *buf, int count);
    void sendUdpMsg(const ReflectorClient *client, const Msg &msg);
    void broadcastUdpMsgExcept(const ReflectorClient *client, const Msg& msg);

};  /* class Reflector */


//} /* namespace */

#endif /* REFLECTOR_INCLUDED */



/*
 * This file has not been truncated
 */
