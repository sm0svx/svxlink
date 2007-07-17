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


#ifndef ASYNC_AUDIO_DEVICE_INCLUDED
#define ASYNC_AUDIO_DEVICE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

#include <string>
#include <map>
#include <list>


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

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class AudioIO;
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
class AudioDevice : public SigC::Object
{
  public:
    /**
     * @brief The different modes to open a device in
     */
    typedef enum
    {
      MODE_NONE,  ///< No mode. The same as close
      MODE_RD,	  ///< Read
      MODE_WR,	  ///< Write
      MODE_RDWR   ///< Both read and write
    } Mode;
  
    static AudioDevice *registerAudioIO(const std::string& dev_name,
      	    AudioIO *audio_io);
    static void unregisterAudioIO(AudioIO *audio_io);
    
    /**
     * @brief 	Check if the audio device has full duplex capability
     * @return	Returns \em true if the device has full duplex capability
     *	      	or else \em false
     */
    bool isFullDuplexCapable(void);
    
    /**
     * @brief 	Open the audio device
     * @param 	mode The mode to open the audio device in (See AudioIO::Mode)
     * @return	Returns \em true on success or else \em false
     */
    bool open(Mode mode);
    
    /**
     * @brief 	Close the audio device
     */
    void close(void);
    
    /**
     * @brief 	Tell the audio device handler that there are audio to be
     *	      	written in the buffer
     */
    void audioToWriteAvailable(void);

    /*
     * @brief	Tell the audio device to flush its buffers
     */
    void flushSamples(void);
    
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
    int samplesToWrite(void) const;
    
    /**
     * @brief 	Return the sample rate
     * @return	Returns the sample rate
     */
    int sampleRate(void) const { return RATE; }
        
    /**
     * @brief 	A signal that is emitted when a block of audio has been
     *	      	received from the audio device
     * @param 	buf   A buffer containing the read samples
     * @param 	count The number of samples in the buffer
     */
    SigC::Signal2<int, float *, int> audioRead;

    /**
     * @brief 	A signal that is emitted when the write buffer is full
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if the buffer full condition has been cleared
     */
    //SigC::Signal1<void, bool> writeBufferFull;
    
            
    
  protected:
    /**
     * @brief 	Default constuctor
     */
    explicit AudioDevice(const std::string& dev_name);
  
    /**
     * @brief 	Destructor
     */
    ~AudioDevice(void);
  
    
  private:
    static const int  RATE = 8000;
    static const int  CHANNELS = 1;
    //static const int  SIZE = 16;
    static const int  FRAG_COUNT = 2;    // 16 frags ~ one second
    static const int  FRAG_SIZE_LOG2 = 10; // 1024 bytes/frag (512 samples)
    static const int  BUF_FRAG_COUNT = 4;
    static std::map<std::string, AudioDevice*>  devices;
    
    std::string       	dev_name;
    int       	      	use_count;
    std::list<AudioIO*> aios;
    Mode      	      	current_mode;
    int       	      	fd;
    FdWatch	      	*read_watch;
    FdWatch	      	*write_watch;
    short      	      	*read_buf;
    int       	      	device_caps;
    bool      	      	use_trigger;
    bool		prebuf;
    float     	      	*samples;
    short     	      	*last_frag;
    bool      	      	use_fillin;
    
    void audioReadHandler(FdWatch *watch);
    void writeSpaceAvailable(FdWatch *watch);
    void closeDevice(void);

};  /* class AudioDevice */


} /* namespace */

#endif /* ASYNC_AUDIO_DEVICE_INCLUDED */



/*
 * This file has not been truncated
 */

