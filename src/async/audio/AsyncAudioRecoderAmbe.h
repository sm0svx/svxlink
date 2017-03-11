/**
@file	 AsyncAudioRecoderAMBE.h
@brief   An audio Recoder that use the AMBE audio codec
@author  Tobias Blomberg / SM0SVX
@date	 2013-10-12

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_RECODER_AMBE_INCLUDED
#define ASYNC_AUDIO_RECODER_AMBE_INCLUDED


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

#include <AsyncAudioRecoder.h>


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
@brief	An audio Recoder that use the AMBE audio codec
@author Tobias Blomberg / SM0SVX
@date   2013-10-12

This class implements an audio Recoder that use the AMBE audio codec.
*/
class AudioRecoderAmbe : public AudioRecoder, public AudioEncoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioRecoderAmbe(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioRecoderAmbe(void);
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "AMBE"; }
  
    /**
     * @brief 	Set an option for the Recoder
     * @param 	name The name of the option
     * @param 	value The value of the option
     */
    virtual void setOption(const std::string &name, const std::string &value);

    /**
     * @brief   Resets encoder to be equivalent to a freshly initialized one
     */
    void reset(void);

    /**
     * @brief 	Write encoded samples into the Recoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    virtual void writeEncodedSamples(void *buf, int size);
 
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
        

  protected:
    
  private:
    int         frame_size;
    
    AudioRecoderAmbe(const AudioRecoderAmbe&);
    AudioRecoderAmbe& operator=(const AudioRecoderAmbe&);
    
};  /* class AudioRecoderAMBE */


} /* namespace */

#endif /* ASYNC_AUDIO_RECODER_AMBE_INCLUDED */



/*
 * This file has not been truncated
 */

