/**
@file	 ReflectorMsg.h
@brief   Reflector protocol message definitions
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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

#ifndef REFLECTOR_MSG_INCLUDED
#define REFLECTOR_MSG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <AsyncMsg.h>
#include <gcrypt.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



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
@brief	Base class for Reflector TCP network messages
@author Tobias Blomberg / SM0SVX
@date   2017-02-12

This is the top most base class for TCP messages. It is typically used as
the argument type for functions that take a TCP message as argument.
*/
class ReflectorMsg : public Async::Msg
{
  public:
    static const uint32_t MAX_PREAUTH_FRAME_SIZE = 64;
    static const uint32_t MAX_POSTAUTH_FRAME_SIZE = 16384;

    /**
     * @brief 	Constuctor
     * @param 	type The message type
     */
    ReflectorMsg(uint16_t type=0) : m_type(type) {}

    /**
     * @brief 	Destructor
     */
    virtual ~ReflectorMsg(void) {}

    /**
     * @brief 	Get the message type
     * @return	Returns the message type
     */
    uint16_t type(void) const { return m_type; }

    ASYNC_MSG_MEMBERS(m_type)

  private:
    uint16_t m_type;

};  /* class ReflectorMsg */


/**
@brief	 Intermediate template base class for Reflector TCP network messages
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This class should be used as the base when implementing new TCP network
protocol messages. The message type is given as the template argument.
*/
template <unsigned msg_type>
class ReflectorMsgBase : public ReflectorMsg
{
  public:
    static const unsigned TYPE  = msg_type;

  protected:
    ReflectorMsgBase(void) : ReflectorMsg(msg_type) {}

}; /* ReflectorMsgBase */


/**
 * @brief   The base class for UDP network messages
 * @author  Tobias Blomberg / SM0SVX
 * @date    2017-02-12

This is the top most base class for UDP messages. It is typically used as
the argument type for functions that take a UDP message as argument.
 */
class ReflectorUdpMsg : public Async::Msg
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	type The message type
     * @param   client_id The client ID
     */
    ReflectorUdpMsg(uint16_t type=0, uint16_t client_id=0, uint16_t seq=0)
      : m_type(type), m_client_id(client_id), m_seq(seq) {}

    /**
     * @brief 	Destructor
     */
    virtual ~ReflectorUdpMsg(void) {}

    /**
     * @brief 	Get the message type
     * @return	Returns the message type
     */
    uint16_t type(void) const { return m_type; }

    /**
     * @brief   Get the clientId
     * @return  Returns the client ID
     */
    uint16_t clientId(void) const { return m_client_id; }

    /**
     * @brief   Get the sequence number
     * @return  Returns the message sequence number
     */
    uint16_t sequenceNum(void) const { return m_seq; }

    ASYNC_MSG_MEMBERS(m_type, m_client_id, m_seq)

  private:
    uint16_t m_type;
    uint16_t m_client_id;
    uint16_t m_seq;
};


/**
@brief	 Intermediate template base class for Reflector UDP network messages
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This class should be used as the base when implementing new UDP network
protocol messages. The message type is given as the template argument.
*/
template <unsigned msg_type>
class ReflectorUdpMsgBase : public ReflectorUdpMsg
{
  public:
    static const unsigned TYPE  = msg_type;

  protected:
    ReflectorUdpMsgBase(void) : ReflectorUdpMsg(msg_type) {}

}; /* ReflectorUdpMsgBase */


/************************** Administrative Messages **************************/

/**
@brief	 Protocol version TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This is the first message exchanged between the client and the server. It tells
the server what protocol version that the client supports. If the client use a
protocol version that the server does not support, the client is denied access.
*/
class MsgProtoVer : public ReflectorMsgBase<5>
{
  public:
    static const uint16_t MAJOR = 1;
    static const uint16_t MINOR = 0;
    MsgProtoVer(void) : m_major(MAJOR), m_minor(MINOR) {}
    uint16_t majorVer(void) const { return m_major; }
    uint16_t minorVer(void) const { return m_minor; }

    ASYNC_MSG_MEMBERS(m_major, m_minor);

  private:
    uint16_t m_major;
    uint16_t m_minor;
}; /* MsgProtoVer */


/**
@brief	 Heartbeat TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by both client and server to indicate to the other side
that the connection is still up.
*/
class MsgHeartbeat : public ReflectorMsgBase<1>
{
  public:
    ASYNC_MSG_NO_MEMBERS
};  /* MsgHeartbeat */


