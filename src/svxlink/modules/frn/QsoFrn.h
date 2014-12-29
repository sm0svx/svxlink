/**
@file	 QsoFrn.h
@brief   Data for Frn Qso.
@author  Tobias Blomberg / SM0SVX
@date	 2004-06-02

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
  class Config;
  class AudioPacer;
  class AudioFifo;
  class AudioPassthrough;
  class TcpClient;
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
@brief	Implements the things needed for Frn QSO.
@author Tobias Blomberg
@date   2004-06-02

A class that implementes the things needed for Frn Qso.
*/
class QsoFrn
  : public Async::AudioSink, public Async::AudioSource, public sigc::trackable
{
  public:
    typedef enum {
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
      STATE_RX_LIST_HEADER,
      STATE_RX_LIST,
      STATE_ERROR
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
     * @brief 	Default constuctor
     */
    QsoFrn(ModuleFrn *module);
  
    /**
     * @brief 	Destructor
     */
    virtual ~QsoFrn(void);
     
    bool initOk(void);

    void connect(void);

    void disconnect(void);

    void squelchOpen(bool is_open);

    std::string stateToString(State state);

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
      * @brief Resume audio output to the sink
      * 
      * This function will be called when the registered audio sink is ready
      * to accept more samples.
      * This function is normally only called from a connected sink object.
      */
     virtual void resumeOutput(void);

     sigc::signal<void, State> stateChange;

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

     void setState(State newState);

  private:
     void reconnect();
     void login(void);
     void sendVoiceData();

     void sendRequest(Request rq);
     int handleCommand(unsigned char *data, int len);
     int handleAudioData(unsigned char *data, int len);
     int handleList(unsigned char *data, int len, bool is_header=false);

     void onConnected(void);
     void onDisconnected(Async::TcpConnection *conn, 
		         Async::TcpConnection::DisconnectReason reason);
     int onDataReceived(Async::TcpConnection *con, void *data, int len);
     void onSendBufferFull(bool is_full);

     void onKeepaliveTimeout(Async::Timer *timer);
     void onConnectTimeout(Async::Timer *timer);
     void onRxTimeout(Async::Timer *timer);

  private:
    static const int    CLIENT_INDEX_SIZE       = 2;
    static const int    TCP_BUFFER_SIZE         = 65536;
    static const int    MAX_CONNECT_RETRY_CNT   = 5;
    static const int    CON_TIMEOUT_TIME        = 30000;
    static const int    RX_TIMEOUT_TIME         = 500;
    static const int    FRAME_COUNT             = 5;
    static const int    PCM_FRAME_SIZE          = 160*2;  // WAV49 has 2x
    static const int    GSM_FRAME_SIZE          = 65;     // WAV49 has 65
    static const int    BUFFER_SIZE             = FRAME_COUNT*PCM_FRAME_SIZE;
    static const int    FRN_AUDIO_PACKET_SIZE   = FRAME_COUNT*GSM_FRAME_SIZE;

    bool                init_ok;

    Async::TcpClient *  tcp_client;
    Async::Timer *      rx_timeout_timer;
    Async::Timer *      con_timeout_timer;
    State               state;
    int                 connect_retry_cnt;
    short               receive_buffer[BUFFER_SIZE];
    short               send_buffer[BUFFER_SIZE];
    int                 send_buffer_cnt;
    gsm                 gsmh;
    int                 lines_to_read;

    std::string         opt_server;
    std::string         opt_port;
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

