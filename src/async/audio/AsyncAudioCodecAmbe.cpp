#include <AsyncAudioCodecAmbe.h>
#include <AsyncSerial.h>

#include <string>
#include <map>
#include <iostream>
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

    template <typename T, typename Key = map<string,string>>
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
            send(init_packet);
        }

        virtual Buffer<> packForDecoding(const Buffer<> &buffer) { assert(!"unimplemented"); return Buffer<>(); }
        virtual Buffer<> unpackDecoded(const Buffer<> &buffer) { assert(!"unimplemented"); return Buffer<>(); }

        virtual Buffer<> packForEncoding(const Buffer<> &buffer) { assert(!"unimplemented"); return Buffer<>(); }
        virtual Buffer<> unpackEncoded(const Buffer<> &buffer) { assert(!"unimplemented"); return Buffer<>(); }

        /**
         * @brief 	Write encoded samples into the decoder
         * @param 	buf  Buffer containing encoded samples
         * @param 	size The size of the buffer
         */
        virtual void writeEncodedSamples(void *buf, int size)
        {
            Buffer<> packet = packForDecoding(Buffer<>((char*)buf,size));
            send(packet);
        }

        virtual void send(const Buffer<> &packet) = 0;

        virtual void callback(const Buffer<> &buffer)
        {
            if(0/*test if buffer contains encoded frame*/)
            {
                // unpack decoded frame
                Buffer<> unpacked = unpackEncoded(buffer);

                // forward encoded samples
                AudioEncoder::writeEncodedSamples(unpacked.data, unpacked.length);
            }
            else if(1/*test if buffer contains decoded frame*/)
            {
                // unpack decoded frame
                Buffer<> unpacked = unpackDecoded(buffer);

                // pass decoded samples into sink
                assert(!"invalid conversion from float * to char *");
                AudioDecoder::sinkWriteSamples((float *)unpacked.data, unpacked.length);
            }
            else
            {
                // deal with result of initialization or with errors if necessary
                cout << "Dv3k initialized";
                device_initialized = true;
            }
        }

        virtual int writeSamples(const float *samples, int count)
        {
          assert(!"unimplemented");
          Buffer<> packet = packForEncoding(Buffer<>((char*)samples,count));
          send(packet);
          return count;
        }

    protected:
      static const char DV3K_TYPE_CONTROL = 0x00;
      static const char DV3K_TYPE_AMBE = 0x01;
      static const char DV3K_TYPE_AUDIO = 0x02;

      static const char DV3K_START_BYTE = 0x61;

      static const char DV3K_CONTROL_RATEP  = 0x0A;
      static const char DV3K_CONTROL_PRODID = 0x30;
      static const char DV3K_CONTROL_VERSTRING = 0x31;
      static const char DV3K_CONTROL_RESET = 0x33;
      static const char DV3K_CONTROL_READY = 0x39;
      static const char DV3K_CONTROL_CHANFMT = 0x15;

      /**
      * @brief 	Default constuctor
      */
      AudioCodecAmbeDv3k(void) : device_initialized(false) {}

    private:
      bool device_initialized;

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
      AudioCodecAmbeDv3kAmbeServer(const Options &options) {

      }

      virtual void send(const Buffer<> &packet) { /* TODO: Send via UDP */ }

    private:
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
        if((it=options.find("TTY_DEVICE"))!=options.end())
            device = (*it).second;
        else
            throw "TODO: error handling";
        if((it=options.find("TTY_BAUDRATE"))!=options.end())
            baudrate = atoi((*it).second.c_str());
        else
            throw "TODO: error handling";

        serial = new Serial(device);
        serial->setParams(baudrate, Serial::PARITY_NONE, 8, 1, Serial::FLOW_NONE);
        serial->open(true);
        serial->charactersReceived.connect(sigc::mem_fun(*this, &AudioCodecAmbeDv3kTty::callbackTty));
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
