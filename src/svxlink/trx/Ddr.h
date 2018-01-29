/**
@file	 Ddr.h
@brief   A receiver class to handle digital drop receivers
@author  Tobias Blomberg / SM0SVX
@date	 2014-07-16

This file contains a class that handle local digital drop receivers.

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2018 Tobias Blomberg / SM0SVX

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


#ifndef DDR_INCLUDED
#define DDR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



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

#include "LocalRxBase.h"
#include "RtlTcp.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/

class WbRxRtlSdr;


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
@brief	A class to handle digital drop receivers
@author Tobias Blomberg
@date   2014-07-16

This class handle local digital drop receivers. A digital drop receiver use
a wideband tuner to receive wideband samples. A narrowband channel is then
extracted from this wideband signal and a demodulator is applied.
*/
class Ddr : public LocalRxBase
{
  public:
    static Ddr *find(const std::string &name);

    /**
     * @brief 	Default constuctor
     */
    explicit Ddr(Async::Config &cfg, const std::string& name);
  
    /**
     * @brief 	Destructor
     */
    virtual ~Ddr(void);
  
    /**
     * @brief 	Initialize the receiver object
     * @return 	Return \em true on success, or \em false on failure
     */
    virtual bool initialize(void);

    /**
     * @brief   Find out which frequency this DDR is tuned to
     * @returns Returns the absolute frequency in Hz of this DDR
     */
    uint32_t nbFq(void) const { return fq; }

    /**
     * @brief   Tell the DDR that the frequency of the wideband tuner changed
     * @param   fq The new tuner frequency
     */
    void tunerFqChanged(uint32_t fq);

    /**
     * @brief   Find out what the pre-demodulation sample rate is
     * @returns Returns the sample rate used before the demodulator
     *
     * This function is used to find out what the sampling rate is before the
     * demodulator. This must be known before using the preDemod signal.
     */
    unsigned preDemodSampleRate(void) const;

    /**
     * @brief   Find out if the receiver is ready for operation
     * @returns Returns \em true if the receiver is ready for operation
     */
    virtual bool isReady(void) const;

    /**
     * @brief   Set the receiver frequency
     * @param   fq The frequency in Hz
     */
    virtual void setFq(unsigned fq);

    /**
     * @brief   Set the receiver modulation mode
     * @param   mod The modulation to set (@see Modulation::Type)
     */
    virtual void setModulation(Modulation::Type mod);

    /**
     * @brief   A signal that is emitted when new I/Q data is available
     * @param   samples The new samples that are available
     *
     * This signal is emitted each time a new block of I/Q samples are
     * available. The samples are taken after all decimation and channel
     * filtering has been done but before demodulation is performed. This make
     * the channel samples available for other types of signal processing.
     */
    sigc::signal<void, const std::vector<RtlTcp::Sample>&> preDemod;
    
  protected:
    /**
     * @brief   Open the audio input source
     * @return  Returns \em true on success or else \em false
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the audio source object is initialized before calling
     * the LocalRxBase::initialize function.
     */
    virtual bool audioOpen(void);

    /**
     * @brief   Close the audio input source
     *
     * This function may be used during the initialization of LocalRxBase so
     * make sure that the audio source object is initialized before calling the
     * LocalRxBase::initialize function.
     */
    virtual void audioClose(void);

    /**
     * @brief   Get the sampling rate of the audio source
     * @return  Returns the sampling rate of the audio source
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the proper sampling rate can be returned before calling
     * the LocalRxBase::initialize function.
     */
    virtual int audioSampleRate(void);

    /**
     * @brief   Get the audio source object
     * @return  Returns an instantiated audio source object
     *
     * This function is used during the initialization of LocalRxBase so make
     * sure that the audio source object is initialized before calling
     * the LocalRxBase::initialize function.
     */
    virtual Async::AudioSource *audioSource(void);
    
  private:
    class Channel;
    typedef std::map<std::string, Ddr*> DdrMap;

    static DdrMap ddr_map;

    Async::Config           &cfg;
    Channel                 *channel;
    WbRxRtlSdr              *rtl;
    double                  fq;

    void updateFqOffset(void);
    
};  /* class Ddr */


//} /* namespace */

#endif /* DDR_INCLUDED */


/*
 * This file has not been truncated
 */
