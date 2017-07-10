#include <AsyncAudioCodecAmbe.h>
#include <AsyncAudioDecimator.h>
#include <AsyncSerial.h>
#include <AsyncUdpSocket.h>
#include <AsyncIpAddress.h>
#include <AsyncDnsLookup.h>

#include <string>
#include <map>
#include <stdlib.h>
#include <string.h>
#include <iostream>


#include <cassert>

using namespace Async;
using namespace std;

namespace {
    /*
        Multiton pattern template. It's similar to the singleton pattern, but
        enables multiple instances through the use of keys.
        NOTE: Manual destruction must be done before program exit. Not thread-safe.

        class Foo : public Multiton<Foo> {};
        Foo &foo = Foo::getRef("foobar");
        foo.bar();
        Foo::destroyAll();
     */

    template <typename T, typename Key = map<string,string> >
    class Multiton
    {
    public:
        static void destroyAll()
        {
            for (typename map<Key, T*>::const_iterator it = instances.begin(); it != instances.end(); ++it)
                delete (*it).second;
            instances.clear();
        }

        static void destroy(const Key &key)
        {
            typename map<Key, T*>::iterator it = instances.find(key);

            if (it != instances.end()) {
                delete (*it).second;
                instances.erase(it);
            }
        }

        static T* getPtr(const Key &key)
        {
            const typename map<Key, T*>::const_iterator it = instances.find(key);

            if (it != instances.end())
                return (T*)(it->second);

            T* instance = T::create(key);
            instances[key] = instance;
            return instance;
        }

        static T& getRef(const Key &key)
        {
            return *getPtr(key);
        }

    protected:
        Multiton() {}
        ~Multiton() {}

    private:
        Multiton(const Multiton&);
        Multiton& operator=(const Multiton&);

        static map<Key, T*> instances;
    };

    template <typename T, typename Key>
    map<Key, T*> Multiton<T, Key>::instances;

    /**
     * @brief Implement shared Dv3k code here
     * (e.g. initialiation and protocol)
     */
    class AudioCodecAmbeDv3k : public AudioCodecAmbe, public Multiton<AudioCodecAmbeDv3k,AudioCodecAmbe::Options> {
    public:
        template <typename T = char>
        struct Buffer {
            Buffer(T *data = (T*) NULL, size_t length = 0) : data(data), length(length) {}
            T *data;
            size_t length;
        };

        static AudioCodecAmbeDv3k *create(const Options &options);

        virtual void init()
        {
          char DV3K_REQ_PRODID[] = {DV3K_START_BYTE, 0x00, 0x01, DV3K_TYPE_CONTROL, DV3K_CONTROL_PRODID};
          Buffer<> init_packet = Buffer<>(DV3K_REQ_PRODID,sizeof(DV3K_REQ_PRODID));
          m_state = RESET;
          send(init_packet);
        }

        virtual void prodid()
        {
          char DV3K_REQ_PRODID[] = {DV3K_START_BYTE, 0x00, 0x01,
                                   DV3K_TYPE_CONTROL, DV3K_CONTROL_PRODID};
          Buffer<> prodid_packet = Buffer<>(DV3K_REQ_PRODID,sizeof(DV3K_REQ_PRODID));
          send(prodid_packet);
          m_state = PRODID;  /* state is requesting prod-id of Stick */
        } /* getProdId */

        virtual void versid()
        {
          char DV3K_REQ_VERSID[] = {DV3K_START_BYTE, 0x00, 0x01,
                                   DV3K_TYPE_CONTROL, DV3K_CONTROL_VERSTRING};
          Buffer<> versid_packet = Buffer<>(DV3K_REQ_VERSID,sizeof(DV3K_REQ_VERSID));
          send(versid_packet);
          m_state = VERSID;  /* state is requesting version-id of Stick */
        } /* versid */

        virtual void ratep()
        {
          char DV3K_REQ_RATEP[] = {DV3K_START_BYTE, 0x00, 0x07,
                                   DV3K_TYPE_CONTROL, 0x40, 0x0b, 0x03,
                                   0x09, 0x21, 0x32, 0x00};
          Buffer<> ratep_packet = Buffer<>(DV3K_REQ_RATEP,sizeof(DV3K_REQ_RATEP));
          send(ratep_packet);
          m_state = RATEP;  /* state is requesting version-id of Stick */
        } /* ratep */


