/**
@file	 NetTrxMsg.h
@brief   Network messages for remote transceivers
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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


#ifndef NET_TRX_MSG_INCLUDED
#define NET_TRX_MSG_INCLUDED


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

#include <Tx.h>


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

namespace NetTrxMsg
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

#define NET_TRX_DEFAULT_TCP_PORT   "5210"
#define NET_TRX_DEFAULT_UDP_PORT   NET_TRX_DEFAULT_TCP_PORT


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

#pragma pack(push, 1)

/**
@brief	Base class for remote transceiver network messages
@author Tobias Blomberg / SM0SVX
@date   2006-04-14
*/
class Msg
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	type The message type
     * @param 	size The message size
     */
    Msg(unsigned type, unsigned size) : m_type(type), m_size(size) {}
  
    /**
     * @brief 	Destructor
     */
    ~Msg(void) {}
  
    /**
     * @brief 	Get the message type
     * @return	Returns the message type
     */
     unsigned type(void) const { return m_type; }
  
    /**
     * @brief 	Get the message size
     * @return	Returns the message size
     */
     unsigned size(void) const { return m_size; }
    
  protected:
  
    /**
     * @brief 	Set the message size
     * @param 	size The new size of the message in bytes
     */
    void setSize(unsigned size) { m_size = size; }
        
  private:
    unsigned m_type;
    unsigned m_size;
    
    Msg(const Msg&);
    Msg& operator=(const Msg&);
    
};  /* class Msg */



/****************************** Common Messages ******************************/

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


class MsgAudio : public Msg
{
  public:
    static const int TYPE = 2;
    static const int MAX_COUNT = 512;
    MsgAudio(float *samples, int count)
      : Msg(TYPE, sizeof(MsgAudio) - sizeof(*samples) * (MAX_COUNT - count))
    {
      assert(count <= MAX_COUNT);
      memcpy(m_samples, samples, count * sizeof(*samples));
      m_count = count;
    }
    float *samples(void) { return m_samples; }
    int count(void) const { return m_count; }
  
  private:
    int   m_count;
    float m_samples[MAX_COUNT];
    
}; /* MsgAudio */



/******************************** RX Messages ********************************/

class MsgMute : public Msg
{
  public:
    static const int TYPE = 100;
    MsgMute(bool do_mute)
      : Msg(TYPE, sizeof(MsgMute)), m_do_mute(do_mute) {}
    int doMute(void) const { return m_do_mute; }
  
  private:
    char  m_do_mute;
    
}; /* MsgMute */


class MsgAddToneDetector : public Msg
{
  public:
    static const int TYPE = 101;
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
    static const int TYPE = 102;
    MsgReset(void) : Msg(TYPE, sizeof(MsgReset)) {}
    
}; /* MsgReset */




class MsgSquelch : public Msg
{
  public:
    static const int TYPE = 150;
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
    static const int TYPE = 151;
    MsgDtmf(char digit, int duration)
      : Msg(TYPE, sizeof(MsgDtmf)), m_digit(digit), m_duration(duration) {}
    char digit(void) const { return m_digit; }
    int duration(void) const { return m_duration; }
  
  private:
    char  m_digit;
    int   m_duration;
    
}; /* MsgDtmf */


class MsgTone : public Msg
{
  public:
    static const int TYPE = 152;
    MsgTone(float tone_fq)
      : Msg(TYPE, sizeof(MsgTone)), m_tone_fq(tone_fq) {}
    float toneFq(void) const { return m_tone_fq; }
  
  private:
    float  m_tone_fq;
    
}; /* MsgTone */





/******************************** TX Messages ********************************/

class MsgSetTxCtrlMode : public Msg
{
  public:
    static const int TYPE = 200;
    MsgSetTxCtrlMode(Tx::TxCtrlMode mode)
      : Msg(TYPE, sizeof(MsgSetTxCtrlMode)), m_mode(mode) {}
    Tx::TxCtrlMode mode(void) const { return m_mode; }
  
  private:
    Tx::TxCtrlMode m_mode;
    
}; /* MsgSetTxCtrlMode */


class MsgEnableCtcss : public Msg
{
  public:
    static const int TYPE = 201;
    MsgEnableCtcss(bool enable)
      : Msg(TYPE, sizeof(MsgEnableCtcss)), m_enable(enable) {}
    bool enable(void) const { return m_enable; }
  
  private:
    bool m_enable;
        
}; /* MsgEnableCtcss */


class MsgSendDtmf : public Msg
{
  public:
    static const int TYPE = 202;
    static const int MAX_DIGITS = 256;
    MsgSendDtmf(const std::string &digits)
      : Msg(TYPE, sizeof(MsgSendDtmf))
    {
      strncpy(m_digits, digits.c_str(), MAX_DIGITS);
      m_digits[MAX_DIGITS] = 0;
      setSize(size() - MAX_DIGITS + strlen(m_digits));
    }
    std::string digits(void) const { return m_digits; }
  
  private:
    char  m_digits[MAX_DIGITS+1];
    
}; /* MsgSendDtmf */


class MsgFlush : public Msg
{
  public:
    static const int TYPE = 203;
    MsgFlush(void)
      : Msg(TYPE, sizeof(MsgFlush)) {}
}; /* MsgFlush */




class MsgTxTimeout : public Msg
{
  public:
    static const int TYPE = 250;
    MsgTxTimeout(void)
      : Msg(TYPE, sizeof(MsgTxTimeout)) {}
}; /* MsgTxTimeout */


class MsgTransmitterStateChange : public Msg
{
  public:
    static const int TYPE = 251;
    MsgTransmitterStateChange(bool is_transmitting)
      : Msg(TYPE, sizeof(MsgTransmitterStateChange)),
      	m_is_transmitting(is_transmitting) {}
    bool isTransmitting(void) const { return m_is_transmitting; }
  
  private:
    bool m_is_transmitting;
    
}; /* MsgTxTimeout */


class MsgAllSamplesFlushed : public Msg
{
  public:
    static const int TYPE = 252;
    MsgAllSamplesFlushed(void)
      : Msg(TYPE, sizeof(MsgAllSamplesFlushed)) {}
}; /* MsgTxTimeout */


#pragma pack(pop)



} /* namespace */


#endif /* NET_TRX_MSG_INCLUDED */



/*
 * This file has not been truncated
 */

