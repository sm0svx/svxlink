/**
@file	 devcal.cpp
@brief   An FM deviation calibration utility
@author  Tobias Blomberg / SM0SVX
@date	 2014-08-08

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

#include <unistd.h>
#include <signal.h>
#include <popt.h>
#include <termios.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncConfig.h>
#include <AsyncFdWatch.h>
#include <Tx.h>
#include <Rx.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#if 0
#include "../trx/Ptt.h"
#endif
#include "../trx/Goertzel.h"
#include "../trx/Emphasis.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define PROGRAM_NAME          "devcal"
#define DEFAULT_MOD_FQ        1000.0f
#define DEFAULT_HEADROOM_DB   6.0f
#define DEFAULT_CALDEV        2404.8f
#define DEFAULT_MAXDEV        5000.0f


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class SineGenerator : public Async::AudioSource
{
  public:
    explicit SineGenerator(void)
      : pos(0), fq(0.0), level(0.0), adj_level(1.0),
        sample_rate(INTERNAL_SAMPLE_RATE), enabled(false)
    {
    }
    
    ~SineGenerator(void)
    {
      enable(false);
    }
    
    void setFq(double tone_fq)
    {
      fq = tone_fq;
    }
    
    void setLevel(double level_percent)
    {
      level = level_percent / 100.0;
    }

    void adjustLevel(double adj_db)
    {
      adj_level = pow(10.0, adj_db / 20.0);
    }

    double levelAdjust(void) const
    {
      return 20.0 * log10(adj_level);
    }
    
    void enable(bool enable)
    {
      if (enable && (fq != 0))
      {
        enabled = true;
        pos = 0;
        writeSamples();
      }
      else
      {
        enabled = false;
        sinkFlushSamples();
      }
    }

    bool isEnabled(void) const
    {
      return enabled;
    }

    void resumeOutput(void)
    {
      writeSamples();
    }
    
    void allSamplesFlushed(void)
    {
    }
    
    
  private:
    static const int BLOCK_SIZE = 128;
    
    unsigned  pos;
    double    fq;
    double    level;
    double    adj_level;
    int       sample_rate;
    bool      enabled;
    
    void writeSamples(void)
    {
      if (!enabled)
      {
        return;
      }
      int written;
      do {
	float buf[BLOCK_SIZE];
	for (int i=0; i<BLOCK_SIZE; ++i)
	{
      	  buf[i] = adj_level * level *
                   sin(2 * M_PI * fq * (pos+i) / sample_rate);
	}
	written = sinkWriteSamples(buf, BLOCK_SIZE);
	pos += written;
      } while (written != 0);
    }
    
};


class Window
{
  public:
    Window(size_t N)
      : N(N)
    {
      w = new float[N];
    }

    ~Window(void)
    {
      delete [] w;
    }

    void normalize(void)
    {
      double mean = 0.0;
      for (size_t n=0; n<N; ++n)
      {
        mean += w[n];
      }
      mean /= N;
      for (size_t n=0; n<N; ++n)
      {
        w[n] /= mean;
      }
    }

    inline size_t size(void) const { return N; }

    inline float operator[](int i) const { return w[i]; }

  protected:
    size_t N;
    float *w;
};


class FlatTopWindow : public Window
{
  public:
    FlatTopWindow(size_t N, bool do_normalize=true)
      : Window(N)
    {
      static const double a0 = 0.21557895;
      static const double a1 = 0.41663158;
      static const double a2 = 0.277263158;
      static const double a3 = 0.083578947;
      static const double a4 = 0.006947368;
      for (unsigned n=0; n<N; ++n)
      {
        w[n] = a0
             - a1 * cos(2*M_PI*n / (N-1))
             + a2 * cos(4*M_PI*n / (N-1))
             - a3 * cos(6*M_PI*n / (N-1))
             + a4 * cos(8*M_PI*n / (N-1));
      }
      if (do_normalize)
      {
        normalize();
      }
    }
};


class DevPrinter : public AudioSink
{
  public:
    DevPrinter(float mod_fq, float max_dev, float headroom_db)
      : block_size(INTERNAL_SAMPLE_RATE / 20), w(block_size),
        g(mod_fq, INTERNAL_SAMPLE_RATE), samp_cnt(0), max_dev(max_dev),
        headroom(pow(10.0, headroom_db/20.0)), adj_level(1.0f), dev_est(0.0),
        block_cnt(0)
    {
    }

    void adjustLevel(double adj_db)
    {
      adj_level = pow(10.0, adj_db / 20.0);
      if (abs(1.0-adj_level) < 0.0001)
      {
        adj_level = 1.0;
      }
    }

    double levelAdjust(void) const
    {
      return 20.0 * log10(adj_level);
    }
    
    virtual int writeSamples(const float *samples, int count)
    {
      for (int i=0; i<count; ++i)
      {
        g.calc(w[samp_cnt] * samples[i]);
        if (++samp_cnt >= block_size)
        {
          float ampl = 2 * sqrt(g.magnitudeSquared()) / block_size;
          ampl *= adj_level;
          float dev = headroom * max_dev * ampl;
          dev_est = (1.0-ALPHA) * dev + ALPHA * dev_est;
          if (++block_cnt >= PRINT_INTERVAL)
          {
            cout << "Deviation: " << dev_est << "\r";
            cout.flush();
            block_cnt = 0;
          }
          g.reset();
          samp_cnt = 0;
        }
      }
      return count;
    }

    virtual void flushSamples(void)
    {
      sourceAllSamplesFlushed();
    }

  private:
    static const double ALPHA = 0.75;         // IIR filter coeff
    static const size_t PRINT_INTERVAL = 5;   // Block count

    int           block_size;
    FlatTopWindow w;
    Goertzel      g;
    int           samp_cnt;
    float         max_dev;
    float         headroom;
    double        adj_level;
    double        dev_est;
    size_t        block_cnt;
};


/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

static void parse_arguments(int argc, const char **argv);
static void stdin_handler(FdWatch *w);
static void sigterm_handler(int signal);


/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

static float mod_fq = DEFAULT_MOD_FQ;
static float headroom_db = DEFAULT_HEADROOM_DB;
static float caldev = DEFAULT_CALDEV;
static float maxdev = DEFAULT_MAXDEV;
static int cal_rx = false;
static int cal_tx = false;
static int flat_fq_response = false;
static string cfgfile;
static string cfgsect;
static FdWatch *stdin_watch = 0;
static SineGenerator *gen = 0;
static DevPrinter *dp = 0;
static Tx *tx = 0;
static Rx *rx = 0;
static float level_adjust_offset = 0.0f;


/****************************************************************************
 *
 * MAIN
 *
 ****************************************************************************/

