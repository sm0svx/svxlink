#include <cmath>
#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AudioSink.h>
#include <AudioSource.h>

using namespace std;
using namespace Async;

class MyClass : public Async::AudioSink, public Async::AudioSource
{
  public:
    MyClass(void)
    {
      	// Create a new audio IO object
      audio_io = new AudioIO("/dev/dsp");
      
      	// Open it for both reading and writing
      audio_io->open(AudioIO::MODE_RDWR);
      
      	// Register this object as the audio source for sound output
      audio_io->registerSource(this);
      
      	// Register the audio device as the audio source for this object
      registerSource(audio_io);
      
      	// Generate a 440Hz sine wave and write it to the audio device
      int sample_rate = audio_io->sampleRate();
      float samples[sample_rate];
      for (int i=0; i<sample_rate; ++i) // Put a sine wave into the buffer
      {
      	samples[i] = 0.33 * sin(2 * M_PI * 440 * i / sample_rate);
      }
      int samples_written = sinkWriteSamples(samples, sample_rate);
      cout << "Wrote " << samples_written << " samples...\n";
    }
    
    ~MyClass(void)
    {
      delete audio_io;
    }

      // AudioSink functions
    int writeSamples(const float *samples, int count)
    {
      	// Just loop incoming samples back to the audio device
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
    AudioIO *audio_io;
    
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
