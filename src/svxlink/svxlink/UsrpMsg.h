/**
@file	 UsrpMsg.h
@brief   Usrp protocol message definitions
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2021-04-26

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
Copyright (C) 2003-2021 Tobias Blomberg / SM0SVX

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

#ifndef USRP_MSG_INCLUDED
#define USRP_MSG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <AsyncMsg.h>


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
 * @brief   Class for Usrp audio network messages
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2021-04-28
 */
class UsrpMsg : public Async::Msg
{
  public:

    UsrpMsg(uint32_t seq=0, uint32_t memory=0, uint32_t keyup=0,
            uint32_t talkgroup=0, uint32_t type=0, uint32_t mpxid=0,
            uint32_t reserved=0, const std::array<int16_t, 160> audio_data={0})
      : m_seq(htonl(seq)), m_memory(memory), m_keyup(htonl(keyup)), 
        m_talkgroup(htonl(talkgroup)), m_type(htonl(type)), m_mpxid(mpxid), 
        m_reserved(reserved), m_audio_data(audio_data) 
        {
          eye = {'U','S','R','P'};
        }

    /**
     * @brief 	Destructor
     */
    virtual ~UsrpMsg(void) {}

    uint32_t type(void) const { return ntohl(m_type); }
    uint32_t seq(void) const { return ntohl(m_seq); }
    uint32_t memory(void) const { return m_memory; }
    uint32_t keyup(void) const { return ntohl(m_keyup); }
    uint32_t mpxid(void) const { return m_mpxid; }
    uint32_t reserved(void) const { return m_reserved; }
    uint32_t talkgroup(void) const { return ntohl(m_talkgroup); }

    void setTg(uint32_t tg) { m_talkgroup = htonl(tg);}
    void setType(uint32_t type) { m_type = htonl(type);}
    void setSeq(uint32_t seq) { m_seq = htonl(seq); }
    void setKeyup(bool keyup) { (keyup ? m_keyup=htonl(1) : m_keyup=0); }
    void setAudiodata(const std::array<int16_t, 160> audio)
    {
      m_audio_data = audio;
    }

    std::array<int16_t, 160>& audioData(void) { return m_audio_data; }
    const std::array<int16_t, 160>& audioData(void) const { return m_audio_data; }

    ASYNC_MSG_MEMBERS(eye, m_seq, m_memory, m_keyup, m_talkgroup, m_type, 
                      m_mpxid, m_reserved, m_audio_data)

  private:
    std::array<char, 4> eye;
    uint32_t m_seq;
    uint32_t m_memory;
    uint32_t m_keyup;
    uint32_t m_talkgroup;          
    uint32_t m_type;
    uint32_t m_mpxid;
    uint32_t m_reserved;
    std::array<int16_t, 160> m_audio_data;
};


/**
 * @brief   Class for Usrp network Stop message
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2021-04-28
 */
class UsrpStopMsg : public Async::Msg
{
  public:

    UsrpStopMsg(uint32_t seq=0, uint32_t talkgroup=0)
      : m_seq(htonl(seq)), m_talkgroup(htonl(talkgroup))
        {
          eye = {'U','S','R','P'};
          m_memory = 0;
          m_keyup = 0;
          m_type = 0;
          m_mpxid = 0;
          m_reserved =0;
        }

    /**
     * @brief 	Destructor
     */
    virtual ~UsrpStopMsg(void) {}

    void setTg(uint32_t tg) { m_talkgroup = htonl(tg);}
    void setSeq(uint32_t seq) { m_seq = htonl(seq); }

    ASYNC_MSG_MEMBERS(eye, m_seq, m_memory, m_keyup, m_talkgroup, m_type, 
                      m_mpxid, m_reserved)

  private:
    std::array<char, 4> eye;
    uint32_t m_seq;
    uint32_t m_memory;
    uint32_t m_keyup;
    uint32_t m_talkgroup;          
    uint32_t m_type;
    uint32_t m_mpxid;
    uint32_t m_reserved;
};


/**
 * @brief   Class for Usrp network Metadata message
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2021-04-28
 */
class UsrpMetaMsg : public Async::Msg
{
  public:

    UsrpMetaMsg(uint32_t seq=0, uint32_t talkgroup=0)
      : m_seq(htonl(seq)), m_talkgroup(htonl(talkgroup))
        {
          eye = {'U','S','R','P'};
          m_memory = 0;
          m_keyup = 0;
          m_type = htonl(2);
          m_mpxid = 0;
          m_reserved = 0;
          m_tlv = 0x08;
          m_tlvlen = 0x15;
          m_dmrid = {0,0,0};
          m_rptid = 0;
          m_tg = {0,0,0};
          m_ts = 0;
          m_cc = 1;
          m_rest.fill(0);
        }

    /**
     * @brief 	Destructor
     */
    virtual ~UsrpMetaMsg(void) {}

    void setTg(uint32_t tg) { m_talkgroup = htonl(tg);}
    void setSeq(uint32_t seq) { m_seq = htonl(seq); }
    
    std::string getCallsign(void) 
    { 
      return std::string(m_callsign.begin(), m_callsign.end());
    }

    // set callsign
    void setCallsign(std::string call)
    {
      for (size_t i=0; i<call.length(); i++)
      m_callsign[i] = call.c_str()[i];
    }
    
    // set DMR-ID
    void setDmrId(uint32_t dmrid)
    { 
      std::array<uint8_t, 3> t;
      t[0] = (dmrid >> 0) & 0xff;
      t[1] = (dmrid >> 8) & 0xff;
      t[2] = (dmrid >> 16) & 0xff;
      m_dmrid = t; 
    }

    void setRptId(uint32_t rptid) { m_rptid = htonl(rptid); }
    void setCC(uint8_t cc) { m_cc = cc; }
    void setTS(uint8_t ts) { m_ts = ts; }

    ASYNC_MSG_MEMBERS(eye, m_seq, m_memory, m_keyup, m_talkgroup, m_type, 
                      m_mpxid, m_reserved, m_tlv, m_tlvlen, m_dmrid, m_rptid,
                      m_tg, m_ts, m_cc, m_callsign, m_rest)
                      
  private:

    std::array<char, 4> eye;         // is always "USRP"
    uint32_t m_seq;                  // sequence number
    uint32_t m_memory;               // =0
    uint32_t m_keyup;                // Tx on/off (1/0)
    uint32_t m_talkgroup;
    uint32_t m_type;                 // type of frame
    uint32_t m_mpxid;
    uint32_t m_reserved;
    uint8_t  m_tlv;                  // see TLV_TAGS
    uint8_t  m_tlvlen;               // TLV-length of MetaData
    std::array<uint8_t, 3> m_dmrid;  // DMR-ID length 3 bytes
    uint32_t m_rptid;                // Repeater-ID length 4 bytes
    std::array<uint8_t, 3> m_tg;     // Talkgroup length 3 bytes
    uint8_t  m_ts;                   // Timeslot length 1 byte
    uint8_t  m_cc;                   // color code length 1 byte
    std::array<char, 6> m_callsign;  // callsign
    std::array<uint8_t, 300> m_rest; // padding the rest with 0x00
};


#endif /* USRP_MSG_INCLUDED */

/*
 * This file has not been truncated
 */