/**
@brief	 Authentication challenge TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

The authentication challenge is what initizlizes the authentication process. It
is sent by the server to the client and essentially it's just a very large
random number. When received by the client, a MsgAuthResponse message is sent.
*/
class MsgAuthChallenge : public ReflectorMsgBase<10>
{
  public:
    static const size_t CHALLENGE_LEN  = 20;
    MsgAuthChallenge(void) : m_challenge(CHALLENGE_LEN)
    {
      gcry_create_nonce(&m_challenge.front(), CHALLENGE_LEN);
    }

    const uint8_t *challenge(void) const
    {
      if (m_challenge.size() != CHALLENGE_LEN)
      {
        return 0;
      }
      return &m_challenge[0];
    }

    ASYNC_MSG_MEMBERS(m_challenge);

  private:
    std::vector<uint8_t> m_challenge;
}; /* MsgAuthChallenge */


/**
@brief	 Authentication response TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

The authentication response message is sent by the client as an answer to the
MsgAuthChallenge message. The received challenge is essentially just a very
large random number which is combined with the authentication key (the clear
text password) into a 'digest'. The digest is sent to the server and the server
can do similar calculations to verify that the authentication key is correct.
Using this mechanism, the clear text password never have to be transmitted over
the network.
*/
class MsgAuthResponse : public ReflectorMsgBase<11>
{
  public:
    static const int      ALGO        = GCRY_MD_SHA1;
    static const size_t   DIGEST_LEN  = 20;
    MsgAuthResponse(void) {}

    /**
     * @brief   Constructor
     * @param   callsign The callsign (username) of the client
     * @param   key The authentication key (clear text password)
     * @param   challenge The authentication challenge received from the server
     */
    MsgAuthResponse(const std::string& callsign, const std::string &key,
                    const unsigned char *challenge)
      : m_digest(DIGEST_LEN), m_callsign(callsign)
    {
      if (!calcDigest(&m_digest.front(), key.c_str(), key.size(), challenge))
      {
        exit(1);
      }
    }

    /**
     * @brief   Get the digest
     */
    const uint8_t *digest(void) const { return &m_digest.front(); }

    /**
     * @brief   Get the callsign
     */
    const std::string& callsign(void) const { return m_callsign; }

    /**
     * @brief   Verify that the given key and challenge match the digest
     * @param   key The authentication key
     * @param   challenge The previously transmitted authentication challenge
     *
     * This function will verify that the given key and challenge match the
     * digest that is embedded in this protocol message. Typically this
     * function is used by the server to verify that the received digest is
     * correct.
     */
    bool verify(const std::string &key, const unsigned char *challenge) const
    {
      unsigned char digest[DIGEST_LEN];
      bool ok = calcDigest(digest, key.c_str(), key.size(), challenge);
      return ok && (m_digest.size() == DIGEST_LEN) &&
             (memcmp(&m_digest.front(), digest, DIGEST_LEN) == 0);
    }

    ASYNC_MSG_MEMBERS(m_callsign, m_digest);

  private:
    std::vector<uint8_t> m_digest;
    std::string          m_callsign;

    bool calcDigest(unsigned char *digest, const char *key,
                    int keylen, const unsigned char *challenge) const
    {
      unsigned char *digest_ptr = 0;
      gcry_md_hd_t hd = { 0 };
      gcry_error_t err = gcry_md_open(&hd, ALGO, GCRY_MD_FLAG_HMAC);
      if (err) goto error;
      err = gcry_md_setkey(hd, key, keylen);
      if (err) goto error;
      gcry_md_write(hd, challenge, MsgAuthChallenge::CHALLENGE_LEN);
      digest_ptr = gcry_md_read(hd, 0);
      memcpy(digest, digest_ptr, DIGEST_LEN);
      gcry_md_close(hd);
      return true;

      error:
        gcry_md_close(hd);
        std::cerr << "*** ERROR: gcrypt error: "
                  << gcry_strsource(err) << "/" << gcry_strerror(err)
                  << std::endl;
        return false;
    }
}; /* MsgAuthResponse */


/**
@brief	 Authentication success TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

The authentication ok message is sent by the server to the client on successful
authentication. After this message has been received by the client, the server
will start accepting other protocol messages.
*/
class MsgAuthOk : public ReflectorMsgBase<12>
{
  public:
    ASYNC_MSG_NO_MEMBERS
};  /* MsgAuthOk */


/**
@brief	 Protocol error TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent when a protocol error is discovered. The receiving part
should immediately terminate the connection upon receiving this message.
*/
class MsgError : public ReflectorMsgBase<13>
{
  public:
    MsgError(const std::string& msg="") : m_msg(msg) {}
    const std::string& message(void) const { return m_msg; }

    ASYNC_MSG_MEMBERS(m_msg)

  private:
    std::string m_msg;
}; /* MsgError */


