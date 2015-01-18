#include <signal.h>

#include <cmath>
#include <cstdlib>
#include <iostream>

#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncConfig.h>

#include "../trx/Ptt.h"


using namespace std;
using namespace Async;


class NoiseGenerator : public Async::AudioSource
{
  public:
    explicit NoiseGenerator(const string& audio_dev, int channel)
      : audio_io(audio_dev, channel), level(0.0), sample_rate(0)
    {
      sample_rate = audio_io.sampleRate();
      audio_io.registerSource(this);
    }
    
    ~NoiseGenerator(void)
    {
      enable(false);
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
      
      if (enable)
      {
      	if (audio_io.open(AudioIO::MODE_WR))
        {
          writeSamples();
        }
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
    double    level;
    int       sample_rate;
    
    void writeSamples(void)
    {
      int written;
      do {
	float buf[BLOCK_SIZE];
	for (int i=0; i<BLOCK_SIZE; ++i)
	{
            // coverity[dont_call]
      	  buf[i] = level * static_cast<float>(rand()) / RAND_MAX;
	}
	written = sinkWriteSamples(buf, BLOCK_SIZE);
      } while (written != 0);
    }
    
};


void sigterm_handler(int signal)
{
  const char *signame = 0;
  switch (signal)
  {
    case SIGTERM:
      signame = "SIGTERM";
      break;
    case SIGINT:
      signame = "SIGINT";
      break;
    default:
      signame = "???";
      break;
  }
  string msg("\n");
  msg += signame;
  msg += " received. Shutting down application...\n";
  cout << msg;
  Application::app().quit();
} /* sigterm_handler */


void level_step(Timer *t, NoiseGenerator *gen)
{
  static unsigned mod_level = 0;
  if (mod_level == 100)
  {
    Application::app().quit();
  }
  cout << "--- Level: " << mod_level << endl;
  gen->setLevel(mod_level++);
}


int main(int argc, const char *argv[])
{
  const float caldev = 2404.8;
  const float maxdev = 5000;
  const float mod_idx = 2.4048;
  const string audio_dev = "alsa:plughw:0";
  const unsigned audio_ch = 0;
  const string tx_name = "TX";

  float mod_fq = caldev / mod_idx;
  //mod_fq = 136.5;
  float mod_level = 100.0 * caldev / maxdev;
  mod_level = 50;
  cout << "--- Modulation frequency : " << mod_fq << " Hz\n";
  cout << "--- Modulation level     : " << mod_level << "%\n";
  cout << "--- Modulation index     : " << mod_idx << "%\n";
  static struct sigaction sigint_oldact, sigterm_oldact;

  CppApplication app;

  Config cfg;
  cfg.setValue(tx_name, "PTT_TYPE", "GPIO");
  cfg.setValue(tx_name, "PTT_PIN", "gpio4_pd0");
  Ptt *ptt = PttFactoryBase::createNamedPtt(cfg, tx_name);
  if ((ptt == 0) || (!ptt->initialize(cfg, tx_name)))
  {
    return false;
  }

  struct sigaction act;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  act.sa_handler = sigterm_handler;
  if (sigaction(SIGTERM, &act, &sigterm_oldact) == -1)
  {
    perror("sigaction");
    exit(1);
  }

  act.sa_handler = sigterm_handler;
  if (sigaction(SIGINT, &act, &sigint_oldact) == -1)
  {
    perror("sigaction");
    exit(1);
  }

  NoiseGenerator gen(audio_dev, audio_ch);
  //gen.setFq(mod_fq);
  gen.setLevel(mod_level);

  AudioIO::setSampleRate(48000);
  AudioIO audio_out(audio_dev, audio_ch);
  gen.registerSink(&audio_out);
  gen.enable(true);
  ptt->setTxOn(true);

  Timer t(1000, Timer::TYPE_PERIODIC);
  //t.expired.connect(sigc::bind(sigc::ptr_fun(&level_step), &gen));

  app.exec();

  ptt->setTxOn(false);

  return 0;
}
