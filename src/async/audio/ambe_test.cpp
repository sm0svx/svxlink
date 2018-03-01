#include <iostream>
#include <cstdlib>
#include <unistd.h>
#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioEncoder.h>
#include <AsyncAudioDecoder.h>
#include <AsyncAudioDebugger.h>

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
  Async::AudioIO::setSampleRate(16000);
  Async::AudioIO::setBlocksize(256);
  Async::AudioIO audio_io("alsa:default", 0);
  if (!audio_io.open(Async::AudioIO::MODE_RDWR))
  {
    std::cout << "*** ERROR: Could not open audio device" << std::endl;
    std::exit(1);
  }
  Async::AudioSource* prev_src = &audio_io;

  std::string codec_name("AMBE");

    // Create the encoder
  Async::AudioEncoder* encoder = Async::AudioEncoder::create(codec_name);
  if (encoder == 0)
  {
    std::cerr << "*** ERROR: Could not create encoder" << std::endl;
    std::exit(1);
  }
  prev_src->registerSink(encoder);
  prev_src = 0;

    // Create AMBE decoder
  Async::AudioDecoder* decoder = Async::AudioDecoder::create(codec_name);
  if (decoder == 0)
  {
    std::cerr << "*** ERROR: Could not create decoder" << std::endl;
    std::exit(1);
  }
  prev_src = decoder;

    // Connect encoder to decoder
  encoder->writeEncodedSamples.connect(
      mem_fun(*decoder, &Async::AudioDecoder::writeEncodedSamples));
  encoder->flushEncodedSamples.connect(
      mem_fun(*decoder, &Async::AudioDecoder::flushEncodedSamples));
  decoder->allEncodedSamplesFlushed.connect(
      mem_fun(*encoder, &Async::AudioEncoder::allEncodedSamplesFlushed));

  //Async::AudioDebugger *debugger = new Async::AudioDebugger;
  //prev_src->registerSink(debugger);
  //prev_src = debugger;

    // Finally route audio back to the audio device
  prev_src->registerSink(&audio_io);

  app.unixSignalCaught.connect(sigc::ptr_fun(&do_quit));
  app.catchUnixSignal(SIGINT);

  app.exec();

  audio_io.close();

    // When both encoder and decoder are released, the AMBE device is released
  encoder->release();
  decoder->release();

  return 0;
}
