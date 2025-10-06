/**
@file	 remotetrx.cpp
@brief   Main file for the RemoteTrx remote transceiver for SvxLink
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-14

This is the main file for the RemoteTrx remote transceiver for the
SvxLink server. It is used to link in remote transceivers to the SvxLink
server core (e.g. via a TCP/IP network).

\verbatim
RemoteTrx - A remote receiver for the SvxLink server
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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
#include <termios.h>
#include <dirent.h>
#include <popt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>
#include <errno.h>

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <sstream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <AsyncFdWatch.h>
#include <AsyncAudioIO.h>
#include <Rx.h>
#include <Tx.h>
#include <common.h>
#include <config.h>
#include <LogWriter.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/REMOTE_TRX.h"
#include "TrxHandler.h"
#include "NetTrxAdapter.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace sigc;
using namespace SvxLink;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define PROGRAM_NAME "RemoteTrx"


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class NetRxAdapterFactory : public RxFactory
{
  public:
    NetRxAdapterFactory(void) : RxFactory("NetTrxAdapter") {}
    
  protected:
    Rx *createRx(Config &cfg, const string& name)
    {
      NetTrxAdapter *adapter = NetTrxAdapter::instance(cfg, name);
      if (adapter == 0)
      {
      	return 0;
      }
      return adapter->rx();
    }
}; /* class NetRxAdapterFactory */


class NetTxAdapterFactory : public TxFactory
{
  public:
    NetTxAdapterFactory(void) : TxFactory("NetTrxAdapter") {}
    
  protected:
    Tx *createTx(Config &cfg, const string& name)
    {
      NetTrxAdapter *adapter = NetTrxAdapter::instance(cfg, name);
      if (adapter == 0)
      {
      	return 0;
      }
      return adapter->tx();
    }
}; /* class NetTxAdapterFactory */



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

static void parse_arguments(int argc, const char **argv);
static void stdinHandler(FdWatch *w);
static void sighup_handler(int signal);
static void sigterm_handler(int signal);
static void handle_unix_signal(int signum);


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/




/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

namespace {
  char*                 pidfile_name = nullptr;
  char*                 logfile_name = nullptr;
  char*                 runasuser = nullptr;
  char*                 config = nullptr;
  int                   daemonize = 0;
  int                   reset = 0;
  int                   quiet = 0;
  FdWatch*              stdin_watch = 0;
  LogWriter             logwriter;
};


