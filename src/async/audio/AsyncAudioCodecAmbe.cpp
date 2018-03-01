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

#include "AsyncAudioCodecAmbe.h"
#include "AsyncAudioDecimator.h"
#include "AsyncAudioInterpolator.h"
#include "AsyncSigCAudioSink.h"
#include "AsyncSigCAudioSource.h"

// FIXME: Temporarily use these filter coefficients for resampling
#include "../../svxlink/trx/multirate_filter_coeff.h"


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

  /**
   * @brief   Represents a DV3k packet
   */
  class Packet
  {
    public:
      /**
       * @brief   Default constructor
       *
       * Initializes a packet with an empty buffer to be filled in later.
       */
      Packet(void) {}

      /**
       * @brief   Constructor that initialize a packet with a packet type
       * @param   type The packet type
       *
       * Initialize the Packet with the specified type and a zero length
       * indicator.
       */
      Packet(uint8_t type)
        : m_buf(DV3K_HEADER_SIZE)
      {
        m_buf[0] = DV3K_START_BYTE;
        m_buf[1] = 0;
        m_buf[2] = 0;
        m_buf[3] = type;
      }

      /**
       * @brief   Constructor that initialize a packet with type and one field
       * @param   type The message type
       * @param   field_id The id of the single field
       *
       * This constructor creates a Packet object with the given type and a
       * single field. The field can not have any extra data so the length
       * indicator is set to 1.
       */
      Packet(uint8_t type, uint8_t field_id)
        : m_buf(DV3K_HEADER_SIZE+1)
      {
        m_buf[0] = DV3K_START_BYTE;
        m_buf[1] = 0;
        m_buf[2] = 1;
        m_buf[3] = type;
        m_buf[4] = field_id;
      }

      /**
       * @brief   Constructor that init a packet with type and multiple fields
       * @param   The type of the packet
       * @param   argv The field id:s and field data
       * @param   argc The number of bytes in the field id:s and field datas
       *
       * This constructor can create a whole packet all at once with packet
       * type and multiple fields.
       */
      Packet(uint8_t type, const uint8_t* argv, size_t argc)
        : m_buf(DV3K_HEADER_SIZE+argc)
      {
        assert(argc <= 0xffff);
        m_buf[0] = DV3K_START_BYTE;
        m_buf[1] = argc >> 8;
        m_buf[2] = argc & 0xff;
        m_buf[3] = type;
        copy(argv, argv+argc, m_buf.begin()+4);
      }

      /**
       * @brief   Return a pointer to the fields area in the packet
       *
       * This function returns a pointer to the fields area of a packet. The
       * fields area must have been previously allocated by some of the
       * available means to do so.
       */
      uint8_t* argBuf(void)
      {
        assert(m_buf.size() > DV3K_HEADER_SIZE);
        return &m_buf.front() + DV3K_HEADER_SIZE;
      }

      /**
       * @brief   Set up size of fields packet area and return a pointer to it
       * @param   len The length of the fields area in bytes
       *
       * This function will resize the packet buffer to the given size, adding
       * the packet header size, and then a pointer to the new fields area is
       * returned.
       */
      uint8_t* argBuf(size_t len)
      {
        m_buf[1] = len >> 8;
        m_buf[2] = len & 0xff;
        uint16_t tot_len = len + DV3K_HEADER_SIZE;
        m_buf.resize(tot_len);
        return argBuf();
      }

      /**
       * @brief   Return a pointer to the start of the packet buffer
       */
      const char* data(void) const
      {
        return reinterpret_cast<const char*>(&m_buf.front());
      }

      /**
       * @brief   Return the size of the packet buffer
       */
      size_t size(void) const { return m_buf.size(); }

      /**
       * @brief   Returns the packet buffer as a hex string (debugging)
       */
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

      /**
       * @brief   Returns the packet type
       */
      uint8_t type(void) const
      {
        assert(m_buf.size() >= DV3K_HEADER_SIZE);
        return m_buf[3];
      }

      /**
       * @brief   Returns the size of the fields area
       */
      size_t fieldsSize(void) const
      {
        assert(m_buf.size() >= DV3K_HEADER_SIZE-1);
        return (static_cast<size_t>(m_buf[1]) << 8) + m_buf[2];
      }

      /**
       * @brief   Returns a count of how many bytes are currently missing
       *
       * This function will calculate how many bytes that are currently missing
       * in the buffer. When starting with a completely empty packet, at least
       * the number of bytes in a minimal packet header is expected. When the
       * header have been received, the packet fields area size can be used to
       * calculate how many bytes are missing.
       */
      size_t missingBytes(void) const
      {
        if (m_buf.size() < DV3K_HEADER_SIZE)
        {
          return DV3K_HEADER_SIZE - m_buf.size();
        }
        return fieldsSize() + DV3K_HEADER_SIZE - m_buf.size();
      }

      /**
       * @brief   Clear the buffer contents
       */
      void clear(void)
      {
        m_buf.clear();
      }

      /**
       * @brief   Add bytes to the end of the buffer
       *
       * NOTE: This function do NOT update the fields length indicator in the
       * packet.
       */
      void addBytes(const uint8_t *buf, size_t len)
      {
        m_buf.insert(m_buf.end(), buf, buf+len);
      }

    private:
      static const size_t DV3K_HEADER_SIZE  = 4;
      static const char   DV3K_START_BYTE   = 0x61;

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

      /**
       * @brief   Called to send speech samples to the encoder
       */
      virtual int writeSpeechSamples(const float *samples, int count)
      {
        assert(count > 0);

        if (m_state != READY)
        {
          return count;
        }

        size_t samples_left = static_cast<size_t>(count);
        while (samples_left > 0)
        {
            // Store incoming floats in a buffer until 160 samples have been
            // received
          size_t to_copy = std::min(DV3K_AUDIO_LEN-m_inbufcnt, samples_left);
          memcpy(m_inbuf+m_inbufcnt, samples, sizeof(float)*to_copy);
          m_inbufcnt += to_copy;
          samples_left -= to_copy;
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
      AudioCodecAmbeDv3k(void)
        : m_state(OFFLINE), m_inbufcnt(0),
          m_decimator(2, coeff_16_8, coeff_16_8_taps),
          m_interpolator(2, coeff_16_8, coeff_16_8_taps)
      {
        m_decimator.setBufferSize(DV3K_AUDIO_LEN);
        AudioSink::setHandler(&m_decimator);
        SigCAudioSink *sigc_sink = new SigCAudioSink;
        m_decimator.registerSink(sigc_sink, true);
        sigc_sink->sigWriteSamples.connect(
            sigc::mem_fun(*this, &AudioCodecAmbeDv3k::writeSpeechSamples));
        sigc_sink->sigFlushSamples.connect(
            sigc::mem_fun(*this, &AudioCodecAmbeDv3k::sourceAllSamplesFlushed));

        m_dec_output.registerSink(&m_interpolator);
        m_dec_output.sigAllSamplesFlushed.connect(
            AudioDecoder::allEncodedSamplesFlushed.make_slot());
        m_interpolator.setBufferSize(2*DV3K_AUDIO_LEN);
        AudioSource::setHandler(&m_interpolator);
      }
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
      State               m_state;
        // FIXME: Use std::vector instead or maybe a Packet to store samples
      float               m_inbuf[640];
      size_t              m_inbufcnt;
      Packet              m_packet;
      AudioDecimator      m_decimator;
      SigCAudioSource     m_dec_output;
      AudioInterpolator   m_interpolator;

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
              int ret = m_interpolator.writeSamples(t, sample_cnt);
              //cout << "### sample_cnt=" << sample_cnt
              //     << " ret=" << ret
              //     << endl;
              if (ret != static_cast<int>(sample_cnt))
              {
                cerr << "*** WARNING: " << (sample_cnt - ret)
                     << " samples dropped in AMBE encoder" << endl;
              }
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


AudioEncoderAmbe *AudioCodecAmbe::allocateEncoder(void)
{
  initializeCodecs();

  for (Codecs::iterator it=codecs.begin(); it!=codecs.end(); ++it)
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


AudioDecoderAmbe *AudioCodecAmbe::allocateDecoder(void)
{
  initializeCodecs();

  for (Codecs::iterator it=codecs.begin(); it!=codecs.end(); ++it)
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
  if (INTERNAL_SAMPLE_RATE != 16000)
  {
    cout << "*** ERROR: The AMBE audio codec can only be used with 16kHz "
            "sampling rate at the moment" << endl;
    return;
  }

  if (!codecs.empty())
  {
    return;
  }

    // FIXME: Only one codec with hardcoded parameters are created right now.
    // A method should be derived for how to get codec configuration into this
    // class.
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
  for (Codecs::iterator it=codecs.begin(); it!=codecs.end(); ++it)
  {
    all_unused &= (!(*it)->m_encoder_in_use && !(*it)->m_decoder_in_use);
  }
  if (all_unused)
  {
    //std::cout << "### Delete all AMBE codecs" << std::endl;
    for (Codecs::iterator it=codecs.begin(); it!=codecs.end(); ++it)
    {
      delete *it;
    }
    codecs.clear();
  }
} /* AudioCodecAmbe::deinitializeCodecs */


/*
 * This file has not been truncated
 */

