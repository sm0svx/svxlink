/**
@file	 EchoLinkQso.h
@brief   Contains a class for creating an EchoLink connection
@author  Tobias Blomberg
@date	 2003-03-11

This file contains a class for doing an EchoLink Qso. For more information, see
the documentation for class EchoLink::Qso.

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003  Tobias Blomberg / SM0SVX

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

/** @example EchoLinkQso_demo.cpp
An example of how to use the EchoLink::Qso class.

A connection is created to ourself. When the connection has been established
the information message is automatically transmitted. Upon reception of the
information message a disconnect is initiated. When the link has been
disconnected the application exits.
*/


#ifndef ECHOLINK_QSO_INCLUDED
#define ECHOLINK_QSO_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sys/time.h>
#include <sigc++/sigc++.h>
#include <stdint.h>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

extern "C" {
#include <gsm.h>
}
#include <AsyncTimer.h>
#include <AsyncIpAddress.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>


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



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace EchoLink
{

/****************************************************************************
 *
 * Forward declarations inside the declared namespace
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
@brief	A class for creating an EchoLink connection
@author Tobias Blomberg
@date   2003-03-11

This class is used to create a connection to another EchoLink node. It only
handles outgoing connections. To handle incoming connections, have a look at
EchoLink::Dispatcher. However, when an incoming connection has been signalled by
the dispatcher, a Qso object should be created to complete the connection. This
logic should be glued together in the main program.

For an example usage, have a look at the code below. A connection is created to
ourself. When the connection has been established the information message is
automatically transmitted. Upon reception of the information message a
disconnect is initiated. When the link has been disconnected the application
exits.

\include EchoLinkQso_demo.cpp
*/
class Qso
  : public sigc::trackable, public Async::AudioSink, public Async::AudioSource
{
  public:
    struct VoicePacket
    {
      struct {
	uint8_t version;
	uint8_t pt;
	uint16_t seqNum;
	uint32_t time;
	uint32_t ssrc;
      } header;
      uint8_t data[1024];
    } __attribute__ ((packed));
    
    struct RawPacket
    {
      VoicePacket *voice_packet;
      int length;
      short *samples;
    };

    /**
     * @brief The type of the connection state
     */
    typedef enum
    {
      STATE_DISCONNECTED, ///< No connection to the remote station
      STATE_CONNECTING,   ///< Connecting to remote station (not established)
      STATE_BYE_RECEIVED, ///< Received a disconnect request from remote station
      STATE_CONNECTED 	  ///< Connected to remote station
    } State;

    /**
     * @brief 	Constructor
     * @param 	ip The    IP-address of the remote station
     * @param 	callsign  Callsign of local user (not remote callsign)
     * @param 	name  	  Name of local user (not remote name)
     * @param 	info  	  Local information to send upon connect
     */
    Qso(const Async::IpAddress& ip, const std::string& callsign="",
      	const std::string& name="", const std::string& info="");
    
    /**
     * @brief 	Destructor
     */
    ~Qso(void);
  
    /**
     * @brief 	Check that the initialization went ok
     * @return	Returns \em true if the initialization was ok or \em false on
     *	      	failure
     *
     * This function should be called after creating a new Qso object to make
     * sure everything went well.
     */
    bool initOk(void) { return init_ok; }
    
    /**
     * @brief 	Set the local callsign
     * @param 	callsign  The callsign to set
     * @return	Returns \em true on success or \em false on failure
     */
    bool setLocalCallsign(const std::string& callsign);
    
    /**
     * @brief 	Retrieve the local callsign
     * @return	Returns the local callsign
     */
    const std::string& localCallsign(void) const { return callsign; }
    
    /**
     * @brief 	Set the local name (name of station operator)
     * @param 	name  The name to set
     * @return	Returns \em true on success or \em false on failure
     */
    bool setLocalName(const std::string& name);

    /**
     * @brief 	Retrieve the local name
     * @return	Returns the local name
     */
    const std::string& localName(void) const { return name; }
    
    /**
     * @brief 	Set the local info
     * @param 	info  The informational message that is sent to the remote
     *	      	      station upon connection.
     */
    void setLocalInfo(const std::string& info);
        
    /**
     * @brief 	Retrieve the local station info
     * @return	Returns the local station info
     */
    const std::string& localInfo(void) const { return local_stn_info; }
    
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
     * @brief 	Initiate a disconnection from the remote station
     * @return	Returns \em true if the disconnection message was sent
     *	      	successfully or \em false on failure
     */
    bool disconnect(void);
    
    /**
     * @brief 	Send info data to the remote station
     * @param 	info The info to send
     * @return	Returns \em true on success or \em false on failure
     */
    bool sendInfoData(const std::string& info="");
    
    /**
     * @brief 	Send chat data to the remote station
     * @param 	msg The message to send
     * @return	Returns \em true on success or \em false on failure
     */
    bool sendChatData(const std::string& msg);
    
    /**
     * @brief 	Get the IP address of the remote station
     * @return	Returns the IP address
     */
    const Async::IpAddress& remoteIp(void) const
    {
      return remote_ip;
    }
    
    /**
     * @brief 	Send a GSM/SPEEX audio packet to the remote station
     * @param 	raw_packet The packet to send
     *
     * This function can be used to send a GSM/SPEEX packet to the remote
     * station. Probably only useful if you received it from the
     * audioReceivedRaw signal. The raw_packet contains both the network
     * packet and the decoded samples, which is beneficial when the audio
     * frame has to be transcoded (SPEEX -> GSM) prior re-transmission.
     */
    bool sendAudioRaw(RawPacket *raw_packet);

    /**
      * @brief Set parameters of the remote station connection
      * @param priv A private string for passing connection parameters
      */
    void setRemoteParams(const std::string& priv);
    
    /**
     * @brief Set the name of the remote station
     * @param name The name to set
     */
    void setRemoteName(const std::string& name) { remote_name = name; }
    
    /**
     * @brief 	Get the remote name
     * @return	Return the name of the remote station
     * @note  	Valid when the connection has been established
     */
    const std::string& remoteName(void) const { return remote_name; }
    
    /**
     * @brief Set the callsign of the remote station
     * @param call The callsign to set
     */
    void setRemoteCallsign(const std::string& call) { remote_call = call; }
    
    /**
     * @brief 	Get the remote callsign
     * @return	Return the callsign of the remote station
     * @note  	Valid when the connection has been established
     */
    const std::string& remoteCallsign(void) const { return remote_call; }
    
    /**
     * @brief 	Find out if the connection is remotely initiated or
     *          locally initiated.
     * @return	Return \em true if the connection is remotely initiated
     *          or else \em false.
     * @note  	Valid when either connect or accept has been called
     */
    bool isRemoteInitiated(void) const { return is_remote_initiated; }
    
    /**
     * @brief 	Find out if there is audio coming in on this connection
     * @return	Return \em true if audio is being received
     *          or else \em false.
     */
    bool receivingAudio(void) const { return receiving_audio; }

    /**
     * @brief 	Get the current state of the connection
     * @return	Return the current connection state (@see State)
     */
    State currentState(void) const { return state; }
    
    /**
     * @brief A signal that is emitted when a station info message is received
     * @param msg The received message
     */
    sigc::signal<void, const std::string&> infoMsgReceived;
    
    /**
     * @brief A signal that is emitted when a chat message is received
     * @param msg The received chat message
     */
    sigc::signal<void, const std::string&> chatMsgReceived;
    
    /**
     * @brief A signal that is emitted when the connection state changes
     * @param state The new connection state
     */
    sigc::signal<void, State> stateChange;
    
    /**
     * @brief A signal that is emitted when the audio receive state changes
     * @param is_receiving  Is \em true when audio is being received and
     *	      	      	    \em false when not
     * @note This signal can be used to control a reception indicator
     */
    sigc::signal<void, bool> isReceiving;
    
    /**
     * @brief A signal that is emitted when an audio datagram has been received
     * @param data A pointer to the buffer that contains the raw audio packet
     *
     * This signal is emitted whenever an audio packet has been received on
     * the connection. It gives access to the GSM/SPEEX packet. This can be used
     * if the encoded data is going to be retransmitted. In this case it is
     * not good to decode and then encode the data again. It will sound awful.
     */
    sigc::signal<void, RawPacket*>  audioReceivedRaw;
    

    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples could be written.
     * If the returned number of written samples is lower than the count
     * parameter value, the sink is not ready to accept more samples.
     * In this case, the audio source requires sample buffering to temporarily
     * store samples that are not immediately accepted by the sink.
     * The writeSamples function should be called on source buffer updates
     * and after a source output request has been received through the
     * requestSamples function.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);

    /**
     * @brief Resume audio output to the sink
     * 
     * This function will be called when the registered audio sink is ready
     * to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void resumeOutput(void);
    
    /**
     * @brief Setting GSM as only codec for echolink connections
     *
     * This function can be called when SvxLink is installed and running
     * on a system with smaller cpu to keep the load low.
     */
    void setUseGsmOnly(void);

  protected:
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink. If it is not reimplemented, a handler must be set
     * that handle the function call.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void);

    
  private:
    struct Private;

    static const int  	KEEP_ALIVE_TIME       	= 10000;
    static const int  	MAX_CONNECT_RETRY_CNT 	= 5;
    static const int  	CON_TIMEOUT_TIME      	= 50000;
    static const int  	RX_INDICATOR_POLL_TIME  = 100;  // 10 times/s
    static const int  	RX_INDICATOR_SLACK      = 100;  // 100ms extra time
    static const int  	RX_INDICATOR_MAX_TIME   = 1000; // Max 1s timeout
    static const int    FRAME_COUNT             = 4;
    static const int  	BUFFER_SIZE      	= FRAME_COUNT*160;
    static const int    BLOCK_TIME              = FRAME_COUNT*1000*160/8000;

    bool      	      	init_ok;
    unsigned char      	sdes_packet[1500];
    int       	      	sdes_length;
    State     	      	state;
    gsm       	      	gsmh;
    uint16_t    	next_audio_seq;
    Async::Timer *	keep_alive_timer;
    int       	      	connect_retry_cnt;
    Async::Timer *      con_timeout_timer;
    std::string    	callsign;
    std::string    	name;
    std::string    	local_stn_info;
    short		receive_buffer[BUFFER_SIZE];
    short		send_buffer[BUFFER_SIZE];
    int       	      	send_buffer_cnt;
    Async::IpAddress  	remote_ip;
    Async::Timer *    	rx_indicator_timer;
    std::string       	remote_name;
    std::string       	remote_call;
    bool		is_remote_initiated;
    bool      	      	receiving_audio;
    bool                use_gsm_only;
    Private             *p;
    int                 rx_timeout_left;

    Qso(const Qso&);
    Qso& operator=(const Qso&);
    void printData(const unsigned char *buf, int len);
    void handleCtrlInput(unsigned char *buf, int len);
    inline void handleByePacket(unsigned char *buf, int len);
    inline void handleSdesPacket(unsigned char *buf, int len);
    void handleAudioInput(unsigned char *buf, int len);
    inline void handleNonAudioPacket(unsigned char *buf, int len);
    inline void handleAudioPacket(unsigned char *buf, int len);
    void micAudioRead(void *buf, size_t len);
    bool sendSdesPacket(void);
    void sendKeepAlive(Async::Timer *timer);
    void setState(State state);
    void connectionTimeout(Async::Timer *timer);
    bool setupConnection(void);
    void cleanupConnection(void);
    bool sendVoicePacket(void);
    void checkRxActivity(Async::Timer *timer);
    bool sendByePacket(void);
    
};  /* class Qso */


} /* namespace */

#endif /* ECHOLINK_QSO_INCLUDED */



/*
 * This file has not been truncated
 */

