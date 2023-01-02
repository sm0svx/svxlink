/**
@file	 ReflectorMsg.h
@brief   Reflector protocol message definitions
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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
    using ClientId = uint16_t;

    /**
     * @brief 	Constuctor
     * @param 	type The message type
     * @param   client_id The client ID
     */
    ReflectorUdpMsg(uint16_t type=0, ClientId client_id=0, uint16_t seq=0)
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
    ClientId clientId(void) const { return m_client_id; }

    /**
     * @brief   Get the sequence number
     * @return  Returns the message sequence number
     */
    uint16_t sequenceNum(void) const { return m_seq; }

    ASYNC_MSG_MEMBERS(m_type, m_client_id, m_seq)

  private:
    uint16_t  m_type;
    ClientId  m_client_id;
    uint16_t  m_seq;
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
@brief	 Protocol version TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This is the first message exchanged between the client and the server. It is
sent by the client to tell the server what protocol version that the client
supports. If the client use a protocol version that the server does not
support, the client is denied access. Alternatively, if the client protocol
version is larger than what the server supports, the server may send a
MsgProtoVerDowngrade message to ask the client to use an older version of the
protocol.
*/
class MsgProtoVer : public ReflectorMsgBase<5>
{
  public:
    static const uint16_t MAJOR = 2;
    static const uint16_t MINOR = 0;
    MsgProtoVer(void) : m_major(MAJOR), m_minor(MINOR) {}
    MsgProtoVer(uint16_t major, uint16_t minor)
      : m_major(major), m_minor(minor) {}
    uint16_t majorVer(void) const { return m_major; }
    uint16_t minorVer(void) const { return m_minor; }

    ASYNC_MSG_MEMBERS(m_major, m_minor);

  private:
    uint16_t m_major;
    uint16_t m_minor;
}; /* MsgProtoVer */


/**
@brief   Protocol version downgrade request
@author  Tobias Blomberg / SM0SVX
@date    2019-10-19

This message is sent by the reflector server to a client that announces a newer
protocol version than the server is able to support. The client should resend
the MsgProtoVer message set to a version number no larger than what the server
indicates in this message. The client must then use only protocol messages
compatible with the lower protocol version.
*/
class MsgProtoVerDowngrade : public ReflectorMsgBase<6>
{
  public:
    MsgProtoVerDowngrade(void)
      : m_major(MsgProtoVer::MAJOR), m_minor(MsgProtoVer::MINOR) {}
    uint16_t majorVer(void) const { return m_major; }
    uint16_t minorVer(void) const { return m_minor; }

    ASYNC_MSG_MEMBERS(m_major, m_minor);

  private:
    uint16_t m_major;
    uint16_t m_minor;
}; /* MsgProtoVerDowngrade */


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
    using ClientId = ReflectorUdpMsg::ClientId;

    MsgServerInfo(ClientId client_id=0,
                  std::vector<std::string> codecs=std::vector<std::string>())
      : m_client_id(client_id), m_codecs(codecs) {}
    ClientId clientId(void) const { return m_client_id; }
    std::vector<std::string>& nodes(void) { return m_nodes; }
    std::vector<std::string>& codecs(void) { return m_codecs; }

    ASYNC_MSG_MEMBERS(m_reserved, m_client_id, m_nodes, m_codecs)

  private:
    uint16_t                  m_reserved = 0;
    ClientId                  m_client_id;
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
    MsgTalkerStart(uint32_t tg=0, const std::string& callsign="")
      : m_tg(tg), m_callsign(callsign) {}

    uint32_t tg(void) const { return m_tg; }
    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_tg, m_callsign);

  private:
    uint32_t    m_tg;
    std::string m_callsign;
}; /* MsgTalkerStart */


/**
@brief	 Talker start TCP network message V1
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the clients to inform about that the
specified node is now the talker. Other nodes starting to send audio will be
ignored. Note that UDP audio messages may be received before this message has
been received. This message is meant to be informational only.
*/
class MsgTalkerStartV1 : public ReflectorMsgBase<104>
{
  public:
    MsgTalkerStartV1(const std::string& callsign="") : m_callsign(callsign) {}

    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_callsign);

  private:
    std::string m_callsign;
}; /* MsgTalkerStartV1 */


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
    MsgTalkerStop(uint32_t tg=0, const std::string& callsign="")
      : m_tg(tg), m_callsign(callsign) {}

    uint32_t tg(void) const { return m_tg; }
    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_tg, m_callsign);

  private:
    uint32_t    m_tg;
    std::string m_callsign;
}; /* MsgTalkerStop */