        /* method to prepare incoming frames from BM network to be decoded later */
        virtual Buffer<> packForDecoding(const Buffer<> &buffer) { return buffer; }

        /* method to handle prepared encoded Data from BM network and send them to AudioSink */
        virtual Buffer<float> unpackDecoded(const Buffer<> &buffer)
        {
          unsigned char *s8_samples = reinterpret_cast<unsigned char *>(buffer.data);
          int count = buffer.length / 2;
          float t[count];
          size_t b = 0;
          for (int a=4; a < (int)buffer.length; a+=2)
          {
            int16_t w = (s8_samples[a] << 8) | (s8_samples[a+1] << 0);
            t[b++] = (float)w / 16384.0;
          }
          Buffer<float> audiobuf(t, b);
          return audiobuf;
        }

        /* method to prepare incoming Audio frames from local RX to be encoded later */
        virtual Buffer<> packForEncoding(const Buffer<> &buffer)
        {
          const unsigned char DV3K_VOICE_FRAME[] =
             {DV3K_START_BYTE, 0x01, 0x42, DV3K_TYPE_AUDIO, 0x00, 0xa0};
          int DV3K_VOICE_FRAME_LEN = 6;

          if (DV3K_AUDIO_LEN != buffer.length)
          {
            cerr << "*** ERROR: Buffer invalid!" << endl;
          }

          Buffer<> dv3k_buffer =
               Buffer<>(new char[330], sizeof(size_t));

          memcpy(dv3k_buffer.data, DV3K_VOICE_FRAME, sizeof(DV3K_VOICE_FRAME));
          memcpy(dv3k_buffer.data + DV3K_VOICE_FRAME_LEN, buffer.data, buffer.length);
          dv3k_buffer.length = DV3K_VOICE_FRAME_LEN + DV3K_AUDIO_LEN;
          return dv3k_buffer;
        }

        virtual Buffer<> unpackEncoded(const Buffer<> &buffer)
        {
          return buffer;
        }

        /**
         * @brief 	Write encoded samples into the decoder
         * @param 	buf  Buffer containing encoded samples
         * @param 	size The size of the buffer
         */
        virtual void writeEncodedSamples(void *buf, int size)
        {
         // const char DV3K_AMBE_HEADERFRAME[] = {DV3K_START_BYTE, 0x00, 0x0b, DV3K_TYPE_AMBE, 0x01, 0x48};

          const unsigned char DV3K_AMBE_HEADERFRAMEOUT[] = {DV3K_START_BYTE, 0x00, 0x0e,
                                                          DV3K_TYPE_AMBE, 0x40, 0x01, 0x48};
          const unsigned char DV3K_WAIT[] = {0x03, 0xa0};
          const int DV3K_WAIT_LEN = 2;

          char ambe_to_dv3k[DV3K_AMBE_HEADER_OUT_LEN + DV3K_AMBE_FRAME_LEN + DV3K_WAIT_LEN];
          Buffer<> buffer;
          buffer.data = reinterpret_cast<char*>(buf);
          buffer.length = size;
          Buffer<>ambe_frame;

           // devide the 27 bytes into a 9 byte long frame, sends them to the dv3k-decoder
          for (int a=0; a<27; a+=DV3K_AMBE_FRAME_LEN)
          {
            memcpy(ambe_to_dv3k, DV3K_AMBE_HEADERFRAMEOUT, DV3K_AMBE_HEADER_OUT_LEN);
            memcpy(ambe_to_dv3k + DV3K_AMBE_HEADER_OUT_LEN, buffer.data+a, DV3K_AMBE_FRAME_LEN);
            memcpy(ambe_to_dv3k + DV3K_AMBE_HEADER_OUT_LEN + DV3K_AMBE_FRAME_LEN, DV3K_WAIT, DV3K_WAIT_LEN);
            ambe_frame = Buffer<>(ambe_to_dv3k, DV3K_AMBE_HEADER_OUT_LEN + DV3K_AMBE_FRAME_LEN + DV3K_WAIT_LEN);
            // sending HEADER + 9 bytes to DV3K-Adapter
            send(ambe_frame);
          }
        }

        virtual void send(const Buffer<> &packet) = 0;

