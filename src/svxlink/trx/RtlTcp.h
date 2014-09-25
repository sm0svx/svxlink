/**
@file	 RtlTcp.h
@brief   An interface class for communicating to the rtl_tcp utility
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

#ifndef RTL_TCP_INCLUDED
#define RTL_TCP_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>
#include <complex>
#include <string>
#include <vector>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncTimer.h>


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
@brief	An interface class for communicating to the rtl_tcp utility
@author Tobias Blomberg / SM0SVX
@date   2014-07-16

Use this class to open and use a network connection to the rtl_tcp utility.
*/
class RtlTcp : public sigc::trackable
{
  public:
    typedef std::complex<float> Sample;

    typedef enum {
	TUNER_UNKNOWN = 0,
	TUNER_E4000,
	TUNER_FC0012,
	TUNER_FC0013,
	TUNER_FC2580,
	TUNER_R820T
    } TunerType;

    static const int GAIN_UNSET = 1000;

    /**
     * @brief 	Constructor
     * @param   remote_host The remote host to connect to
     * @param   remote_port The TCP port to connect to
     */
    RtlTcp(const std::string &remote_host="localhost",
           uint16_t remote_port=1234);
  
    /**
     * @brief 	Destructor
     */
    ~RtlTcp(void) {}
  
    /**
     * @brief 	Enable printing of distorsion warnings
     * @param 	enable Set to \em true to enable printing
     */
    void enableDistPrint(bool enable);

    /**
     * @brief   Set the center frequency of the tuner
     * @param   fq The new center frequency, in Hz, to set
     */
    void setCenterFq(uint32_t fq);

    /**
     * @brief   Read the currently set center frequency
     * @returns Returns the currently set tuner frequency in Hz
     */
    uint32_t centerFq(void) { return center_fq; }

    /**
     * @brief   Set the tuner sample rate
     * @param   rate The new sample, in Hz, rate to set
     */
    void setSampleRate(uint32_t rate);

    /**
     * @brief   Get the currently set sample rate
     * @returns Returns the currently set sample rate in Hz
     */
    uint32_t sampleRate(void) const { return samp_rate; }

    /**
     * @brief   Set the gain mode
     * @param   mode The gain mode to set: 0=automatic, 1=manual
     *
     * Use this function to choose if automatic or manual gain mode should
     * be used. When set to manual, the setGain function can be used to set
     * the desired gain.
     */
    void setGainMode(uint32_t mode);

    /**
     * @brief   Set manual gain
     * @param   gain The gain in tenths of a dB to set (105=10.5dB)
     *
     * Use this function to set the gain when manual gain mode is selected.
     * Set the gain mode using the setGainMode function.
     */
    void setGain(int32_t gain);

    /**
     * @brief   Set frequency correction factor
     * @param   corr The frequency correction factor in PPM
     *
     * Use this function to set the frequency correction factor for the tuner.
     * The correction factor is given in parts per million (PPM). That is,
     * how many Hz per MHz the tuner is off.
     */
    void setFqCorr(uint32_t corr);

    /**
     * @brief   Set tuner IF gain for the specified stage
     * @param   stage The number of the gain stage to set
     * @param   gain The gain in tenths of a dB to set (105=10.5dB)
     *
     * Use this function to set the IF gain for a specific stage in the tuner.
     * How many stages that are available is tuner dependent. For example,
     * the E4000 tuner have stages 1 to 6.
     */
    void setTunerIfGain(uint16_t stage, int16_t gain);

    /**
     * @brief   Enable or disable test mode
     * @param   enable Set to \em true to enable testing
     *
     * Use this function to enable a testing mode in the tuner. Instead of
     * returning real samples the tuner will return a 8 bit counter value
     * instead. This can be used to verify that no samples are dropped.
     */
    void enableTestMode(bool enable);

    /**
     * @brief   Enable or disable the digital AGC of the RTL2832
     * @param   enable Set to \em true to enable the digital AGC
     */
    void enableDigitalAgc(bool enable);

    /* Missing commands:
     *   9 - set direct sampling
     *   a - set offset tuning
     *   b - set rtl xtal
     *   c - set tuner xtal
     *   d - set tuner gain by index
     */

    /**
     * @brief   Return the tuner type
     * @returns Returns a number representing the tuner type
     *
     * Use this function to return the tuner type. The tuner type is available
     * after the connection has been estbalished with rtl_tcp. To get a string
     * representation of the tuner type, use the tunerTypeString function.
     * To compare to a specific numerical tuner type, use the TunerType enum.
     */
    TunerType tunerType(void) const { return tuner_type; }

    /**
     * @brief   Get the tuner type as a string
     * @param   type The tuner type to convert to a string
     * @returns Returns a string representation of the tuner type
     */
    const char *tunerTypeString(TunerType type) const;

    /**
     * @brief   Get the tuner type as a string
     * @returns Returns a string representation of the tuner type
     *
     * Use this function to return a string representation of the tuner type.
     * The tuner type is available after the connection has been estbalished
     * with rtl_tcp.
     * To compare to a specific numerical tuner type, use the tunerType
     * function and the TunerType enum instead.
     */
    const char *tunerTypeString(void) const
    {
      return tunerTypeString(tuner_type);
    }

    /**
     * @brief   Get a list of tuner gains available for the current tuner
     * @returns Returns a vector of available tuner gains
     *
     * This function will return a list of tuner gains that are available for
     * the current tuner. The connection must have been established with the
     * tuner before calling this function.
     * The gain values are given in tenhts of a dB so 105=10.5dB.
     */
    std::vector<int> getTunerGains(void) const;

    /**
     * @brief   A signal that is emitted when new samples have been received
     * @param   samples A vector of received samples
     *
     * Connecting to this signal is the way to get samples from the DVB-T
     * dongle. The format is a vector of complex floats (I/Q) with a range from
     * -1 to 1.
     */
    sigc::signal<void, std::vector<Sample> > iqReceived;
    
  protected:
    
  private:
    static const unsigned MAX_IF_GAIN_STAGES = 10;

    uint32_t          samp_rate;
    int               block_size;
    Async::TcpClient  con;
    TunerType         tuner_type;
    uint32_t          tuner_gain_count;
    bool              center_fq_set;
    uint32_t          center_fq;
    bool              samp_rate_set;
    int32_t           gain_mode;
    int32_t           gain;
    bool              fq_corr_set;
    uint32_t          fq_corr;
    int               tuner_if_gain[MAX_IF_GAIN_STAGES];
    bool              test_mode_set;
    bool              test_mode;
    bool              use_digital_agc_set;
    bool              use_digital_agc;
    int               dist_print_cnt;
    Async::Timer      reconnect_timer;

    RtlTcp(const RtlTcp&);
    RtlTcp& operator=(const RtlTcp&);
    void sendCommand(char cmd, uint32_t param);
    void connected(void);
    void disconnected(Async::TcpConnection *c,
                      Async::TcpConnection::DisconnectReason reason);
    void updateSettings(void);
    int dataReceived(Async::TcpConnection *con, void *buf, int count);
    
};  /* class RtlTcp */



//} /* namespace */

#endif /* RTL_TCP_INCLUDED */


/*
 * This file has not been truncated
 */
