#include <iostream>
#include <cstdlib>

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <AsyncFdWatch.h>
#include <AsyncTimer.h>
#include <AsyncAudioIO.h>

#include <Rx.h>

#include "version/SIGLEV_DET_CAL.h"

using namespace std;
using namespace SigC;
using namespace Async;

#define PROGRAM_NAME "SigLevDetCal"

static const int ITERATIONS = 5;

static Config cfg;
static Rx *rx;
static float open_sum = 0.0f;
//static float open_close_sum = 0.0f;
//static float last_open_siglev = 0.0f;
static float close_sum = 0.0f;
//static int open_close_cnt = 0;
static float siglev_slope = 1.0;
static float siglev_offset = 0.0;


#if 0
void squelchOpen(bool is_open)
{
  if (is_open)
  {
    last_open_siglev = rx->signalStrength();
    cout << "--- Release the PTT\n";
  }
  else
  {
    open_close_sum += last_open_siglev - rx->signalStrength();
    close_sum += rx->signalStrength();
    if (++open_close_cnt == ITERATIONS)
    {
      float open_close_mean = open_close_sum / ITERATIONS;
      float close_mean = close_sum / ITERATIONS;
      
      open_close_mean /= siglev_slope;
      close_mean -= siglev_offset;
      close_mean /= siglev_slope;
    
      float new_siglev_slope = 100.0 / open_close_mean;
      float new_siglev_offset = -close_mean * new_siglev_slope;
      
      printf("\n");
      printf("SIGLEV_SLOPE=%.2f\n", new_siglev_slope);
      printf("SIGLEV_OFFSET=%.2f\n", new_siglev_offset);
      printf("\n");
      exit(0);
    }
    else
    {
      cout << "--- Press the PTT (" << (ITERATIONS - open_close_cnt)
      	   << " more times)\n";
    }
  }
} /* squelchOpen */
#endif


void sample_squelch_close(Timer *t)
{
  static int count = 0;
  printf("Signal strength=%.3f\n", rx->signalStrength());
  
  close_sum += rx->signalStrength();
  
  if (++count == ITERATIONS)
  {
    delete t;

    float open_close_mean = (open_sum - close_sum) / ITERATIONS;
    float close_mean = close_sum / ITERATIONS;

    open_close_mean /= siglev_slope;
    close_mean -= siglev_offset;
    close_mean /= siglev_slope;

    float new_siglev_slope = 100.0 / open_close_mean;
    float new_siglev_offset = -close_mean * new_siglev_slope;

    cout << endl;
    cout << "--- Put the config variables below in the configuration file\n";
    cout << "--- section for the receiver.\n";
    printf("SIGLEV_SLOPE=%.2f\n", new_siglev_slope);
    printf("SIGLEV_OFFSET=%.2f\n", new_siglev_offset);
    cout << endl;
    
    //rx->setVerbose(true);
    
    Application::app().quit();
  }
  else
  {
    t->reset();
  }
} /* sample_squelch_close */


void start_squelch_close_measurement(FdWatch *w)
{
  int ch = getchar();
  
  if (ch == '\n')
  {
    cout << "--- Starting squelch close measurement\n";
    delete w;
    Timer *timer = new Timer(1000);
    timer->expired.connect(slot(&sample_squelch_close));
  }
} /* start_squelch_close_measurement */


void sample_squelch_open(Timer *t)
{
  static int count = 0;
  printf("Signal strength=%.3f\n", rx->signalStrength());
  
  open_sum += rx->signalStrength();
  
  if (++count == ITERATIONS)
  {
    delete t;
    
    FdWatch *w = new FdWatch(0, FdWatch::FD_WATCH_RD);
    w->activity.connect(slot(&start_squelch_close_measurement));
    
    cout << endl;
    cout << "--- Release the PTT.\n";
    cout << "--- Open the squelch on the SvxLink receiver with no input signal\n";
    cout << "--- so that SvxLink will only receive noise. This will represent\n";
    cout << "--- the weakest possible input signal.\n";
    cout << "--- Press ENTER when ready.\n";
  }
  else
  {
    t->reset();
  }
} /* sample_squelch_open */


