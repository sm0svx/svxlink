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

#define MODE_NONE 0
#define MODE_DMR 1
#define MODE_P25 2
#define MODE_NXDN 3


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/

static const unsigned DEFAULT_UDP_HEARTBEAT_TX_CNT_RESET = 15;
static const unsigned UDP_HEARTBEAT_RX_CNT_RESET         = 60;
static const unsigned DEFAULT_TG_SELECT_TIMEOUT          = 30;
static const int      DEFAULT_TMP_MONITOR_TIMEOUT        = 3600;
static const int      USRP_AUDIO_FRAME_LEN               = 160;
static const int      USRP_HEADER_LEN                    = 32;

enum { USRP_TYPE_VOICE=0, USRP_TYPE_DTMF=1, USRP_TYPE_TEXT=2, 
       USRP_TYPE_PING=3, USRP_TYPE_TLV=4, USRP_TYPE_VOICE_ADPCM = 5, 
       USRP_TYPE_VOICE_ULAW = 6 };

enum { TLV_TAG_BEGIN_TX = 0, TLV_TAG_AMBE = 1, TLV_TAG_END_TX = 2,
       TLV_TAG_TG_TUNE  = 3, TLV_TAG_PLAY_AMBE= 4, TLV_TAG_REMOTE_CMD= 5,
       TLV_TAG_AMBE_49  = 6, TLV_TAG_AMBE_72  = 7, TLV_TAG_SET_INFO = 8,
       TLV_TAG_IMBE     = 9, TLV_TAG_DSAMBE   = 10, TLV_TAG_FILE_XFER= 11
};

//std::map<uint8_t, std::string> selected_mode;
const std::string selected_mode[] = { "*NONE", "*DMR", "*P25", "*NXDN" };

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
class UsrpAudioMsg : public Async::Msg
{
  public:

    UsrpAudioMsg(uint32_t seq=0, uint32_t memory=0, uint32_t keyup=0,
            uint32_t talkgroup=0, uint32_t type=0, uint32_t mpxid=0,
            uint32_t reserved=0, const std::array<int16_t, 160> audio_data={0})
      : m_seq(htole32(seq)), m_memory(htonl(memory)), m_keyup(keyup),
        m_talkgroup(htonl(talkgroup)), m_type(htonl(type)), 
        m_mpxid(htonl(mpxid)), m_reserved(htonl(reserved)), 
        m_audio_data(audio_data) 
        {
          eye = {'U','S','R','P'};
        }

    /**
     * @brief 	Destructor
     */
    virtual ~UsrpAudioMsg(void) {}

    uint32_t type(void) const { return ntohl(m_type); }
    uint32_t seq(void) const { return ntohl(m_seq); }
    uint32_t memory(void) const { return ntohl(m_memory); }
    uint32_t keyup(void) const { return m_keyup; }
    uint32_t mpxid(void) const { return ntohl(m_mpxid); }
    uint32_t reserved(void) const { return ntohl(m_reserved); }
    uint32_t talkgroup(void) const { return ntohl(m_talkgroup); }

    void setTg(uint32_t tg) { m_talkgroup = htole32(tg);}
    void setType(uint32_t type) { m_type = htole32(type);}
    void setSeq(uint32_t seq) { m_seq = htole32(seq); }
    void setKeyup(bool keyup) { (keyup ? m_keyup=1 : m_keyup=0); }

    void setAudioData(int16_t in[USRP_AUDIO_FRAME_LEN*2])
    {
      for (size_t x=0; x<USRP_AUDIO_FRAME_LEN; x++)
      {
        m_audio_data[x] = htons(in[x]);
      }
    }

    std::array<int16_t, USRP_AUDIO_FRAME_LEN>& audioData(void) 
          { return m_audio_data; }
    const std::array<int16_t, USRP_AUDIO_FRAME_LEN>& audioData(void) 
          const { return m_audio_data; }

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
    std::array<int16_t, USRP_AUDIO_FRAME_LEN> m_audio_data;
};


/**
 * @brief   Class for Usrp network header message
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2021-04-28
 */
class UsrpHeaderMsg : public Async::Msg
{
  public:

    UsrpHeaderMsg(uint32_t seq=0, uint32_t keyup=0, uint32_t talkgroup=0, 
                  uint32_t type=0)
      : m_seq(htole32(seq)), m_keyup(keyup),
        m_talkgroup(htole32(talkgroup)), m_type(htole32(type)) 
        {
          eye = {'U','S','R','P'};
          m_memory = 0;
          m_keyup = 0;
          m_type = 0;
          m_mpxid = 0;
          m_reserved = 0;
        }