/**
@brief	 Talker stop TCP network message V1
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This message is sent by the server to the clients to inform about that the
specified node has stopped talking. Note that audio packets may be received
even after this message has been received. It should be used for informational
purposes only.
*/
class MsgTalkerStopV1 : public ReflectorMsgBase<105>
{
  public:
    MsgTalkerStopV1(const std::string& callsign="") : m_callsign(callsign) {}

    const std::string& callsign(void) const { return m_callsign; }

    ASYNC_MSG_MEMBERS(m_callsign);

  private:
    std::string m_callsign;
}; /* MsgTalkerStopV1 */


/**
@brief	 Choose talk group TCP network message
@author  Tobias Blomberg / SM0SVX
@date    2019-07-25

This message is sent by the client to choose which talk group to use for
communication.
*/
class MsgSelectTG : public ReflectorMsgBase<106>
{
  public:
    MsgSelectTG(void) : m_tg(0) {}
    MsgSelectTG(uint32_t tg) : m_tg(tg) {}

    uint32_t tg(void) const { return m_tg; }

    ASYNC_MSG_MEMBERS(m_tg);

  private:
    uint32_t m_tg;
}; /* MsgSelectTG */


/**
@brief   Choose which talk groups to monitor
@author  Tobias Blomberg / SM0SVX
@date    2019-08-08

This message is sent by the client to choose which talk groups to monitor for
activity when idle.
*/
class MsgTgMonitor : public ReflectorMsgBase<107>
{
  public:
    MsgTgMonitor(void) {}
    MsgTgMonitor(const std::set<uint32_t>& tgs) : m_tgs(tgs) {}

    const std::set<uint32_t>& tgs(void) const { return m_tgs; }

    ASYNC_MSG_MEMBERS(m_tgs);

  private:
    std::set<uint32_t> m_tgs;
}; /* MsgTgMonitor */


#if 0
/**
@brief   Send information about the client to the server
@author  Tobias Blomberg / SM0SVX
@date    2019-08-08

This is an optional message that can be sent by the client to inform the server
about client properties. It can be resent at any time to update the client
information.
*/
class MsgNodeInfo : public ReflectorMsgBase<108>
{
  public:
    class Site : public Async::Msg
    {
      public:
        Site(void)
          : m_qth_lat(std::numeric_limits<int16_t>::min()),
            m_qth_lon(std::numeric_limits<int16_t>::min()),
            m_antenna_height(std::numeric_limits<int32_t>::min()),
            m_antenna_dir(-1), m_rf_frequency(0) {}

        Site& setQthName(const std::string& name)
        {
          m_qth_name = name;
          return *this;
        }
        const std::string& qthName(void) const { return m_qth_name; }
        bool qthNameIsValid(void) const { return !m_qth_name.empty(); }

        Site& setQthPos(double lat, double lon)
        {
          m_qth_lat = static_cast<int16_t>(1000000.0 * lat);
          m_qth_lon = static_cast<int16_t>(1000000.0 * lon);
          return *this;
        }
        bool qthPosIsValid(void) const
        {
          return (m_qth_lat != std::numeric_limits<int16_t>::min()) &&
                 (m_qth_lon != std::numeric_limits<int16_t>::min());
        }
        double qthLat(void) const { return m_qth_lat / 1000000.0; }
        double qthLon(void) const { return m_qth_lon / 1000000.0; }

        Site& setAntennaHeight(int32_t height)
        {
          m_antenna_height = height;
          return *this;
        }
        bool antennaHeightIsValid(void) const
        {
          return m_antenna_height != std::numeric_limits<int32_t>::min();
        }
        int32_t antennaHeight(void) const { return m_antenna_height; }

        Site& setAntennaDirection(float dir)
        {
          if (dir < 0.0f)
          {
            m_antenna_dir = -1;
          }
          else
          {
            m_antenna_dir = static_cast<int16_t>(10.0f * dir);
          }
          return *this;
        }
        bool antennaDirectionIsValid(void) const { return m_antenna_dir >= 0; }
        float antennaDirection(void) const { return m_antenna_dir / 10.0f; }

