/**
@file	 AsyncAudioIO.h
@brief   Contains a class for handling audio input/output to an audio device
@author  Tobias Blomberg
@date	 2003-03-23

This file contains a class for handling audio input and output to an audio
device. See usage instruction in the class documentation.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003  Tobias Blomberg

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

/** @example  AsyncAudioIO_demo.cpp
An example of how to use the Async::AudioIO class
*/



#ifndef ASYNC_AUDIO_IO_INCLUDED
#define ASYNC_AUDIO_IO_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/signal_system.h>

#include <cstdio>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>
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

namespace Async
{

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
@brief	A class for handling audio input/output to an audio device
@author Tobias Blomberg
@date   2003-03-23

This is a class for handling audio input and output to an audio
device. For now, the AudioIO class only works with 16 bit mono samples at 8 Khz
sampling rate. An example usage is shown below.

\include AsyncAudioIO_demo.cpp
*/
class AudioIO : public SigC::Object
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
  
    /**
     * @brief Default constructor
     */
    AudioIO(void);
    
    /**
     * @brief Destructor
     */
    ~AudioIO(void);
  
    /**
     * @brief 	Check if the audio device is capable of full duplex operation
     * @return	Return \em true if the device is capable of full duplex or
     *	        \em false if it is not
     */
    bool isFullDuplexCapable(void);
  
    /**
     * @brief 	Open the audio device in the specified mode
     * @param 	mode The mode to open the audio device in. See
     *	      	Async::AudioIO::Mode for more information
     * @return	Returns \em true on success or else \em false on failure
     */
    bool open(Mode mode);
  
    /**
     * @brief 	Close the adio device
     */
    void close(void);
  
    /**
     * @brief 	Write samples to the audio device
     * @param 	buf   The buffer that contains the samples to write
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples written on success or else
     *	      	-1 on failure
     */
    int write(const short *buf, int count);
    
    /**
     * @brief 	Write an audio file to the audio device
     * @param 	filename  The name of the file to play
     * @return	Returns \em true on success or \em false on failure
     *
     * This function outputs a file to the audio device. The file should be
     * in raw format, 16 bit signed samples @ 8kHz. All other audio output
     * will be discarded during the time the file is played. When the file
     * has been fully written, the fileWritten signal will be emitted.
     */
    bool writeFile(const std::string& filename);

  
    /**
     * @brief 	A signal that is emitted when a block of audio has been
     *	      	received from the audio device
     * @param 	buf   A buffer containing the read samples
     * @param 	count The number of samples in the buffer
     */
    SigC::Signal2<void, short *, int> audioRead;

    /**
     * @brief 	A signal that is emitted when the write buffer is full
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if the buffer full condition has been cleared
     */
    SigC::Signal1<void, bool> writeBufferFull;
    
    /**
     * @brief 	A signal that is emitted when a file has been fully written
     * @param 	success Set to \em true on success or \em false on failure
     *
     * Writing a file to the audio device is initiated by calling the method
     * AudioIO::writeFile. When the file has been fully written or an error
     * occurs, this signal will be emitted.
     */
    SigC::Signal1<void, bool> fileWritten;

            
  protected:
    
  private:
    static const int  RATE = 8000;
    static const int  CHANNELS = 1;
    //static const int  SIZE = 16;
    //static const int  FRAG_COUNT = 32;    // 32 frags ~ one second
    //static const int  FRAG_SIZE_LOG2 = 8; // 256 bytes/frag
    static const int  BUF_FRAG_COUNT = 4;
	
    int       	      fd;
    Mode      	      mode;
    Async::FdWatch *  read_watch;
    Async::FdWatch *  write_watch;
    char *    	      read_buf;
    
    int    	      file;
    Mode      	      old_mode;
    Async::Timer *    write_file_flush_timer;

    void audioReadHandler(Async::FdWatch *watch);
    void writeSpaceAvailable(Async::FdWatch *watch);
    void writeFromFile(void);
    void writeFileDone(Async::Timer *timer);

};  /* class AudioIO */


} /* namespace */

#endif /* ASYNC_AUDIO_IO_INCLUDED */



/*
 * This file has not been truncated
 */

