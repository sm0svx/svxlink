#include <iostream>

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <version/SIGLEV_DET_CAL.h>

#include <Rx.h>

using namespace std;
using namespace SigC;
using namespace Async;

static const int ITERATIONS = 5;

static Config cfg;
static Rx *rx;
static float open_close_sum = 0.0f;
static float last_open_siglev = 0.0f;
static float close_sum = 0.0f;
static int open_close_cnt = 0;
static float siglev_slope = 1.0;
static float siglev_offset = 0.0;


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


int main(int argc, char **argv)
{
  CppApplication app;
  
  cout << "Signal level detector calibrator v" SIGLEV_DET_CAL_VERSION
       << endl << endl;
  
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
  
  rx = Rx::create(cfg, rx_name);
  if ((rx == 0) || !rx->initialize())
  {
    cerr << "*** ERROR: Could not initialize receiver \"" << rx_name << "\"\n";
    exit(1);
  }
  rx->squelchOpen.connect(slot(&squelchOpen));
  rx->mute(false);
  
  string value;
  if (cfg.getValue(rx_name, "SIGLEV_SLOPE", value))
  {
    siglev_slope = atof(value.c_str());
  }
  if (cfg.getValue(rx_name, "SIGLEV_OFFSET", value))
  {
    siglev_offset = atof(value.c_str());
  }
  
  cout << "--- Press the PTT (" << ITERATIONS << " more times)\n";
  
  app.exec();
  
  delete rx;
  
} /* main */