        Site& setRfFrequency(uint64_t rf_frequency)
        {
          m_rf_frequency = rf_frequency;
          return *this;
        }
        bool rfFrequencyIsValid(void) const { return m_rf_frequency > 0; }
        uint64_t rfFrequency(void) const { return m_rf_frequency; }

        Site& setCtcssFrequencies(std::vector<float> ctcss_frequencies)
        {
          m_ctcss_frequencies = ctcss_frequencies;
          return *this;
        }
        bool ctcssFrequenciesIsValid(void) const
        {
          return !m_ctcss_frequencies.empty();
        }
        const std::vector<float>& ctcssFrequencies(void) const
        {
          return m_ctcss_frequencies;
        }

        ASYNC_MSG_MEMBERS(m_qth_name, m_qth_lat, m_qth_lon,
                          m_antenna_height, m_antenna_dir, m_rf_frequency,
                          m_ctcss_frequencies);

      private:
        std::string         m_qth_name;
        int32_t             m_qth_lat;
        int32_t             m_qth_lon;
        int32_t             m_antenna_height;
        int16_t             m_antenna_dir;
        uint64_t            m_rf_frequency;
        std::vector<float>  m_ctcss_frequencies;
    };

    class RxSite : public Site
    {
      public:
        RxSite& setRxName(const std::string& name)
        {
          m_rx_name = name;
          return *this;
        }
        bool rxNameIsValid(void) const { return !m_rx_name.empty(); }
        const std::string& rxName(void) const { return m_rx_name; }

        ASYNC_MSG_DERIVED_FROM(Site)
        ASYNC_MSG_MEMBERS(m_rx_name)

      private:
        std::string m_rx_name;
    };
    typedef std::vector<RxSite> RxSites;

    class TxSite : public Site
    {
      public:
        TxSite(void) : m_tx_power(0) {}

        TxSite& setTxName(const std::string& name)
        {
          m_tx_name = name;
          return *this;
        }
        bool txNameIsValid(void) const { return !m_tx_name.empty(); }
        const std::string& txName(void) const { return m_tx_name; }

        TxSite& setTxPower(float power)
        {
          m_tx_power = static_cast<uint16_t>(1000.0f * power);
          return *this;
        }
        bool txPowerIsValid(void) const { return m_tx_power > 0; }
        float txPower(void) const { return m_tx_power / 1000.0f; }

        ASYNC_MSG_DERIVED_FROM(Site)
        ASYNC_MSG_MEMBERS(m_tx_name, m_tx_power)

      private:
        std::string m_tx_name;
        uint16_t    m_tx_power;
    };
    typedef std::vector<TxSite> TxSites;

    MsgNodeInfo(void) {}
    MsgNodeInfo(const std::string& sw_info) : m_sw_info(sw_info) {}

    MsgNodeInfo& setSwInfo(const std::string& sw_info)
    {
      m_sw_info = sw_info;
      return *this;
    }
    const std::string& swInfo(void) const { return m_sw_info; }

    MsgNodeInfo& setTxSites(const std::vector<TxSite>& tx_sites)
    {
      m_tx_sites = tx_sites;
      return *this;
    }
    const std::vector<TxSite>& txSites(void) const
    {
      return m_tx_sites;
    }

    MsgNodeInfo& setRxSites(const std::vector<RxSite>& rx_sites)
    {
      m_rx_sites = rx_sites;
      return *this;
    }
    const std::vector<RxSite>& rxSites(void) const
    {
      return m_rx_sites;
    }

    ASYNC_MSG_MEMBERS(m_sw_info, m_rx_sites, m_tx_sites);

  private:
    std::string m_sw_info;
    RxSites     m_rx_sites;
    TxSites     m_tx_sites;
}; /* MsgNodeInfo */
#endif


/**
@brief   Request QSY to a randomly chosen talk group
@author  Tobias Blomberg / SM0SVX
@date    2019-08-17

This message is sent by a client to request that all clients on the current
talk group is QSYed to the given talk group. If the talk group is set to zero,
the server assigns a randomly chosen talk group. This can be used to easily QSY
from a call channel once communication is established with the desired
partners.

The server sends this message to all clients in a talk group to request that
they QSY to the given talk group. It is up to the client to honor the request
or not by selecting the talk group.
*/
class MsgRequestQsy : public ReflectorMsgBase<109>
{
  public:
    MsgRequestQsy(uint32_t tg=0) : m_tg(tg) {}

