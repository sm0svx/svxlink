#include <AsyncAudioCodecAmbe.h>
#include <AsyncSerial.h>

#include <string>
#include <map>

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
    class AudioCodecAmbeDv3k : public AudioCodecAmbe, public Multiton<AudioCodecAmbe,AudioCodecAmbe::Options> {
    public:
        template <typename T = void>
        struct Buffer {
            Buffer(T *data = NULL, size_t length = 0) : data(data), length(length) {}
            T *data;
            size_t length;
        };

        static AudioCodecAmbeDv3k *create(const Options &options);

        virtual void init()
        {
            assert(!"unimplemented");
            send(Buffer<>());
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
            Buffer<> packet = packForDecoding(Buffer<>(buf,size));
            send(packet);
        }

        virtual void send(const Buffer<> &packet);

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
                assert(!"invalid conversion from float * to void *");
                AudioDecoder::sinkWriteSamples((float *)unpacked.data, unpacked.length);
            }
            else
            {
                // deal with result of initialization or with errors if necessarry
                assert(!"unimplemented");
            }
        }

        virtual int writeSamples(const float *samples, int count)
        {
          assert(!"unimplemented");
          Buffer<> packet = packForEncoding(Buffer<>((void*)samples,count));
          send(packet);
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
      AudioCodecAmbeDv3kAmbeServer(const Options &options) {

      }

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
        assert(!"implementation incomplete");
        // TODO: parse options for TTY and BAUDRATE
        serial = new Serial(options.find("TTY")->second);
        //TODO: serial->setParams(int speed, Parity parity, int bits, int stop_bits, Flow flow);
        serial->open(true);
        serial->charactersReceived.connect(sigc::mem_fun(*this, &AudioCodecAmbeDv3kTty::callbackTty));
        init();
      }

      virtual void send(void *frame, int size) {
        assert(!"invalid conversion from void * to char *");
        serial->write((char *)frame, size);
      }

    protected:
      virtual void callbackTty(const char *buf, int count)
      {
        assert(!"conversion unimplemented");
        callback(Buffer<>((void*)buf,count));
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
