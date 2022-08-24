/**
@file	 AsyncAudioFifo.h
@brief   A FIFO for handling audio samples
@author  Tobias Blomberg / SM0SVX
@date	 2007-10-06

Implements a FIFO (with some extra functionality) for storing samples.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2009  Tobias Blomberg / SM0SVX

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


#ifndef ASYNC_AUDIO_FIFO_INCLUDED
#define ASYNC_AUDIO_FIFO_INCLUDED


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

#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>


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
@brief	A FIFO class for handling audio samples
@author Tobias Blomberg / SM0SVX
@date   2007-10-06

This class implements a FIFO for handling audio samples. The FIFO also have
some additional features. Output can be started or stopped and it can be
instructed to buffer some samples before starting to output audio.
Samples can be automatically output using the normal audio pipe infrastructure
or samples could be read on demand using the readSamples method.
*/
class AudioFifo : public AudioSink, public AudioSource
{
  public:
    /**
     * @brief 	Constuctor
     * @param   fifo_size This is the size of the fifo expressed in number
     *                    of samples.
     */
    explicit AudioFifo(unsigned fifo_size);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioFifo(void);
  
    /**
     * @brief	Set the size of the FIFO
     * @param	new_size  This is the size of the fifo expressed in number
     *                    of samples.
     *
     * Use this function to set the size of the FIFO. In doing this, the
     * FIFO will also be cleared.
     */
    void setSize(unsigned new_size);
    
    /**
     * @brief 	Check if the FIFO is empty
     * @return	Returns \em true if the FIFO is empty or else \em false
     */
    bool empty(void) const { return !is_full && (tail == head); }
    
    /**
     * @brief 	Check if the FIFO is full
     * @return	Returns \em true if the FIFO is full or else \em false
     *
     * This function is used to check if the FIFO is full or not. The FIFO can
     * only reach the buffer full condition if overwrite is false. The overwrite
     * mode is set by the setOverwrite function.
     */
    bool full(void) const { return is_full; }
    
    /**
     * @brief 	Find out how many samples there are in the FIFO
     * @param	ignore_prebuf Set to \em true to not report pre-buffered
		samples.
     * @return	Returns the number of samples in the FIFO
     */
    unsigned samplesInFifo(bool ignore_prebuf=false) const;
    
    /**
     * @brief 	Set the overwrite mode
     * @param 	overwrite Set to \em true to overwrite or else \em false
     *
     * The FIFO can operate in overwrite or normal mode. When in normal mode,
     * it will not be possible to add any more samples to the FIFO when it
     * is full. Samples has to be removed first. When overwrite is set, newly
     * added samples will overwrite the oldest samples in the buffer so the FIFO
     * will never get full but instead samples are lost.
     * Use this function to set the overwrite mode.
     */
    void setOverwrite(bool overwrite) { do_overwrite = overwrite; }
    
    /**
     * @brief 	Check the overwrite mode
     * @return	Returns \em true if overwrite is enabled or else \em false
     *
     * The FIFO can operate in overwrite or normal mode. When in normal mode,
     * it will not be possible to add any more samples to the FIFO when it
     * is full. Samples has to be removed first. When overwrite is set, newly
     * added samples will overwrite the oldest samples in the buffer so the FIFO
     * will never get full but instead samples are lost.
     * Use this function the check the current overwrite mode. Use the
     * setOverwrite function to set the overwrite mode.
     */
    bool overwrite(void) const { return do_overwrite; }
    
    /**
     * @brief 	Clear all samples from the FIFO
     *
     * This will immediately reset the FIFO and discard all samples.
     * The source will be told that all samples have been flushed.
     */
    void clear(void);

    /**
     * @brief	Set the number of samples that must be in the fifo before
     *		any samples are written out from it.
     * @param	prebuf_samples The number of samples
     */
    void setPrebufSamples(unsigned prebuf_samples);
    
    /**
     * @brief   Enable/disable the fifo buffer
     * @param   enable Set to \em true to enable buffering or else \em false
     *
     * Use this method to turn buffering on and off. When buffering is off,
     * no incoming samples will be stored in the fifo. If there are samples
     * in the fifo at the time when buffering is disabled they will be sent
     * out in the normal way.
     * Don't disable buffering when pre-buffering is used. This will get
     * you into trouble.
     */
    void enableBuffering(bool enable);
    
    /**
     * @brief   Check if buffering is enabled or disabled
     * @return  Returns \em true if buffering is enabled or else \em false
     */
    bool bufferingEnabled(void) const { return buffering_enabled; }
    
    /**
     * @brief 	Write samples into the FIFO
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into the FIFO. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the FIFO to flush the previously written samples
     *
     * This function is used to tell the FIFO to flush previously written
     * samples.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);
    
    /**
     * @brief Resume audio output to the connected sink
     * 
     * This function will be called when the registered audio sink is ready
     * to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void resumeOutput(void);
    
    
  protected:
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void);
    
    
  private:    
    float     	*fifo;
    unsigned    fifo_size;
    unsigned    head, tail;
    bool      	do_overwrite;
    bool      	output_stopped;
    unsigned  	prebuf_samples;
    bool      	prebuf;
    bool      	is_flushing;
    bool      	is_full;
    bool        buffering_enabled;
    bool      	disable_buffering_when_flushed;
    bool      	is_idle;
    bool      	input_stopped;
    
    void writeSamplesFromFifo(void);

};  /* class AudioFifo */


} /* namespace */

#endif /* ASYNC_AUDIO_FIFO_INCLUDED */


/*
 * This file has not been truncated
 */

