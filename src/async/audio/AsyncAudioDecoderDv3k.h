/**
@file	 AsyncAudioDecoderDv3k.h
@brief   An audio decoder that use the Opus audio codec
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-04-02

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

#ifndef ASYNC_AUDIO_DECODER_DV3K_INCLUDED
#define ASYNC_AUDIO_DECODER_DV3K_INCLUDED


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

#include <AsyncAudioDecoder.h>
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

namespace Async
{
  class Serial;
};


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
const char DV3000_START_BYTE   = 0x61U;
const char DV3000_TYPE_CONTROL = 0x00U;
const char DV3000_TYPE_AMBE    = 0x01U;
const char DV3000_TYPE_AUDIO   = 0x02U;
const char DV3000_CONTROL_RATEP  = 0x0AU;
const char DV3000_CONTROL_PRODID = 0x30U;
const char DV3000_CONTROL_READY  = 0x39U;

const char DVD_REQ_NAME[] = {0x04, 0x20, 0x01, 0x00};
const unsigned int DVD_REQ_NAME_LEN = 4U;

const char DV3000_REQ_PRODID[] = {DV3000_START_BYTE, 0x00U, 0x01U, DV3000_TYPE_CONTROL, DV3000_CONTROL_PRODID};
const unsigned int DV3000_REQ_PRODID_LEN    = 5U;

const char DV3000_REQ_RATEP[] = {DV3000_START_BYTE, 0x00U, 0x0DU, DV3000_TYPE_CONTROL, DV3000_CONTROL_RATEP, 0x01U, 0x30U, 0x07U, 0x63U, 0x40U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x00U, 0x48U};
const unsigned int DV3000_REQ_RATEP_LEN     = 17U;

const unsigned char DV3000_AUDIO_HEADER[] = {DV3000_START_BYTE, 0x01U, 0x42U, DV3000_TYPE_AUDIO, 0x00U, 0xA0U};
const unsigned char DV3000_AUDIO_HEADER_LEN = 6U;

const unsigned char DV3000_AMBE_HEADER[] = {DV3000_START_BYTE, 0x00U, 0x0BU, DV3000_TYPE_AMBE, 0x01U, 0x48U};
const unsigned char DV3000_AMBE_HEADER_LEN  = 6U;

const unsigned int DV3000_HEADER_LEN = 4U;



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
@brief	An audio decoder that use the Opus audio codec
@author Tobias Blomberg / SM0SVX
@date   2017-03-16

This class implements an audio decoder that use the Opus audio codec.
*/
class AudioDecoderDv3k : public Async::AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderDv3k(void);

    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderDv3k(void);

    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "DV3K"; }

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
    virtual void writeEncodedSamples(void *buf, int size);


  protected:


  private:

    AudioDecoderDv3k(const AudioDecoderDv3k&);
    AudioDecoderDv3k& operator=(const AudioDecoderDv3k&);

    std::string      device;
    int              baudrate;
    Async::Serial    *dv3kfd;

    enum STATE {
      PROD_ID,
      RATEP,
    };

    STATE m_state;
    unsigned char outbuf[200];
    int outcnt;

    void openDv3k(void);
    void charactersReceived(char *buf, int len);
    void handleControl(char *buf, int len);
    void sendAudio(char *buf, int len);
    void sendToDv3k(unsigned char *buf, int len);

};  /* class AudioDecoderDv3k */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_DV3K_INCLUDED */



/*
 * This file has not been truncated
 */