int main(int argc, const char *argv[])
{
  setlocale(LC_ALL, "");
  CppApplication app;
  app.catchUnixSignal(SIGINT);
  app.catchUnixSignal(SIGTERM);
  app.unixSignalCaught.connect(sigc::ptr_fun(&sigterm_handler));

  parse_arguments(argc, const_cast<const char **>(argv));

  /*
  const string audio_dev = "alsa:plughw:0";
  const unsigned audio_ch = 0;
  */

  float mod_idx = caldev / mod_fq;
  float mod_level = 100.0 * caldev / (maxdev * pow(10.0, headroom_db / 20.0));
  cout << "--- Modulation frequency  : " << mod_fq << " Hz\n";
  cout << "--- Calibration deviation : " << caldev << " Hz\n";
  cout << "--- Maximum deviation     : " << maxdev << " Hz\n";
  cout << "--- Modulation index      : " << mod_idx << "\n";
  cout << "--- Headroom              : " << headroom_db << "dB\n";
  cout << "--- Peak sample level     : "
       << (20.0*log10(mod_level / 100.0)) << "dBF (" << mod_level << "%)\n";
  cout << endl;

  ios::fmtflags old_cout_flags(cout.flags());
  cout.precision(2);
  cout << fixed;

  Config cfg;
  if (!cfg.open(cfgfile))
  {
    cerr << "*** ERROR: Could not open configuration file \""
         << cfgfile << "\".\n";
    exit(1);
  }


  if (cal_tx)
  {
    cfg.getValue(cfgsect, "MASTER_GAIN", level_adjust_offset);
    cout << "MASTER_GAIN=" << level_adjust_offset << endl;

    gen = new SineGenerator;
    gen->setFq(mod_fq);
    gen->setLevel(mod_level);
    AudioSource *prev_src = gen;

    if (!flat_fq_response)
    {
      DeemphasisFilter *deemph = new DeemphasisFilter;
      prev_src->registerSink(deemph, true);
      prev_src = deemph;
    }
    
    tx = TxFactory::createNamedTx(cfg, cfgsect);
    if ((tx == 0) || !tx->initialize())
    {
      cerr << "*** ERROR: Could not initialize traceiver object\n";
      exit(1);
    }
    prev_src->registerSink(tx);
    tx->setTxCtrlMode(Tx::TX_AUTO);
    gen->enable(true);
  }
  else if (cal_rx)
  {
    cfg.getValue(cfgsect, "PREAMP", level_adjust_offset);
    cout << "PREAMP=" << level_adjust_offset << endl;

    cfg.setValue(cfgsect, "SQL_DET", "OPEN");
    cfg.setValue(cfgsect, "DTMF_MUTING", "0");

    rx = RxFactory::createNamedRx(cfg, cfgsect);
    if ((rx == 0) || !rx->initialize())
    {
      cerr << "*** ERROR: Could not initialize receiver object\n";
      exit(1);
    }
    rx->setVerbose(false);
    AudioSource *prev_src = rx;

    if (!flat_fq_response)
    {
      PreemphasisFilter *preemph = new PreemphasisFilter;
      prev_src->registerSink(preemph, true);
      prev_src = preemph;
    }
    
    dp = new DevPrinter(mod_fq, maxdev, headroom_db);
    prev_src->registerSink(dp, true);
    rx->setMuteState(Rx::MUTE_NONE);
  }

#if 0
  AudioIO audio_out(audio_dev, audio_ch);
  gen.registerSink(&audio_out);
  gen.enable(true);
  ptt->setTxOn(true);

  Ptt *ptt = PttFactoryBase::createNamedPtt(cfg, cfgsect);
  if ((ptt == 0) || (!ptt->initialize(cfg, cfgsect)))
  {
    return false;
  }
#endif

  struct termios org_termios;
  struct termios termios;
  tcgetattr(STDIN_FILENO, &org_termios);
  termios = org_termios;
  termios.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &termios);
  stdin_watch = new FdWatch(STDIN_FILENO, FdWatch::FD_WATCH_RD);
  stdin_watch->activity.connect(sigc::ptr_fun(&stdin_handler));

  app.exec();

