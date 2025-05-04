/**
@file	 AprsTcpClient.h
@brief   Contains an implementation of APRS updates via TCP
@author  Adi Bier / DL1HRC
@date	 2008-11-01

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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


#ifndef APRS_TCP_CLIENT
#define APRS_TCP_CLIENT


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LocationInfo.h"
#include "AprsClient.h"


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
@brief	Aprs-logics
@author Adi Bier / DL1HRC
@date   2008-11-01
*/
class AprsTcpClient : public AprsClient, public sigc::trackable
{
  public:
     AprsTcpClient(LocationInfo::Cfg &loc_cfg, const std::string &server,
                   int port);
     ~AprsTcpClient(void);

     void updateQsoStatus(int action, const std::string& call,
                          const std::string& info,
                          const std::list<std::string>& calls) override;
     void update3rdState(const std::string& call,
                         const std::string& info) override;
     void igateMessage(const std::string& info) override;

  private:
    typedef std::vector<std::string> StrList;

    LocationInfo::Cfg   &loc_cfg;
    std::string		server;
    int			port;
    Async::TcpClient<>* con;
    Async::Timer        *beacon_timer;
    Async::Timer        *reconnect_timer;
    Async::Timer        *offset_timer;

    int			num_connected;

    std::string         recv_buf;

    void  sendMsg(std::string aprsmsg);
    std::string addrStr(const std::string& source) const;
    std::string posStr(const std::string& symbol) const;
    std::string timeStr(void) const;
    std::string phgStr(void) const;
    std::string toneStr(void) const;
    std::string txOffsetStr(void) const;
    std::string frequencyStr(void) const;
    std::string rangeStr(void) const;
    std::string addresseeStr(const std::string& call) const;
    std::string prependSpaceIfNotEmpty(const std::string& str) const;
    void  sendAprsBeacon(Async::Timer *t);

    void  tcpConnected(void);
    void  aprsLogin(void);
    short getPasswd(const std::string& call) const;

    void  decodeAprsPacket(std::string frame);
    int   tcpDataReceived(Async::TcpClient<>::TcpConnection *con, void *buf,
                          int count);
    void  tcpDisconnected(Async::TcpClient<>::TcpConnection *con,
                          Async::TcpClient<>::DisconnectReason reason);
    void  reconnectAprsServer(Async::Timer *t);
    void  startNormalSequence(Async::Timer *t);
    void  disconnect(void);

};  /* class AprsTcpClient */


#endif /* APRS_TCP_CLIENT */

/*
 * This file has not been truncated
 */
