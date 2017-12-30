/**
@file    QsoFrn.h
@brief   Free Radio Network (FRN) QSO module
@author  sh123
@date    2014-12-30

This file contains a class that implementes the things needed for one
Frn Qso.

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

#ifndef QSO_FRN_INCLUDED
#define QSO_FRN_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/
#include <string>
#include <vector>
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/
extern "C" {
#include <gsm.h>
}

#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>
#include <AsyncTcpClient.h>
#include <CppStdCompat.h>


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
  class AudioFifo;
  class AudioPassthrough;
  class TcpConnection;
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
class AudioFifo;
class ModuleFrn;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/
typedef std::vector<std::string> FrnList;


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
@brief	Free Radio Network (FRN) QSO module
@author sh123
@date   2014-12-30

A class that implementes the things needed for FRN QSO.
*/
class QsoFrn
  : public Async::AudioSink, public Async::AudioSource, public sigc::trackable
{
  public:
    typedef enum {
      STATE_ERROR,
      STATE_DISCONNECTED,
      STATE_CONNECTING,
      STATE_CONNECTED,
      STATE_LOGGING_IN_1,
      STATE_LOGGING_IN_2,
      STATE_IDLE,
      STATE_TX_AUDIO_WAITING,
      STATE_TX_AUDIO_APPROVED,
      STATE_TX_AUDIO,
      STATE_RX_AUDIO,
      STATE_RX_CLIENT_LIST_HEADER,
      STATE_RX_CLIENT_LIST,
      STATE_RX_LIST
    } State;

    typedef enum {
      DT_IDLE = 0,
      DT_DO_TX,
      DT_VOICE_BUFFER,
      DT_CLIENT_LIST,
      DT_TEXT_MESSAGE,
      DT_NET_NAMES,
      DT_ADMIN_LIST,
      DT_ACCESS_LIST,
      DT_BLOCK_LIST,
      DT_MUTE_LIST,
      DT_ACCESS_MODE
    } Response;

    typedef enum {
      RQ_RX0,
      RQ_TX0,
      RQ_TX1,
      RQ_P
    } Request;

    /**
     * @brief   Default constuctor
     */
    QsoFrn(ModuleFrn *module);

    /**
     * @brief   Destructor
     */
    virtual ~QsoFrn(void);

    /**
     * @brief Call to check if QSO was ctor-ed succesfully
     *
     * @return true if QSO was constructed correctly
     *
     * Call after qso contruction to make sure that it was ctor-ed correctly.
     */
    bool initOk(void);

    /**
     * @brief Method to put QSO to live
     */
    void connect(bool is_backup=false);

    /**
     * @brief Disconnect QSO from server, put it to disconnected state
     */
    void disconnect(void);

    /**
     * @brief Called by FRN module to notify that squelch is opened
     *
     * @param true if squelch is open or false if closed
     */
    void squelchOpen(bool is_open);

    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
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
      * @brief   Tell the sink to flush the previously written samples
      *
      * This function is used to tell the sink to flush previously written
      * samples. When done flushing, the sink should call the
      * sourceAllSamplesFlushed function.
      * This function is normally only called from a connected source object.
      */
     virtual void flushSamples(void);

     /**
      * @brief Return true if qso is connected to the server
      *
      * @return true if qso is connected
      */
     bool isConnected() const { return state >= STATE_IDLE; }

     /**
      * @brief return number of clients on the channel
      *
      * @return count of clients on the channel including myself
      */
     int clientsCount() const { return client_list.size(); }

     /**
      * @brief Resume audio output to the sink
      *
      * This function will be called when the registered audio sink is ready
      * to accept more samples.
      * This function is normally only called from a connected sink object.
      */
     virtual void resumeOutput(void);

     /**
      * @brief Do not send anything to rig
      *
      * @param true to disable audio stream to rig
      */
     void setRfDisabled(bool disabled) { is_rf_disabled = disabled; }

     /**
      * @brief Return true if rf tx disabled for the qso
      *
      * @return true if rf tx is disabled
      */
     bool isRfDisabled() const { return is_rf_disabled; }

     /**
      * @brief  QSO is erroring out and cannot recover itself
      */
     sigc::signal<void> error;

     /**
      * @brief QSO internal state has been changed
      * @param QSO state enum value
      */
     sigc::signal<void, State> stateChange;

 protected:
     /**
      * @brief The registered sink has flushed all samples
      *
      * This function will be called when all samples have been flushed in the
      *
      * registered sink. If it is not reimplemented, a handler must be set
      * that handle the function call.
      * This function is normally only called from a connected sink object.
      */
     virtual void allSamplesFlushed(void);

    /**
     * @brief Converts QSO state enum to string for debugging
     * @param QSO state enum
     * @return String representation of the enum value
     *
     * This function is used to convert QSO state enum to readable
     * string for debugging purposes
     */
    std::string stateToString(State state);

    /**
     * @brief Called internally to set new QSO state
     *
     * @param New state to set
     */
    void setState(State newState);

    /**
     * @brief Emitted when FRN list is fully populated
     *
     * @param vector of strings populated from FRN
     */
    sigc::signal<void, const FrnList& > frnListReceived;

    /**
     * @brief Emitted when FRN client list is fully populated
     *
     * @param vector of strings populated from FRN
     */
    sigc::signal<void, const FrnList& > frnClientListReceived;

    /**
     * @brief Emitted when started receiving voice from client
     *
     * @param string with client xml data
     */
    sigc::signal<void, const std::string& > rxVoiceStarted;

     /**
     * @brief Called when started receiving voice from the client
     *
     * @param xml FRN format client descrition
     */
    void onRxVoiceStarted(const std::string &client_description) const;

    /**
     * @brief Called when completed FRN list is received
     *
     * @param xml FRN format client descrition line
     */
    void onFrnListReceived(const FrnList &list) const;

    /**
     * @brief Called when completed FRN client list is received
     *
     * @param xml FRN format client descrition line
     */
    void onFrnClientListReceived(const FrnList &list);

  private:

    /**
     * @brief Initiate connection to FRN server when connection was lost
     *
     * Starts new connection retry. If maximum retry count is reached sets
     * QSO state to error.
     */
    void reconnect();

    /**
     * @brief Starts initial FRN server login process
     *
     * Sends connection string with credentials and other required information
     * to the FRN server, sets proper QSO state.
     */
    void login(void);

    /**
     * @brief Sends pcm raw frames to FRN server
     *
     * @param Pcm buffer with samples
     * @param Size of buffer
     */
    void sendVoiceData(short *data, int len);

    /**
     * @brief Sends FRN client request to the server
     *
     * @param Request to send out
     *
     * This method is used to send FRN control request.
     */
    void sendRequest(Request rq);

    /**
     * @brief Consumed and handles incoming data when in loging in stage
     *
     * @param pointer to received data
     * @param length of incoming data
     * @param true if logging in stage 1, else stage 2
     * @return count of bytes consumed
     *
     * Handles incoming data when in logging in stage 1 or 2,
     * switches to next state if all required data was consumed.
     */
    int handleLogin(unsigned char *data, int len, bool stage_one);

    /**
     * @brief Consume and handle one incoming FRN command
     * @param pointer to received data
     * @param length of received data
     * @return count of bytes consumed
     *
     * Consumes and handles one incoming FRN command and sets
     * internal QSO state accordingly.
     */
    int handleCommand(unsigned char *data, int len);

    /**
     * @brief Consume and handle one incoming audio frame
     *
     * @param pointer to received data
     * @param length of received data
     * @return count of bytes consumed
     *
     * Consumes and handles one incoming FRN audio frame. Decodes 
     * from gsm wav49 format and sends to audio sink.
     */
    int handleAudioData(unsigned char *data, int len);

    /**
     * @brief Consume and handle FRN incoming string lists
     *
     * @param pointer to received data
     * @param length of received data
     * @return count of bytes consumed
     *
     * Method is used to consume and handle FRN incoming list
     * header
     */
    int handleListHeader(unsigned char *data, int len);

    /**
     * @brief Consume and handle FRN incoming string lists
     *
     * @param pointer to received data
     * @param length of received data
     * @return count of bytes consumed
     *
     * Method is used to consume and handle FRN incoming lists, for
     * example list of users, chat messages, etc.
     */
    int handleList(unsigned char *data, int len);

    /**
     * @brief Called when connection to FRN server is established
     *
     * Called when connection to FRN server is established, updates
     * internal state, starts timeout timer.
     */
    void onConnected(void);

    /**
     * @brief Called when connection to FRN server is lost
     *
     * Called when connection to FRN server is lost, based on
     * disconnect reason sets the internal QSO state and start
     * reconnection if needed.
     */
    void onDisconnected(Async::TcpConnection *conn,
        Async::TcpConnection::DisconnectReason reason);

    /**
     * @brief Called when new data is received from FRN server
     *
     * Called when new data is received from FRN server, calls proper
     * handler based on the current state.
     */
    int onDataReceived(Async::TcpConnection *con, void *data, int len);

    /**
     * @brief Called when FRN connection buffer is full
     */
    void onSendBufferFull(bool is_full);

    /**
     * @brief Called when no pings were received during timeout time
     *
     * Initiates reconnection when no pings/data was received from
     * the FRN serer during timeout time.
     */
    void onConnectTimeout(Async::Timer *timer);

    /**
     * @brief Called when no voice data was received during timeout time
     *
     * Called when no more voice data is received during timeout time,
     * which is treated as end of voice data receive state.
     */
    void onRxTimeout(Async::Timer *timer);

    /**
     * @brief Called when wait for tx timer timeouts
     *
     * Called when server does not respond withing timeout time
     * to TX request
     */
    void onTxWaitTimeout(Async::Timer *timer);

    /**
     * @brief Called when no pings/data were received during timeout time
     *
     * Called when no pings and data were received during timeout time
     */
    void onKeepaliveTimeout(Async::Timer *timer);

    /**
     * @brief Delayed reconnect timer callback
     *
     * Called to start delayed reconnection to FRN server
     */
    void onDelayedReconnect(Async::Timer *timer);

  private:
    static const int        CLIENT_INDEX_SIZE       = 2;
    static const int        TCP_BUFFER_SIZE         = 65536;
    static const int        FRAME_COUNT             = 5;
    static const int        PCM_FRAME_SIZE          = 160*2;  // WAV49 has 2x
    static const int        GSM_FRAME_SIZE          = 65;     // WAV49 has 65
    static const int        BUFFER_SIZE             = FRAME_COUNT*PCM_FRAME_SIZE;
    static const int        FRN_AUDIO_PACKET_SIZE   = FRAME_COUNT*GSM_FRAME_SIZE;

    static const int        CON_TIMEOUT_TIME        = 30000;
    static const int        RX_TIMEOUT_TIME         = 1000;
    static const int        KEEPALIVE_TIMEOUT_TIME  = 5000;

    static const int        MAX_CONNECT_RETRY_CNT   = 10;
    static const int        RECONNECT_TIMEOUT_TIME  = 5*1000;
    static const int        RECONNECT_MAX_TIMEOUT   = 2*60*1000;
    static CONSTEXPR float  RECONNECT_BACKOFF       = 1.2f;

    bool                init_ok;

    Async::TcpClient<>* tcp_client;
    Async::Timer *      rx_timeout_timer;
    Async::Timer *      con_timeout_timer;
    Async::Timer *      keepalive_timer;
    Async::Timer *      reconnect_timer;
    State               state;
    int                 connect_retry_cnt;
    short               receive_buffer[BUFFER_SIZE];
    short               send_buffer[BUFFER_SIZE];
    int                 send_buffer_cnt;
    gsm                 gsmh;
    int                 lines_to_read;
    FrnList             cur_item_list;
    FrnList             client_list;
    bool                is_receiving_voice;
    bool                is_rf_disabled;
    int                 reconnect_timeout_ms;
    std::string         server;
    std::string         port;

    bool                opt_frn_debug;
    std::string         opt_server;
    std::string         opt_port;
    std::string         opt_server_backup;
    std::string         opt_port_backup;
    std::string         opt_version;
    std::string         opt_email_address;
    std::string         opt_dyn_password;
    std::string         opt_callsign_and_user;
    std::string         opt_client_type;
    std::string         opt_band_and_channel;
    std::string         opt_description;
    std::string         opt_country;
    std::string         opt_city_city_part;
    std::string         opt_net;
};


//} /* namespace */

#endif /* QSO_FRN_INCLUDED */


/*
 * This file has not been truncated
 */