#if 0
  ptt->setTxOn(false);
#endif
  delete gen;
  delete tx;
  delete rx;

  delete stdin_watch;
  tcsetattr(STDIN_FILENO, TCSANOW, &org_termios);

  cout.flags(old_cout_flags);

  return 0;
}



/****************************************************************************
 *
 * Functions
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Function:  parse_arguments
 * Purpose:   Parse the command line arguments.
 * Input:     argc  - Number of arguments in the command line
 *    	      argv  - Array of strings with the arguments
 * Output:    Returns 0 if all is ok, otherwise -1.
 * Author:    Tobias Blomberg, SM0SVX
 * Created:   2000-06-13
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
static void parse_arguments(int argc, const char **argv)
{
  poptContext optCon;
  const struct poptOption optionsTable[] =
  {
    POPT_AUTOHELP
    {"modfq", 'f', POPT_ARG_FLOAT | POPT_ARGFLAG_SHOW_DEFAULT, &mod_fq, 0,
	    "The frequency of the sine wave to modulate with",
            "<frequency in hz>"},
    {"caldev", 'd', POPT_ARG_FLOAT | POPT_ARGFLAG_SHOW_DEFAULT, &caldev, 0,
	    "The deviation to calibrate with", "<deviation in Hz>"},
    {"maxdev", 'm', POPT_ARG_FLOAT | POPT_ARGFLAG_SHOW_DEFAULT, &maxdev, 0,
	    "The maximum deviation for the channel", "<deviation in Hz>"},
    {"headroom", 'H', POPT_ARG_FLOAT | POPT_ARGFLAG_SHOW_DEFAULT,
            &headroom_db, 0, "The headroom to use", "<headroom in dB>"},
    /*
    {"config", 0, POPT_ARG_STRING, &config, 0,
	    "Specify the configuration file to use", "<filename>"},
    */
    {NULL, 'r', POPT_ARG_NONE, &cal_rx, 0, "Do receiver calibration", NULL},
    {NULL, 't', POPT_ARG_NONE, &cal_tx, 0, "Do transmitter calibration", NULL},
    {NULL, 'F', POPT_ARG_NONE, &flat_fq_response, 0,
            "Flat TX/RX frequency response (no emphasis)", NULL},
    {NULL, 0, 0, NULL, 0}
  };
  int err;
  
  optCon = poptGetContext(PROGRAM_NAME, argc, argv, optionsTable, 0);
  poptSetOtherOptionHelp(optCon, "<config file> <config section>");
  poptReadDefaultConfig(optCon, 0);
  
  err = poptGetNextOpt(optCon);
  if (err != -1)
  {
    cerr << "*** ERROR: " << poptBadOption(optCon, POPT_BADOPTION_NOALIAS)
         << ": " << poptStrerror(err) << endl;
    poptPrintUsage(optCon, stderr, 0);
    exit(1);
  }

    /* Parse arguments that do not begin with '-' (leftovers) */
  const char *arg = 0;
  int argcnt = 0;
  while ((arg = poptGetArg(optCon)) != NULL)
  {
    switch (argcnt++)
    {
      case 0:
        cfgfile = arg;
        break;
      case 1:
        cfgsect = arg;
        break;
      default:
        cerr << "*** ERROR: Too many command line arguments\n";
        poptPrintUsage(optCon, stderr, 0);
        exit(1);
    }
  }

  if (!(cal_rx ^ cal_tx))
  {
    cerr << "*** ERROR: Either -r or -t command line options must be given\n";
    poptPrintUsage(optCon, stderr, 0);
    exit(1);
  }

  if (cfgfile.empty())
  {
    cerr << "*** ERROR: Configuration file not given\n";
    poptPrintUsage(optCon, stderr, 0);
    exit(1);
  }
  if (cfgsect.empty())
  {
    cerr << "*** ERROR: Configuration section not given\n";
    poptPrintUsage(optCon, stderr, 0);
    exit(1);
  }

  poptFreeContext(optCon);

} /* parse_arguments */


