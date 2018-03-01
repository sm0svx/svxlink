/**
@file	 AsyncAudioCodecAmbe.cpp
@brief   Contains a class to encode/decode ambe to/from voice
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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cassert>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncSerial.h>
#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>
#include <AsyncDnsLookup.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include <AsyncAudioCodecAmbe.h>
//#include <AsyncAudioDecimator.h>


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace Async;
using namespace std;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

std::vector<AudioCodecAmbe*> AudioCodecAmbe::codecs;


/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


namespace {
#if 0
  std::ostream& operator<<(std::ostream& stream,
                           const std::vector<uint8_t>& vec)
  {
    for (size_t i=0; i<vec.size(); ++i)
    {
      unsigned byte = static_cast<unsigned>((&vec.front())[i]) & 0xff;
      stream << "<" << hex << setw(2) << setfill('0') 
             << byte << dec << ">";
    }
    return stream;
  }
#endif

  class Packet
  {
    public:
      Packet(void) {}

      Packet(uint8_t type)
        : m_buf(4)
      {
        m_buf[0] = DV3K_START_BYTE;
        m_buf[1] = 0;
        m_buf[2] = 0;
        m_buf[3] = type;
      }

      Packet(uint8_t type, uint8_t arg1)
        : m_buf(5)
      {
        m_buf[0] = DV3K_START_BYTE;
        m_buf[1] = 0;
        m_buf[2] = 1;
        m_buf[3] = type;
        m_buf[4] = arg1;
      }

      Packet(uint8_t type, const uint8_t* argv, size_t argc)
        : m_buf(4+argc)
      {
        m_buf[0] = DV3K_START_BYTE;
        m_buf[1] = 0;
        m_buf[2] = argc;
        m_buf[3] = type;
        for (size_t i=0; i<argc; ++i)
        {
          m_buf[4+i] = argv[i];
        }
      }

      uint8_t* argBuf(void)
      {
        assert(m_buf.size() > 4);
        return &m_buf.front() + 4;
      }

      uint8_t* argBuf(size_t len)
      {
        m_buf[1] = len >> 8;
        m_buf[2] = len & 0xff;
        uint16_t tot_len = len + 4;
        m_buf.resize(tot_len);
        return argBuf();
      }

      const char* data(void) const
      {
        return reinterpret_cast<const char*>(&m_buf.front());
      }
      size_t size(void) const { return m_buf.size(); }

      std::string toString(void) const
      {
        std::ostringstream ss;
        for (size_t i=0; i<m_buf.size(); ++i)
        {
          unsigned byte = static_cast<unsigned>((&m_buf.front())[i]) & 0xff;
          ss << "<" << hex << setw(2) << setfill('0') 
             << byte << ">";
        }
        return ss.str();
      }

      uint8_t type(void) const
      {
        assert(m_buf.size() >= 4);
        return m_buf[3];
      }

      size_t fieldsSize(void) const
      {
        assert(m_buf.size() >= 3);
        return (static_cast<size_t>(m_buf[1]) << 8) + m_buf[2];
      }

      size_t missingBytes(void) const
      {
        if (m_buf.size() < 4)
        {
          return 4 - m_buf.size();
        }
        return fieldsSize() + 4 - m_buf.size();
      }

      void clear(void)
      {
        m_buf.clear();
      }

      void addBytes(const uint8_t *buf, size_t len)
      {
        m_buf.insert(m_buf.end(), buf, buf+len);
      }

    private:
      static const char DV3K_START_BYTE = 0x61;

      std::vector<uint8_t> m_buf;
  }; /* Packet */


  /**
   * @brief Implement shared Dv3k code here (e.g. initialiation and protocol)
   */
  class AudioCodecAmbeDv3k : public AudioCodecAmbe
  {
    public:
      /**
       * @brief Write encoded samples into the decoder
       * @param buf  Buffer containing encoded samples
       * @param size The size of the buffer
       */
      virtual void writeEncodedSamples(const void *buf, int size)
      {
        //uint8_t fields[] = { 0x03, 0xa0, 0x08, 0x81, 0x00 };
        //Packet packet(DV3K_TYPE_CHANNEL, fields, sizeof(fields));
        Packet packet(DV3K_TYPE_CHANNEL);
        memcpy(packet.argBuf(size), buf, size);
        send(packet);
      }

        /* Method is called to send speech samples to the encoder */
      virtual int writeSamples(const float *samples, int count)
      {
        if (m_state != READY)
        {
          return count;
        }

        while (count > 0)
        {
            // Store incoming floats in a buffer until 160 samples have been
            // received
          size_t to_copy = std::min(DV3K_AUDIO_LEN-m_inbufcnt,
                                    static_cast<size_t>(count));
          memcpy(m_inbuf+m_inbufcnt, samples, sizeof(float)*to_copy);
          m_inbufcnt += to_copy;
          count -= to_copy;
          samples += to_copy;
          if (m_inbufcnt == DV3K_AUDIO_LEN)
          {
            sendSpeechPacket();
            m_inbufcnt = 0;
          }
        }
        return count;
      }

    protected:
      static const char DV3K_TYPE_CONTROL       = 0x00;
      static const char DV3K_TYPE_CHANNEL       = 0x01;
      static const char DV3K_TYPE_SPEECH        = 0x02;

      //static const char DV3K_CONTROL_RATEP      = 0x0A;
      static const char DV3K_CONTROL_PRODID     = 0x30;
      static const char DV3K_CONTROL_VERSTRING  = 0x31;
      static const char DV3K_CONTROL_RESET      = 0x33;
      //static const char DV3K_CONTROL_READY      = 0x39;
      //static const char DV3K_CONTROL_CHANFMT    = 0x15;

      static const size_t   DV3K_AUDIO_LEN      = 160;

      /**
       * @brief Default constuctor
       */
      AudioCodecAmbeDv3k(void) : m_state(OFFLINE), m_inbufcnt(0) {}
      virtual ~AudioCodecAmbeDv3k(void) {}

      void reset(void)
      {
          // Reset the Dv3k stick (ThumbDV dongle)
        send(Packet(DV3K_TYPE_CONTROL, DV3K_CONTROL_RESET));
        m_state = RESET;
      }

      void callback(const std::vector<uint8_t>& buffer)
      {
        //cout << "### callback: buffer=" << buffer << endl;
        const uint8_t *ptr = &buffer.front();
        size_t len = buffer.size();
        while (len > 0)
        {
          size_t missing_bytes = m_packet.missingBytes();
          assert(missing_bytes > 0);
          size_t copy_bytes = std::min(missing_bytes, len);
          m_packet.addBytes(ptr, copy_bytes);
          ptr += copy_bytes;
          len -= copy_bytes;
          if (m_packet.missingBytes() == 0)
          {
            handlePacket();
            m_packet.clear();
          }
        }
      }

      virtual void send(const Packet& packet) = 0;

    private:
      enum State
      {
        OFFLINE, RESET, INIT, PRODID, VERSID, CPARAMS, READY, WARNING, ERROR
      };
      State     m_state;
      float     m_inbuf[640];
      size_t    m_inbufcnt;
      Packet    m_packet;

      AudioCodecAmbeDv3k(const AudioCodecAmbeDv3k&);
      AudioCodecAmbeDv3k& operator=(const AudioCodecAmbeDv3k&);

      void prodid(void)
      {
          // Reads the product id from dv3k stick, just to give it out for
          // debug purposes
        send(Packet(DV3K_TYPE_CONTROL, DV3K_CONTROL_PRODID));
        m_state = PRODID;  /* state is requesting prod-id of Stick */
      } /* prodid */

      void versid(void)
      {
        send(Packet(DV3K_TYPE_CONTROL, DV3K_CONTROL_VERSTRING));
        m_state = VERSID;  /* state is requesting version-id of Stick */
      } /* versid */

      void setCodecParams(void)
      {
        uint8_t codec_args[] = {
          0x40,             // PKT_CHANNEL0
          0x0b, 0x03,       // PKT_INIT (initialize both endocder and decoder)
          0x09, 0x21,       // PKT_RATET (#33 = speech rate 2450, FEC rate 1150)
          0x32, 0x00,       // PKT_COMPAND (companding disabled)
          //0x05, 0x00, 0x40, // PKT_ECMODE (Enable noise suppression)
        };
        send(Packet(DV3K_TYPE_CONTROL, codec_args, sizeof(codec_args)));
        m_state = CPARAMS;  /* state is requesting version-id of Stick */
      } /* setCodecParams */

      void handlePacket(void)
      {
        assert(m_packet.missingBytes() == 0);

        //cout << "### Complete packet:"
        //     << " type=" << (static_cast<unsigned>(m_packet.type()) & 0xff)
        //     << " fieldsSize=" << m_packet.fieldsSize()
        //     << " buf=" << m_packet.toString()
        //     << endl;

        if (m_packet.type() == DV3K_TYPE_CONTROL)       // Control packet
        {
          handleControlPacket();
        }
        else if (m_packet.type() == DV3K_TYPE_CHANNEL)  // Channel (AMBE) packet
        {
          AudioEncoder::writeEncodedSamples(m_packet.argBuf(),
                                            m_packet.fieldsSize());
        }
        else if (m_packet.type() == DV3K_TYPE_SPEECH)   // Speech packet
        {
          handleSpeechPacket();
        }
        else
        {
          cout << "*** WARNING: Unknown DV3K packet type received: "
               << (static_cast<unsigned>(m_packet.type()) & 0xff) << endl;
        }
      }

      void handleControlPacket(void)
      {
        if (m_packet.fieldsSize() < 1)
        {
          cerr << "*** WARNING: Illegal control packet recieved from DV3k "
               << "AMBE codec hardware" << endl;
          return;
        }

          // FIXME: Parse incoming control messages/answers
#if 0
        uint8_t *fields = m_packet.argBuf();
        uint8_t field_id = *fields++;
        switch (field_id)
        {
          case 0x40: // PKT_CHANNEL0
            break;
          case 0x05: // PKT_ECMODE
            break;
          case 0x06: // PKT_DCMODE
            break;
          case 0x32: // PKT_COMPAND
            break;
          case 0x09: // PKT_RATET
            break;
          case 0x0A: // PKT_RATEP
            break;
          case 0x0B: // PKT_INIT
            break;
          case 0x10: // PKT_LOWPOWER
            break;
          case 0x30: // PKT_PRODID
            break;
          case 0x31: // PKT_VERSTRING
            break;
          case 0x39: // PKT_READY
            break;
        }
#endif

        if (m_state == RESET)
        {
          cout << "--- DV3K: Reset OK" << endl;
          prodid();
        }
        else if (m_state == PRODID)
        {
            /* give out product name of DV3k */
          std::string product_id(reinterpret_cast<char*>(m_packet.argBuf()+1),
                                 m_packet.fieldsSize()-1);
          cout << "--- DV3K (ProdID): "  << product_id << endl;
          versid();
        }
        else if (m_state == VERSID)
        {
            /* give out version of DV3k */
          std::string version_id(reinterpret_cast<char*>(m_packet.argBuf()+1),
                                 m_packet.fieldsSize()-1);
          cout << "--- DV3K (VersID): " << version_id << endl;
          setCodecParams();
        }
        else if (m_state == CPARAMS)
        {
            // Sending configuration/rate params to DV3k was OK
          cout << "--- DV3K: Ready" << endl;
          m_state = READY;
        }
      }

      void handleSpeechPacket(void)
      {
        uint8_t *fields = reinterpret_cast<uint8_t*>(m_packet.argBuf());
        while (fields < m_packet.argBuf()+m_packet.fieldsSize())
        {
          uint8_t field_ident = *fields++;
          switch (field_ident)
          {
            case 0x40: // PKT_CHANNEL0
              cout << "### Received PKT_CHANNEL0 speech packet field" << endl;
              break;

            case 0x00: // SPEECHD
            {
              const size_t sample_cnt = *fields++;
              float t[sample_cnt];
              size_t b = 0;
              for (size_t i=0; i<sample_cnt*2; i+=2)
              {
                int16_t w = static_cast<int16_t>(*fields++) << 8;
                w |= *fields++;
                t[b++] = static_cast<float>(w) / 32767.0;
              }
              AudioDecoder::sinkWriteSamples(t, sample_cnt);
              break;
            }

            case 0x02: // CMODE
              // FIXME: Implement CMODE handling
              fields += 2;
              cout << "### Received CMODE speech packet field" << endl;
              break;

            case 0x08: // TONE
              // FIXME: Implement TONE handling
              fields += 2;
              cout << "### Received TONE speech packet field" << endl;
              break;

            default:
            {
              std::cout << "*** WARNING: Unknown AMBE speech packet field "
                           "identifier: " << field_ident << endl;
              return;
            }
          }
        }
      }

      void sendSpeechPacket(void)
      {
        Packet packet(DV3K_TYPE_SPEECH);
        const uint8_t DV3K_VOICE_FRAME[] = { 0x00, DV3K_AUDIO_LEN };
        uint8_t *dv3k_buffer = packet.argBuf(sizeof(DV3K_VOICE_FRAME) +
                                             2*DV3K_AUDIO_LEN);
        memcpy(dv3k_buffer, DV3K_VOICE_FRAME, sizeof(DV3K_VOICE_FRAME));

          // Convert samples to int16_t and pack into packet buffer
        uint8_t *t_data = dv3k_buffer + sizeof(DV3K_VOICE_FRAME);
        for (size_t i=0; i<DV3K_AUDIO_LEN; ++i)
        {
          int16_t w = static_cast<int16_t>(m_inbuf[i] * 32767.0);
          t_data[2*i] = w >> 8;
          t_data[2*i+1] = w & 0xff;
        }

        send(packet);
      }
  };


  /**
   * TODO: Implement communication with Dv3k via UDP here.
   */
