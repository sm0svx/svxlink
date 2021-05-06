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


USRP protocol description provided by  Waldek/SP2ONG
USRP protocol used to send via UDP analog audio (2021.05.05)

The USRP was implemented in Allstar (in 2010, KA1RBI) and used by various
hamradio applications like
MMDVMHost, USRP2M17, Analog_Bridge, Analog_Reflector etc

UDP USRP FRAME:
==============
USRP_HEADER (32 bytes) + DATA


Typical sequence for send audio:

USRP_HEADER (Type_Frame=2, PTT=0) + DATA (The first byte is set 0x08 (TLV TAG
see below) and contain MetaDATA (see below))
USRP_HEADER (Type_Frame=0, PTT=1) + DATA (Audio)
....
USRP_HEADER (Type_Frame=0, PTT=0)


The first frame type = 2 (USRP_TYPE_TEXT) with MetaData is not obligatory and 
can be omitted, but it can be useful when,
for example, we want to send information about the callsign source from the
analogue to for example M17 or callsign and its DMR ID to DMR.

At current in USRP header are used only the following information:
   USRP string in 0-3 bytes
   Sequnece number (4-7 bytes)
   PTT status  (15 byte) 1 = ON , 0 = OFF
   Type of Frame (20 byte) 0 Audio data, 2 MetaData

The rest of the information in USRP header should be set to 0

USRP Header:
==============================
Bytes  Value            Notes
0-3    USRP             string char: USRP
4-7    Sequence number
8-11   Memory?          Set to 0
12-15  PTT              PTT value is stored in 15 byte. 1 for PTT ON start
                        transmission, 0 for PTT OFF stop transmission
16-19  Talkgroup?       Set to 0
20-23  Type_Frame       Type frame is stored in 20 byte. 0 for Audio
                        (USRP_TYPE_VOICE), 2 for MetaData (USRP_TYPE_TEXT)
24-27  Mpxid?           Set to 0
28-31  Reserved?        Set to 0

The next part from 32 Bytes is contained AUDIO data or MetaData.

Audio signed 16-bit little-endian PCM audio at 8000 samples/sec, 160 samples
per packet.

USRP_VOICE_FRAME_SIZE (160*sizeof(short))  // 0.02 * 8k

Dump with USRP Audio data PTT ON = 1

