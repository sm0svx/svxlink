#include <math.h>

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
      audio_io = new AudioIO("/dev/dsp");
      audio_io->open(AudioIO::MODE_RDWR);
      audio_io->audioRead.connect(slot(this, &MyClass::onAudioRead));
      	/* Or just coonect the AudioIO object to itself... */
      //audio_io->audioRead.connect(slot(audio_io, &AudioIO::write));
      
      short samples[8000];
      for (int i=0; i<8000; ++i) // Put a sine wave into the buffer
      {
      	samples[i] = static_cast<short int>(10000 * sin(2*M_PI*440*i/8000));
      }
      int samples_written = audio_io->write(samples, 8000);
      cout << "Wrote " << samples_written << " samples...\n";
    }
    
    ~MyClass(void)
    {
      delete audio_io;
    }

  private:
    AudioIO *audio_io;
    
    int onAudioRead(short *samples, int count)
    {
      //cout << count << " samples read...\n";
      return audio_io->write(samples, count);
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
