/**
@file	 AsyncAudioDevice.h
@brief   Handle OSS audio devices
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-20

Implements the low level interface to an OSS audio device.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_DEVICE_OSS_INCLUDED
#define ASYNC_AUDIO_DEVICE_OSS_INCLUDED


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

class FdWatch;


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
@brief	A class that implements the low level interface to an OSS audio device
@author Tobias Blomberg / SM0SVX
@date   2004-03-20

This class implements the low level interface to an OSS audio device. This
class is not intended to be used by the end user of the Async library. It is
used by the Async::AudioIO class, which is the Async API frontend for using
audio in an application.
*/
class AudioDeviceOSS : public Async::AudioDevice
{
  public:
    /**
     * @brief 	Constuctor
     * @param 	dev_name  The name of the device to associate this object with
     */
    explicit AudioDeviceOSS(const std::string& dev_name);
  
    /**
     * @brief 	Destructor
     */
    ~AudioDeviceOSS(void);
  
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
    //static const int  BUF_FRAG_COUNT = 4;
    
    int       	      	fd;
    FdWatch	      	*read_watch;
    FdWatch	      	*write_watch;
    int       	      	device_caps;
    bool      	      	use_trigger;
    size_t              frag_size;
    
    void audioReadHandler(FdWatch *watch);
    void writeSpaceAvailable(FdWatch *watch);
    
};  /* class AudioDevice */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_OSS_INCLUDED */



/*
 * This file has not been truncated
 */

