/**
@file	 NetRxMsg.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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

/** @example NetRxMsg_demo.cpp
An example of how to use the NetRxMsg class
*/


#ifndef NET_RX_MSG_INCLUDED
#define NET_RX_MSG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cassert>


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

namespace NetRxMsg
{


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

#define NET_RX_DEFAULT_TCP_PORT   "5210"
#define NET_RX_DEFAULT_UDP_PORT   NET_RX_DEFAULT_TCP_PORT


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2006-04-14

A_detailed_class_description
*/
class Msg
{
  public:
    /**
     * @brief 	Default constuctor
     */
    Msg(unsigned type, unsigned size) : m_type(type), m_size(size) {}
  
    /**
     * @brief 	Destructor
     */
    ~Msg(void) {}
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
     unsigned type(void) const { return m_type; }
     unsigned size(void) const { return m_size; }
    
  protected:
    
  private:
    unsigned m_type;
    unsigned m_size;
    
    Msg(const Msg&);
    Msg& operator=(const Msg&);
    
};  /* class Msg */


class MsgHeartbeat : public Msg
{
  public:
    static const int TYPE = 0;
    MsgHeartbeat(void) : Msg(TYPE, sizeof(MsgHeartbeat)) {}
    
};  /* MsgHeartbeat */


class MsgAuth : public Msg
{
  public:
    static const int TYPE = 1;
    MsgAuth(const char *password)
      : Msg(TYPE, sizeof(MsgAuth))
    {
      strncpy(m_password, password, sizeof(m_password));
      m_password[sizeof(m_password)-1] = 0;
    }
    
    const char *password(void) const { return m_password; }
  
  private:
    char m_password[16];
    
}; /* MsgAuth */


class MsgSquelch : public Msg
{
  public:
    static const int TYPE = 2;
    MsgSquelch(bool is_open, float signal_strength, int sql_rx_id)
      : Msg(TYPE, sizeof(MsgSquelch)), m_is_open(is_open),
      	m_signal_strength(signal_strength), m_sql_rx_id(sql_rx_id) {}
    bool isOpen(void) const { return m_is_open; }
    float signalStrength(void) const { return m_signal_strength; }
    int sqlRxId(void) const { return m_sql_rx_id; }
  
  private:
    bool  m_is_open;
    float m_signal_strength;
    int   m_sql_rx_id;
    
}; /* MsgSquelch */


class MsgDtmf : public Msg
{
  public:
    static const int TYPE = 3;
    MsgDtmf(char digit)
      : Msg(TYPE, sizeof(MsgDtmf)), m_digit(digit) {}
    char digit(void) const { return m_digit; }
  
  private:
    char  m_digit;
    
}; /* MsgDtmf */


class MsgTone : public Msg
{
  public:
    static const int TYPE = 4;
    MsgTone(float tone_fq)
      : Msg(TYPE, sizeof(MsgTone)), m_tone_fq(tone_fq) {}
    float toneFq(void) const { return m_tone_fq; }
  
  private:
    float  m_tone_fq;
    
}; /* MsgTone */


class MsgAudio : public Msg
{
  public:
    static const int TYPE = 5;
    MsgAudio(float *samples, int count)
      : Msg(TYPE, sizeof(MsgAudio))
    {
      assert(count <= 512);
      memcpy(m_samples, samples, count * sizeof(*samples));
      m_count = count;
    }
    float *samples(void) { return m_samples; }
    int count(void) const { return m_count; }
  
  private:
    float m_samples[512];
    int   m_count;
    
}; /* MsgAudio */


class MsgMute : public Msg
{
  public:
    static const int TYPE = 6;
    MsgMute(bool do_mute)
      : Msg(TYPE, sizeof(MsgMute)), m_do_mute(do_mute) {}
    int doMute(void) const { return m_do_mute; }
  
  private:
    char  m_do_mute;
    
}; /* MsgMute */


class MsgAddToneDetector : public Msg
{
  public:
    static const int TYPE = 7;
    MsgAddToneDetector(float fq, int bw, float thresh, int required_duration)
      : Msg(TYPE, sizeof(MsgAddToneDetector)), m_fq(fq), m_bw(bw),
      	m_thresh(thresh), m_required_duration(required_duration) {}
    float fq(void) const { return m_fq; }
    int bw(void) const { return m_bw; }
    float thresh(void) const { return m_thresh; }
    int requiredDuration(void) const { return m_required_duration; }
  
  private:
    float m_fq;
    int   m_bw;
    float m_thresh;
    int   m_required_duration;
    
}; /* MsgAddToneDetector */


class MsgReset : public Msg
{
  public:
    static const int TYPE = 8;
    MsgReset(void) : Msg(TYPE, sizeof(MsgReset)) {}
    
}; /* MsgReset */




} /* namespace */


#endif /* NET_RX_MSG_INCLUDED */



/*
 * This file has not been truncated
 */