    /**
     * @brief 	Destructor
     */
    virtual ~UsrpHeaderMsg(void) {}
    
    uint32_t type(void) const { return ntohl(m_type); }
    uint32_t seq(void) const { return ntohl(m_seq); }
    uint32_t memory(void) const { return ntohl(m_memory); }
    bool keyup(void) const { return (m_keyup & 1 ? true : false); }
    uint32_t mpxid(void) const { return ntohl(m_mpxid); }
    uint32_t reserved(void) const { return ntohl(m_reserved); }
    uint32_t talkgroup(void) const { return ntohl(m_talkgroup); }

    void setTg(uint32_t tg) { m_talkgroup = htole32(tg); }
    void setSeq(uint32_t seq) { m_seq = htole32(seq); }

    ASYNC_MSG_MEMBERS(eye, m_seq, m_memory, m_keyup, m_talkgroup, 
                      m_type, m_mpxid, m_reserved)

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

/*
template <typename P> 
class UsrpMsg  {
    UsrpMsg(UsrpHeaderMsg header, P payload) : header(header), payload(p) {}
    
private:
    UsrpHeaderMsg header;
    P payload;
};


UrspHeaderMsg h = ....
if(condition(h)) {
        
    std::vector<uint8_t> payload =...
    UsrpMsg<std::vector> msg(h, payload);

}
*/

/**
 * @brief   Class for UsrpMetaTextMsg message
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2021-07-02
 */
 class UsrpMetaTextMsg : public Async::Msg
 {
   public:

     UsrpMetaTextMsg(uint32_t seq=0, uint32_t keyup=0, 
                     uint32_t talkgroup=0, uint32_t type=0) 
        : m_seq(htole32(seq)), m_keyup(keyup), 
          m_talkgroup(htole32(talkgroup)), m_type(htole32(type)) 
     {
       eye = {'U','S','R','P'};
       m_memory = 0;
       m_keyup = 0;
       m_talkgroup = 0;
       m_type = htobe32(USRP_TYPE_TEXT);
       m_mpxid = 0;
       m_reserved = 0;
     }
     
     /**
      * @brief 	Destructor
      */
     virtual ~UsrpMetaTextMsg(void) {}
     
     /**
      * public methods
      */
     uint32_t type(void) const { return ntohl(m_type); }
     uint32_t seq(void) const { return m_seq; }
     uint32_t tg(void) const { return ntohl(m_talkgroup); }
     uint32_t keyup(void) const { return ntohl(m_keyup); }
     uint32_t mpx(void) const { return ntohl(m_mpxid); }
     uint32_t res(void) const { return ntohl(m_reserved); }

     // return true if the message is TLV (first character == 0x08)
     bool isTlv(void) 
     { 
       return (m_meta == 0x08 ? true : false); 
     }
    
     ASYNC_MSG_MEMBERS(eye, m_seq, m_memory, m_keyup, m_talkgroup, 
                       m_type, m_mpxid, m_reserved, m_meta)

    private:
      std::array<char, 4> eye;
      uint32_t m_seq;
      uint32_t m_memory;
      uint32_t m_keyup;
      uint32_t m_talkgroup;          
      uint32_t m_type;
      uint32_t m_mpxid;
      uint32_t m_reserved;
      uint8_t m_meta;
}; /* class UsrpMetaTextMsg */
 

/**
 * @brief   Class for Usrp network Metadata message (TLV)
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2021-04-28
 */
class UsrpTlvMetaMsg : public Async::Msg
{
  public:

    UsrpTlvMetaMsg(uint32_t seq=0) : m_seq(htole32(seq))
      {
         eye = {'U','S','R','P'};
         m_memory = 0;
         m_keyup = 0;
         m_talkgroup = 0;
         m_type = htobe32(USRP_TYPE_TEXT);
         m_mpxid = 0;
         m_reserved = 0;
         m_tlv = TLV_TAG_SET_INFO;
         m_tlvlen = 0x13;
         m_dmrid = {0,0,0};
         m_rptid = 0;
         m_tg = {0,0,0};
         m_ts = 0;
         m_cc = 1;
         m_meta.fill(0);
       }

    /**
     * @brief 	Destructor
     */
    virtual ~UsrpTlvMetaMsg(void) {}

    // set the own talk group
    void setTg(uint32_t tg) 
    { 
      std::array<uint8_t, 3> t;
      t[0] = (tg >> 16) & 0xff;
      t[1] = (tg >> 8) & 0xff;
      t[2] = (tg >> 0) & 0xff;
      m_tg = t;
    }
    
