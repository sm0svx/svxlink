/**
@file	 ModuleEchoLink.h
@brief   A module that provides EchoLink connection possibility
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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


#ifndef MODULE_ECHOLINK_INCLUDED
#define MODULE_ECHOLINK_INCLUDED


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

#include <Module.h>
#include <EchoLinkQso.h>
#include <EchoLinkStationData.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "QsoImpl.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Timer;
};
namespace EchoLink
{
  class Directory;
  class StationData;
};


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

class MsgHandler;
class AudioPacer;
class QsoImpl;
  

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
@brief	A module for providing EchoLink connections
@author Tobias Blomberg
@date   2004-03-07
*/
class ModuleEchoLink : public Module
{
  public:
    ModuleEchoLink(void *dl_handle, Logic *logic, const std::string& cfg_name);
    ~ModuleEchoLink(void);
    bool initialize(void);
    void spellCallsign(const std::string& callsign);
    
  private:
    EchoLink::Directory *dir;
    Async::Timer      	*dir_refresh_timer;
    std::string       	mycall;
    std::string       	location;
    std::string       	sysop_name;
    std::string       	description;
    std::string       	allow_ip;
    bool      	      	remote_activation;
    int       	      	pending_connect_id;
    std::string       	last_message;
    QsoImpl		*outgoing_con_pending;
    std::list<QsoImpl*>	qsos;
    unsigned       	max_connections;
    unsigned       	max_qsos;
    QsoImpl   	      	*talker;
    bool      	      	squelch_is_open;

    void moduleCleanup(void);
    const char *name(void) const { return "EchoLink"; }
    void activateInit(void);
    void deactivateCleanup(void);
    void dtmfDigitReceived(char digit);
    void dtmfCmdReceived(const std::string& cmd);
    void squelchOpen(bool is_open);
    int audioFromRx(short *samples, int count);
    void allMsgsWritten(void);

    void onStatusChanged(EchoLink::StationData::Status status);
    void onStationListUpdated(void);
    void onError(const std::string& msg);
    void onIncomingConnection(const Async::IpAddress& ip,
      	    const std::string& callsign, const std::string& name);
    void onChatMsgReceived(QsoImpl *qso, const std::string& msg);
    void onIsReceiving(bool is_receiving, QsoImpl *qso);
    void onDestroyMe(QsoImpl *qso);

    void getDirectoryList(Async::Timer *timer=0);

    void createOutgoingConnection(const EchoLink::StationData *station);
    int audioFromRemote(short *samples, int count, QsoImpl *qso);
    void audioFromRemoteRaw(QsoImpl::GsmVoicePacket *packet, QsoImpl *qso);
    QsoImpl *findFirstTalker(void) const;
    void broadcastTalkerStatus(void);
    void updateDescription(void);

};  /* class ModuleEchoLink */


//} /* namespace */

#endif /* MODULE_ECHOLINK_INCLUDED */



/*
 * This file has not been truncated
 */
