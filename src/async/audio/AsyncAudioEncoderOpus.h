/**
@file	 AsyncAudioEncoderOpus.h
@brief   An audio encoder that encodes samples using the Opus codec
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

#ifndef ASYNC_AUDIO_ENCODER_OPUS_INCLUDED
#define ASYNC_AUDIO_ENCODER_OPUS_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <opus.h>


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
@brief	An audio encoder that encodes samples using the Opus codec
@author Tobias Blomberg / SM0SVX
@date   2013-10-12

This class implements an audio encoder that use the Opus audio codec.
*/
class AudioEncoderOpus : public AudioEncoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioEncoderOpus(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~AudioEncoderOpus(void);
  
    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "OPUS"; }
  
    /**
     * @brief   Set the size of each encoded frame
     * @param   new_frame_size_ms The new frame size in milliseconds
     * @returns Returns the new fram size
     *
     * Use this function to set the size of each encoded frame.
     * Valid values for the frame size are 2.5, 5, 10, 20, 40 or 60
     * milliseconds.
     */
    float setFrameSize(float new_frame_size_ms);

    /**
     * @brief   Get the current frame size
     * @returns Returns the current frame size
     */
    int frameSize(void) const { return frame_size; }
    
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
     * @brief   Set the complexity to use
     * @param   new_comp The new complexity value to set (0-10)
     * @returns The new complexity value
     *
     * Use this function to set the complexity used by the Opus encoder.
     * Values between 0 to 10 are possible. Lower complexity values are
     * lighter on the CPU but give poorer quality.
     */
    opus_int32 setComplexity(opus_int32 new_comp);
    
    /**
     * @brief   Get the current complexity
     * @returns Returns the current complexity
     */
    opus_int32 complexity(void);
    
    /**
     * @brief   Set the bitrate to use
     * @param   new_bitrate The new bitrate to set (500 - 512000 bps)
     * @returns The new bitrate
     *
     * Use this function to set the bitrate used by the Opus encoder.
     * The chosen bitrate is returned.
     */
    int setBitrate(int new_bitrate);
    
    /**
     * @brief   Get the current bitrate
     * @returns Returns the current bitrate
     */
    opus_int32 bitrate(void);
    
    /**
     * @brief   Enable or disable the variable bitrate mode
     * @param   enable Set to \em true to enable or \em false to disable VBR
     * @returns Returns if VBR is enabled or not
     *
     * Use this function to enable or disable the variable bitrate (VBR) mode.
     * Variable bitrate mode consume more bandwidth when needed to keep a
     * certain quality level. VBR is enabled by default.
     */
    bool enableVbr(bool enable);
    
    /**
     * @brief   Find out if variable bitrate is enabled or disabled
     * @returns Returns the current frame size
     */
    bool vbrEnabled(void);
    
    /**
     * @brief   Enable or disable constrained VBR
     * @param   enable Set to \em true to enable or \em false to disable
     * @returns Returns if constrained VBR is enabled or not
     *
     * Constrained VBR is enabled by default.
     */
    bool enableConstrainedVbr(bool enable);
    
    /**
     * @brief   Find out if variable bitrate is enabled or disabled
     * @returns Returns the current frame size
     */
    bool constrainedVbrEnabled(void);
    
    /**
     * @brief   Set the maximum audio bandwidth that the encoder will use
     * @param   new_bw The new bandwidth to set
     * @returns Returns the current maximum bandwidth setting
     *
     * Use this function to set the maximum audio bandwidth for the encoder.
     * The bandwidth is not given in Hz but as a number of constants.
     * OPUS_BANDWIDTH_NARROWBAND (4 kHz), OPUS_BANDWIDTH_MEDIUMBAND (6 kHz),
     * OPUS_BANDWIDTH_WIDEBAND (8 kHz), OPUS_BANDWIDTH_SUPERWIDEBAND (12 kHz),
     * OPUS_BANDWIDTH_FULLBAND (20 kHz) (default) 
     */
    opus_int32 setMaxBandwidth(opus_int32 new_bw);

    /**
     * @brief   Get the current maximum audio bandwidth setting
     * @returns Returns the current maximum audio bandwidth setting
     */
    opus_int32 maxBandwidth(void);

    /**
     * @brief   Set the maximum audio bandwidth that the encoder will use
     * @param   new_bw The new bandwidth to set
     * @returns Returns the current maximum bandwidth setting
     *
     * Use this function to set the audio bandwidth for the encoder. This will
     * constrain the encoder to only use the specified bandwidth. Normally one
     * should use the setMaxBandwidth function since that will let the encoder
     * reduce the bandwith when appropriate.
     * The bandwidth is not given in Hz but as a number of constants.
     * OPUS_BANDWIDTH_NARROWBAND (4 kHz), OPUS_BANDWIDTH_MEDIUMBAND (6 kHz),
     * OPUS_BANDWIDTH_WIDEBAND (8 kHz), OPUS_BANDWIDTH_SUPERWIDEBAND (12 kHz),
     * OPUS_BANDWIDTH_FULLBAND (20 kHz) (default) 
     */
    opus_int32 setBandwidth(opus_int32 new_bw);

    /**
     * @brief   Get the current audio bandwidth setting
     * @returns Returns the current audio bandwidth setting
     */
    opus_int32 bandwidth(void);

    /**
     * @brief   Set which signal type to optimize the encoder for
     * @param   new_type The new signal type to set
     * @returns Returns the new signal type
     *
     * Use this function to set the signal type that Opus should optimize the
     * encoder for. Possible values are:
     * OPUS_AUTO: Select automatically (default)
     * OPUS_SIGNAL_VOICE: Bias thresholds towards choosing LPC or Hybrid modes. 
     * OPUS_SIGNAL_MUSIC: Bias thresholds towards choosing MDCT modes. 
     */
    opus_int32 setSignalType(opus_int32 new_type);

    /**
     * @brief   Get the currently configured signal type
     * @returns Returns the currently configured signal type
     */
    opus_int32 signalType(void);

    /**
     * @brief   Set which application type to optimize the encoder for
     * @param   new_type The new application type to set
     * @returns Returns the new application type
     *
     * Use this function to set the application type that Opus should optimize
     * the encoder for. Possible values are:
     * OPUS_APPLICATION_VOIP: Process for improved speech intelligibility.
     * OPUS_APPLICATION_AUDIO: Favor faithfulness to the original input. 
     * OPUS_APPLICATION_RESTRICTED_LOWDELAY: Configure the minimum possible
     *   coding delay by disabling certain modes of operation. 
     */
    opus_int32 setApplicationType(opus_int32 new_app);

    /**
     * @brief   Get the currently configured application type
     * @returns Returns the currently configured application type
     */
    opus_int32 applicationType(void);

    /**
     * @brief   Enable or disable inband FEC
     * @param   enable Set to \em true to enable or \em false to disable
     * @returns Returns if inband FEC is enabled or not
     *
     * Inband FEC is disabled by default.
     */
    bool enableInbandFec(bool enable);
    
    /**
     * @brief   Find out if variable bitrate is enabled or disabled
     * @returns Returns the current frame size
     */
    bool inbandFecEnabled(void);
    
    /**
     * @brief   Configures the encoder's expected packet loss percentage
     * @param   new_pl_perc Loss percentage in the range 0-100 (default: 0).
     */
    opus_int32 setExpectedPacketLoss(opus_int32 new_pl_perc);

    /**
     * @brief   Get the currently configured expected packet loss percentage
     * @returns Returns the currently configured packet loss percentage
     */
    opus_int32 expectedPacketLoss(void);

    /**
     * @brief   Enable or disable discontinuous transmission (DTX)
     * @param   enable Set to \em true to enable or \em false to disable
     * @returns Returns if DTX is enabled or not
     *
     * Discontinuous transmission (DTX) is disabled by default.
     */
    bool enableDtx(bool enable);
    
    /**
     * @brief   Find out if variable bitrate is enabled or disabled
     * @returns Returns the current frame size
     */
    bool dtxEnabled(void);
    