    uint32_t tg(void) const { return m_tg; }

    ASYNC_MSG_MEMBERS(m_tg);

  private:
    uint32_t m_tg;
}; /* MsgRequestQsy */


/**
@brief   Publish state event
@author  Tobias Blomberg / SM0SVX
@date    2019-10-04

This message is sent by a client to inform the reflector server about a
published state event.
*/
class MsgStateEvent : public ReflectorMsgBase<110>
{
  public:
    MsgStateEvent(const std::string& src="", const std::string& name="",
                  const std::string msg="")
      : m_src(src), m_name(name), m_msg(msg) {}

    const std::string& src(void) const { return m_src; }
    const std::string& name(void) const { return m_name; }
    const std::string& msg(void) const { return m_msg; }

    ASYNC_MSG_MEMBERS(m_src, m_name, m_msg);

  private:
    std::string m_src;
    std::string m_name;
    std::string m_msg;
}; /* MsgStateEvent */


/**
@brief   Client information
@author  Tobias Blomberg / SM0SVX
@date    2019-10-06

This message is sent by a client to inform the reflector server about various
facts about the client. JSON is used so that information can be added without
redefining the message type.
*/
class MsgNodeInfo : public ReflectorMsgBase<111>
{
  public:
    MsgNodeInfo(const std::string& json="")
      : m_json(json) {}

    const std::string& json(void) const { return m_json; }

    ASYNC_MSG_MEMBERS(m_json);

  private:
    std::string m_json;
}; /* MsgNodeInfo */


/**
@brief   Signal strength values base
@author  Tobias Blomberg / SM0SVX
@date    2019-10-13

This is the base class used by the client for sending signal strength values to
the reflector. This class defines the actual message contents. It should not be
used directly but instead either the TCP- or UDP-version should be used.
*/
class MsgSignalStrengthValuesBase
{
  public:
    class Rx : public Async::Msg
    {
      public:
        Rx(void) : m_id('?'), m_siglev(-1), m_flags(0) {}
        Rx(char id, int16_t siglev) : m_id(id), m_siglev(siglev), m_flags(0) {}
        void setEnabled(bool is_enabled) { setBit(BIT_ENABLED, is_enabled); }
        bool enabled(void) const { return getBit(BIT_ENABLED); }
        void setSqlOpen(bool is_open) { setBit(BIT_SQL_OPEN, is_open); }
        bool sqlOpen(void) const { return getBit(BIT_SQL_OPEN); }
        void setActive(bool is_active) { setBit(BIT_ACTIVE, is_active); }
        bool active(void) const { return getBit(BIT_ACTIVE); }
        char id(void) const { return m_id; }
        int16_t siglev(void) const { return m_siglev; }

        ASYNC_MSG_MEMBERS(m_id, m_siglev, m_flags)

      private:
        typedef enum {BIT_ENABLED=0, BIT_SQL_OPEN, BIT_ACTIVE} Bit;

        char              m_id;
        uint8_t           m_siglev;
        uint8_t           m_flags;

        bool getBit(Bit bitno) const
        {
          uint8_t bit = 1 << static_cast<size_t>(bitno);
          return (m_flags & bit) != 0;
        }
        void setBit(Bit bitno, bool is_enabled)
        {
          uint8_t bit = 1 << static_cast<size_t>(bitno);
          m_flags = (m_flags & ~bit) | (is_enabled ? bit : 0);
        }
    };
    typedef std::vector<Rx> Rxs;

    MsgSignalStrengthValuesBase(void) {}
    MsgSignalStrengthValuesBase(const Rxs& rxs) : m_rxs(rxs) {}
    Rxs& rxs(void) { return m_rxs; }
    void pushBack(const Rx& rx) { m_rxs.push_back(rx); }

  protected:
    Rxs m_rxs;
}; /* MsgSignalStrengthValuesBase */


/**
@brief   Signal strength values
@author  Tobias Blomberg / SM0SVX
@date    2019-10-13

This message is used by a client to send signal strength values to the
reflector.
*/
struct MsgSignalStrengthValues
  : public MsgSignalStrengthValuesBase, public ReflectorMsgBase<112>
{
  ASYNC_MSG_MEMBERS(m_rxs)
}; /* MsgSignalStrengthValues */


