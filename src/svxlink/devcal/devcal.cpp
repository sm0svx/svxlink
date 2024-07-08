/**
@file	 devcal.cpp
@brief   An FM deviation calibration utility
@author  Tobias Blomberg / SM0SVX
@date	 2014-08-08

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

#include <unistd.h>
#include <signal.h>
#include <popt.h>
#include <termios.h>

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncAudioIO.h>
#include <AsyncAudioSplitter.h>
#include <AsyncConfig.h>
#include <AsyncFdWatch.h>
#include <Tx.h>
#include <Rx.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/DEVCAL.h"
#if 0
#include "../trx/Ptt.h"
#endif
#include "../trx/Goertzel.h"
#include "../trx/Emphasis.h"
#include "../trx/RtlSdr.h"
#include "../trx/Ddr.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace std::placeholders;
using namespace Async;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define PROGRAM_NAME          "devcal"
#define DEFAULT_MOD_FQS       "1000.0"
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
    explicit SineGenerator(const vector<float> &fqs)
      : pos(0), fqs(fqs), level(0.0), adj_level(1.0),
        sample_rate(INTERNAL_SAMPLE_RATE), enabled(false)
    {
    }
    
    ~SineGenerator(void)
    {
      enable(false);
    }
    
    void setLevel(double level_percent)
    {
      level = level_percent / 100.0;
      if (fqs.size() > 1)
      {
        level /= pow(10.0, 3.0/20.0) * (fqs.size() - 1);
      }
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
      if (enable && !fqs.empty())
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
    
    unsigned      pos;
    vector<float> fqs;
    double        level;
    double        adj_level;
    int           sample_rate;
    bool          enabled;
    
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
      	  buf[i] = 0.0f;
          for (vector<float>::const_iterator it = fqs.begin();
              it != fqs.end();
              ++it)
          {
            const float &fq = *it;
            buf[i] += adj_level * level *
                      sin(2 * M_PI * fq * (pos+i) / sample_rate);
          }
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
    DevPrinter(unsigned samp_rate, const vector<float> &mod_fqs,
               float max_dev=1.0f, float headroom_db=0.0f)
      : block_size(samp_rate / 20), w(block_size),
        g(mod_fqs.size()), samp_cnt(0), max_dev(max_dev),
        headroom(pow(10.0, headroom_db/20.0)), adj_level(1.0f), dev_est(0.0),
        block_cnt(0), pwr_sum(0.0), tot_dev_est(0.0f), amp_sum(0.0),
        fqerr_est(0.0), carrier_fq(0.0)
    {
      for (size_t i=0; i<mod_fqs.size(); ++i)
      {
        g[i].initialize(mod_fqs[i], samp_rate);
      }
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

    void setCarrierFq(double carrier_fq)
    {
      this->carrier_fq = carrier_fq;
    }

    double carrierFq(void) const { return carrier_fq; }
    
    virtual int writeSamples(const float *samples, int count)
    {
      for (int i=0; i<count; ++i)
      {
        pwr_sum += static_cast<double>(samples[i]) * samples[i];
        amp_sum += samples[i];

        float windowed = w[samp_cnt] * samples[i];
        for (size_t i=0; i<g.size(); ++i)
        {
          g[i].calc(windowed);
        }
        if (++samp_cnt >= block_size)
        {
          double avg_power = pwr_sum / block_size;
          double tot_dev = sqrt(avg_power) * sqrt(2);
          tot_dev *= adj_level;
          tot_dev *= headroom * max_dev;
          tot_dev_est = (1.0-ALPHA) * tot_dev + ALPHA * tot_dev_est;
          pwr_sum = 0.0;

          double dev = 0.0;
          for (size_t i=0; i<g.size(); ++i)
          {
            dev += g[i].magnitudeSquared();
          }
          dev = 2 * sqrt(dev) / block_size;
          dev *= adj_level;
          dev *= headroom * max_dev;
          dev_est = (1.0-ALPHA) * dev + ALPHA * dev_est;

          double fqerr = amp_sum / block_size;
          fqerr *= adj_level;
          fqerr *= headroom * max_dev;
          amp_sum = 0.0;
          fqerr_est = (1.0-ALPHA) * fqerr + ALPHA * fqerr_est;

          if (++block_cnt >= PRINT_INTERVAL)
          {
            cout << "\r\033[K" "Tone dev=" << dev_est
                 << "  Full bw dev=" << tot_dev_est 
                 << "  Carrier freq err=" << fqerr_est;
            if (carrier_fq > 0.0)
            {
              int ppm_err =
                static_cast<int>(round(1000000.0 * fqerr_est / carrier_fq));
              cout << "(" << ppm_err << "ppm)";
            }
            cout.flush();
            block_cnt = 0;
          }
          for (size_t i=0; i<g.size(); ++i)
          {
            g[i].reset();
          }
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
    static CONSTEXPR double ALPHA = 0.9;        //!< IIR filter coeff
    static CONSTEXPR size_t PRINT_INTERVAL = 5; //!< Block count

    int           block_size;
    FlatTopWindow w;
    vector<Goertzel> g;
    int           samp_cnt;
    float         max_dev;
    double        headroom;
    double        adj_level;
    double        dev_est;
    size_t        block_cnt;
    double        pwr_sum;
    double        tot_dev_est;
    double        amp_sum;
    double        fqerr_est;
    double        carrier_fq;
};


class DevMeasure : public sigc::trackable
{
  public:
    DevMeasure(unsigned samp_rate, const vector<float> &mod_fqs,
               double carrier_fq=0.0)
      : iold(0.0f), qold(0.0f), dev_print(samp_rate, mod_fqs),
        samp_rate(samp_rate)
    {
      dev_print.setCarrierFq(carrier_fq);
    }

    void processPreDemod(const vector<RtlSdr::Sample> &preDemod)
    {
      vector<float> audio;
      audio.reserve(preDemod.size());

      for (vector<RtlSdr::Sample>::const_iterator it = preDemod.begin();
           it != preDemod.end();
           ++it)
      {
        float I = it->real();
        float Q = it->imag();
        double demod = atan2(Q*iold - I*qold, I*iold + Q*qold);
        iold = I;
        qold = Q;
        audio.push_back(samp_rate * demod / (2.0 * M_PI));
      }
      dev_print.writeSamples(&audio[0], audio.size());
    }

  private:
    float         iold;
    float         qold;
    DevPrinter    dev_print;
    unsigned      samp_rate;
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

static const char *mod_fqs_str = DEFAULT_MOD_FQS;
static float headroom_db = DEFAULT_HEADROOM_DB;
static float caldev = DEFAULT_CALDEV;
static float maxdev = DEFAULT_MAXDEV;
static int cal_rx = false;
static int cal_tx = false;
static int measure = false;
static bool wb_mode = false;
static int flat_fq_response = false;
static string cfgfile;
static string cfgsect;
static FdWatch *stdin_watch = 0;
static SineGenerator *gen = 0;
static DevPrinter *dp = 0;
static Tx *tx = 0;
static Rx *rx = 0;
static float level_adjust_offset = 0.0f;
static vector<float> mod_fqs;
static const char *audio_dev = "alsa:default";
static const unsigned audio_ch = 0;



/****************************************************************************
 *
 * MAIN
 *
 ****************************************************************************/