#if OPUS_MAJOR > 0
    /**
     * @brief   Configures the depth of signal being encoded
     * @param   new_depth The new LSB depth (8-24)
     * @returns The new LSB depth
     *
     * This is a hint which helps the encoder identify silence and
     * near-silence.
     */
    opus_int32 setLsbDepth(opus_int32 new_depth);

    /**
     * @brief   Get the current bitrate
     * @returns Returns the current bitrate
     */
    opus_int32 lsbDepth(void);
#endif

    /**
     * @brief   Resets encoder to be equivalent to a freshly initialized one
     */
    void reset(void);

#if 0
    /**
     * @brief 	Set the number of frames that are sent in each packet
     * @param 	fpp Frames per packet
     */
    void setFramesPerPacket(unsigned fpp);
#endif 
    
    /**
     * @brief   Translate a bandwidth id to a string
     * @param   bw The bandwidth id
     * @returns Returns the string corresponding to the given bandwidth id
     */
    static const char *bandwidthStr(opus_int32 bw);

    /**
     * @brief   Translate a signal type id to a string
     * @param   bw The signal type id
     * @returns Returns the string corresponding to the given signal type id
     */
    static const char *signalTypeStr(opus_int32 type);

    /**
     * @brief   Translate a application type id to a string
     * @param   bw The application type id
     * @returns Returns the string corresponding to the given application
     *          type id
     */
    static const char *applicationTypeStr(opus_int32 type);

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
    OpusEncoder *enc;
    int       frame_size;
    float     *sample_buf;
    int       buf_len;
    //int       frames_per_packet;
    //int       frame_cnt;
    
    AudioEncoderOpus(const AudioEncoderOpus&);
    AudioEncoderOpus& operator=(const AudioEncoderOpus&);
    
};  /* class AudioEncoderOpus */


} /* namespace */

#endif /* ASYNC_AUDIO_ENCODER_OPUS_INCLUDED */



/*
 * This file has not been truncated
 */

