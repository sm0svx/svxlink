/**
@file	 RtlTcp.h
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

/** @example RtlTcp_demo.cpp
An example of how to use the RtlTcp class
*/


#ifndef WBRX_RTL_TCP_INCLUDED
#define WBRX_RTL_TCP_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <complex>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2014-07-16

A_detailed_class_description

\include RtlTcp_demo.cpp
*/
class RtlTcp : public sigc::trackable
{
  public:
    typedef std::complex<float> Sample;
    
    static const int GAIN_UNSET = -1000;

    /**
     * @brief 	Default constructor
     */
    RtlTcp(const std::string &remote_host="localhost", uint16_t remote_port=1234)
      : con(remote_host, remote_port, BLOCK_SIZE), tuner_type(0),
        center_fq_set(false), center_fq(100000000), samp_rate_set(false),
        samp_rate(2048000), gain_mode(-1), gain(GAIN_UNSET), fq_corr_set(false),
        fq_corr(0), test_mode_set(false), test_mode(false),
        use_digital_agc_set(false), use_digital_agc(false)
    {
      con.dataReceived.connect(mem_fun(*this, &RtlTcp::dataReceived));
      con.connected.connect(mem_fun(*this, &RtlTcp::connected));
      con.disconnected.connect(mem_fun(*this, &RtlTcp::disconnected));
      con.connect();

      for (int i=0; i<MAX_IF_GAIN_STAGES; ++i)
      {
        tuner_if_gain[i] = GAIN_UNSET;
      }
    }

  
    /**
     * @brief 	Destructor
     */
    ~RtlTcp(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void setCenterFq(uint32_t fq)
    {
      center_fq = fq;
      center_fq_set = true;
      sendCommand(1, fq);
    }

    void setSampleRate(uint32_t rate)
    {
      samp_rate = rate;
      samp_rate_set = true;
      sendCommand(2, rate);
    }

    void setGainMode(uint32_t mode)
    {
      gain_mode = mode;
      sendCommand(3, mode);
    }

    void setGain(uint32_t gain)
    {
      this->gain = gain;
      sendCommand(4, gain);
    }

    void setFqCorr(uint32_t corr)
    {
      fq_corr = corr;
      fq_corr_set = true;
      sendCommand(5, corr);
    }

    void setTunerIfGain(uint16_t stage, int16_t gain)
    {
      if (stage >= MAX_IF_GAIN_STAGES)
      {
        return;
      }
      tuner_if_gain[stage] = gain;
      uint32_t param = stage;
      param <<= 16;
      param |= (uint16_t)gain;
      sendCommand(6, param);
    }

    void enableTestMode(bool enable)
    {
      test_mode = enable;
      test_mode_set = true;
      sendCommand(7, enable ? 1 : 0);
    }

    void enableDigitalAgc(bool enable)
    {
      use_digital_agc = enable;
      use_digital_agc_set = true;
      sendCommand(8, enable ? 1 : 0);
    }

    /* Missing commands:
     *   9 - set direct sampling
     *   a - set offset tuning
     *   b - set rtl xtal
     *   c - set tuner xtal
     *   d - set tuner gain by index
     */

    sigc::signal<void, std::vector<Sample> > iqReceived;

    
  protected:
    
  private:
    static const int BLOCK_SIZE = 8 * 2 * 960;
    static const unsigned MAX_IF_GAIN_STAGES = 10;

    Async::TcpClient con;
    uint32_t  tuner_type;
    uint32_t  tuner_gain_count;
    bool      center_fq_set;
    uint32_t  center_fq;
    bool      samp_rate_set;
    uint32_t  samp_rate;
    int32_t   gain_mode;
    uint32_t  gain;
    bool      fq_corr_set;
    uint32_t  fq_corr;
    int       tuner_if_gain[MAX_IF_GAIN_STAGES];
    bool      test_mode_set;
    bool      test_mode;
    bool      use_digital_agc_set;
    bool      use_digital_agc;

    RtlTcp(const RtlTcp&);
    RtlTcp& operator=(const RtlTcp&);
    void sendCommand(char cmd, uint32_t param)
    {
      if (con.isConnected())
      {
        std::cout << "### sendCommand(" << (int)cmd << ", " << param << ")" << std::endl;
        char msg[5];
        msg[0] = cmd;
        msg[4] = param & 0xff;
        msg[3] = (param >> 8) & 0xff;
        msg[2] = (param >> 16) & 0xff;
        msg[1] = (param >> 24) & 0xff;
        con.write(msg, sizeof(msg));
      }
    }

    void connected(void)
    {
      std::cout << "Connected\n";
    }

    void disconnected(Async::TcpConnection *c, Async::TcpConnection::DisconnectReason reason)
    {
      std::cout << "Disconnected\n";
    }

    void updateSettings(void)
    {
      if (samp_rate_set)
      {
        setSampleRate(samp_rate);
      }
      if (fq_corr_set)
      {
        setFqCorr(fq_corr);
      }
      if (center_fq_set)
      {
        setCenterFq(center_fq);
      }
      if (gain_mode >= 0)
      {
        setGainMode(gain_mode);
      }
      if ((gain_mode == 1) && (gain > GAIN_UNSET))
      {
        setGain(gain);
      }
      for (int i=0; i<MAX_IF_GAIN_STAGES; ++i)
      {
        if (tuner_if_gain[i] > GAIN_UNSET)
        {
          setTunerIfGain(i, tuner_if_gain[i]);
        }
      }
      if (test_mode_set)
      {
        enableTestMode(test_mode);
      }
      if (use_digital_agc_set)
      {
        enableDigitalAgc(use_digital_agc);
      }
    }

    int dataReceived(Async::TcpConnection *con, void *buf, int count)
    {
      //std::cout << "### Data received: " << count << " bytes\n";
      if (tuner_type == 0)
      {
        if (count < 12)
        {
          return 0;
        }
        char *ptr = reinterpret_cast<char *>(buf);
        if (strncmp(ptr, "RTL0", 4) != 0)
        {
          std::cout << "*** ERROR: Expected magic RTL0\n";
          exit(1);
        }
        tuner_type = ntohl(*reinterpret_cast<uint32_t *>(ptr+4));
        tuner_gain_count = ntohl(*reinterpret_cast<uint32_t *>(ptr+8));
        std::cout << "### tuner_type=" << tuner_type
             << " tuner_gain_count=" << tuner_gain_count << std::endl;
        updateSettings();
        return 12;
      }

      if (count < BLOCK_SIZE)
      {
        return 0;
      }
      count = BLOCK_SIZE;

      int samp_count = count / 2;
      std::complex<uint8_t> *samples = reinterpret_cast<std::complex<uint8_t>*>(buf);
      std::vector<Sample> iq;
      iq.reserve(samp_count);
      for (int idx=0; idx<samp_count; ++idx)
      {
        float i = samples[idx].real();
        i = i / 127.5f - 1.0f;
        float q = samples[idx].imag();
        q = q / 127.5f - 1.0f;
        iq.push_back(std::complex<float>(i, q));
      }
      iqReceived(iq);

        // FIXME: Returning 0 terminates the connection. This have to be
        // handled. Fix Async::TcpConnection?
      return 2 * samp_count;
    }
    
};  /* class RtlTcp */



//} /* namespace */

#endif /* WBRX_RTL_TCP_INCLUDED */



/*
 * This file has not been truncated
 */
