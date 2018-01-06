#include <time.h>

#include <iostream>
#include <cstdlib>

#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>

using namespace std;
using namespace Async;

# define clock_timersub(a, b, result)                                         \
  do {                                                                        \
    (result)->tv_sec = (a)->tv_sec - (b)->tv_sec;                             \
    (result)->tv_nsec = (a)->tv_nsec - (b)->tv_nsec;                          \
    if ((result)->tv_nsec < 0) {                                              \
      --(result)->tv_sec;                                                     \
      (result)->tv_nsec += 1000000000;                                        \
    }                                                                         \
  } while (0)


class SampleTimer : public AudioSink
{
  public:
    SampleTimer(void)
      : sample_cnt(0)
    {
      clock_gettime(CLOCK_MONOTONIC, &start);
    }

    int writeSamples(const float *samples, int cnt)
    {
      sample_cnt += cnt;
      if (sample_cnt >= 30 * 48000)
      {
        clock_gettime(CLOCK_MONOTONIC, &stop);
        Application::app().quit();
      }
      return cnt;
    }

    void flushSamples(void)
    {
      sourceAllSamplesFlushed();
    }

    unsigned calibratedSampleRate(void)
    {
      struct timespec diff;
      clock_timersub(&stop, &start, &diff);
      unsigned diff_ms = diff.tv_sec * 1000 + diff.tv_nsec / 1000000;
      return 1000 * sample_cnt / diff_ms;
    }


  private:
    unsigned sample_cnt;
    struct timespec start;
    struct timespec stop;

};


int main(int argc, const char **argv)
{
  CppApplication app;

  AudioIO::setSampleRate(48000);
  AudioIO::setBlocksize(1024);
  AudioIO::setBlockCount(4);

  AudioIO audio_in("alsa:hw:0", 0);
  AudioSource *prev_src = &audio_in;

  SampleTimer sample_timer;
  prev_src->registerSink(&sample_timer);
  prev_src = 0;

  if (!audio_in.open(AudioIO::MODE_RD))
  {
    cerr << "*** ERROR: Could not open audio device for reading\n";
    exit(1);
  }

  app.exec();

  unsigned calibrated = sample_timer.calibratedSampleRate();
  cout << "Calibrated sample rate: " << calibrated
       << " (" << (calibrated / 48000.0) << ")\n";

  return 0;
}