/****************************************************************************
 *
 * MAIN
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Function:  main
 * Purpose:   Start everything...
 * Input:     argc  - The number of arguments passed to this program
 *    	      	      (including the program name).
 *    	      argv  - The arguments passed to this program. argv[0] is the
 *    	      	      program name.
 * Output:    Return 0 on success, else non-zero.
 * Author:    Tobias Blomberg, SM0SVX
 * Created:   2006-04-14
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
int main(int argc, char **argv)
{
  setlocale(LC_ALL, "");

  CppApplication app;
  app.catchUnixSignal(SIGHUP);
  app.catchUnixSignal(SIGINT);
  app.catchUnixSignal(SIGTERM);
  app.unixSignalCaught.connect(sigc::ptr_fun(&handle_unix_signal));

  parse_arguments(argc, const_cast<const char **>(argv));

  struct sigaction act;
  act.sa_handler = SIG_IGN;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(SIGPIPE, &act, NULL) == -1)
  {
    perror("sigaction");
    exit(1);
  }

  if (daemonize && (daemon(1, 0) == -1))
  {
    perror("daemon");
    exit(1);
  }

  if (quiet || (logfile_name != 0))
  {
    int devnull = open("/dev/null", O_RDWR);
    if (devnull == -1)
    {
      perror("open(/dev/null)");
      exit(1);
    }

    if (quiet)
    {
        /* Redirect stdout to /dev/null */
      dup2(devnull, STDOUT_FILENO);
    }

    if (logfile_name != 0)
    {
      logwriter.setDestinationName(logfile_name);
      if (!quiet)
      {
        logwriter.redirectStdout();
      }
      logwriter.redirectStderr();
      logwriter.start();

        /* Redirect stdin to /dev/null */
      if (dup2(devnull, STDIN_FILENO) == -1)
      {
        perror("dup2(stdin)");
        exit(1);
      }
    }
    close(devnull);
  }

  if (pidfile_name != NULL)
  {
    FILE *pidfile = fopen(pidfile_name, "w");
    if (pidfile == 0)
    {
      char err[256];
      sprintf(err, "fopen(\"%s\")", pidfile_name);
      perror(err);
      fflush(stderr);
      exit(1);
    }
    fprintf(pidfile, "%d\n", getpid());
    fclose(pidfile);
  }

  const char *home_dir = 0;
  if (runasuser != NULL)
  {
      // Setup supplementary group IDs
    if (initgroups(runasuser, getgid()))
    {
      perror("initgroups");
      exit(1);
    }

    struct passwd *passwd = getpwnam(runasuser);
    if (passwd == NULL)
    {
      perror("getpwnam");
      exit(1);
    }
    if (setgid(passwd->pw_gid) == -1)
    {
      perror("setgid");
      exit(1);
    }
    if (setuid(passwd->pw_uid) == -1)
    {
      perror("setuid");
      exit(1);
    }
    home_dir = passwd->pw_dir;
  }

  if (home_dir == 0)
  {
    home_dir = getenv("HOME");
  }
  if (home_dir == 0)
  {
    home_dir = ".";
  }
  
  Config cfg;
  string cfg_filename;
  if (config != NULL)
  {
    cfg_filename = string(config);
    if (!cfg.open(cfg_filename))
    {
      cerr << "*** ERROR: Could not open configuration file: "
      	   << config << endl;
      exit(1);
    }
  }
  else
  {
    cfg_filename = string(home_dir);
    cfg_filename += "/.svxlink/remotetrx.conf";
    if (!cfg.open(cfg_filename))
    {
      cfg_filename = SVX_SYSCONF_INSTALL_DIR "/remotetrx.conf";
      if (!cfg.open(cfg_filename))
      {
	cfg_filename = SYSCONF_INSTALL_DIR "/remotetrx.conf";
	if (!cfg.open(cfg_filename))
	{
	  cerr << "*** ERROR: Could not open configuration file";
          if (errno != 0)
          {
            cerr << " (" << strerror(errno) << ")";
          }
          cerr << ".\n";
	  cerr << "Tried the following paths:\n"
      	       << "\t" << home_dir << "/.svxlink/remotetrx.conf\n"
      	       << "\t" SVX_SYSCONF_INSTALL_DIR "/remotetrx.conf\n"
	       << "\t" SYSCONF_INSTALL_DIR "/remotetrx.conf\n"
	       << "Possible reasons for failure are: None of the files exist,\n"
	       << "you do not have permission to read the file or there was a\n"
	       << "syntax error in the file.\n";
	  exit(1);
	}
      }
    }
  }
  string main_cfg_filename(cfg_filename);
  
  string cfg_dir;
  if (cfg.getValue("GLOBAL", "CFG_DIR", cfg_dir))
  {
    if (cfg_dir[0] != '/')
    {
      int slash_pos = main_cfg_filename.rfind('/');
      if (slash_pos != -1)
      {
      	cfg_dir = main_cfg_filename.substr(0, slash_pos+1) + cfg_dir;
      }
      else
      {
      	cfg_dir = string("./") + cfg_dir;
      }
    }
    
    DIR *dir = opendir(cfg_dir.c_str());
    if (dir == NULL)
    {
      cerr << "*** ERROR: Could not read from directory spcified by "
      	   << "configuration variable GLOBAL/CFG_DIR=" << cfg_dir << endl;
      exit(1);
    }
    
    struct dirent *dirent;
    while ((dirent = readdir(dir)) != NULL)
    {
      char *dot = strrchr(dirent->d_name, '.');
      if ((dot == NULL) || (dirent->d_name[0] == '.') ||
          (strcmp(dot, ".conf") != 0))
      {
      	continue;
      }
      cfg_filename = cfg_dir + "/" + dirent->d_name;
      if (!cfg.open(cfg_filename))
       {
	 cerr << "*** ERROR: Could not open configuration file: "
	      << cfg_filename << endl;
	 exit(1);
       }
    }
    
    if (closedir(dir) == -1)
    {
      cerr << "*** ERROR: Error closing directory specified by"
      	   << "configuration variable GLOBAL/CFG_DIR=" << cfg_dir << endl;
      exit(1);
    }
  }

  std::string tstamp_format = "%c";
  cfg.getValue("GLOBAL", "TIMESTAMP_FORMAT", tstamp_format);
  logwriter.setTimestampFormat(tstamp_format);

  cout << PROGRAM_NAME " v" REMOTE_TRX_VERSION
          " Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX\n\n";
  cout << PROGRAM_NAME " comes with ABSOLUTELY NO WARRANTY. "
          "This is free software, and you are\n";
  cout << "welcome to redistribute it in accordance with the "
          "terms and conditions in the\n";
  cout << "GNU GPL (General Public License) version 2 or later.\n";

  cout << "\nUsing configuration file: " << main_cfg_filename << endl;
  
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
  
  size_t card_channels = 2;
  cfg.getValue("GLOBAL", "CARD_CHANNELS", card_channels);
  AudioIO::setChannels(card_channels);

  struct termios org_termios = {0};
  if (logfile_name == 0)
  {
    struct termios termios;
    tcgetattr(STDIN_FILENO, &org_termios);
    termios = org_termios;
    termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &termios);

    stdin_watch = new FdWatch(STDIN_FILENO, FdWatch::FD_WATCH_RD);
    stdin_watch->activity.connect(sigc::ptr_fun(&stdinHandler));
  }
  
  NetRxAdapterFactory net_rx_adapter_factory;
  NetTxAdapterFactory net_tx_adapter_factory;

  vector<TrxHandler*> trx_handlers;
  vector<string> trxs;
  value = "";
  cfg.getValue("GLOBAL", "TRXS", value);
  splitStr(trxs, value, ",");
  for (unsigned i=0; i<trxs.size(); ++i)
  {
    cout << "Setting up trx \"" << trxs[i] << "\"\n";
    TrxHandler *trx_handler = new TrxHandler(cfg, trxs[i]);
    if (!trx_handler->initialize())
    {
      cerr << "*** ERROR: Failed to setup trx " << trxs[i] << endl;
      delete trx_handler;
      continue;
    }
    trx_handlers.push_back(trx_handler);
    cout << endl;
  }

  if (!trx_handlers.empty())
  {
    if (reset)
    {
      std::cout << "Initialization done. Exiting." << std::endl;
      Async::Application::app().quit();
    }
    std::cout << "NOTICE: Initialization done. Starting main application."
              << std::endl;
    app.exec();
  }
  else
  {
    cerr << "*** ERROR: No trxs successfully initialized. Bailing out...\n";
  }

  for (vector<TrxHandler*>::iterator it = trx_handlers.begin();
       it != trx_handlers.end();
       ++it)
  {
    delete *it;
  }
  trx_handlers.clear();

  if (stdin_watch != 0)
  {
    delete stdin_watch;
    tcsetattr(STDIN_FILENO, TCSANOW, &org_termios);
  }

  return 0;
} /* main */



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
  int print_version = 0;

  poptContext optCon;
  const struct poptOption optionsTable[] =
  {
    POPT_AUTOHELP
    {"pidfile", 0, POPT_ARG_STRING, &pidfile_name, 0,
            "Specify the name of the pidfile to use", "<filename>"},
    {"logfile", 0, POPT_ARG_STRING, &logfile_name, 0,
            "Specify the logfile to use (stdout and stderr)", "<filename>"},
    {"runasuser", 0, POPT_ARG_STRING, &runasuser, 0,
            "Specify the user to run SvxLink as", "<username>"},
    {"config", 0, POPT_ARG_STRING, &config, 0,
	    "Specify the configuration file to use", "<filename>"},
    /*
    {"int_arg", 'i', POPT_ARG_INT, &int_arg, 0,
	    "Description of int argument", "<an int>"},
    */
    {"daemon", 0, POPT_ARG_NONE, &daemonize, 0,
	    "Start " PROGRAM_NAME " as a daemon", NULL},
    {"reset", 0, POPT_ARG_NONE, &reset, 0,
	    "Initialize all hardware to initial state then quit", NULL},
    {"quiet", 0, POPT_ARG_NONE, &quiet, 0,
	    "Don't print any info messages, just warnings and errors", NULL},
    {"version", 0, POPT_ARG_NONE, &print_version, 0,
	    "Print the application version string", NULL},
    {NULL, 0, 0, NULL, 0}
  };
  int err;
  //const char *arg = NULL;
  //int argcnt = 0;
  
  optCon = poptGetContext(PROGRAM_NAME, argc, argv, optionsTable, 0);
  poptReadDefaultConfig(optCon, 0);
  
  err = poptGetNextOpt(optCon);
  if (err != -1)
  {
    fprintf(stderr, "\t%s: %s\n",
	    poptBadOption(optCon, POPT_BADOPTION_NOALIAS),
	    poptStrerror(err));
    exit(1);
  }

  /*
  printf("string_arg  = %s\n", string_arg);
  printf("int_arg     = %d\n", int_arg);
  printf("bool_arg    = %d\n", bool_arg);
  */
  
    /* Parse arguments that do not begin with '-' (leftovers) */
  /*
  arg = poptGetArg(optCon);
  while (arg != NULL)
  {
    printf("arg %2d      = %s\n", ++argcnt, arg);
    arg = poptGetArg(optCon);
  }
  */

  poptFreeContext(optCon);

  if (print_version)
  {
    std::cout << REMOTE_TRX_VERSION << std::endl;
    exit(0);
  }
} /* parse_arguments */


