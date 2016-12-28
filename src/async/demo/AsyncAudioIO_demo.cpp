#include <iostream>
#include <cmath>

#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>

using namespace std;
using namespace Async;


class SineGenerator : public Async::AudioSource
{
  public:
    explicit SineGenerator(const string& audio_dev, int channel)
      : audio_io(audio_dev, channel), pos(0), fq(0.0), level(0.0),
        sample_rate(0)
    {
      sample_rate = audio_io.sampleRate();
      audio_io.registerSource(this);
    }

    ~SineGenerator(void)
    {
      enable(false);
    }

    void setFq(double tone_fq)
    {
      fq = tone_fq;
    }
    
    void setLevel(int level_percent)
    {
      level = level_percent / 100.0;
    }

    void enable(bool enable)
    {
      if (enable == (audio_io.mode() != AudioIO::MODE_NONE))
      {
        return;
      }

      if (enable && (fq != 0))
      {
        if (audio_io.open(AudioIO::MODE_WR))
        {
          pos = 0;
          writeSamples();
        }
        else
        {
          printf("Failed to open audio device\n");
        }
      }
      else
      {
        audio_io.close();
      }
    }

    void resumeOutput(void)
    {
      if (audio_io.mode() != AudioIO::MODE_NONE)
      {
        writeSamples();
      }
    }

    void allSamplesFlushed(void)
    {
    }

  private:
    static const int BLOCK_SIZE = 256;

    AudioIO   audio_io;
    unsigned  pos;
    double    fq;
    double    level;
    int       sample_rate;

    void writeSamples(void)
    {
      int written;
      do {
        float buf[BLOCK_SIZE];
        for (int i=0; i<BLOCK_SIZE; ++i)
        {
          buf[i] = level * sin(2 * M_PI * fq * (pos+i) / sample_rate);
        }
        written = sinkWriteSamples(buf, BLOCK_SIZE);
        pos += written;
      } while (written != 0);
    }

};



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
      
      SineGenerator *sine_gen = new SineGenerator("alsa:default", 1);
      sine_gen->setFq(1000);
      sine_gen->setLevel(2);
      sine_gen->enable(true);
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
