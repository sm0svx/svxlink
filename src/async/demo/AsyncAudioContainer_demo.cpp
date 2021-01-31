//
// This example application will open the default audio device for reading
// (recording) and write the audio to three different file formats
// concurrently: raw, wav and opus. Recording will stop after five seconds.
// Listen to the recordings using the following commands.
//
//   aplay -fS16_LE -r16000 AsyncAudioContainer_demo.raw
//   aplay AsyncAudioContainer_demo.wav
//   opusdec AsyncAudioContainer_demo.opus - | aplay -fS16_LE -r16000
//

#include <iostream>
#include <fstream>
#include <cstring>
#include <AsyncCppApplication.h>
#include <AsyncTimer.h>
#include <AsyncAudioSplitter.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioContainer.h>


class AudioFileWriter : public Async::AudioSink
{
  public:
    AudioFileWriter(Async::AudioContainer* container)
      : m_container(container)
    {
        // Create the container and set it to handle incoming audio
      assert (m_container != nullptr);
      m_container->writeBlock.connect(
          sigc::mem_fun(*this, &AudioFileWriter::handleBlock));
      setHandler(m_container);

        // Open file using an extension matching the container format
      std::ostringstream oss;
      std::string filename(std::string("AsyncAudioContainer_demo.") +
                           m_container->filenameExtension());
      std::cout << "Opening file " << filename << std::endl;
      m_ofs.open(filename, std::ofstream::out);
      assert(m_ofs.is_open());

        // Reserve space for the file header which is written at the end
      if (m_container->headerSize() > 0)
      {
        char dummy[m_container->headerSize()];
        std::memset(dummy, 0, m_container->headerSize());
        m_ofs.write(dummy, sizeof(dummy));
      }
      assert(m_ofs.good());
    }

    ~AudioFileWriter(void)
    {
        // Flush stream and write end of stream record if format support it
      m_container->endStream();

        // If format use file header, write it to beginning of file
      if (m_container->headerSize() > 0)
      {
        std::cout << "Writing " << m_container->filenameExtension()
                  << " file header (size=" << m_container->headerSize()
                  << " bytes)" << std::endl;
        m_ofs.seekp(0);
        m_ofs.write(m_container->header(), m_container->headerSize());
        assert(m_ofs.good());
      }

        // Close the file
      std::cout << "Closing " << m_container->filenameExtension()
                << " file" << std::endl;
      m_ofs.close();

        // Clean up
      clearHandler();
      delete m_container;
      m_container = 0;
    }

  private:
    Async::AudioContainer*  m_container = 0;
    std::ofstream           m_ofs;

    void handleBlock(const char* buf, size_t len)
    {
      std::cout << "Writing " << m_container->filenameExtension()
                << " audio block (size=" << len << " bytes)" << std::endl;
      m_ofs.write(buf, len);
      assert(m_ofs.good());
    }
};


int main(int argc, char **argv)
{
  Async::CppApplication app;

    // Create a new audio IO object
  Async::AudioIO audio_io("alsa:default", 0);

    // Open it for reading
  audio_io.open(Async::AudioIO::MODE_RD);

    // Create an audio splitter and connect it to the audio device
  Async::AudioSplitter splitter;
  audio_io.registerSink(&splitter);

    // Create a wav file writer
  AudioFileWriter* wav_writer = 0;
  Async::AudioContainer* container = Async::createAudioContainer("wav");
  if (container != nullptr)
  {
    wav_writer = new AudioFileWriter(container);
    splitter.addSink(wav_writer);
  }

    // Create a pcm (16bit signed integer raw) file writer
  AudioFileWriter* pcm_writer = 0;
  container = Async::createAudioContainer("vnd.svxlink.pcm");
  if (container != nullptr)
  {
    pcm_writer = new AudioFileWriter(container);
    splitter.addSink(pcm_writer);
  }

    // Create an ogg/opus file writer
  AudioFileWriter* opus_writer = 0;
  container = Async::createAudioContainer("opus");
  if (container != nullptr)
  {
    opus_writer = new AudioFileWriter(container);
    splitter.addSink(opus_writer);
  }

    // Start a timer to quit the application after five seconds
  Async::Timer duration_timer(5000);
  duration_timer.expired.connect([&](Async::Timer*){
        Async::Application::app().quit();
      });

    // Start main application loop
  app.exec();

    // Clean up
  delete pcm_writer;
  delete wav_writer;
  delete opus_writer;
}