    // set USRP_TYPE
    void setType(uint32_t type)
    {
      m_type = htobe32(type);   
    }
    
    // set the squence
    void setSeq(uint32_t seq) { m_seq = htole32(seq); }
    
    // set the own callsign
    void setCallsign(std::string call)
    {
      for (size_t i=0; i<call.length(); i++)
      {
        m_meta[i] = call.c_str()[i];
      }
      m_tlvlen = static_cast<int>(call.length() + 13) & 0xff;
    }
    
    // set Metadata
    void setMetaData(std::string metadata)
    {
    }
    
    // set DMR-ID
    void setDmrId(uint32_t dmrid)
    {
      std::array<uint8_t, 3> t;
      t[0] = (dmrid >> 16) & 0xff;
      t[1] = (dmrid >> 8) & 0xff;
      t[2] = (dmrid >> 0) & 0xff;
      m_dmrid = t;
    }

    // set the own repeater ID
    void setRptId(uint32_t rptid) { m_rptid = htole32(rptid); }
    
    // set the color code
    void setCC(uint8_t cc) { m_cc = cc; }
    
    // set Time slot
    void setTS(uint8_t ts) 
    {
      (ts > 4 ? m_ts = 4 : m_ts = ts);
    }

    void setTlv(uint8_t tlv)
    {
      m_tlv = tlv;
    }

    void setTlvLen(uint8_t tlvlen)
    {
      m_tlvlen = tlvlen;    
    }

    // returns the callsing of the talker
    std::string getCallsign(void)
    {
      uint8_t i = 0;
      uint8_t call[9];
      if (m_tlv == TLV_TAG_SET_INFO && m_tlvlen < 0x16)
      {
        for(i=0;i<(m_tlvlen-13);i++)
        {
          call[i] = m_meta[i];
          if (m_meta[i] == 0x00) break;
        }
      }
      return std::string(call, call+i);
    }

    // returns MetaData info
    std::string getMetaInfo(void)
    {
      uint8_t metainfo[306];
      uint8_t z;
      if (m_tlv == TLV_TAG_SET_INFO)
      {
        for (z=0;z<306;z++)
        {
          metainfo[z] = m_meta[z];
          if (m_meta[z] == 0x00) break;
        }
        return std::string(metainfo, metainfo+z);
      }
      return "";
    }
    
    // returns the TYPE of message
    uint32_t type(void) const { return ntohl(m_type); }
    
    // returns the current talkgroup of the talker
    uint32_t getTg(void)
    {
      return ((m_tg[0] << 16) + (m_tg[1] << 8) + (m_tg[2] & 0xff));
    }

    // returns the DMR ID of the talker
    uint32_t getDmrId(void)
    { 
      return (m_dmrid[2] & 0xff) + (m_dmrid[1] << 8) + (m_dmrid[0] << 16);
    }

    // getTlv
    uint8_t getTlv(void) { return m_tlv; }

    // getTS
    uint8_t getTS(void) { return (m_ts > 4 ? 4 : m_ts); }
    
    // getTlvLen
    uint8_t getTlvLen(void) { return m_tlvlen; }

    ASYNC_MSG_MEMBERS(eye, m_seq, m_memory, m_keyup, m_talkgroup, m_type, 
                      m_mpxid, m_reserved, m_tlv, m_tlvlen, m_dmrid, m_rptid,
                      m_tg, m_ts, m_cc, m_meta)
                      
  private:

    std::array<char, 4> eye;         // is always "USRP"
    uint32_t m_seq;                  // sequence number
    uint32_t m_memory;               // set to 0 - not used
    uint32_t m_keyup;                // Tx on/off (1/0)
    uint32_t m_talkgroup;            // current talkgroup
    uint32_t m_type;                 // type of frame
    uint32_t m_mpxid;                // set to 0 - not used
    uint32_t m_reserved;             // set to 0 - not used
    uint8_t  m_tlv;                  // see TLV_TAGS
    uint8_t  m_tlvlen;               // TLV-length of MetaData
    std::array<uint8_t, 3> m_dmrid;  // DMR-ID length 3 bytes
    uint32_t m_rptid;                // Repeater-ID length 4 bytes
    std::array<uint8_t, 3> m_tg;     // Talkgroup length 3 bytes
    uint8_t  m_ts;                   // Timeslot length 1 byte
    uint8_t  m_cc;                   // color code length 1 byte
    std::array<uint8_t, 306> m_meta; // padding the rest with 0x00
};

#endif /* USRP_MSG_INCLUDED */

/*
 * This file has not been truncated
 */