0x0000:  4500 017c 524b 4000 4011 e923 7f00 0001  E..|RK@.@..#....
0x0010:  7f00 0001 d432 d431 0168 ff7b 5553 5250  .....2.1.h.{USRP
0x0020:  0000 000b 0000 0000 0000 0001 0000 0000  ................
0x0030:  0000 0000 0000 0000 0000 0000 eaff efff  ................
0x0040:  d5ff c3ff c4ff c6ff c9ff caff e1ff eeff  ................

The last USRP frame PTT OFF = 0

0x0000:  4500 017c 9b6d 4000 4011 a001 7f00 0001  E..|.m@.@.......
0x0010:  7f00 0001 d432 d431 0168 ff7b 5553 5250  .....2.1.h.{USRP
0x0020:  0000 0050 0000 0000 0000 0000 0000 0000  ...P............
0x0030:  0000 0000 0000 0000 0000 0000 0000 0000  ................
0x0040:  0000 0000 0000 0000 0000 0000 0000 0000  ................


The PTT status in the USRP of the header can be used to control PTT or 
SQL, or information about start and end transmission in an application that 
is receiving data via USRP.


The DATA type USRP_TYPE_TEXT
=============================

If the first byte of USRP_TYPE_TEXT start with 0x08 (see below TLV Tags number) 
contain MetaDATA like DMIR ID, Repeater ID, Talk Group, Tiem Slot, Color Code, 
Callsign

MetaData for TLV_Tag 8 is follwoing:

0x08,TLVLenght,dmrID, repeaterID, tg, ts, cc, callsign, 0

TLVLength = 3 + 4 + 3 + 1 + 1 + len(callsign) + 1


USRP_TYPE_TEXT with TLV_TAG 8 from Analog Bridge version 1.6.1 and USRP2M17
=================================================================================
TLV Tags         Set 0x08 (see TLV_TAGS)
TLV Length       Value of lenght of MetaData where TLVLenght = 3 + 4 + 3 + 1 +
                 1 + len(call) + 1 
DMR Id           Lenght 3 bytes. Set 0 or ((dmr_id >> 16) & 0xff),((dmr_id >> 8)
                 & 0xff),(dmr_id & 0xff)
RPT Id           Lenght 4 bytes. Set 0 or ((rpt_id >> 32) & 0xff),((rpt_id >> 16)
                 & 0xff),((rpt_id >> 8) & 0xff),(rpt_id & 0xff)
TalkGroup        Lenght 3 bytes. Set 0 or ((tg >> 16) & 0xff),((tg >> 8) & 0xff),
                 (tg & 0xff)
TimeSlot#        Length 1 byte. Set 0 or Time Slot number
ColorCode        Length 1 byte. Set 0 or value of ColorCode
Callsing         Put the callisgn
End of MetData   Set 0

Minimum information in Metadata is Callsign for example when we exchanged USRP
data between Analog and M17. For DMR it will be useful to add DMR ID for callsign

Example dump of MetaData from Analog Bridge v1.6.2 Type of frmae USRP_TYPE_TXT
(2) with TLV TAG = 8 and TVL Length = 0x13 for callsign N0CALL


0x0000:  0000 0000 0000 0000 0000 0000 0800 4500  ..............E.
0x0010:  017c cbb9 4000 4011 6fb5 7f00 0001 7f00  .|..@.@.o.......
0x0020:  0001 8613 8614 0168 ff7b 5553 5250 0000  .......h.{USRP..
0x0030:  00da 0000 0000 0000 0000 0000 0000 0200  ................
0x0040:  0000 0000 0000 0000 0000 0813 12d6 8700  ................
0x0050:  0000 0000 0000 0000 4e30 4341 4c4c 0000  ........N0CALL..
0x0060:  0000 0000 0000 0000 0000 0000 0000 0000  ................


Below example dump from Analog Reflector USRP_TYPE_TXT with TLV TAG = 8
Analog Reflector use longer TLV (0x15) compare to Analog Bridge (0x13) for
example for the same length callsign N0CALL
but I did not find information as to what the extra 2 bytes are used in AR

0x0000:  0000 0000 0000 0000 0000 0000 0800 4500  ..............E.
0x0010:  017c 5f49 4000 4011 dc25 7f00 0001 7f00  .|_I@.@..%......
0x0020:  0001 d432 d431 0168 ff7b 5553 5250 0000  ...2.1.h.{USRP..
0x0030:  0000 0000 0000 0000 0000 0000 0000 0200  ................
0x0040:  0000 0000 0000 0000 0000 0815 12d6 8700  ............'.Z.
0x0050:  0000 0000 0000 0000 4e30 4341 4c4c 0000  ........N0CALL..
0x0060:  0000 0000 0000 0000 0000 0000 0000 0000  ................

Remarks: The Analog Bridge v1.6.2 set in field CALLSIGN information like:
{"call":"N3ABC","name":"Club station"} if is connected with Analog Refelctor

Other information

TYPE of USRP frames
#######################
USRP_TYPE_VOICE = 0  (Audio data)
USRP_TYPE_DTMF = 1
USRP_TYPE_TEXT = 2   (Meta data)
USRP_TYPE_PING = 3
USRP_TYPE_TLV = 4
USRP_TYPE_VOICE_ADPCM = 5
USRP_TYPE_VOICE_ULAW = 6

TLV tags
#######################
TLV_TAG_BEGIN_TX    = 0
TLV_TAG_AMBE        = 1
TLV_TAG_END_TX      = 2
TLV_TAG_TG_TUNE     = 3
TLV_TAG_PLAY_AMBE   = 4
TLV_TAG_REMOTE_CMD  = 5
TLV_TAG_AMBE_49     = 6
TLV_TAG_AMBE_72     = 7
TLV_TAG_SET_INFO    = 8
TLV_TAG_IMBE        = 9
TLV_TAG_DSAMBE      = 10
TLV_TAG_FILE_XFER   = 11

Information base on:
https://github.com/AllStarLink/ASL-Asterisk/blob/develop/asterisk/channels/chan_usrp.h
https://github.com/AllStarLink/ASL-Asterisk/blob/develop/asterisk/channels/chan_usrp.c
https://github.com/DVSwitch/MMDVM_Bridge/blob/master/dvswitch.sh
https://github.com/DVSwitch/USRP_Client/blob/master/pyUC.py
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

static const int      USRP_AUDIO_FRAME_LEN               = 160;

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
      : m_seq(htole32(seq)), m_memory(memory), m_keyup(keyup), 
        m_talkgroup(talkgroup), m_type(htonl(type)), m_mpxid(mpxid), 
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
    uint32_t keyup(void) const { return m_keyup; }
    uint32_t mpxid(void) const { return m_mpxid; }
    uint32_t reserved(void) const { return m_reserved; }
    uint32_t talkgroup(void) const { return m_talkgroup; }

    void setTg(uint32_t tg) { m_talkgroup = htonl(tg);}
    void setType(uint32_t type) { m_type = htonl(type);}
    void setSeq(uint32_t seq) { m_seq = seq; }
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
      : m_seq(htole32(seq)), m_keyup(htole32(keyup)),
             m_talkgroup(talkgroup), m_type(type) 
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
    virtual ~UsrpHeaderMsg(void) {}
    
    uint32_t type(void) const { return ntohl(m_type); }
    uint32_t seq(void) const { return ntohl(m_seq); }
    uint32_t memory(void) const { return m_memory; }
    bool keyup(void) const { return (ntohl(m_keyup) & 1 ? true : false); }
    uint32_t mpxid(void) const { return m_mpxid; }
    uint32_t reserved(void) const { return m_reserved; }
    uint32_t talkgroup(void) const { return ntohl(m_talkgroup); }

    void setTg(uint32_t tg) { m_talkgroup = htole32(tg); }
    void setSeq(uint32_t seq) { m_seq = htole32(seq); }

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
      : m_seq(htole32(seq)), m_talkgroup(htole32(talkgroup))
        {
          eye = {'U','S','R','P'};
          m_memory = 0;
          m_keyup = 0;
          m_type = htobe32(0x02);
          m_mpxid = 0;
          m_reserved = 0;
          m_tlv = 0x08;
          m_tlvlen = 0x13;
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

    // set the own talk group
    void setTg(uint32_t tg) 
    { 
      m_talkgroup = htole32(tg);
      std::array<uint8_t, 3> t;
      t[0] = (tg >> 16) & 0xff;
      t[1] = (tg >> 8) & 0xff;
      t[2] = (tg >> 0) & 0xff;
      m_tg = t;
    }
    
    // set the squence
    void setSeq(uint32_t seq) { m_seq = htole32(seq); }
    
    // set the own callsign
    void setCallsign(std::string call)
    {
      for (size_t i=0; i<call.length(); i++)
      {
        m_callsign[i] = call.c_str()[i];
      }
      m_tlvlen = static_cast<int>(call.length() + 13) & 0xff;
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

    // returns the callsing of the talker
    std::string getCallsign(void)
    { 
      return std::string(m_callsign.begin(), m_callsign.end());
    }
    
    // returns the current talkgroup of the talker
    uint32_t getTg(void)
    {
      return m_talkgroup;
    }
    
    // returns the DMR ID of the talker
    uint32_t getDmrId(void)
    { 
      return (m_dmrid[2] & 0xff) + (m_dmrid[1] << 8) + (m_dmrid[0] << 16);
    }
    
    ASYNC_MSG_MEMBERS(eye, m_seq, m_memory, m_keyup, m_talkgroup, m_type, 
                      m_mpxid, m_reserved, m_tlv, m_tlvlen, m_dmrid, m_rptid,
                      m_tg, m_ts, m_cc, m_callsign, m_rest)
                      
  private:

    std::array<char, 4> eye;         // is always "USRP"
    uint32_t m_seq;                  // sequence number
    uint32_t m_memory;               // set to 0 - not used
    uint32_t m_keyup;                 // Tx on/off (1/0)
    uint32_t m_talkgroup;            // current talkgroup
    uint32_t m_type;                  // type of frame
    uint32_t m_mpxid;                // set to 0 - not used
    uint32_t m_reserved;             // set to 0 - not used
    uint8_t  m_tlv;                  // see TLV_TAGS
    uint8_t  m_tlvlen;               // TLV-length of MetaData
    std::array<uint8_t, 3> m_dmrid;  // DMR-ID length 3 bytes
    uint32_t m_rptid;                // Repeater-ID length 4 bytes
    std::array<uint8_t, 3> m_tg;     // Talkgroup length 3 bytes
    uint8_t  m_ts;                   // Timeslot length 1 byte
    uint8_t  m_cc;                   // color code length 1 byte
    std::array<char, 6> m_callsign;  // own callsign
    std::array<uint8_t, 300> m_rest; // padding the rest with 0x00
};

#endif /* USRP_MSG_INCLUDED */

/*
 * This file has not been truncated
 */
