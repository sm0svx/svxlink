#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioLADSPAPlugin.h>
#include <AsyncFdWatch.h>

int main(void)
{
  Async::CppApplication app;

  Async::AudioIO audio_io("alsa:default", 0);
  audio_io.open(Async::AudioIO::MODE_RDWR);
  Async::AudioSource* prev_src = &audio_io;

  std::string label("tap_pitch");
  Async::AudioLADSPAPlugin p1(label);
  if (!p1.initialize())
  {
    std::cout << "*** ERROR: Could not instantiate LADSPA plugin instance "
                 "with label " << label << std::endl;
    exit(1);
  }
  p1.setControl(0, 4);
  p1.print();
  prev_src->registerSink(&p1);
  prev_src = &p1;

  label = "tap_vibrato";
  Async::AudioLADSPAPlugin p2(label);
  if (!p2.initialize())
  {
    std::cout << "*** ERROR: Could not instantiate LADSPA plugin instance "
                 "with label " << label << std::endl;
    exit(1);
  }
  p2.setControl(0, 10);
  p2.setControl(1, 10);
  p2.print();
  prev_src->registerSink(&p2);
  prev_src = &p2;

  prev_src->registerSink(&audio_io);

  app.exec();

  return 0;
}