#if 0
  class AudioCodecAmbeDv3kAmbeServer : public AudioCodecAmbeDv3k
  {
    public:
      /**
       * @brief  Default constuctor
       *
       * TODO: parse device_id for IP and PORT
       */
      AudioCodecAmbeDv3kAmbeServer(const std::string &device_id)
        : ambesock(0), dns(0)
      {
      }

      virtual ~AudioCodecAmbeDv3kAmbeServer(void)
      {
        delete dns;
        dns = 0;
        delete ambesock;
      }

      virtual bool initialize(const DeviceOptions &options)
      {
        DeviceOptions::const_iterator it;

        if ((it = options.find("AMBESERVER_HOST")) != options.end())
        {
          ambehost = (*it).second;
        }
        else
        {
          cerr << "*** ERROR: Parameter AMBESERVER_HOST not defined." << endl;
          return false;
        }

        if((it = options.find("AMBESERVER_PORT")) != options.end())
        {
          ambeport = atoi((*it).second.c_str());
        }
        else
        {
          cerr << "*** ERROR: Parameter AMBESERVER_PORT not defined." << endl;
          return false;
        }
        udpInit();
        return true;
      }

    protected:
      virtual void send(const Packet& packet)
      {
        ambesock->write(ambehost, ambeport, packet.data(), packet.size());
      }

    private:
      int         ambeport;
      string      ambehost;
      UdpSocket*  ambesock;
      IpAddress	  ip_addr;
      DnsLookup*  dns;

      AudioCodecAmbeDv3kAmbeServer(const AudioCodecAmbeDv3kAmbeServer&);
      AudioCodecAmbeDv3kAmbeServer& operator=(
          const AudioCodecAmbeDv3kAmbeServer&);

      void callbackUdp(const IpAddress& addr, uint16_t port,
                       void *buf, int count)
      {
        const uint8_t *packet_buf = reinterpret_cast<uint8_t*>(buf);
        callback(std::vector<uint8_t>(packet_buf, packet_buf+count));
      } /* callbackUdp */

      void udpInit(void)
      {
        if (ip_addr.isEmpty())
        {
          dns = new DnsLookup(ambehost);
          dns->resultsReady.connect(mem_fun(*this,
                &AudioCodecAmbeDv3kAmbeServer::dnsResultsReady));
          return;
        }

        delete ambesock;
        ambesock = new UdpSocket();
        ambesock->dataReceived.connect(mem_fun(*this,
              &AudioCodecAmbeDv3kAmbeServer::callbackUdp));

        cout << "--- DV3k: UdpSocket " << ambehost << ":" << ambeport
          << " created." << endl;
        reset();
      } /* udpInit */

        /* called-up when dns has been resolved */
      void dnsResultsReady(DnsLookup& dns_lookup)
      {
        vector<IpAddress> result = dns->addresses();

        delete dns;
        dns = 0;
        if (result.empty() || result[0].isEmpty())
        {
          ip_addr.clear();
          cout << "*** ERROR: Could not found host." << endl;
          return;
        }
        ip_addr = result[0];
        udpInit();
      } /* dnsResultReady */
  }; /* AudioCodecAmbeDv3kAmbeServer */