void start_squelch_open_measurement(FdWatch *w)
{
  int ch = getchar();
  
  if (ch == '\n')
  {
    cout << "--- Starting squelch open measurement\n";
    delete w;
    Timer *timer = new Timer(1000);
    timer->expired.connect(slot(&sample_squelch_open));
  }
  
} /* start_squelch_open_measurement */


int main(int argc, char **argv)
{
  CppApplication app;
  
  cout << PROGRAM_NAME " v" SIGLEV_DET_CAL_VERSION " (" __DATE__ 
          ") Copyright (C) 2011 Tobias Blomberg / SM0SVX\n\n";
  cout << PROGRAM_NAME " comes with ABSOLUTELY NO WARRANTY. "
          "This is free software, and you\n";
  cout << "are welcome to redistribute it in accordance with the "
          "terms and conditions in\n";
  cout << "the GNU GPL (General Public License) version 2 or later.\n\n";

  if (argc < 3)
  {
    cerr << "Usage: siglevdetcal <config file> <receiver section>\n";
    exit(1);
  }
  string cfg_file(argv[1]);
  string rx_name(argv[2]);
  
  if (!cfg.open(cfg_file))
  {
    cerr << "*** ERROR: Could not open config file \"" << cfg_file << "\"\n";
    exit(1);
  }
  
  string value;
  if (cfg.getValue("GLOBAL", "CARD_SAMPLE_RATE", value))
  {
    int rate = atoi(value.c_str());
    if (rate == 48000)
    {
      AudioIO::setBlocksize(1024);
      AudioIO::setBlockCount(4);
    }
    else if (rate == 16000)
    {
      AudioIO::setBlocksize(512);
      AudioIO::setBlockCount(2);
    }
    #if INTERNAL_SAMPLE_RATE <= 8000
    else if (rate == 8000)
    {
      AudioIO::setBlocksize(256);
      AudioIO::setBlockCount(2);
    }
    #endif
    else
    {
      cerr << "*** ERROR: Illegal sound card sample rate specified for "
      	      "config variable GLOBAL/CARD_SAMPLE_RATE. Valid rates are "
	      #if INTERNAL_SAMPLE_RATE <= 8000
	      "8000, "
	      #endif
	      "16000 and 48000\n";
      exit(1);
    }
    AudioIO::setSampleRate(rate);
    cout << "--- Using sample rate " << rate << "Hz\n";
  }
  
  string rx_type;
  if (!cfg.getValue(rx_name, "TYPE", rx_type))
  {
    cerr << "*** ERROR: Config variable " << rx_name << "/TYPE not set. "
         << "Are you sure \"" << rx_name << "\" is an existing receiver config "
	 << "section?\n";
    exit(1);
  }
  if (rx_type != "Local")
  {
    cerr << "*** WARNING: The given receiver config section is not for a "
         << "local receiver. You are on your own...\n";
  }
  
  rx = RxFactory::createNamedRx(cfg, rx_name);
  if ((rx == 0) || !rx->initialize())
  {
    cerr << "*** ERROR: Could not initialize receiver \"" << rx_name << "\"\n";
    exit(1);
  }
  //rx->squelchOpen.connect(slot(&squelchOpen));
  rx->mute(false);
  rx->setVerbose(false);
  
  if (cfg.getValue(rx_name, "SIGLEV_SLOPE", value))
  {
    siglev_slope = atof(value.c_str());
  }
  if (cfg.getValue(rx_name, "SIGLEV_OFFSET", value))
  {
    siglev_offset = atof(value.c_str());
  }
  
  FdWatch *w = new FdWatch(0, FdWatch::FD_WATCH_RD);
  w->activity.connect(slot(&start_squelch_open_measurement));
  
  //cout << "--- Press the PTT (" << ITERATIONS << " more times)\n";
  
  cout << "--- Adjust the audio input level to a suitable level.\n";
  cout << "--- Transmit a strong signal into the SvxLink receiver.\n";
  cout << "--- This will represent the strongest possible input signal.\n";
  cout << "--- Don't release the PTT until told so.\n";
  cout << "--- Press ENTER when ready.\n";
  
  app.exec();
  
  delete rx;
  
} /* main */