        virtual void callback(const Buffer<> &buffer)
        {
          uint32_t tlen = 0;
          int type = 0;
          Buffer<> dv3k_buffer = Buffer<>(new char[512], sizeof(size_t));

          if (buffer.data[0] == DV3K_START_BYTE
                           && buffer.length >= DV3K_HEADER_LEN)
          {
            tlen = buffer.data[2] + buffer.data[1] * 256;
            stored_bufferlen = 0;

             // is it only a part of the 160byte audio frame?
            if (tlen + DV3K_HEADER_LEN > buffer.length)
            {
               // store incoming data to a temporary buffer
              t_buffer = Buffer<>(new char[512], sizeof(size_t));
              memcpy(t_buffer.data, buffer.data, buffer.length);
              t_buffer.length = buffer.length;
              stored_bufferlen = tlen;
              return;
            }
            dv3k_buffer.data = buffer.data;
            dv3k_buffer.length = buffer.length;
          }
          else if (stored_bufferlen > 0)
          {
            memcpy(t_buffer.data + t_buffer.length, buffer.data, buffer.length);
            t_buffer.length += buffer.length;
            t_b = t_buffer.length;

            /* decoded audio frames are 320 bytes long / 324 bytes with header
               cut after 324 bytes and send them to the
               packForDecoder + AudioDecoder::sinkWriteSamples */
            if (t_buffer.length >= stored_bufferlen + DV3K_HEADER_LEN)
            {

              // prepare a complete frame to be handled by following methods
              memcpy(dv3k_buffer.data, t_buffer.data, stored_bufferlen + DV3K_HEADER_LEN);
              dv3k_buffer.length = stored_bufferlen + DV3K_HEADER_LEN;

              // move the rest of the buffer in t_buffer to the begin
              memmove(t_buffer.data, t_buffer.data + (stored_bufferlen + DV3K_HEADER_LEN),
                        t_buffer.length - stored_bufferlen - DV3K_HEADER_LEN);

              t_buffer.length = t_b - stored_bufferlen - DV3K_HEADER_LEN;
              stored_bufferlen = t_buffer.data[2] + t_buffer.data[1] * 256;
            }
            else return;
          }
          else
          {
            cout << "*** ERROR: DV3k frame to short." << endl;
            cout << "stored_bufferlen=" << stored_bufferlen << ",buffer.len="
                 << buffer.length << ", t_buffer.len=" << t_buffer.length
                 << endl;
            //init();
            return;
          }

          type = dv3k_buffer.data[3];

          /* test the type of incoming frame */
          if (type == DV3K_TYPE_CONTROL)
          {
            if (m_state == RESET)
            {
              /* reset the device just bto be sure */
              cout << "--- DV3K: Reset OK" << endl;
              prodid();
            }
            else if (m_state == PRODID)
            {
              /* give out product name of DV3k */
              cout << "--- DV3K (ProdID): "  << dv3k_buffer.data+5 << endl;
              versid();
            }
            else if (m_state == VERSID)
            {
              /* give out version of DV3k */
              cout << "--- DV3K (VersID): " << dv3k_buffer.data+5 << endl;
              ratep();
            }
            else if (m_state == RATEP)
            {
              /* sending configuration/rate params to DV3k was OK*/
              cout << "--- DV3K: Ready" << endl;
              m_state = READY;
            }
          }
          /* test if buffer contains encoded frame (AMBE) */
          else if (type == DV3K_TYPE_AMBE)
          {
            // prepare encoded Frames to be send to BM network
           // Buffer<> unpacked = unpackEncoded(dv3k_buffer);
            // use only the dmr audio frame, not the DV3k-header from dongle
            memcpy(r_buf + r_bufcnt, buffer.data + DV3K_AMBE_HEADER_IN_LEN, DV3K_AMBE_FRAME_LEN);
            r_bufcnt += DV3K_AMBE_FRAME_LEN;
            
            // collect at least 27 bytes
            while (r_bufcnt >= REWIND_DMR_AUDIO_FRAME_LENGTH)
            {
              char rt[REWIND_DMR_AUDIO_FRAME_LENGTH];
              memcpy(rt, r_buf, REWIND_DMR_AUDIO_FRAME_LENGTH);
               // forward encoded samples
              AudioEncoder::writeEncodedSamples(rt, REWIND_DMR_AUDIO_FRAME_LENGTH);
              r_bufcnt -= REWIND_DMR_AUDIO_FRAME_LENGTH;
              memmove(r_buf, r_buf + REWIND_DMR_AUDIO_FRAME_LENGTH, r_bufcnt);
            }
          }
          /* or is a raw 8kHz audio frame */
          else if (type == DV3K_TYPE_AUDIO)
          {
            // unpack decoded frame
            Buffer<float> unpacked = unpackDecoded(dv3k_buffer);

            // pass decoded samples into sink
            AudioDecoder::sinkWriteSamples(unpacked.data, unpacked.length);
          }
          else
          {
            /* frame is unknown */
            cout << "--- WARNING: received unkown DV3K type." << endl;
          }
        }