static void stdin_handler(FdWatch *w)
{
  char buf[1];
  int cnt = ::read(STDIN_FILENO, buf, 1);
  if (cnt == -1)
  {
    cerr << "*** ERROR: Read error while reading from stdin\n";
    Application::app().quit();
    return;
  }
  else if (cnt == 0)
  {
      /* Stdin file descriptor closed */
    delete stdin_watch;
    stdin_watch = 0;
    return;
  }
  
  switch (toupper(buf[0]))
  {
    case 'Q':
      cout << "\nQ pressed. Shutting down application...\n";
      Application::app().quit();
      break;

    case '+':
    {
      if (cal_tx)
      {
        double gain = gen->levelAdjust() + 0.01;
        gen->adjustLevel(gain);
        cout << "MASTER_GAIN="
             << (gen->levelAdjust() + level_adjust_offset);
      }
      else if (cal_rx)
      {
        double gain = dp->levelAdjust() + 0.01;
        dp->adjustLevel(gain);
        cout << "PREAMP="
             << (dp->levelAdjust() + level_adjust_offset);
      }
      cout << "      \n";
      break;
    }
    
    case '-':
    {
      if (cal_tx)
      {
        double gain = gen->levelAdjust() - 0.01;
        gen->adjustLevel(gain);
        cout << "MASTER_GAIN="
             << (gen->levelAdjust() + level_adjust_offset);
      }
      else if (cal_rx)
      {
        double gain = dp->levelAdjust() - 0.01;
        dp->adjustLevel(gain);
        cout << "PREAMP="
             << (dp->levelAdjust() + level_adjust_offset);
      }
      cout << "      \n";
      break;
    }

    case '0':
    {
      if (cal_tx)
      {
        gen->adjustLevel(-level_adjust_offset);
        cout << "MASTER_GAIN="
             << (gen->levelAdjust() + level_adjust_offset);
      }
      else if (cal_rx)
      {
        dp->adjustLevel(-level_adjust_offset);
        cout << "PREAMP="
             << (dp->levelAdjust() + level_adjust_offset);
      }
      cout << "      \n";
      break;
    }

    case 'T':
      if (cal_tx)
      {
        gen->enable(!gen->isEnabled());
      }
      break;
    
    default:
      break;
  }
} /* stdin_handler */


static void sigterm_handler(int signal)
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


/*
 * This file has not been truncated
 */
