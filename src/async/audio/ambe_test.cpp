#include <iostream>
#include <unistd.h>
#include <AsyncCppApplication.h>
#include <AsyncAudioCodecAmbe.h>
#include <AsyncAudioIO.h>
#include <AsyncTimer.h>

void do_quit(int signum)
{
  std::cout << "--- Exiting..." << std::endl;
  Async::Application::app().quit();
}


int main()
{
  std::cout << "--- Starting..." << std::endl;
  Async::CppApplication app;

    // Create audio device
  Async::AudioIO::setSampleRate(8000);
  Async::AudioIO::setBlocksize(256);
  Async::AudioIO audio_io("alsa:plughw:0", 0);
  if (!audio_io.open(Async::AudioIO::MODE_RDWR))
  {
    std::cout << "*** ERROR: Could not open audio device" << std::endl;
    exit(1);
  }
  Async::AudioSource* prev_src = &audio_io;

    // Create AMBE codec
  //Async::AudioCodecAmbe* codec = Async::AudioCodecAmbe::create("TTY");
  Async::AudioEncoder* encoder = Async::AudioEncoder::create("AMBE");
  prev_src->registerSink(encoder);

  Async::AudioDecoder* decoder = Async::AudioDecoder::create("AMBE");
#if 0
  Async::AudioCodecAmbe::Options options;
  options["TTY_DEVICE"] = "/dev/ttyUSB0";
  //options["TTY_BAUDRATE"] = "230400";
  options["TTY_BAUDRATE"] = "460800";
  if (!codec->initialize(options))
  {
    std::cerr << "*** ERROR: AMBE codec initialization failed" << std::endl;
    exit(1);
  }
#endif
  encoder->writeEncodedSamples.connect(
      mem_fun(*decoder, &Async::AudioDecoder::writeEncodedSamples));
  encoder->flushEncodedSamples.connect(
      mem_fun(*decoder, &Async::AudioDecoder::flushEncodedSamples));
  decoder->allEncodedSamplesFlushed.connect(
      mem_fun(*encoder, &Async::AudioEncoder::allEncodedSamplesFlushed));
  prev_src = decoder;

  prev_src->registerSink(&audio_io);

  app.unixSignalCaught.connect(sigc::ptr_fun(&do_quit));
  app.catchUnixSignal(SIGINT);

  app.exec();

  audio_io.close();
  encoder->release();
  decoder->release();

  return 0;
}
