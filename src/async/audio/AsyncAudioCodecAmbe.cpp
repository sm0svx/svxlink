#include <AsyncAudioCodecAmbe.h>
#include <AsyncSerial.h>
#include <AsyncUdpHandler.h>

#include <string>
#include <map>

using namespace Async;

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

    template <typename T, typename Key = std::map<std::string,std::string>>
    class Multiton
    {
    public:
        static void destroyAll()
        {
            for (auto it = instances.begin(); it != instances.end(); ++it)
                delete (*it).second;
            instances.clear();
        }

        static void destroy(const Key &key)
        {
            auto it = instances.find(key);

            if (it != instances.end()) {
                delete (*it).second;
                instances.erase(it);
            }
        }

        static T* getPtr(const Key &key)
        {
            const auto it = instances.find(key);

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

        static std::map<Key, T*> instances;
    };

    template <typename T, typename Key>
    std::map<Key, T*> Multiton<T, Key>::instances;

    /**
     * @brief Implement shared Dv3k code here
     * (e.g. initialiation and protocol)
     */
    class AudioCodecAmbeDv3k : public AudioCodecAmbe, public Multiton<AudioCodecAmbe,AudioCodecAmbe::Options> {
    public:
        static AudioCodecAmbeDv3k *create(const Options &options);

        virtual void init()
        {
            void *init_frame = NULL /* TODO */;
            void size = 0 /* TODO */;
            send(init_frame,size);
        }

        virtual std::pair<void*,int> packForDecoding(void* buf, int size) { /* TODO */ }
        virtual std::pair<void*,int> unpackDecoded(void* buf, int size) { /* TODO */ }

        virtual std::pair<void*,int> packForEncoding(void* buf, int size) { /* TODO */ }
        virtual std::pair<void*,int> unpackDecoded(void* buf, int size) { /* TODO */ }

        /**
         * @brief 	Write encoded samples into the decoder
         * @param 	buf  Buffer containing encoded samples
         * @param 	size The size of the buffer
         */
        virtual void writeEncodedSamples(void *buf, int size)
        {
            (frame,count) = packForDecoding(buf,size);
            send(frame,count);
        }

        virtual void send(void *frame, int size);

        virtual void callback(void *frame, int size)
        {
            if(frame is encoded frame)
            {
                // unpack decoded frame
                (samples,count) = unpackEncoded(frame,size);
                // ..
                AudioEncoder::writeEncodedSamples(buf, sizeof(buf));(samples, count);
            }
            else if(frame is decoded frame)
            {
                // unpack decoded frame
                (samples,count) = unpackDecoded(frame,size);
                // ..
                AudioDecoder::sinkWriteSamples(samples, count);
            }
            else
            {
                // deal with result of initialization or with errors if necessarry
            }
        }

        virtual int writeSamples(const float *samples, int count)
        {
          if (count > std::numeric_limits<uint16_t>::max())
          {
            count = std::numeric_limits<uint16_t>::max();
          }
          else if (count < 0)
          {
            return -1;
          }
          uint8_t buf[2];
          buf[0] = static_cast<uint8_t>(count & 0xff);
          buf[1] = static_cast<uint8_t>(count >> 8);
          packForEncoding(...);
          AudioEncoder::writeEncodedSamples(buf, sizeof(buf));
          return count;
        }

    protected:
      /**
      * @brief 	Default constuctor
      */
      AudioCodecAmbeDv3k(void) {}

    private:
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
      AudioCodecAmbeDv3kAmbeServer(const Options &options) {}

      virtual void send(void *frame, int size) { /* TODO: Send via UDP */ }

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
        // TODO: parse options for TTY and BAUDRATE
        serial = new Serial(options.find("TTY")->second);
        serial->Serial(const std::string& serial_port);
        //TODO: serial->setParams(int speed, Parity parity, int bits, int stop_bits, Flow flow);
        serial->open(true);
        serial->charactersReceived.connect(sigc::mem_fun(*this, &AudioCodecAmbeDv3kTty::callback));
        init();
      }

      virtual void send(void *frame, int size) {
        serial->write(frame, size);
      }

    private:
      Serial *serial;

      AudioCodecAmbeDv3kTty(const AudioCodecAmbeDv3kTty&);
      AudioCodecAmbeDv3kTty& operator=(const AudioCodecAmbeDv3kTty&);
    };

    AudioCodecAmbeDv3k *AudioCodecAmbeDv3k::create(const Options &options) {
        auto type_it = options.find("TYPE");
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
    auto type_it = options.find("TYPE");
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
