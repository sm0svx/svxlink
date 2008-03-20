/**
@file	 SampleFifo.h
@brief   A FIFO for handling samples
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-07

Implements a FIFO for storing samples.

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


#ifndef SAMPLE_FIFO_INCLUDED
#define SAMPLE_FIFO_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <sigc++/sigc++.h>

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
@date   2004-03-07

This class implements a FIFO for handling audio samples.
*/
class SampleFifo : public SigC::Object
{
  public:
    /**
     * @brief 	Default constuctor
     */
    explicit SampleFifo(int fifo_size);
  
    /**
     * @brief 	Destructor
     */
    ~SampleFifo(void);
  
    /**
     * @brief 	Add samples to the FIFO
     * @param 	samples A buffer containing samples to put into the FIFO
     * @param 	count The number of samples to put into the FIFO
     * @return	Return the number of samples that have been put into the FIFO
     */
    int addSamples(float *samples, int count);
    
    /**
     * @brief 	Stop automatic audio output via the writeSamples signal
     * @param 	stop Set to \em true to stop audio output or else \em false
     *
     * This FIFO has the ability to automatically output audio samples using
     * the writeSamples signal. The samples are output as fast as the receiver
     * can handle them. Flow control is handled by the receiver setting the
     * return value from the writeSamples signal and by calling the
     * writeBufferFull method. When the FIFO run out of samples, the
     * allSamplesWritten signal is emitted.
     * Use this function to enable/disable the automatic output of samples.
     * By default, automatic output is enabled.
     */
    void stopOutput(bool stop);
    
    /**
     * @brief 	Check if the FIFO is empty
     * @return	Returns \em true if the FIFO is empty or else \em false
     */
    bool empty(void) const { return tail == head; }
    
    /**
     * @brief 	Check if the FIFO is full
     * @return	Returns \em true if the FIFO is full or else \em false
     *
     * This function is used to check if the FIFO is full or not. The FIFO can
     * only reach the buffer full condition if overwrite is false. The overwrite
     * mode is set by the setOverwrite function.
     */
    bool full(void) const;
    
    /**
     * @brief 	Find out how many samples there are in the FIFO
     * @param	ignore_prebuf Set to \em true to not report pre-buffered
		samples.
     * @return	Returns the number of samples in the FIFO
     */
    unsigned samplesInFifo(bool ignore_prebuf=false) const;
    
    /**
     * @brief 	Indicate to the FIFO if the receiver is ready or not to accept
     *	      	more samples
     * @param 	is_full Set to \em false if more samples can be accepted
     *	      	      	or \em true if not.
     *
     * This FIFO has the ability to automatically output audio samples using
     * the writeSamples signal. The samples are output as fast as the receiver
     * can handle them. Flow control is handled by the receiver setting the
     * return value from the writeSamples signal and by calling this function.
     * When the FIFO run out of samples, the allSamplesWritten signal is
     * emitted.
     * Use this function to stop or start the output of samples.
     */
    void writeBufferFull(bool is_full);
    
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
     * @brief 	Read samples from the FIFO
     * @param 	samples A buffer to put the read samples into
     * @param 	count The number of samples to read. This value must not be
     *        	      larger than the size of the "samples" buffer given in
     *	      	      the first argument.
     * @return	Returns the number of samples actually read
     *
     * This function can be used to read samples from the FIFO if the automatic
     * output of samples is not enabled. Automatic output of samples and this
     * function must not be used at the sample time.
     */
    int readSamples(float *samples, int count);
    
    /**
     * @brief 	Clear all samples from the FIFO
     */
    void clear(void) { tail = head; allSamplesWritten(); }

    /**
     * @brief	Set the number of samples that must be in the fifo before
     *		any samples are written out from it.
     * @param	prebuf_samples The number of samples
     */
    void setPrebufSamples(unsigned prebuf_samples);
    
    /**
     * @brief	Flush out the samples in the buffer
     */
    void flushSamples(void);
    
    /**
     * @brief 	Set a name for this FIFO used for debugging
     * @param 	name The name to use for this FIFO
     */
    void setDebugName(const std::string& name) { debug_name = name; }


    /**
     * @brief 	A signal that is emitted when the FIFO is full
     * @param 	is_full Set to \em true if the buffer is full or \em false
     *	      	      	if the buffer full condition has been cleared
     */
    SigC::Signal1<void, bool> fifoFull;
    
    
    /**
     * @brief 	A signal that is emitted when the FIFO want to output samples
     * @param 	samples A buffer containing the samples to output
     * @param 	count The number of samples in the output buffer
     * @return	Return the number of samples actually read by the receiver
     *
     * This FIFO has the ability to automatically output audio samples using
     * the this signal. The samples are output as fast as the receiver
     * can handle them. Flow control is handled by the receiver setting the
     * return value from this signal and by calling the writeBufferFull method.
     * When the FIFO run out of samples, the allSamplesWritten signal is
     * emitted.
     *
     * If the value returned by the connected slot (receiver) differs from the
     * value of the second argument of this signal (count), output will be
     * stopped until the writeBufferFull function is called. This is how the
     * flow control is handled.
     */
    SigC::Signal2<int, float *, int>  	    writeSamples;
    
    /**
     * @brief 	A signal that is emitted when the FIFO has been emptied
     *
     * This FIFO has the ability to automatically output audio samples using
     * the the writeSamples signal. The samples are output as fast as the
     * receiver can handle them. Flow control is handled by the receiver setting
     * the return value from the writeSamples signal and by calling the
     * writeBufferFull function.
     * When the FIFO run out of samples, this signal is emitted.
     */
    SigC::Signal0<void>     	      	    allSamplesWritten;
    
    
  protected:
    
  private:    
    float     	*fifo;
    int       	fifo_size;
    int       	head, tail;
    bool      	is_stopped;
    bool      	do_overwrite;
    bool      	write_buffer_is_full; // Bad name! This does NOT indicate that
      	      	      	      	      // the FIFO is full..
    unsigned  	prebuf_samples;
    bool      	prebuf;
    bool      	do_flush;
    std::string debug_name;
    
    void writeSamplesFromFifo(void);

};  /* class SampleFifo */


} /* namespace */

#endif /* SAMPLE_FIFO_INCLUDED */


/*
 * This file has not been truncated
 */

