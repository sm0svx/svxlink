/**
@file	 server.h
@brief   Header file or svxserver instance
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2015-08-13

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#ifndef SVXSERVER_INCLUDED
#define SVXSERVER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTcpServer.h>
#include <Tx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "NetTrxMsg.h"


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
@brief	 SvxLinkServer app
@author	 Adi Bier / DL1HRC
@date	 2015-08-13
*/

class SvxServer : public sigc::trackable
{

  public:

    /**
     * @brief 	Default constuctor
     */
    SvxServer(Async::Config &cfg);


    /**
     * @brief 	Default destuctor
     */
    ~SvxServer(void);


  protected:

  private:

    Async::TcpServer<> *server;
    Async::Timer * heartbeat_timer;
    Async::Timer * sql_timer;
    Async::Timer * sql_resettimer;
    Async::Timer * audio_timer;
    Async::TcpConnection *master;

    typedef enum
    {
      STATE_DISC, STATE_VER_WAIT, STATE_AUTH_WAIT, STATE_READY
    } State;

    struct Cons
    {
      Async::TcpConnection *con;
      State   state;
      std::string  callsign;
      struct timeval last_msg;
      struct timeval sent_msg;
      bool    is_rtrx;
      Tx::TxCtrlMode tx_mode;
      bool sql_open;
      int  tg;
      std::string   rxcodec;
      std::string   txcodec;
      char      recv_buf[4096];
      unsigned  recv_cnt;
      unsigned  recv_exp;
      bool  blocked;
    };

    typedef std::map<std::pair<const std::string, uint16_t>, Cons> Clients;
    Clients clients;

    std::string     auth_key;
    NetTrxMsg::MsgAuthChallenge *auth_msg;
    struct timeval l_time;
    int sql_timeout;
    int hbto;
    int sql_resettimeout;

    unsigned char   auth_challenge[NetTrxMsg::MsgAuthChallenge::CHALLENGE_LEN];

    void sendExcept(Async::TcpConnection *con, NetTrxMsg::Msg *msg);
    void clientConnected(Async::TcpConnection *incoming_con);
    void clientDisconnected(Async::TcpConnection *con,
      	      	      	Async::TcpConnection::DisconnectReason reason);
    int tcpDataReceived(Async::TcpConnection *con, void *data, int size);
    void handleMsg(Async::TcpConnection *con, NetTrxMsg::Msg *msg);
    void sendMsg(Async::TcpConnection *con, NetTrxMsg::Msg *msg);
    void hbtimeout(Async::Timer *t);
    void sqltimeout(Async::Timer *t);
    void sqlresettimeout(Async::Timer *t);
    void audiotimeout(Async::Timer *t);
    void resetAll(void);
    bool isMaster(Async::TcpConnection *con);
    void setMaster(Async::TcpConnection *con);
    void resetMaster(Async::TcpConnection *con);
    bool hasMaster(void);

    SvxServer(const SvxServer&);
    SvxServer& operator=(const SvxServer&);

};  /* class SvxServer */

//} /* namespace */

#endif /* SVXSERVER_INCLUDED */


/*
 * This file has not been truncated
 */