#endif

  class AudioCodecAmbeDv3kTty : public AudioCodecAmbeDv3k
  {
    public:
      /**
       * @brief Constuctor
       * @param device_id The device identifier string
       */
      AudioCodecAmbeDv3kTty(void) : m_serial(0) {}

      /**
       * @brief Destructor
       */
      virtual ~AudioCodecAmbeDv3kTty(void)
      {
        m_serial->close();
        delete m_serial;
      }

      virtual bool initialize(const DeviceOptions &options)
      {
        DeviceOptions::const_iterator it = options.find("TTY_DEVICE");
        string device;
        if (it != options.end())
        {
          device = (*it).second;
        }
        else
        {
          cerr << "*** ERROR: Parameter TTY_DEVICE not defined." << endl;
          return false;
        }

        int baudrate;
        it = options.find("TTY_BAUDRATE");
        if(it != options.end())
        {
          baudrate = atoi((*it).second.c_str());
        }
        else
        {
          cerr << "*** ERROR: Parameter TTY_BAUDRATE not defined." << endl;
          return false;
        }

        if ((baudrate != 230400) && (baudrate != 460800))
        {
          cerr << "*** ERROR: AMBE TTY_BAUDRATE must be 230400 or 460800."
               << endl;
          return false;
        }

        m_serial = new Serial(device);
        if (!(m_serial->open(true)))
        {
          delete m_serial;
          m_serial = 0;
          cerr << "*** ERROR: Can not open serial device " << device << endl;
          return false;
        }
        if (!m_serial->setParams(baudrate, Serial::PARITY_NONE, 8, 1,
                               Serial::FLOW_HW))
        {
          delete m_serial;
          m_serial = 0;
          cerr << "*** ERROR: Can not set parameters for serial device: "
               << device << endl;
          return false;
        }
        m_serial->charactersReceived.connect(
            sigc::mem_fun(*this, &AudioCodecAmbeDv3kTty::callbackTty));
        reset();
        return true;
      }

    protected:
      virtual void send(const Packet& packet)
      {
        //cout << "### send: " << packet.toString() << endl;
        int ret = m_serial->write(packet.data(), packet.size());
        assert(static_cast<size_t>(ret) == packet.size());
      }

    private:
      Serial *m_serial;

      AudioCodecAmbeDv3kTty(const AudioCodecAmbeDv3kTty&);
      AudioCodecAmbeDv3kTty& operator=(const AudioCodecAmbeDv3kTty&);

      void callbackTty(const char *buf, int count)
      {
        callback(std::vector<uint8_t>(buf, buf+count));
      } /* callbackTty */
  }; /* class AudioCodecAmbeDv3kTty */
}


