/**
@file	 AprsTcpClient.h
@brief   Contains an implementation of APRS updates via TCP
@author  Adi Bier / DL1HRC
@date	 2008-11-01

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Config;
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
class AprsTcpClient : public SigC::Object 
{
  public:
     static AprsTcpClient *selectAprsServer(void);

     AprsTcpClient(Async::Config &cfg, const std::string &name);
     ~AprsTcpClient(void);

     void  sendMsg(const char *aprsmsg);
     void  sendAprsBeacon(Async::Timer *t);
     void  sendAprsInfo(std::string info, int numberofstations);
     
  protected:

  private:
    static const int RECV_BUF_SIZE = 4096;
    
    int    nrofstn;
    size_t recv_buf_len;

    Async::Config     	&cfg;
    Async::TcpClient 	*con;
    Async::Timer        *beacon_timer;
    Async::Timer        *reconnect_timer;
    Async::Timer        *offset_timer;

    int                 beaconinterval;
    int                 timeroffset;
    int			ctcss;
    int			tcp_port;

    std::string 	frequency;

    std::string       	name;
    std::string         aprshost;
    std::string         aprspath;
    std::string       	tone;
    std::string       	rng;
    std::string         phgd; 
    std::string         el_call;
    std::string         info;
    std::string         aprslon;
    std::string         aprslat;
    std::string 	destination; 
    std::string         aprs_comment;
    std::string		el_tok;
    

    void  tcpConnected(void);
    void  aprsLogin(void);
    short callpass(const char *theCall);
    int   tcpDataReceived(Async::TcpClient::TcpConnection *con, void *buf,
                          int count);
    void  tcpDisconnected(Async::TcpClient::TcpConnection *con,
                          Async::TcpClient::DisconnectReason reason);
    void  reconnectNextAprsServer(Async::Timer *t);
    void  startNormalSequence(Async::Timer *t);

};  /* class AprsTcpClient */


#endif /* APRS_TCP_CLIENT */

/*
 * This file has not been truncated
 */