/**
@brief	 Server information TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the client to inform about server and
connection properties.
*/
class MsgServerInfo : public ReflectorMsgBase<100>
{
  public:
    MsgServerInfo(uint32_t client_id=0,
                  std::vector<std::string> codecs=std::vector<std::string>())
      : m_client_id(client_id), m_codecs(codecs) {}
    uint32_t clientId(void) const { return m_client_id; }
    std::vector<std::string>& nodes(void) { return m_nodes; }
    std::vector<std::string>& codecs(void) { return m_codecs; }

    ASYNC_MSG_MEMBERS(m_client_id, m_nodes, m_codecs)

  private:
    uint32_t                  m_client_id;
    std::vector<std::string>  m_nodes;
    std::vector<std::string>  m_codecs;
}; /* MsgServerInfo */


/**
@brief	 Node list TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the client at the start of the connection
to inform about what nodes are connected at the moment.
*/
class MsgNodeList : public ReflectorMsgBase<101>
{
  public:
    MsgNodeList(void) {}
    MsgNodeList(const std::vector<std::string>& nodes) : m_nodes(nodes) {}
    std::vector<std::string>& nodes(void) { return m_nodes; }

    ASYNC_MSG_MEMBERS(m_nodes);

  private:
    std::vector<std::string> m_nodes;
}; /* MsgNodeList */


/**
@brief	 Node joined TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the clients to inform about that a new
node has connected to the reflector.
*/
class MsgNodeJoined : public ReflectorMsgBase<102>
{
  public:
    MsgNodeJoined(const std::string& callsign="") : m_callsign(callsign) {}

    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_callsign);

  private:
    std::string m_callsign;
}; /* MsgNodeJoined */


/**
@brief	 Node left TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the clients to inform about that a node
has disconnected from the reflector.
*/
class MsgNodeLeft : public ReflectorMsgBase<103>
{
  public:
    MsgNodeLeft(const std::string& callsign="") : m_callsign(callsign) {}

    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_callsign);

  private:
    std::string m_callsign;
}; /* MsgNodeLeft */


/**
@brief	 Talker start TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the clients to inform about that the
specified node is now the talker. Other nodes starting to send audio will be
ignored. Note that UDP audio messages may be received before this message has
been received. This message is meant to be informational only.
*/
class MsgTalkerStart : public ReflectorMsgBase<104>
{
  public:
    MsgTalkerStart(const std::string& callsign="") : m_callsign(callsign) {}

    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_callsign);

  private:
    std::string m_callsign;
}; /* MsgTalkerStart */


/**
@brief	 Talker stop TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the clients to inform about that the
specified node has stopped talking. Note that audio packets may be received
even after this message has been received. It should be used for informational
purposes only.
*/
class MsgTalkerStop : public ReflectorMsgBase<105>
{
  public:
    MsgTalkerStop(const std::string& callsign="") : m_callsign(callsign) {}

    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_callsign);

  private:
    std::string m_callsign;
}; /* MsgTalkerStop */





/***************************** UDP Messages *****************************/

/**
@brief	 Heartbeat UDP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

The UDP heartbeat message is sent by both the client and the server to inform
the other side that the UDP connection is still up. It also serves the purpose
to keep the path open through firewalls.
*/
class MsgUdpHeartbeat : public ReflectorUdpMsgBase<1>
{
  public:
    ASYNC_MSG_NO_MEMBERS
};  /* MsgUdpHeartbeat */


/**
@brief	 Audio UDP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This is the message used to transmit audio to the other side.
*/
class MsgUdpAudio : public ReflectorUdpMsgBase<101>
{
  public:
    MsgUdpAudio(void) {}
    MsgUdpAudio(const void *buf, int count)
    {
      if (count > 0)
      {
        const uint8_t *bbuf = reinterpret_cast<const uint8_t*>(buf);
        m_audio_data.assign(bbuf, bbuf+count);
      }
    }
    std::vector<uint8_t>& audioData(void) { return m_audio_data; }

    ASYNC_MSG_MEMBERS(m_audio_data)

  private:
    std::vector<uint8_t> m_audio_data;
}; /* MsgUdpAudio */


/**
@brief	 Audio flush UDP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is used to indicate 'end of audio stream' to the other side.
*/
class MsgUdpFlushSamples : public ReflectorUdpMsgBase<102>
{
  public:
    ASYNC_MSG_NO_MEMBERS
}; /* MsgUdpFlushSamples */


/**
@brief	 All audio flushed UDP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is used to indicate to the other side that all audio has been
written to the final destination.
*/
class MsgUdpAllSamplesFlushed : public ReflectorUdpMsgBase<103>
{
  public:
    ASYNC_MSG_NO_MEMBERS
}; /* MsgUdpAllSamplesFlushed */


#endif /* REFLECTOR_MSG_INCLUDED */



/*
 * This file has not been truncated
 */