        /* method is called up to send encoded AMBE frames to BM network */
        virtual int writeSamples(const float *samples, int count)
        {
           // store incoming floats in a buffer until 160 or more samples
           // are reached
          Buffer<> ambe_buf;
          
          memcpy(inbuf + bufcnt, samples, sizeof(float)*count);
          bufcnt += count;
          char t_data[bufcnt];
          while (bufcnt >= DV3K_AUDIO_LEN)
          {
            for (int a = 0; a<DV3K_AUDIO_LEN; a++)
            {
              int32_t w = (int) ( (inbuf[a] + inbuf[a+1]) * 2048.0);
        //      if (w > 1000 || w < -1000) cout << w << ",";
              t_data[a] = w >> 8;
              t_data[a+1] = w & 0x00ff;
            }

            ambe_buf.data = t_data;
            ambe_buf.length = DV3K_AUDIO_LEN;
            Buffer<> packet = packForEncoding(ambe_buf);
            send(packet);

            bufcnt -= DV3K_AUDIO_LEN;
            memmove(inbuf, inbuf + DV3K_AUDIO_LEN-1, bufcnt);
          }
          inbuf[bufcnt] = '\0';
          return count;
        }

    protected:
      static const char DV3K_TYPE_CONTROL = 0x00;
      static const char DV3K_TYPE_AMBE = 0x01;
      static const char DV3K_TYPE_AUDIO = 0x02;
      static const char DV3K_HEADER_LEN = 0x04;
      static const char DSTAR_AUDIO_BLOCK_SIZE=160;

      static const char DV3K_START_BYTE = 0x61;

      static const char DV3K_CONTROL_RATEP  = 0x0A;
      static const char DV3K_CONTROL_PRODID = 0x30;
      static const char DV3K_CONTROL_VERSTRING = 0x31;
      static const char DV3K_CONTROL_RESET = 0x33;
      static const char DV3K_CONTROL_READY = 0x39;
      static const char DV3K_CONTROL_CHANFMT = 0x15;

      static const uint16_t DV3K_AUDIO_LEN = 320;
      static const uint16_t DV3K_AMBE_HEADER_IN_LEN = 6;
      static const uint16_t DV3K_AMBE_HEADER_OUT_LEN = 7;
      static const uint16_t DV3K_AMBE_FRAME_LEN = 9;     
      static const uint16_t REWIND_DMR_AUDIO_FRAME_LENGTH = 27;

      /**
      * @brief 	Default constuctor
      */
      //AudioCodecAmbeDv3k(void) : device_initialized(false) {}
      AudioCodecAmbeDv3k(void) : m_state(OFFLINE), bufcnt(0), 
      r_bufcnt(0) {}

    private:
      enum STATE {
        OFFLINE, RESET, INIT, PRODID, VERSID, RATEP, READY, WARNING, ERROR
      };
      STATE m_state;
      uint32_t stored_bufferlen;
      int act_framelen;
      Buffer<> t_buffer;
      int t_b;
      float inbuf[640];
      uint32_t bufcnt;
      uint32_t r_bufcnt;
      char r_buf[100];

      AudioCodecAmbeDv3k(const AudioCodecAmbeDv3k&);
      AudioCodecAmbeDv3k& operator=(const AudioCodecAmbeDv3k&);
    };

    /**
     * TODO: Implement communication with Dv3k via UDP here.
     */
    class AudioCodecAmbeDv3kAmbeServer : public AudioCodecAmbeDv3k {
    public:
      /**
      * @brief  Default constuctor
      *         TODO: parse options for IP and PORT
      */
      AudioCodecAmbeDv3kAmbeServer(const Options &options)
      {
        Options::const_iterator it;

        if ((it=options.find("AMBESERVER_HOST"))!=options.end())
        {
          ambehost = (*it).second;
        }
        else
        {
          throw "*** ERROR: Parameter AMBESERVER_HOST not defined.";
        }

        if((it=options.find("AMBESERVER_PORT"))!=options.end())
        {
          ambeport = atoi((*it).second.c_str());
        }
        else
        {
          throw "*** ERROR: Parameter AMBESERVER_PORT not defined.";
        }
        udpInit();
      }