AudioCodecAmbe *AudioCodecAmbe::allocateEncoder(void)
{
  initializeCodecs();

  for (CodecVector::iterator it=codecs.begin(); it!=codecs.end(); ++it)
  {
    if (!(*it)->m_encoder_in_use)
    {
      (*it)->m_encoder_in_use = true;
      return *it;
    }
  }

  return 0;
} /* AudioCodecAmbe::allocateEncoder */


void AudioCodecAmbe::releaseEncoder(void)
{
  //std::cout << "### releaseEncoder" << std::endl;
  assert(m_encoder_in_use);
  m_encoder_in_use = false;
  deinitializeCodecs();
} /* AudioCodecAmbe::releaseEncoder */


AudioCodecAmbe *AudioCodecAmbe::allocateDecoder(void)
{
  initializeCodecs();

  for (CodecVector::iterator it=codecs.begin(); it!=codecs.end(); ++it)
  {
    if (!(*it)->m_decoder_in_use)
    {
      (*it)->m_decoder_in_use = true;
      return *it;
    }
  }

  return 0;
} /* AudioCodecAmbe::allocateDecoder */


void AudioCodecAmbe::releaseDecoder(void)
{
  //std::cout << "### releaseDecoder" << std::endl;
  assert(m_decoder_in_use);
  m_decoder_in_use = false;
  deinitializeCodecs();
} /* AudioCodecAmbe::releaseDecoder */


void AudioCodecAmbe::initializeCodecs(void)
{
  if (!codecs.empty())
  {
    return;
  }

  AudioCodecAmbe *codec = new AudioCodecAmbeDv3kTty;
  DeviceOptions options;
  options["TTY_DEVICE"] = "/dev/ttyUSB0";
  options["TTY_BAUDRATE"] = "460800";
  if ((codec == 0) || !codec->initialize(options))
  {
    std::cerr << "*** ERROR: Could not initialize AMBE codec" << std::endl;
    delete codec;
    return;
  }
  codecs.push_back(codec);
} /* AudioCodecAmbe::initializeCodecs */


void AudioCodecAmbe::deinitializeCodecs(void)
{
  bool all_unused = true;
  for (CodecVector::iterator it=codecs.begin(); it!=codecs.end(); ++it)
  {
    all_unused &= (!(*it)->m_encoder_in_use && !(*it)->m_decoder_in_use);
  }
  if (all_unused)
  {
    //std::cout << "### Delete all AMBE codecs" << std::endl;
    for (CodecVector::iterator it=codecs.begin(); it!=codecs.end(); ++it)
    {
      delete *it;
    }
    codecs.clear();
  }
} /* AudioCodecAmbe::deinitializeCodecs */



