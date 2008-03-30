#include <cmath>
#include <iostream>
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
        audio_io.open(AudioIO::MODE_WR);
        pos = 0;
        writeSamples();
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
    static const int BLOCK_SIZE = 128;

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
      audio_in = new AudioIO("/dev/dsp1", 1);
      audio_in->open(AudioIO::MODE_RD);
      
      audio_out = new AudioIO("/dev/dsp1", 0);
      audio_out->open(AudioIO::MODE_WR);
      
      /*
      SineGenerator *sine_gen = new SineGenerator("/dev/dsp1", 1);
      sine_gen->setFq(440);
      sine_gen->setLevel(5);
      sine_gen->enable(true);
      */
      
      	// Open it for both reading and writing
      //audio_io->open(AudioIO::MODE_RDWR);
      
      	// Register this object as the audio source for sound output
      audio_out->registerSource(this);
      
      	// Register the audio device as the audio source for this object
      registerSource(audio_in);
      
      	// Generate a 440Hz sine wave and write it to the audio device
      /*
      int sample_rate = audio_io->sampleRate();
      float samples[sample_rate];
      for (int i=0; i<sample_rate; ++i) // Put a sine wave into the buffer
      {
      	samples[i] = 0.33 * sin(2 * M_PI * 440 * i / sample_rate);
      }
      int samples_written = sinkWriteSamples(samples, sample_rate);
      cout << "Wrote " << samples_written << " samples...\n";
      */
      
    }
    
    ~MyClass(void)
    {
      delete audio_in;
      delete audio_out;
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
    AudioIO *audio_in;
    AudioIO *audio_out;
    
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