      /* initialize the udp socket */
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
        init();
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

      virtual void send(const Buffer<> &packet)
      {
        ambesock->write(ambehost, ambeport, packet.data, packet.length);
      }

      ~AudioCodecAmbeDv3kAmbeServer()
      {
        delete dns;
        dns = 0;
        delete ambesock;
      }

   protected:
      virtual void callbackUdp(const IpAddress& addr, uint16_t port,
                                         void *buf, int count)
      {
        callback(Buffer<>(reinterpret_cast<char *>(buf), count));
      }

    private:

      int ambeport;
      string ambehost;
      UdpSocket * ambesock;
      IpAddress	ip_addr;
      DnsLookup	*dns;

      AudioCodecAmbeDv3kAmbeServer(const AudioCodecAmbeDv3kAmbeServer&);
      AudioCodecAmbeDv3kAmbeServer& operator=(const AudioCodecAmbeDv3kAmbeServer&);
    };

    /**
     * TODO: Implement communication with Dv3k via TTY here.
     */
    class AudioCodecAmbeDv3kTty : public AudioCodecAmbeDv3k {
    public:
      /**
      * @brief 	Default constuctor
      */
      AudioCodecAmbeDv3kTty(const Options &options) {
        Options::const_iterator it;
        int baudrate;
        string device;

        if ((it=options.find("TTY_DEVICE"))!=options.end())
        {
          device = (*it).second;
        }
        else
        {
          throw "*** ERROR: Parameter AMBE_TTY_DEVICE not defined.";
        }

        if((it=options.find("TTY_BAUDRATE"))!=options.end())
        {
          baudrate = atoi((*it).second.c_str());
        }
        else
        {
          throw "*** ERROR: Parameter AMBE_TTY_BAUDRATE not defined.";
        }

        if (baudrate != 230400 && baudrate != 460800)
        {
          throw "*** ERROR: AMBE_TTY_BAUDRATE must be 230400 or 460800.";
        }

        serial = new Serial(device);
        serial->setParams(baudrate, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE);
        if (!(serial->open(true)))
        {
          cerr << "*** ERROR: Can not open device " << device << endl;
          throw;
        }
        serial->charactersReceived.connect(
             sigc::mem_fun(*this, &AudioCodecAmbeDv3kTty::callbackTty));
        init();
      }

      virtual void send(const Buffer<> &packet) {
        serial->write(packet.data, packet.length);
      }

      ~AudioCodecAmbeDv3kTty()
      {
          serial->close();
          delete serial;
      }

    protected:
      virtual void callbackTty(const char *buf, int count)
      {
        callback(Buffer<>(const_cast<char *>(buf),count));
      }

    private:
      Serial *serial;

      AudioCodecAmbeDv3kTty(const AudioCodecAmbeDv3kTty&);
      AudioCodecAmbeDv3kTty& operator=(const AudioCodecAmbeDv3kTty&);
    };

    AudioCodecAmbeDv3k *AudioCodecAmbeDv3k::create(const Options &options) {
        Options::const_iterator type_it = options.find("TYPE");
        if(type_it!=options.end())
        {
            if(type_it->second=="AMBESERVER")
                return new AudioCodecAmbeDv3kAmbeServer(options);
            else if(type_it->second=="TTY")
                return new AudioCodecAmbeDv3kTty(options);
            else
                throw "unknown Ambe codec TYPE";
        }
        else
            throw "unspecified Ambe codec TYPE";
    }
}


AudioCodecAmbe *AudioCodecAmbe::create(const Options &options) {
    Options::const_iterator type_it = options.find("TYPE");
    if(type_it!=options.end())
    {
        if(type_it->second=="AMBESERVER" || type_it->second=="TTY")
            return AudioCodecAmbeDv3k::getPtr(options);
        else
            throw "unknown Ambe codec TYPE";
    }
    else
        throw "unspecified Ambe codec TYPE";
}
