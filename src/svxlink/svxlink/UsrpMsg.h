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
 * @brief   The base class for Usrp network messages
 * @author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
 * @date    2021-04-28

 */
class UsrpMsg : public Async::Msg
{
  public:
        
    UsrpMsg(uint32_t seq=0, uint32_t memory=0, uint32_t keyup=0,
            uint32_t talkgroup=0, uint32_t type=0, uint32_t mpxid=0,
            uint32_t reserved=0, const std::array<int16_t, 160> audio_data={0})
      : m_seq(type), m_memory(memory), m_keyup(keyup), m_talkgroup(talkgroup),
        m_type(type), m_mpxid(mpxid), m_reserved(reserved), 
        m_audio_data(audio_data) 
        {
          eye = {'U','S','R','P'};
        }

    /**
     * @brief 	Destructor
     */
    virtual ~UsrpMsg(void) {}

    uint32_t type(void) const { return m_type; }
    uint32_t seq(void) const { return m_seq; }
    uint32_t memory(void) const { return m_memory; }
    uint32_t keyup(void) const { return m_keyup; }
    uint32_t mpxid(void) const { return m_mpxid; }
    uint32_t reserved(void) const { return m_reserved; }
    uint32_t talkgroup(void) const { return m_talkgroup; }
    
    void setTg(uint32_t tg) { m_talkgroup = tg;}
    void setType(uint32_t type) { m_type = type;}
    void setSeq(uint32_t seq) { m_seq = seq; }
    void setKeyup(bool keyup) { (keyup ? m_keyup=1 : m_keyup=0); }
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

#endif /* USRP_MSG_INCLUDED */

/*
 * This file has not been truncated
 */
