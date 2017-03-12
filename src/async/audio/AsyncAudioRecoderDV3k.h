/**
@file	 AsyncAudioRecoderDV3k.h
@brief   Contains an audio pipe class for amplification/attenuation
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-03-10

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2017  Tobias Blomberg / SM0SVX

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

#ifndef ASYNC_AUDIO_RECODER_DV3K
#define ASYNC_AUDIO_RECODER_DV3K


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <cmath>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioRecoder.h>
#include <AsyncSerial.h>


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

#define DV3K_TYPE_CONTROL 0x00
#define DV3K_TYPE_AMBE 0x01
#define DV3K_TYPE_AUDIO 0x02

/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	An audio pipe class for AudioRecoderDV3k
@author Tobias Blomberg / SM0SVX
@date   2006-07-08

Use this class to amplify or attenuate an audio stream.
*/
class AudioRecoderDV3k : public AudioRecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioRecoderDV3k(void);

    /**
     * @brief 	Destructor
     */
    virtual ~AudioRecoderDV3k(void);

    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "DV3k"; }

    /**
     * @brief 	Set an option for the decoder
     * @param 	name The name of the option
     * @param 	value The value of the option
     */
    virtual void setOption(const std::string &name, const std::string &value);

    /**
     * @brief 	Write encoded samples into the decoder
     * @param 	buf  Buffer containing encoded samples
     * @param 	size The size of the buffer
     */
    virtual void writeDecodedSamples(void *buf, int size);



  protected:
    void processSamples(float *dest, const float *src, int count)
    {
      for (int i=0; i<count; ++i)
      {
      	dest[i] = src[i] * m_gain;
      }
    }


  private:
    AudioRecoderDV3k(const AudioRecoderDV3k&);
    AudioRecoderDV3k& operator=(const AudioRecoderDV3k&);

    Async::Serial *dv3kdev;

   static const unsigned char DV3K_START_BYTE   = 0x61;
   static const unsigned char DV3K_CONTROL_RATEP  = 0x0A;
   static const unsigned char DV3K_CONTROL_PRODID = 0x30;
   static const unsigned char DV3K_CONTROL_VERSTRING = 0x31;
   static const unsigned char DV3K_CONTROL_RESET = 0x33;
   static const unsigned char DV3K_CONTROL_READY = 0x39;
   static const unsigned char DV3K_CONTROL_CHANFMT = 0x15;


    struct dv3k_packet {
      unsigned char start_byte;
      struct {
        unsigned short payload_length;
        unsigned char packet_type;
      } header;
      union {
        struct {
          unsigned char field_id;
          union {
            char prodid[16];
            char ratep[12];
            char version[48];
            short chanfmt;
          } data;
        } ctrl;
        struct {
          unsigned char field_id;
          unsigned char num_samples;
          short samples[160];
          unsigned char cmode_field_id;
          short cmode_value;
        } audio;
        struct {
          struct {
            unsigned char field_id;
            unsigned char num_bits;
            unsigned char data[9];
          } data;
          struct {
            unsigned char field_id;
            unsigned short value;
          } cmode;
          struct {
            unsigned char field_id;
            unsigned char tone;
            unsigned char amplitude;
          } tone;
        } ambe;
    } payload;
  };

    struct dv3k_packet responsePacket;
    struct dv3k_packet ctrlPacket;


    float m_gain;
    std::string port;
    int baud;


    bool  createDV3k(void);
    bool initDV3k(void);
    void onCharactersReceived(char *buf, int count);
    void processResponse(char *buf, int count);

};  /* class AudioRecoderDV3k */


} /* namespace */

#endif /* ASYNC_AUDIO_RECODER_DV3K */



/*
 * This file has not been truncated
 */