int main(int argc, const char *argv[])
{
  cout << PROGRAM_NAME " v" DEVCAL_VERSION
          " Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX\n\n";
  cout << PROGRAM_NAME " comes with ABSOLUTELY NO WARRANTY. "
          "This is free software, and you\n";
  cout << "are welcome to redistribute it in accordance with the "
          "terms and conditions in\n";
  cout << "the GNU GPL (General Public License) version 2 or later.\n\n";

  setlocale(LC_ALL, "");
  CppApplication app;
  app.catchUnixSignal(SIGINT);
  app.catchUnixSignal(SIGTERM);
  app.unixSignalCaught.connect(sigc::ptr_fun(&sigterm_handler));

  parse_arguments(argc, const_cast<const char **>(argv));

  vector<float> mod_idxs(mod_fqs.size());
  transform(mod_fqs.begin(), mod_fqs.end(), mod_idxs.begin(),
      std::bind(divides<float>(), caldev, _1));
  float mod_level = 100.0 * caldev / (maxdev * pow(10.0, headroom_db / 20.0));
  cout << "--- Modulation frequencies [Hz] : ";
  copy(mod_fqs.begin(), mod_fqs.end(), ostream_iterator<float>(cout, " "));
  cout << endl;
  cout << "--- Calibration deviation [Hz]  : " << caldev << endl;
  cout << "--- Maximum deviation [Hz]      : " << maxdev << endl;
  cout << "--- Modulation indexes          : ";
  copy(mod_idxs.begin(), mod_idxs.end(), ostream_iterator<float>(cout, " "));
  cout << endl;
  cout << "--- Headroom [dB]               : " << headroom_db << endl;
  cout << "--- Peak sample level [dBF]     : "
       << (20.0*log10(mod_level / 100.0)) << " (" << mod_level << "%)\n";
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

  AudioIO *audio_io = 0;
  if (cal_tx)
  {
    cfg.getValue(cfgsect, "MASTER_GAIN", level_adjust_offset);
    cout << "--- Initial MASTER_GAIN=" << level_adjust_offset << endl;

    cout << "--- Use +, - and 0 to adjust MASTER_GAIN\n";
    cout << "--- Use T to toggle the transmitter on and off\n";

    gen = new SineGenerator(mod_fqs);
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
    //gen->enable(true);
  }
  else if (cal_rx)
  {
    cfg.getValue(cfgsect, "PREAMP", level_adjust_offset);
    cout << "--- Initial PREAMP=" << level_adjust_offset << endl;

    cout << "--- Setting SQL_DET=OPEN\n";
    cfg.setValue(cfgsect, "SQL_DET", "OPEN");
    cout << "--- Setting DTMF_MUTING=0\n";
    cfg.setValue(cfgsect, "DTMF_MUTING", "0");

    cout << "--- Use +, - and 0 to adjust PREAMP\n";

    rx = RxFactory::createNamedRx(cfg, cfgsect);
    if ((rx == 0) || !rx->initialize())
    {
      cerr << "*** ERROR: Could not initialize receiver object\n";
      exit(1);
    }
    rx->setVerbose(false);
    AudioSource *prev_src = rx;

    AudioSplitter *splitter = new AudioSplitter;
    prev_src->registerSink(splitter);
    prev_src = splitter;

    if (!flat_fq_response)
    {
      PreemphasisFilter *preemph = new PreemphasisFilter;
      prev_src->registerSink(preemph, true);
      prev_src = preemph;
    }
    
    dp = new DevPrinter(INTERNAL_SAMPLE_RATE, mod_fqs, maxdev, headroom_db);
    prev_src->registerSink(dp, true);
    prev_src = 0;

    if (audio_dev[0] != '\0')
    {
      audio_io = new AudioIO(audio_dev, audio_ch);
      if (!audio_io->open(AudioIO::MODE_WR))
      {
        cerr << "*** WARNING: Could not open audio output device \""
             << audio_dev << "\"\n";
      }
      else
      {
        splitter->addSink(audio_io, true);
      }
    }

    rx->setMuteState(Rx::MUTE_NONE);
  }
  else if (measure)
  {
    cout << "--- Setting SQL_DET=OPEN\n";
    cfg.setValue(cfgsect, "SQL_DET", "OPEN");
    cout << "--- Setting DTMF_MUTING=0\n";
    cfg.setValue(cfgsect, "DTMF_MUTING", "0");
    string wbrx_sect;
    if (cfg.getValue(cfgsect, "WBRX", wbrx_sect))
    {
      cout << "--- Setting " << wbrx_sect << "/SAMPLE_RATE to default value\n";
      cfg.setValue(wbrx_sect, "SAMPLE_RATE", "");
    }

    rx = RxFactory::createNamedRx(cfg, cfgsect);
    if ((rx == 0) || !rx->initialize())
    {
      cerr << "*** ERROR: Could not initialize receiver object\n";
      exit(1);
    }
    rx->setVerbose(false);
    AudioSource *prev_src = rx;

    Ddr *ddr = dynamic_cast<Ddr*>(rx);
    if (ddr == 0)
    {
      cerr << "*** ERROR: An rtl-sdr receiver is needed to measure deviation\n";
      exit(1);
    }
    if (wb_mode)
    {
      ddr->setModulation(Modulation::MOD_WBFM);
    }
    DevMeasure *dev_measure = new DevMeasure(ddr->preDemodSampleRate(), 
                                             mod_fqs, ddr->nbFq());
    ddr->preDemod.connect(mem_fun(dev_measure, &DevMeasure::processPreDemod));

    if (audio_dev[0] != '\0')
    {
      audio_io = new AudioIO(audio_dev, audio_ch);
      if (!audio_io->open(AudioIO::MODE_WR))
      {
        cerr << "*** WARNING: Could not open audio output device \""
          << audio_dev << "\"\n";
      }
      else
      {
        prev_src->registerSink(audio_io);
        rx->setMuteState(Rx::MUTE_NONE);
      }
    }
  }

  cout << "--- Use Q or Ctrl+C to quit\n\n";

  struct termios org_termios;
  struct termios termios;
  tcgetattr(STDIN_FILENO, &org_termios);
  termios = org_termios;
  termios.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &termios);
  stdin_watch = new FdWatch(STDIN_FILENO, FdWatch::FD_WATCH_RD);
  stdin_watch->activity.connect(sigc::ptr_fun(&stdin_handler));

  app.exec();

  if (audio_io != 0)
  {
    audio_io->close();
    delete audio_io;
  }
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
    {"modfqs", 'f', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,
            &mod_fqs_str, 0,
	    "The frequencies of the sine waves to modulate with",
            "<frequences in hz>"},
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
    {"rxcal", 'r', POPT_ARG_NONE, &cal_rx, 0, "Do receiver calibration", NULL},
    {"txcal", 't', POPT_ARG_NONE, &cal_tx, 0,
            "Do transmitter calibration", NULL},
    {"flat", 'F', POPT_ARG_NONE, &flat_fq_response, 0,
            "Flat TX/RX frequency response (no emphasis)", NULL},
    {"measure", 'M', POPT_ARG_NONE, &measure, 0, "Measure deviation", NULL},
    {"wide", 'w', POPT_ARG_NONE, &wb_mode, 0, "Wideband mode", NULL},
    {"audiodev", 'a', POPT_ARG_STRING | POPT_ARGFLAG_SHOW_DEFAULT,
            &audio_dev, 0,
	    "The audio device to use for audio output",
            "<type:dev>"},
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

  if ((static_cast<int>(cal_rx) + cal_tx + measure) != 1)
  {
    cerr << "*** ERROR: There must be one and only one of the -r, -t and -M "
            "command line switches\n";
    poptPrintUsage(optCon, stderr, 0);
    exit(1);
  }

  if (wb_mode)
  {
    if (!measure)
    {
      cerr << "*** ERROR: Wideband mode is only valid in measure mode\n";
      exit(1);
    }
  }

  SvxLink::splitStr(mod_fqs, mod_fqs_str, ",");
  /*
  for (vector<float>::iterator it = mod_fqs.begin(); it != mod_fqs.end(); ++it)
  {
    cout << *it << " ";
  }
  cout << endl;
  */
  if (mod_fqs.empty())
  {
    cerr << "*** ERROR: Modulation frequency is unset\n";
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
        cout << "\r\033[K" "MASTER_GAIN="
             << (gen->levelAdjust() + level_adjust_offset)
             << endl;
      }
      else if (cal_rx)
      {
        double gain = dp->levelAdjust() + 0.01;
        dp->adjustLevel(gain);
        cout << "\r\033[K" "PREAMP="
             << (dp->levelAdjust() + level_adjust_offset)
             << endl;
      }
      break;
    }
    
    case '-':
    {
      if (cal_tx)
      {
        double gain = gen->levelAdjust() - 0.01;
        gen->adjustLevel(gain);
        cout << "\r\033[K" "MASTER_GAIN="
             << (gen->levelAdjust() + level_adjust_offset)
             << endl;
      }
      else if (cal_rx)
      {
        double gain = dp->levelAdjust() - 0.01;
        dp->adjustLevel(gain);
        cout << "\r\033[K" "PREAMP="
             << (dp->levelAdjust() + level_adjust_offset)
             << endl;
      }
      break;
    }

    case '0':
    {
      if (cal_tx)
      {
        gen->adjustLevel(-level_adjust_offset);
        cout << "MASTER_GAIN="
             << (gen->levelAdjust() + level_adjust_offset)
             << endl;
      }
      else if (cal_rx)
      {
        dp->adjustLevel(-level_adjust_offset);
        cout << "\r\033[K" "PREAMP="
             << (dp->levelAdjust() + level_adjust_offset)
             << endl;
      }
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
