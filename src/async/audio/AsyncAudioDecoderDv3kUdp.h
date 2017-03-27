/**
@file	 AsyncAudioDecoderDv3kUdp.h
@brief   An audio decoder that use the Opus audio codec
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-03-16

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

#ifndef ASYNC_AUDIO_DECODER_DV3KUDP_INCLUDED
#define ASYNC_AUDIO_DECODER_DV3KUDP_INCLUDED


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
#include <AsyncDnsLookup.h>
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
  class UdpHandler;
  class DnsLookup;
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
class AudioDecoderDv3kUdp : public Async::AudioDecoder
{
  public:
    /**
     * @brief 	Default constuctor
     */
    AudioDecoderDv3kUdp(void);

    /**
     * @brief 	Destructor
     */
    virtual ~AudioDecoderDv3kUdp(void);

    /**
     * @brief   Get the name of the codec
     * @returns Return the name of the codec
     */
    virtual const char *name(void) const { return "AMBESERVER"; }

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

    int                   port;
    std::string           host;
    Async::UdpHandler      *m_udp_sock;
    Async::IpAddress	  ip_addr;
    Async::DnsLookup	  *dns;

    AudioDecoderDv3kUdp(const AudioDecoderDv3kUdp&);
    AudioDecoderDv3kUdp& operator=(const AudioDecoderDv3kUdp&);

    void connect(void);
    void dnsResultsReady(DnsLookup& dns_lookup);
    void onDataReceived(const IpAddress& addr, uint16_t port,
                                         void *buf, int count);
    void sendData(void *data, int size);


};  /* class AudioDecoderDv3kUdp */


} /* namespace */

#endif /* ASYNC_AUDIO_DECODER_DV3KUDP_INCLUDED */



/*
 * This file has not been truncated
 */

