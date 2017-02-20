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

#include <msgpack.hpp>
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

#define MSG_DEFINE(_CLASSNAME_, ...) \
  public: \
    _CLASSNAME_(const msgpack::object &obj) { fromMsgPackObj(obj); } \
    virtual bool haveData(void) const { return true; } \
    MSGPACK_DEFINE(__VA_ARGS__)


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
class Msg
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	type The message type
     */
    Msg(unsigned type) : m_type(type) {}

    /**
     * @brief 	Destructor
     */
    virtual ~Msg(void) {}

    /**
     * @brief 	Get the message type
     * @return	Returns the message type
     */
    unsigned type(void) const { return m_type; }

    virtual bool haveData(void) const { return false; }

     /**
      * @brief  Pack the message into the given packer object
      * @param  packer The packer object to pack the message into
      */
     void msgpack_pack(msgpack::packer<msgpack::sbuffer>& packer) const
     {
       pack(packer);
     }

  protected:
    /**
     * @brief 	Implemented by the deried class to pack message into buffer
     * @param 	packer The packer object to use
     */
    virtual void pack(msgpack::packer<msgpack::sbuffer>& packer) const = 0;

  private:
    unsigned m_type;

};  /* class Msg */


template <class T, unsigned msg_type>
class MsgBase : public Msg
{
  public:
    static const unsigned TYPE  = msg_type;
    void fromMsgPackObj(const msgpack::object& obj)
    {
#if MSGPACK_VERSION_MAJOR < 1
      obj.convert(dynamic_cast<T*>(this));
#else
      obj.convert(*dynamic_cast<T*>(this));
#endif
    }

  protected:
    MsgBase(void) : Msg(T::TYPE) {}

  private:
    virtual void pack(msgpack::packer<msgpack::sbuffer>& packer) const
    {
      dynamic_cast<const T*>(this)->msgpack_pack(packer);
    }
}; /* MsgBase */


/************************** Administrative Messages **************************/

class MsgProtoVer : public MsgBase<MsgProtoVer, 5>
{
  public:
    static const uint16_t MAJOR = 1;
    static const uint16_t MINOR = 3;
    MsgProtoVer(void) : m_major(MAJOR), m_minor(MINOR) {}
    uint16_t majorVer(void) const { return m_major; }
    uint16_t minorVer(void) const { return m_minor; }

  private:
    uint16_t m_major;
    uint16_t m_minor;

    MSG_DEFINE(MsgProtoVer, m_major, m_minor);
}; /* MsgProtoVer */


class MsgHeartbeat : public MsgBase<MsgHeartbeat, 1>
{
};  /* MsgHeartbeat */


class MsgAuthChallenge : public MsgBase<MsgAuthChallenge, 10>
{
  public:
    static const int CHALLENGE_LEN  = 20;
    MsgAuthChallenge(void) : m_challenge(CHALLENGE_LEN)
    {
      gcry_create_nonce(&m_challenge.front(), CHALLENGE_LEN);
    }

    const uint8_t *challenge(void) const { return &m_challenge.front(); }

  private:
    std::vector<uint8_t> m_challenge;

    MSG_DEFINE(MsgAuthChallenge, m_challenge);
}; /* MsgAuthChallenge */


class MsgAuthResponse : public MsgBase<MsgAuthResponse, 11>
{
  public:
    static const int      ALGO        = GCRY_MD_SHA1;
    static const int      DIGEST_LEN  = 20;
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

    MSG_DEFINE(MsgAuthResponse, m_callsign, m_digest);
}; /* MsgAuthResponse */


class MsgAuthOk : public MsgBase<MsgAuthOk, 12>
{
};  /* MsgAuthOk */


class MsgError : public MsgBase<MsgError, 13>
{
  public:
    MsgError(const std::string& msg) : m_msg(msg) {}
    const std::string& message(void) const { return m_msg; }

  private:
    std::string m_msg;

    MSG_DEFINE(MsgError, m_msg)
}; /* MsgError */


class MsgServerInfo : public MsgBase<MsgServerInfo, 100>
{
  public:
    MsgServerInfo(uint32_t client_id) : m_client_id(client_id) {}
    uint32_t clientId(void) { return m_client_id; }

  private:
    uint32_t  m_client_id;

    MSG_DEFINE(MsgServerInfo, m_client_id)
}; /* MsgServerInfo */


class MsgAudio : public MsgBase<MsgAudio, 101>
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

  private:
    std::vector<uint8_t> m_audio_data;

    MSG_DEFINE(MsgAudio, m_audio_data)
}; /* MsgAudio */


//} /* namespace */

#endif /* REFLECTOR_MSG_INCLUDED */



/*
 * This file has not been truncated
 */
