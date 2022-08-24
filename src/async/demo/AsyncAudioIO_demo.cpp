#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>
#include <AsyncAudioGenerator.h>

using namespace std;
using namespace Async;

class MyClass : public Async::AudioSink, public Async::AudioSource
{
  public:
    MyClass(void)
    {
        // Create a new audio IO object
      audio_io = new AudioIO("alsa:default", 0);

        // Open it for both reading and writing
      audio_io->open(AudioIO::MODE_RDWR);

        // Register this object as the audio source for sound output
      audio_io->registerSource(this);

        // Register the audio device as the audio source for this object
      registerSource(audio_io);

        // Create a sine generator
      sine_gen = new AudioGenerator;
      sine_gen->setFq(1000);
      sine_gen->setPower(-24);

        // Create an audio device for the sine generator
      AudioIO *sine_io = new AudioIO("alsa:default", 1);
      sine_io->open(AudioIO::MODE_WR);
      sine_gen->registerSink(sine_io, true);

        // Enable the sine generator
      sine_gen->enable(true);
    }

    ~MyClass(void)
    {
      delete audio_io;
      delete sine_gen;
    }

      // AudioSink functions
    int writeSamples(const float *samples, int count)
    {
        // Just loop incoming samples back to the audio device
        // We could do some processing here if we wanted to.
      return sinkWriteSamples(samples, count);
    }

    void flushSamples(void)
    {
      sinkFlushSamples();
    }

      // AudioSource functions
    void resumeOutput(void)
    {
      sourceResumeOutput();
    }

    void allSamplesFlushed(void)
    {
      sourceAllSamplesFlushed();
    }

  private:
    AudioIO *         audio_io;
    AudioGenerator *  sine_gen;
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
