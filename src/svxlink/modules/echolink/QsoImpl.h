/**
@file	 QsoImpl.h
@brief   Data for one EchoLink Qso.
@author  Tobias Blomberg / SM0SVX
@date	 2004-06-02

This file contains a class that implementes the things needed for one
EchoLink Qso.

\verbatim
A module (plugin) for the multi purpose tranciever frontend system.
Copyright (C) 2004-2025 Tobias Blomberg / SM0SVX

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

#ifndef QSO_IMPL_INCLUDED
#define QSO_IMPL_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>
#include <EchoLinkQso.h>
#include <EchoLinkStationData.h>


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
  class AudioPacer;
  class AudioPassthrough;
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
class EventHandler;
class AsyncTimer;
class ModuleEchoLink;


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
@brief	Implements the things needed for one EchoLink QSO.
@author Tobias Blomberg
@date   2004-06-02

A class that implementes the things needed for one EchoLink Qso.
*/
class QsoImpl
  : public Async::AudioSink, public Async::AudioSource, public sigc::trackable
{
  public:
    /**
     * @brief 	Default constuctor
     */
    QsoImpl(const EchoLink::StationData &station, ModuleEchoLink *module);
  
    /**
     * @brief 	Destructor
     */
    virtual ~QsoImpl(void);
  
    /**
     * @brief 	Check that the initialization went ok
     * @return	Returns \em true if the initialization was ok or \em false on
     *	      	failure
     *
     * This function should be called after creating a new Qso object to make
     * sure everything went well.
     */
    bool initOk(void);
    
    /**
     * @brief 	Called by the module core when the logic core idle state
     *	      	changes
     * @param 	is_idle Set to \em true if the logic core is idle or else
     *	      	\em false
     */
    void logicIdleStateChanged(bool is_idle);
    
    /**
     * @brief 	Send a raw GSM audio packet to the remote station
     * @param 	packet The packet to send
     *
     * This function can be used to send a raw GSM packet to the remote
     * station. Probably only useful if you received it from the
     * audioReceivedRaw signal.
     */
    bool sendAudioRaw(EchoLink::Qso::RawPacket *packet);
    
    /**
     * @brief 	Initiate a connection to the remote station
     * @return	Returns \em true if the connect message was sent ok or \em false
     *        	on failure
     *
     * Use this function to connect to the remote station. The \em StateChange
     * signal will be emitted to indicate that a connection is in progress.
     * When the connection has been established, the \em stateChange signal
     * will be emitted again. On failure to connect, the \em stateChange
     * signal will be emitted to indicate that the disconnected state
     * has been entered again.
     */
    bool connect(void);
    
    /**
     * @brief 	Accept an incoming connection
     * @return	Returns \em true if the connect message was sent successfully or
     *	      	\em false on failure
     *
     * Use this function to accept an incoming connection. Incoming connections
     * are signalled through the EchoLink::Dispatcher. When an incoming
     * connection has been received, a Qso object should be created and this
     * function should be called to accept the connection. Be sure to check that
     * a valid callsign has connected. At least if the EchoLink node is
     * connected to a radio transmitter.
     *
     * The difference between the connect and accept functions are that the
     * accept function goes right into the connected state. The remote station
     * is assumed to be present. This might not be true in some strange cases.
     * In such a strange case, the connection will timeout after a while.
     */
    bool accept(void);
    
    /**
     * @brief 	Reject the connection
     * @param 	perm If \em true the rejection is permanent. If \em false
     *               the rejection is temporary.
     */
    void reject(bool perm);
    
    bool disconnect(void) { return m_qso.disconnect(); }

    bool sendInfoData(const std::string& info="")
    {
      return m_qso.sendInfoData(info);
    }
    
    EchoLink::Qso::State currentState(void) const
    {
      return m_qso.currentState();
    }

    void setRemoteParams(const std::string& priv) { m_qso.setRemoteParams(priv); }

    void setRemoteName(const std::string& name) { m_qso.setRemoteName(name); }

    const std::string& remoteName(void) const { return m_qso.remoteName(); }
    
    void setRemoteCallsign(const std::string& call)
    {
      m_qso.setRemoteCallsign(call);
    }
    
    const std::string& remoteCallsign(void) const
    {
      return m_qso.remoteCallsign();
    }
    
    bool sendChatData(const std::string& msg)
    {
      return m_qso.sendChatData(msg);
    }

    bool receivingAudio(void) const { return m_qso.receivingAudio(); }


    bool connectionRejected(void) const { return reject_qso; }
    
    
    /**
     * @brief 	Return the StationData object associated with this QSO
     * @return	Returns a reference to the associated StationData object
     */
    const EchoLink::StationData& stationData(void) const { return station; }

    /**
     * @brief Tell the QSO object if "listen only" is enabled or not
     * @param enable Set to \em true if enabled or \em false otherwise
     *
     * This function is used to inform the QSO object if "listen only" is
     * enabled or not. It does not block the audio. This have to be done
     * outside the QSO object. It only affects info data sent to the remote
     * station.
     */
    void setListenOnly(bool enable);

    /**
     * @brief   Called when the squelch state changes
     * @param   is_open \em True if the squelch is open or else \em false
     */
    void squelchOpen(bool is_open);

    /**
     * @brief A signal that is emitted when the connection state changes
     * @param qso The QSO object
     * @param state The new connection state
     */
    sigc::signal<void, QsoImpl*, EchoLink::Qso::State> stateChange;
    
    /**
     * @brief A signal that is emitted when a chat message is received
     * @param qso The QSO object
     * @param msg The received chat message
     */
    sigc::signal<void, QsoImpl*, const std::string&> chatMsgReceived;
    
    /**
     * @brief A signal that is emitted when an info message is received
     * @param qso The QSO object
     * @param msg The received info message
     */
    sigc::signal<void, QsoImpl*, const std::string&> infoMsgReceived;

    /**
     * @brief A signal that is emitted when the audio receive state changes
     * @param is_receiving  Is \em true when audio is being received and
     *                      \em false when not
     * @param qso The QSO object
     * @note This signal can be used to control a reception indicator
     */
    sigc::signal<void, bool, QsoImpl*> isReceiving;
    
    /**
     * @brief A signal that is emitted when an audio datagram has been received
     * @param packet A pointer to the buffer that contains the raw GSM audio
     * @param qso The QSO object
     */
    sigc::signal<void, EchoLink::Qso::RawPacket*, QsoImpl*> audioReceivedRaw;
    
    /**
     * @brief 	A signal that is emitted when the qso object should be destroyed
     * @param 	qso The QSO object
     */
    sigc::signal<void, QsoImpl*> destroyMe;
    
        
  protected:
    
  private:
    EchoLink::Qso     	    m_qso;
    ModuleEchoLink    	    *module;
    EventHandler      	    *event_handler;
    MsgHandler	      	    *msg_handler;
    Async::AudioSelector    *output_sel;
    bool      	      	    init_ok;
    bool      	      	    reject_qso;
    std::string       	    last_message;
    std::string       	    last_info_msg;
    Async::Timer      	    *idle_timer;
    bool      	      	    disc_when_done;
    int       	      	    idle_timer_cnt;
    int       	      	    idle_timeout;
    Async::Timer	    *destroy_timer;
    EchoLink::StationData   station;
    Async::AudioPassthrough *sink_handler;
    std::string             sysop_name;
    bool                    logic_is_idle;
    
    void allRemoteMsgsWritten(void);
    void onInfoMsgReceived(const std::string& msg);
    void onChatMsgReceived(const std::string& msg);
    void onStateChange(EchoLink::Qso::State state);
    void idleTimeoutCheck(Async::Timer *t);
    void destroyMeNow(Async::Timer *t);
    bool getConfigValue(const std::string& section, const std::string& tag,
                        std::string& value);
    void processEvent(const std::string& event);

};  /* class QsoImpl */


//} /* namespace */

#endif /* QSO_IMPL_INCLUDED */



/*
 * This file has not been truncated
 */

