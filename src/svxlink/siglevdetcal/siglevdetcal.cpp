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
      float siglev_slope = 100.0 / (open_close_sum / ITERATIONS);
      float siglev_offset = -(close_sum / ITERATIONS) * siglev_slope;
      printf("SIGLEV_SLOPE=%.2f\n", siglev_slope);
      printf("SIGLEV_OFFSET=%.2f\n", siglev_offset);
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
  cout << "Signal level detector calibrator v" SIGLEV_DET_CAL_VERSION
       << endl << endl;
  
  if (argc < 3)
  {
    cerr << "Usage: siglevdetcal <config file> <receiver section>\n";
    exit(1);
  }
  string cfg_file(argv[1]);
  string rx_name(argv[2]);
  
  CppApplication app;
  
  if (!cfg.open(cfg_file))
  {
    cerr << "*** ERROR: Could not open config file \"" << cfg_file << "\"\n";
    exit(1);
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
    cout << "*** WARNING: Configuration variable " << rx_name
         << "/SIGLEV_SLOPE is set. Please remove it.\n\n";
  }
  if (cfg.getValue(rx_name, "SIGLEV_OFFSET", value))
  {
    cout << "*** WARNING: Configuration variable " << rx_name
         << "/SIGLEV_OFFSET is set. Please remove it.\n\n";
  }
  
  cout << "--- Press the PTT (" << ITERATIONS << " more times)\n";
  
  app.exec();
  
  delete rx;
  
} /* main */
