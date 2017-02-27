/**
@file	 ReflectorMsg.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
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

/** @example Template_demo.cpp
An example of how to use the ReflectorMsg class
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
@brief	Base class for Reflector network messages
@author Tobias Blomberg / SM0SVX
@date   2017-02-12
*/
class ReflectorMsg : public Async::Msg
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	type The message type
     * @param   size The payload size
     */
    ReflectorMsg(uint16_t type=0, uint16_t size=0)
      : m_type(type), m_size(size) {}

    /**
     * @brief 	Destructor
     */
    virtual ~ReflectorMsg(void) {}

    /**
     * @brief 	Get the message type
     * @return	Returns the message type
     */
    uint16_t type(void) const { return m_type; }
    void setSize(uint16_t size) { m_size = size; }
    uint16_t size(void) const { return m_size; }

    ASYNC_MSG_MEMBERS(m_type, m_size)

  private:
    uint16_t m_type;
    uint16_t m_size;

};  /* class ReflectorMsg */


template <unsigned msg_type>
class ReflectorMsgBase : public ReflectorMsg
{
  public:
    static const unsigned TYPE  = msg_type;

  protected:
    ReflectorMsgBase(void) : ReflectorMsg(msg_type) {}

}; /* ReflectorMsgBase */


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
    uint16_t clientId(void) const { return m_client_id; }
    uint16_t sequenceNum(void) const { return m_seq; }

    ASYNC_MSG_MEMBERS(m_type, m_client_id, m_seq)

  private:
    uint16_t m_type;
    uint16_t m_client_id;
    uint16_t m_seq;
};


template <unsigned msg_type>
class ReflectorUdpMsgBase : public ReflectorUdpMsg
{
  public:
    static const unsigned TYPE  = msg_type;

  protected:
    ReflectorUdpMsgBase(void) : ReflectorUdpMsg(msg_type) {}

}; /* ReflectorUdpMsgBase */


/************************** Administrative Messages **************************/

class MsgProtoVer : public ReflectorMsgBase<5>
{
  public:
    static const uint16_t MAJOR = 0;
    static const uint16_t MINOR = 1;
    MsgProtoVer(void) : m_major(MAJOR), m_minor(MINOR) {}
    uint16_t majorVer(void) const { return m_major; }
    uint16_t minorVer(void) const { return m_minor; }

    ASYNC_MSG_MEMBERS(m_major, m_minor);

  private:
    uint16_t m_major;
    uint16_t m_minor;
}; /* MsgProtoVer */


class MsgHeartbeat : public ReflectorMsgBase<1>
{
  public:
    ASYNC_MSG_NO_MEMBERS
};  /* MsgHeartbeat */


class MsgUdpHeartbeat : public ReflectorUdpMsgBase<1>
{
  public:
    ASYNC_MSG_NO_MEMBERS
};  /* MsgUdpHeartbeat */


class MsgAuthChallenge : public ReflectorMsgBase<10>
{
  public:
    static const int CHALLENGE_LEN  = 20;
    MsgAuthChallenge(void) : m_challenge(CHALLENGE_LEN)
    {
      gcry_create_nonce(&m_challenge.front(), CHALLENGE_LEN);
    }

    const uint8_t *challenge(void) const { return &m_challenge[0]; }

    ASYNC_MSG_MEMBERS(m_challenge);

  private:
    std::vector<uint8_t> m_challenge;
}; /* MsgAuthChallenge */


class MsgAuthResponse : public ReflectorMsgBase<11>
{
  public:
    static const int      ALGO        = GCRY_MD_SHA1;
    static const int      DIGEST_LEN  = 20;
    MsgAuthResponse(void) {}
    MsgAuthResponse(const std::string& callsign, const std::string &key,
                    const unsigned char *challenge)
      : m_digest(DIGEST_LEN), m_callsign(callsign)
    {
      if (!calcDigest(&m_digest.front(), key.c_str(), key.size(), challenge))
      {
        exit(1);
      }
    }

    const uint8_t *digest(void) const { return &m_digest.front(); }
    const std::string& callsign(void) const { return m_callsign; }

    bool verify(const std::string &key, const unsigned char *challenge) const
    {
      unsigned char digest[DIGEST_LEN];
      bool ok = calcDigest(digest, key.c_str(), key.size(), challenge);
      return ok && (memcmp(&m_digest.front(), digest, DIGEST_LEN) == 0);
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


class MsgAuthOk : public ReflectorMsgBase<12>
{
  public:
    ASYNC_MSG_NO_MEMBERS
};  /* MsgAuthOk */


class MsgError : public ReflectorMsgBase<13>
{
  public:
    MsgError(const std::string& msg="") : m_msg(msg) {}
    const std::string& message(void) const { return m_msg; }

    ASYNC_MSG_MEMBERS(m_msg)

  private:
    std::string m_msg;
}; /* MsgError */


class MsgServerInfo : public ReflectorMsgBase<100>
{
  public:
    MsgServerInfo(uint32_t client_id=0) : m_client_id(client_id) {}
    uint32_t clientId(void) { return m_client_id; }

    ASYNC_MSG_MEMBERS(m_client_id)

  private:
    uint32_t  m_client_id;
}; /* MsgServerInfo */


class MsgAudio : public ReflectorUdpMsgBase<101>
{
  public:
    MsgAudio(void) {}
    MsgAudio(const void *buf, int count)
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
}; /* MsgAudio */


//} /* namespace */

#endif /* REFLECTOR_MSG_INCLUDED */



/*
 * This file has not been truncated
 */
