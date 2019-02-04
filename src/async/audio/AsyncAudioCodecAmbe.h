/**
@file	 AsyncAudioCodecAmbe.h
@brief   Contains a class to encode/decode AMBE to/from voice
@author  Christian Stussak / Imaginary & Tobias Blomberg / SM0SVX
         & Adi Bier / DL1HRC
@date	 2017-07-10

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2018 Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_CODEC_AMBE_INCLUDED
#define ASYNC_AUDIO_CODEC_AMBE_INCLUDED

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <vector>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioEncoder.h>
#include <AsyncAudioDecoder.h>


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
 * @brief AMBE encoder
 *
 * This is the type for an AMBE encoder. An instance of this class can not be
 * created or deleted directly since the constructor and destructor has
 * protected scope. Instead the AudioCodecAmbe::allocateEncoder() class
 * function must be used to get an instance of an encoder and the
 * AudioEncoderAmbe::release() member function must be used to free the
 * instance.
 */
class AudioEncoderAmbe : public AudioEncoder
{
  public:
    virtual void release(void) { releaseEncoder(); }

  protected:
    AudioEncoderAmbe(void) {}
    virtual ~AudioEncoderAmbe(void) {}
    virtual void releaseEncoder(void) = 0;
}; /* class AudioEncoderAmbe */


/**
 * @brief AMBE decoder
 *
 * This is the type for an AMBE decoder. An instance of this class can not be
 * created or deleted directly since the constructor and destructor has
 * protected scope. Instead the AudioCodecAmbe::allocateDecoder() class
 * function must be used to get an instance of a decoder and the
 * AudioDecoderAmbe::release() member function must be used to free the
 * instance.
 */
class AudioDecoderAmbe : public AudioDecoder
{
  public:
    virtual void release(void) { releaseDecoder(); }

  protected:
    AudioDecoderAmbe(void) {}
    virtual ~AudioDecoderAmbe(void) {}
    virtual void releaseDecoder(void) = 0;
}; /* class AudioDecoderAmbe */


/**
@brief  Implements usage of an AMBE hardware codec device
@author Christian Stussak / porst17 & Tobias Blomberg / SM0SVX
@date   2018-02-25

This class implements usage of an AMBE hardware audio codec device. As with all
audio codecs this class should not normally be used directly but instead the
Async::Audio::Encoder::create() and Async::AudioDecoder::create() class
functions should be used.
*/
class AudioCodecAmbe : public AudioEncoderAmbe, public AudioDecoderAmbe
{
  public:
    /**
     * @brief Allocate an unused encoder
     */
    static AudioEncoderAmbe *allocateEncoder(void);

    /**
     * @brief Allocate an unused decoder
     */
    static AudioDecoderAmbe *allocateDecoder(void);

    /**
     * @brief   Get the name of the codec
     * @return  Return the name of the codec
     */
    virtual const char *name(void) const { return "AMBE"; }

  protected:
    typedef std::map<std::string, std::string> DeviceOptions;

    /**
    * @brief 	Default constuctor
    */
    AudioCodecAmbe(void) : m_encoder_in_use(false), m_decoder_in_use(false) {}

    /**
     * @brief 	Destructor
     */
    virtual ~AudioCodecAmbe(void) {}

    /**
     * @brief   Initialize the codec device
     * @param   options Codec device options
     */
    virtual bool initialize(const DeviceOptions& options=DeviceOptions()) = 0;

    /**
     * @brief Release a previously allocated encoder
     */
    virtual void releaseEncoder(void);

    /**
     * @brief Release a previously allocated decoder
     */
    virtual void releaseDecoder(void);

  private:
    typedef std::vector<AudioCodecAmbe*> Codecs;

    static Codecs codecs;

    bool m_encoder_in_use;
    bool m_decoder_in_use;

    static void initializeCodecs(void);
    static void deinitializeCodecs(void);

    AudioCodecAmbe(const AudioCodecAmbe&);
    AudioCodecAmbe& operator=(const AudioCodecAmbe&);
};  /* class AudioCodecAmbe */

} /* namespace */

#endif /* ASYNC_AUDIO_CODEC_AMBE_INCLUDED */

/*
 * This file has not been truncated
 */