/**
@brief  Tx status message
@author  Tobias Blomberg / SM0SVX
@date    2019-10-19

This message is used by a client to send tx status to the reflector server.
*/
class MsgTxStatus : public ReflectorMsgBase<113>
{
  public:
    class Tx : public Async::Msg
    {
      public:
        Tx(void) : m_id('?'), m_flags(0) {}
        Tx(char id) : m_id(id), m_flags(0) {}
        void setTransmit(bool transmit) { setBit(BIT_TRANSMIT, transmit); }
        bool transmit(void) const { return getBit(BIT_TRANSMIT); }
        char id(void) const { return m_id; }

        ASYNC_MSG_MEMBERS(m_id, m_flags)

      private:
        typedef enum {BIT_TRANSMIT=0} Bit;

        char              m_id;
        uint8_t           m_flags;

        bool getBit(Bit bitno) const
        {
          uint8_t bit = 1 << static_cast<size_t>(bitno);
          return (m_flags & bit) != 0;
        }
        void setBit(Bit bitno, bool is_enabled)
        {
          uint8_t bit = 1 << static_cast<size_t>(bitno);
          m_flags = (m_flags & ~bit) | (is_enabled ? bit : 0);
        }
    };
    typedef std::vector<Tx> Txs;

    MsgTxStatus(void) {}
    MsgTxStatus(const Txs& txs) : m_txs(txs) {}
    Txs& txs(void) { return m_txs; }
    void pushBack(const Tx& tx) { m_txs.push_back(tx); }

    ASYNC_MSG_MEMBERS(m_txs)

  private:
    Txs m_txs;
}; /* class MsgTxStatus */


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
@brief   Audio UDP network message
@author  Tobias Blomberg / SM0SVX
@date    2017-02-12

This is the message used to transmit audio to the other side.
*/
class MsgUdpAudio : public ReflectorUdpMsgBase<101>
{
  public:
    MsgUdpAudio(void) {}
    MsgUdpAudio(const std::vector<uint8_t>& audio_data)
      : m_audio_data(audio_data) {}
    MsgUdpAudio(const void *buf, int count)
    {
      if (count > 0)
      {
        const uint8_t *bbuf = reinterpret_cast<const uint8_t*>(buf);
        m_audio_data.assign(bbuf, bbuf+count);
      }
    }
    std::vector<uint8_t>& audioData(void) { return m_audio_data; }
    const std::vector<uint8_t>& audioData(void) const { return m_audio_data; }

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


/**
@brief   Signal strength values
@author  Tobias Blomberg / SM0SVX
@date    2019-10-06

This message is used by a client to send signal strength values to the
reflector.
*/
struct MsgUdpSignalStrengthValues
  : public MsgSignalStrengthValuesBase, public ReflectorUdpMsgBase<104>
{
  ASYNC_MSG_MEMBERS(m_rxs)
}; /* MsgUdpSignalStrengthValues */


#if 0
/**
@brief	 Audio UDP network message V2
@author  Tobias Blomberg / SM0SVX
@date    2019-07-25

This is the message used to transmit audio to the other side.
*/
class MsgUdpAudio : public ReflectorUdpMsgBase<101>
{
  public:
    MsgUdpAudio(void) : m_tg(0) {}
    MsgUdpAudio(const MsgUdpAudioV1& msg_v1)
      : m_tg(0), m_audio_data(msg_v1.audioData()) {}
    MsgUdpAudio(uint32_t tg, const void *buf, int count)
      : m_tg(tg)
    {
      if (count > 0)
      {
        const uint8_t *bbuf = reinterpret_cast<const uint8_t*>(buf);
        m_audio_data.assign(bbuf, bbuf+count);
      }
    }
    uint32_t tg(void) { return m_tg; }
    std::vector<uint8_t>& audioData(void) { return m_audio_data; }

    ASYNC_MSG_MEMBERS(m_tg, m_audio_data)

  private:
    uint32_t              m_tg;
    std::vector<uint8_t>  m_audio_data;
}; /* MsgUdpAudio */
#endif


#endif /* REFLECTOR_MSG_INCLUDED */



/*
 * This file has not been truncated
 */
