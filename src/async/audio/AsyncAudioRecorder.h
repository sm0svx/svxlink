/**
@file	 AsyncAudioRecorder.h
@brief   Contains a class for recording raw audio to a file
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-29

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2009 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_RECORDER_INCLUDED
#define ASYNC_AUDIO_RECORDER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <stdio.h>
#include <stdint.h>
#include <sigc++/sigc++.h>
#include <sys/time.h>

#include <string>

#include <AsyncAudioSink.h>


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
@brief	A class for recording raw audio to a file
@author Tobias Blomberg / SM0SVX
@date   2005-08-29

Use this class to stream audio into a file. The audio is stored in raw format,
(only samples no header) or WAV format.
*/
class AudioRecorder : public Async::AudioSink
{
  public:
    typedef enum { FMT_AUTO, FMT_RAW, FMT_WAV } Format;
    
    /**
     * @brief 	Default constuctor
     * @param 	filename The name of the file to record audio to
     * @param 	fmt The file format (@see Format)
     * @param 	sample_rate The sample rate (defaults to INTERNAL_SAMPLE_RATE)
     */
    explicit AudioRecorder(const std::string& filename,
      	      	      	   AudioRecorder::Format fmt=FMT_AUTO,
			   int sample_rate=INTERNAL_SAMPLE_RATE);
  
    /**
     * @brief 	Destructor
     */
    ~AudioRecorder(void);
  
    /**
     * @brief 	Initialize the recorder
     * @return	Return \em true if the initialization was successful
     *
     * This function will initialize the recorder and open the file.
     * On error, this function returns \em false. The error message can be
     * retrieved using the errorMsg function.
     */
    bool initialize(void);
    
    /**
     * @brief   Set the maximum length of this recording
     * @param   time_ms The maximum time in milliseconds
     * @param   hw_time_ms The high watermark time in milliseconds
     * 
     * Use this function to set the maximum time for a recording. When the
     * limit has been reached, the file will be closed and any incoming samples
     * will be thrown away.
     * The high watermark time is a soft limit. After reaching the high
     * watermark, the recording will be ended as soon as the audio source
     * call flushSamples. The high watermark need to be set to less than
     * the maximum time to have any effect. A time of 0 disables it.
     * The sampling rate given in the constructor call is used to calculate
     * the time. Setting the time to 0 will allow the file to grow indefinetly.
     */
    void setMaxRecordingTime(unsigned time_ms, unsigned hw_time_ms=0);
    
    /**
     * @brief   Close the file
     * @returns Return \em true if closing went well or \em false otherwise
     *
     * This function will close the file being recorded to. When the file has
     * been closed, all samples coming in after that will be discarded.
     * If an error occurr, this function will return \em false. The error
     * message can be retrieved using the errorMsg function.
     */
    bool closeFile(void);

    /**
     * @brief   Find out how many samples that have been written so far
     * @return  Returns the number of samples written so far
     */
    unsigned samplesWritten(void) const { return samples_written; }

    /**
     * @brief   The timestamp of the first stored sample
     * @returns Returns the timestamp
     */
    const struct timeval &beginTimestamp(void) const { return begin_timestamp; }

    /**
     * @brief   The timestamp of the last stored sample
     * @returns Returns the timestamp
     */
    const struct timeval &endTimestamp(void) const { return end_timestamp; }

    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);

    /**
     * @brief   Return the current error message
     * @returns Returns the currently set error message
     *
     * This function is used to retrieve the last set error message. It can
     * for example be used when the initialize method return false or when the
     * error signal is emitted.
     */
    std::string errorMsg(void) const { return errmsg; }

    /**
     * @brief   A signal that's emitted when the max recording time is reached
     *
     * This signal will be emitted when any of the maximum recording time or
     * the high watermark limits are reached. Before this signal is emitted,
     * the file is closed. It is safe to delete the audio recorder from the
     * slot that is connected to this signal.
     */
    sigc::signal<void> maxRecordingTimeReached;

    /**
     * @brief   This signal is emitted when an error occurrs in the recorder
     *
     * This signal will be emitted when an error occurrs in the recorder.
     * After emitting the signal the associated file will be closed. All audio
     * that is written after an error has occurred will be thrown away. To open
     * a new file, call the initialize method again.
     */
    sigc::signal<void> errorOccurred;

  private:
    std::string     filename;
    FILE      	    *file;
    unsigned        samples_written;
    Format    	    format;
    int       	    sample_rate;
    unsigned        max_samples;
    unsigned        high_water_mark;
    bool            high_water_mark_reached;
    struct timeval  begin_timestamp;
    struct timeval  end_timestamp;
    std::string     errmsg;
    
    AudioRecorder(const AudioRecorder&);
    AudioRecorder& operator=(const AudioRecorder&);
    bool writeWaveHeader(void);
    int store32bitValue(char *ptr, uint32_t val);
    int store16bitValue(char *ptr, uint16_t val);
    void setErrMsgFromErrno(const std::string &fname);

};  /* class AudioRecorder */


} /* namespace */

#endif /* ASYNC_AUDIO_RECORDER_INCLUDED */



/*
 * This file has not been truncated
 */

