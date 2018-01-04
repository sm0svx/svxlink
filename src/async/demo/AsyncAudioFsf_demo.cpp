#include <iostream>
#include <cstring>
#include <cstdlib>

#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioFsf.h>

// Read samples from the soundcard, filter it and then output it to the same
// soundcard
int main(int argc, const char **argv)
{
  Async::CppApplication app;

  Async::AudioIO::setSampleRate(16000);
  Async::AudioIO audio_io("alsa:plughw:0", 0);
  Async::AudioSource *prev_src = &audio_io;

    // A bandpass filter centered at 1000Hz with an approximate passband
    // bandwidth of 400Hz and a stopband attenuation of about 40dB.
  const size_t N = 128;   // N/fs=125Hz binwidth
  float coeff[N/2+1];
  std::memset(coeff, 0, sizeof(coeff));
  coeff[6] = 0.39811024;  // 750Hz
  coeff[7] = 1.0;         // 875Hz
  coeff[8] = 1.0;         // 1000Hz
  coeff[9] = 1.0;         // 1125Hz
  coeff[10] = 0.39811024; // 1250Hz
  Async::AudioFsf fsf(N, coeff);
  prev_src->registerSink(&fsf, true);
  prev_src = &fsf;

  prev_src->registerSink(&audio_io);
  prev_src = 0;

  if (!audio_io.open(Async::AudioIO::MODE_RDWR))
  {
    std::cout << "*** ERROR: Could not open audio device" << std::endl;
    exit(1);
  }

  app.exec();

  return 0;
}
