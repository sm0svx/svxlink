#include <iostream>
#include <unistd.h>
#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioEncoder.h>
#include <AsyncAudioDecoder.h>

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
  Async::AudioIO audio_io("alsa:default", 0);
  if (!audio_io.open(Async::AudioIO::MODE_RDWR))
  {
    std::cout << "*** ERROR: Could not open audio device" << std::endl;
    exit(1);
  }
  Async::AudioSource* prev_src = &audio_io;

    // Create AMBE encoder
  Async::AudioEncoder* encoder = Async::AudioEncoder::create("AMBE");
  if (encoder == 0)
  {
    std::cerr << "*** ERROR: Could not create encoder" << std::endl;
    exit(1);
  }
  prev_src->registerSink(encoder);
  prev_src = 0;

    // Create AMBE decoder
  Async::AudioDecoder* decoder = Async::AudioDecoder::create("AMBE");
  if (decoder == 0)
  {
    std::cerr << "*** ERROR: Could not create decoder" << std::endl;
    exit(1);
  }
  prev_src = decoder;

    // Connect encoder to decoder
  encoder->writeEncodedSamples.connect(
      mem_fun(*decoder, &Async::AudioDecoder::writeEncodedSamples));
  encoder->flushEncodedSamples.connect(
      mem_fun(*decoder, &Async::AudioDecoder::flushEncodedSamples));
  decoder->allEncodedSamplesFlushed.connect(
      mem_fun(*encoder, &Async::AudioEncoder::allEncodedSamplesFlushed));

    // Finally route audio back to the audio device
  prev_src->registerSink(&audio_io);

  app.unixSignalCaught.connect(sigc::ptr_fun(&do_quit));
  app.catchUnixSignal(SIGINT);

  app.exec();

    // When both encoder and decoder are released, the AMBE device is released
  encoder->release();
  decoder->release();

  return 0;
}
