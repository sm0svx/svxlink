/**
@file	 siglevdetcal.cpp
@brief   Signal level detector calibration utility
@author  Tobias Blomberg / SM0SVX
@date	 2008-03-30

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2023 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cstdlib>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <AsyncFdWatch.h>
#include <AsyncTimer.h>
#include <AsyncAudioIO.h>

#include <LocalRxBase.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SIGLEV_DET_CAL.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define PROGRAM_NAME "SigLevDetCal"

struct CtcssMeasurement
{
  double sum = 0.0;
  size_t count = 0;
};


/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

static const int INTERVAL = 100;
static const int ITERATIONS = 150;

static Config cfg;
static LocalRxBase *rx;
static double open_sum = 0.0f;
static double close_sum = 0.0f;
static float siglev_slope = 10.0;
static float siglev_offset = 0.0;
static std::map<float, CtcssMeasurement> ctcss_snr_sum;
static std::map<float, float> ctcss_open_snr;
static std::map<float, float> ctcss_close_snr;


/****************************************************************************
 *
 * Local Functions
 *
 ****************************************************************************/

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
  printf("Signal strength=%.3f\n",
         siglev_offset + siglev_slope * rx->signalStrength());
  
  close_sum += rx->signalStrength();
  
  if (++count == ITERATIONS)
  {
    delete t;

    float open_close_mean = (open_sum - close_sum) / ITERATIONS;
    float close_mean = close_sum / ITERATIONS;

    float new_siglev_slope = 100.0 / open_close_mean;
    float new_siglev_offset = -close_mean * new_siglev_slope;
    for (const auto& entry : ctcss_snr_sum)
    {
      ctcss_close_snr[entry.first] = entry.second.sum / entry.second.count;
    }

    cout << endl;
    cout << "--- Results\n";
    printf("Mean SNR for the CTCSS tones : ");
    if (!ctcss_close_snr.empty())
    {
      printf("\n");
      for (const auto& entry : ctcss_close_snr)
      {
        float snr = ctcss_open_snr[entry.first] - entry.second;
        printf("    %5.1f : %+5.1fdB\n", entry.first, snr);
      }
    }
    else
    {
      printf("N/A (CTCSS not enabled)\n");
    }
    //printf("Dynamic range for the siglev measurement : %.1fdB\n",
    //       10.0 * open_close_mean);

    cout << endl;
    cout << "--- Put the config variables below in the configuration file\n";
    cout << "--- section for " << rx->name() << ".\n";
    printf("SIGLEV_SLOPE=%.2f\n", new_siglev_slope);
    printf("SIGLEV_OFFSET=%.2f\n", new_siglev_offset);
    if (!ctcss_close_snr.empty())
    {
      printf("CTCSS_SNR_OFFSETS=");
      for (auto it = ctcss_close_snr.begin();
           it != ctcss_close_snr.end(); ++it)
      {
        if (it->first != ctcss_close_snr.begin()->first)
        {
          printf(",");
        }
        printf("%.1f:%+.2f", it->first, it->second);
      }
      printf("\n");
    }
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

    ctcss_snr_sum.clear();

    Timer *timer = new Timer(INTERVAL);
    timer->expired.connect(sigc::ptr_fun(&sample_squelch_close));
  }
} /* start_squelch_close_measurement */


void sample_squelch_open(Timer *t)
{
  static int count = 0;
  printf("Signal strength=%.3f\n",
      siglev_offset + siglev_slope * rx->signalStrength());
  
  open_sum += rx->signalStrength();
  
  if (++count == ITERATIONS)
  {
    delete t;
    
    for (const auto& entry : ctcss_snr_sum)
    {
      ctcss_open_snr[entry.first] = entry.second.sum / entry.second.count;
    }

    FdWatch *w = new FdWatch(0, FdWatch::FD_WATCH_RD);
    // must explicitly specify name space for ptr_fun() to avoid conflict
    // with ptr_fun() in /usr/include/c++/4.5/bits/stl_function.h
    w->activity.connect(sigc::ptr_fun(&start_squelch_close_measurement));
    
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
    ctcss_snr_sum.clear();
    Timer *timer = new Timer(INTERVAL);
    // must explicitly specify name space for ptr_fun() to avoid conflict
    // with ptr_fun() in /usr/include/c++/4.5/bits/stl_function.h
    timer->expired.connect(sigc::ptr_fun(&sample_squelch_open));
  }
  
} /* start_squelch_open_measurement */


void ctcss_snr_updated(float snr, float fq)
{
  auto& measurement = ctcss_snr_sum[fq];
  measurement.sum += snr;
  measurement.count += 1;
}


/****************************************************************************
 *
 * MAIN
 *
 ****************************************************************************/

int main(int argc, char **argv)
{
  CppApplication app;
  
  cout << PROGRAM_NAME " v" SIGLEV_DET_CAL_VERSION
          " Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX\n\n";
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

    // Make sure we have CTCSS squelch enabled
  //cfg.setValue(rx_name, "SQL_DET", "CTCSS");

    // Make sure that the squelch will not open during calibration
  cfg.setValue(rx_name, "CTCSS_OPEN_THRESH", "100");
  cfg.setValue(rx_name, "SIGLEV_OPEN_THRESH", "10000");
  
    // Make sure we are using the "Noise" siglev detector
  //cfg.setValue(rx_name, "SIGLEV_DET", "NOISE");

    // Read the configured siglev slope and offset, then clear them so that
    // they cannot affect the measurement.
  cfg.getValue(rx_name, "SIGLEV_SLOPE", siglev_slope);
  cfg.setValue(rx_name, "SIGLEV_SLOPE", "1.0");
  cfg.getValue(rx_name, "SIGLEV_OFFSET", siglev_offset);
  cfg.setValue(rx_name, "SIGLEV_OFFSET", "0.0");
  
  rx = dynamic_cast<LocalRxBase*>(RxFactory::createNamedRx(cfg, rx_name));
  if (rx == 0)
  {
    cerr << "*** ERROR: The given receiver config section is not for a "
         << "local receiver. Calibration can only be done locally.\n";
    exit(1);
  }
  if (!rx->initialize())
  {
    cerr << "*** ERROR: Could not initialize receiver \"" << rx_name << "\"\n";
    exit(1);
  }
  // must explicitly specify name space for ptr_fun() to avoid conflict
  // with ptr_fun() in /usr/include/c++/4.5/bits/stl_function.h
  //rx->squelchOpen.connect(sigc::ptr_fun(&squelchOpen));
  rx->ctcssSnrUpdated.connect(sigc::ptr_fun(&ctcss_snr_updated));
  rx->setMuteState(Rx::MUTE_NONE);
  rx->setVerbose(false);
  
  FdWatch *w = new FdWatch(0, FdWatch::FD_WATCH_RD);
  // must explicitly specify name space for ptr_fun() to avoid conflict
  // with ptr_fun() in /usr/include/c++/4.5/bits/stl_function.h
  w->activity.connect(sigc::ptr_fun(&start_squelch_open_measurement));
  
  //cout << "--- Press the PTT (" << ITERATIONS << " more times)\n";
  
  cout << "--- Adjust the audio input level to a suitable level.\n";
  cout << "--- Transmit a strong signal into the SvxLink receiver.\n";
  cout << "--- This will represent the strongest possible input signal.\n";
  cout << "--- Don't release the PTT until told so.\n";
  cout << "--- Press ENTER when ready.\n";
  
  app.exec();
  
  delete rx;
  
} /* main */
