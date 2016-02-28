/**
@file	 ModuleEchoLink.h
@brief   A module that provides EchoLink connection possibility
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2014 Tobias Blomberg / SM0SVX

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
#include <vector>

#include <sys/types.h>
#include <regex.h>


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

#include "version/SVXLINK.h"



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

namespace Async
{
  class Timer;
  class AudioSplitter;
  class AudioValve;
  class AudioSelector;
  class Pty;
};
namespace EchoLink
{
  class Directory;
  class StationData;
  class Proxy;
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
class QsoImpl;
class LocationInfo;
  

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
    const char *compiledForVersion(void) const { return SVXLINK_VERSION; }

    
  protected:
    /**
     * @brief 	Notify the module that the logic core idle state has changed
     * @param 	is_idle Set to \em true if the logic core is idle or else
     *	      	\em false.
     *
     * This function is called by the logic core when the idle state changes.
     */
    virtual void logicIdleStateChanged(bool is_idle);


  private:
    typedef enum
    {
      STATE_NORMAL,
      STATE_CONNECT_BY_CALL,
      STATE_DISCONNECT_BY_CALL
    } State;
    typedef std::vector<EchoLink::StationData> StnList;
    struct NumConStn
    {
      unsigned        num_con;
      struct timeval  last_con;

      NumConStn(unsigned num, struct timeval &t) : num_con(num), last_con(t) {}
    };
    typedef std::map<const std::string, NumConStn> NumConMap;

    static const int	  DEFAULT_AUTOCON_TIME = 3*60*1000; // Three minutes

    EchoLink::Directory   *dir;
    Async::Timer      	  *dir_refresh_timer;
    std::string       	  mycall;
    std::string       	  location;
    std::string       	  sysop_name;
    std::string       	  description;
    std::string       	  allow_ip;
    bool      	      	  remote_activation;
    int       	      	  pending_connect_id;
    std::string       	  last_message;
    std::vector<QsoImpl*> outgoing_con_pending;
    std::vector<QsoImpl*> qsos;
    unsigned       	  max_connections;
    unsigned       	  max_qsos;
    QsoImpl   	      	  *talker;
    bool      	      	  squelch_is_open;
    State		  state;
    StnList		  cbc_stns;
    Async::Timer          *cbc_timer;
    Async::Timer	  *dbc_timer;
    regex_t   	      	  *drop_incoming_regex;
    regex_t   	      	  *reject_incoming_regex;
    regex_t   	      	  *accept_incoming_regex;
    regex_t   	      	  *reject_outgoing_regex;
    regex_t   	      	  *accept_outgoing_regex;
    EchoLink::StationData last_disc_stn;
    Async::AudioSplitter  *splitter;
    Async::AudioValve 	  *listen_only_valve;
    Async::AudioSelector  *selector;
    unsigned              num_con_max;
    time_t                num_con_ttl;
    time_t                num_con_block_time;
    NumConMap             num_con_map;
    Async::Timer          *num_con_update_timer;
    bool		  reject_conf;
    int   	      	  autocon_echolink_id;
    int   	      	  autocon_time;
    Async::Timer	  *autocon_timer;
    EchoLink::Proxy       *proxy;

    void moduleCleanup(void);
    void activateInit(void);
    void deactivateCleanup(void);
    //bool dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceived(const std::string& cmd);
    void dtmfCmdReceivedWhenIdle(const std::string &cmd);
    void squelchOpen(bool is_open);
    int audioFromRx(float *samples, int count);
    void allMsgsWritten(void);
    void commandHandler(const void *buf, size_t count); // WIM
    Async::Pty                      *pty;

    void onStatusChanged(EchoLink::StationData::Status status);
    void onStationListUpdated(void);
    void onError(const std::string& msg);
    void onIncomingConnection(const Async::IpAddress& ip,
      	    const std::string& callsign, const std::string& name,
      	    const std::string& priv);
    void onStateChange(QsoImpl *qso, EchoLink::Qso::State qso_state);
    void onChatMsgReceived(QsoImpl *qso, const std::string& msg);
    void onIsReceiving(bool is_receiving, QsoImpl *qso);
    void destroyQsoObject(QsoImpl *qso);

    void getDirectoryList(Async::Timer *timer=0);

    void createOutgoingConnection(const EchoLink::StationData &station);
    int audioFromRemote(float *samples, int count, QsoImpl *qso);
    void audioFromRemoteRaw(EchoLink::Qso::RawPacket *packet,
      	      	      	    QsoImpl *qso);
    QsoImpl *findFirstTalker(void) const;
    void broadcastTalkerStatus(void);
    void updateDescription(void);
    void updateEventVariables(void);
    void connectByCallsign(std::string cmd);
    void handleConnectByCall(const std::string& cmd);
    void cbcTimeout(Async::Timer *t);
    void disconnectByCallsign(const std::string &cmd);
    void handleDisconnectByCall(const std::string& cmd);
    void dbcTimeout(Async::Timer *t);
    int numConnectedStations(void);
    int listQsoCallsigns(std::list<std::string>& call_list);
    void handleCommand(const std::string& cmd);
    void commandFailed(const std::string& cmd);
    void connectByNodeId(int node_id);
    void checkIdle(void);
    void checkAutoCon(Async::Timer *timer=0);
    bool numConCheck(const std::string &callsign);
    void numConUpdate(void);
    void replaceAll(std::string &str, const std::string &from,
                    const std::string &to) const;

};  /* class ModuleEchoLink */


//} /* namespace */

#endif /* MODULE_ECHOLINK_INCLUDED */



/*
 * This file has not been truncated
 */
