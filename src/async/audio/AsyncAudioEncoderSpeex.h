/**
@file	 AsyncAudioEncoderSpeex.h
@brief   An audio encoder that encodes samples using the Speex codec
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-16

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_ENCODER_SPEEX_INCLUDED
#define ASYNC_AUDIO_ENCODER_SPEEX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <speex/speex.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioEncoder.h>


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
@brief	An audio encoder that encodes samples using the Speex codec
@author Tobias Blomberg / SM0SVX
@date   2008-10-15

This class implements an audio encoder that use the Speex audio codec.
*/
class AudioEncoderSpeex : public AudioEncoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioEncoderSpeex(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioEncoderSpeex(void);
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "SPEEX"; }
  
    /**
     * @brief 	Set an option for the encoder
     * @param 	name The name of the option
     * @param 	value The value of the option
     */
    virtual void setOption(const std::string &name, const std::string &value);
    
    /**
     * @brief Print codec parameter settings
     */
    virtual void printCodecParams(void);
    
    /**
     * @brief 	Set the number of frames that are sent in each packet
     * @param 	fpp Frames per packet
     */
    void setFramesPerPacket(unsigned fpp);
    
    /**
     * @brief   Set the quality to use
     * @param   quality The new quality to set (0-10)
     * @returns The new quality value
     *
     * Use this function to set the quality used by the Speex encoder.
     * Values between 0 to 10 are possible. Low values give poorer quality
     * and lower bitrate and higher values give better quality.
     */
    void setQuality(int quality);
    
    /**
     * @brief   Set the bitrate to use
     * @param   new_bitrate The new bitrate to set
     * @returns The new bitrate
     *
     * Use this function to set the bitrate used by the Speex encoder.
     * Not all bitrates are possible. The chosen bitrate is returned.
     */
    int setBitrate(int new_bitrate);
    
    /**
     * @brief   Get the current bitrate
     * @returns Returns the current bitrate
     */
    int bitrate(void);
    
    /**
     * @brief   Set the complexity to use
     * @param   new_comp The new complexity value to set (0-10)
     * @returns The new complexity value
     *
     * Use this function to set the complexity used by the Speex encoder.
     * Values between 0 to 10 are possible. Lower complexity values are
     * lighter on the CPU but give poorer quality. The difference between
     * lowest and highest value is about 2dB SNR. The highest complexity is
     * about five times as heavy on the CPU as the lowest complexity.
     */
    int setComplexity(int new_comp);
    
    /**
     * @brief   Get the current complexity
     * @returns Returns the current complexity
     */
    int complexity(void);
    
    /**
     * @brief   Get the current frame size
     * @returns Returns the current frame size
     */
    int frameSize(void) const { return frame_size; }
    
    /**
     * @brief   Enable or disable the variable bitrate mode
     * @param   enable Set to \em true to enable or \em false to disable VBR
     *
     * Use this function to enable or disable the variable bitrate (VBR) mode.
     * Variable bitrate mode consume more bandwidth when needed to keep a
     * certain quality level.
     */
    void enableVbr(bool enable);
    
    /**
     * @brief   Find out if variable bitrate is enabled or disabled
     * @returns Returns the current frame size
     */
    bool vbrEnabled(void);
    
    /**
     * @brief   Set the VBR quality to use
     * @param   quality The new quality value to set (0-10)
     * @returns The new quality value is returned
     */
    int setVbrQuality(int quality);
    
    /**
     * @brief   Get the current VBR quality
     * @returns Returns the current VBR quality
     */
    int vbrQuality(void);
    
    //int setVbrMaxBitrate(int bitrate);
    //int vbrMaxBitrate(void);
    
    /**
     * @brief   Set the average bitrate to use
     * @param   new_abr The new average bitrate to set
     * @returns The new average bitrate value is returned
     */
    int setAbr(int new_abr);
    
    /**
     * @brief   Get the currently set average bitrate
     * @returns Returns the currently set average bitrate
     */
    int abr(void);
    
    //bool enableHighpass(bool enable);
    //bool highpassEnabled(void);
    
    
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
    SpeexBits bits;
    void      *enc_state;
    int       frame_size;
    float     *sample_buf;
    int       buf_len;
    int       frames_per_packet;
    int       frame_cnt;
    
    AudioEncoderSpeex(const AudioEncoderSpeex&);
    AudioEncoderSpeex& operator=(const AudioEncoderSpeex&);
    
};  /* class AudioEncoderSpeex */


} /* namespace */

#endif /* ASYNC_AUDIO_ENCODER_SPEEX_INCLUDED */



/*
 * This file has not been truncated
 */

