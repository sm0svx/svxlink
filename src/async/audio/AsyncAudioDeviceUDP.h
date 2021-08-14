/**
@file	 AsyncAudioDevice.h
@brief   Handle simple streaming of audio samples via UDP
@author  Tobias Blomberg / SM0SVX
@date	 2012-06-25

Implements a simple "audio interface" that stream samples via
UDP. This can for example be used to stream audio to/from
GNU Radio.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2012 Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_DEVICE_UDP_INCLUDED
#define ASYNC_AUDIO_DEVICE_UDP_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncTimer.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioDevice.h"


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

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class UdpSocket;
class Timer;


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
@brief	
@author Tobias Blomberg / SM0SVX
@date   2012-06-25

Implements a simple "audio interface" that stream samples via
UDP. This can for example be used to stream audio to/from
GNU Radio.
*/
class AudioDeviceUDP : public Async::AudioDevice
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	dev_name  The name of the device to associate this object with
     */
    explicit AudioDeviceUDP(const std::string& dev_name);
  
    /**
     * @brief 	Destructor
     */
    ~AudioDeviceUDP(void);
  
    /**
     * @brief 	Find out what the read (recording) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    virtual size_t readBlocksize(void);

    /**
     * @brief 	Find out what the write (playback) blocksize is set to
     * @return	Returns the currently set blocksize in samples per channel
     */
    virtual size_t writeBlocksize(void);

    /**
     * @brief 	Check if the audio device has full duplex capability
     * @return	Returns \em true if the device has full duplex capability
     *	      	or else \em false
     */
    virtual bool isFullDuplexCapable(void);
    
    /**
     * @brief 	Tell the audio device handler that there are audio to be
     *	      	written in the buffer
     */
    virtual void audioToWriteAvailable(void);

    /**
     * @brief	Tell the audio device to flush its buffers
     */
    virtual void flushSamples(void);
    
    /**
     * @brief 	Find out how many samples there are in the output buffer
     * @return	Returns the number of samples in the output buffer on
     *          success or -1 on failure.
     *
     * This function can be used to find out how many samples there are
     * in the output buffer at the moment. This can for example be used
     * to find out how long it will take before the output buffer has
     * been flushed.
     */
    virtual int samplesToWrite(void) const;
    
    
  protected:
    /**
     * @brief 	Open the audio device
     * @param 	mode The mode to open the audio device in (See AudioIO::Mode)
     * @return	Returns \em true on success or else \em false
     */
    virtual bool openDevice(Mode mode);

    /**
     * @brief 	Close the audio device
     */
    virtual void closeDevice(void);

    
  private:
    size_t              block_size;
    Async::UdpSocket    *sock;
    int16_t             *read_buf;
    size_t              read_buf_pos;
    IpAddress           ip_addr;
    uint16_t            port;
    Async::Timer        *pace_timer;
    bool                zerofill_on_underflow;

    void audioReadHandler(const Async::IpAddress &ip, uint16_t port,
                          void *buf, int count);
    void audioWriteHandler(void);
    
};  /* class AudioDeviceUDP */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_UDP_INCLUDED */



/*
 * This file has not been truncated
 */

