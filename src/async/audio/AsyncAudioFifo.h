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
#include <AsyncAudioValve.h>
#include <SigCAudioSource.h>


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
class AudioFifo : public SigC::Object, public AudioSink, public AudioSource
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
     * @brief 	Find out how many samples there are in the FIFO
     * @param	ignore_prebuf Set to \em true to not report pre-buffered
		samples.
     * @return	Returns the number of samples in the FIFO
     */
    unsigned samplesInFifo(void) const;

    /**
     * @brief 	Find out how many samples the FIFO can accept
     * @return	Returns the amount of free space in the FIFO
     */
    unsigned spaceAvail(void) const;
    
    /**
     * @brief 	Clear all samples from the FIFO
     *
     * This will immediately reset the FIFO and discard all samples.
     * The source will be told that all samples have been flushed.
     */
    void clear(void);

    /**
     * @brief   Enable/disable the fifo buffer
     * @param   enable Set to \em true to enable buffering or else \em false
     *
     * Use this method to turn buffering on and off. When buffering is off,
     * no incoming samples will be stored in the fifo. If there are samples
     * in the fifo at the time when buffering is disabled they will be sent
     * out in the normal way until the fifo is flushed.
     */
    void enableBuffering(bool enable);
    
    /**
     * @brief   Check if buffering is enabled or disabled
     * @return  Returns \em true if buffering is enabled or else \em false
     */
    bool bufferingEnabled(void) const { return buffering_enabled; }

    /**
     * @brief   Enable/disable the fifo output
     * @param   enable Set to \em true to enable output or else \em false
     *
     * Use this method to turn the output on and off. When buffering is off,
     * no more samples will be written to the sink.
     */
    void enableOutput(bool enable) { valve.setOpen(enable); }

    /**
     * @brief   Check if output is enabled or disabled
     * @return  Returns \em true if output is enabled or else \em false
     */
    bool outputEnabled(void) const { return valve.isOpen(); }
    
    /**
     * @brief 	Write samples into the FIFO
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into the FIFO. If it
     * returns 0, no more samples could be written.
     * If the returned number of written samples is lower than the count
     * parameter value, the sink is not ready to accept more samples.
     * In this case, the audio source requires sample buffering to temporarily
     * store samples that are not immediately accepted by the sink.
     * The writeSamples function should be called on source buffer updates
     * and after a source output request has been received through the
     * requestSamples function.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count);

    /**
     * @brief 	Tell the sink that there are samples available on request
     *
     * This function is used to tell the sink that there are samples available
     * that can be requested by calling the sourceRequestSamples function.
     * This function is normally only called from a connected source object.
     */
    virtual void availSamples(void);
    
    /**
     * @brief 	Tell the FIFO to flush the previously written samples
     *
     * This function is used to tell the FIFO to flush previously written
     * samples.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void);

    
  protected:

    /**
     * @brief Request audio samples from this source
     * 
     * This function will be called when the registered audio sink is ready
     * to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void onRequestSamples(int count);
  
    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    virtual void onAllSamplesFlushed(void);
    
    
  private:

    typedef enum
    {
      STREAM_IDLE, STREAM_ACTIVE, STREAM_FLUSHING
    } StreamState;
    
    AudioValve      valve;
    SigCAudioSource sigsrc;
                  
    float     	*fifo;
    unsigned    fifo_size;
    unsigned    head, tail;
    StreamState stream_state;
    bool        is_full;
    bool        buffering_when_empty;
    bool        buffering_enabled;
    
    int writeSamplesFromFifo(int count);
    void flushSamplesFromFifo(void);

};  /* class AudioFifo */


} /* namespace */

#endif /* ASYNC_AUDIO_FIFO_INCLUDED */


/*
 * This file has not been truncated
 */