static void stdinHandler(FdWatch *w)
{
  char buf[1];
  int cnt = ::read(STDIN_FILENO, buf, 1);
  if (cnt == -1)
  {
    fprintf(stderr, "*** ERROR: Reading from stdin failed\n");
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
      Application::app().quit();
      break;
    
    case '\n':
      putchar('\n');
      break;
    /*
    case '0': case '1': case '2': case '3':
    case '4': case '5': case '6': case '7':
    case '8': case '9': case 'A': case 'B':
    case 'C': case 'D': case '*': case '#':
      logic->dtmfDigitDetected(buf[0]);
      break;
    */    
    default:
      break;
  }
}


static void sighup_handler(int signal)
{
  if (logfile_name == 0)
  {
    cout << "Ignoring SIGHUP\n";
    return;
  }
  std::cout << "SIGHUP received" << std::endl;
  logwriter.reopenLogfile();
} /* sighup_handler */


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
      signame = "Unknown signal";
      break;
  }

  std::cout << "\nNOTICE: " << signame
            << " received. Shutting down application..."
            << std::endl;
  Application::app().quit();
} /* sigterm_handler */


static void handle_unix_signal(int signum)
{
  switch (signum)
  {
    case SIGHUP:
      sighup_handler(signum);
      break;
    case SIGINT:
    case SIGTERM:
      sigterm_handler(signum);
      break;
  }
} /* handle_unix_signal */



/*
 * This file has not been truncated
 */
