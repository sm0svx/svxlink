#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>

using namespace std;
using namespace Async;

class MyClass : public SigC::Object
{
  public:
    MyClass(void)
    {
      audio_io = new AudioIO;
      audio_io->open(AudioIO::MODE_RDWR);
      audio_io->audioRead.connect(slot(this, &MyClass::onAudioRead));
      short samples[8000]; // Put some audio data into the buffer
      audio_io->write(samples, sizeof(samples));
    }
    
    ~MyClass(void)
    {
      delete audio_io;
    }

  private:
    AudioIO *audio_io;
    
    void onAudioRead(short *samples, int count)
    {
      cout << count << " samples read...\n";
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
