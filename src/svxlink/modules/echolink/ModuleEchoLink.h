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
namespace EchoLink
{
  class Directory;
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
class AudioPaser;
  

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
    ModuleEchoLink(void *dl_handle, Logic *logic, int id,
      	      	   const std::string& cfg_name);
    ~ModuleEchoLink(void);

  private:
    EchoLink::Directory *dir;
    EchoLink::Qso     	*qso;
    Async::Timer      	*dir_refresh_timer;
    std::string       	callsign;
    std::string       	sysop_name;
    std::string       	description;
    std::string       	allow_ip;
    MsgHandler	      	*msg_handler;
    AudioPaser       	*msg_paser;
    
    const char *name(void) const { return "EchoLink"; }
    void activateInit(void);
    void deactivateCleanup(void);
    void dtmfDigitReceived(char digit);
    void dtmfCmdReceived(const std::string& cmd);
    void playHelpMsg(void);
    void squelchOpen(bool is_open);
    int audioFromRx(short *samples, int count);

    void onStatusChanged(EchoLink::StationData::Status status);
    void onStationListUpdated(void);
    void onError(const std::string& msg);
    void onIncomingConnection(const Async::IpAddress& ip,
      	    const std::string& callsign, const std::string& name);
    void onInfoMsgReceived(const std::string& msg);
    void onStateChange(EchoLink::Qso::State state);
    void onIsReceiving(bool is_receiving);
    void onAudioReceived(short *samples, int count);

    void getDirectoryList(Async::Timer *timer);
    void spellCallsign(const std::string& callsign);

    void allMsgsWritten(void);

};  /* class ModuleEchoLink */


//} /* namespace */

#endif /* MODULE_ECHOLINK_INCLUDED */



/*
 * This file has not been truncated
 */
