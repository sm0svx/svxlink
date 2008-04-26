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
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

#include <iostream>


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

#include <version/REMOTE_TRX.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "TrxHandler.h"
#include "NetTrxAdapter.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace SigC;



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

class NetTrxAdapterFactory : public RxFactory
{
  public:
    NetTrxAdapterFactory(void) : RxFactory("NetTrxAdapter") {}
    
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
}; /* class TrxAdapterFactory */



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

static void parse_arguments(int argc, const char **argv);
static void stdinHandler(FdWatch *w);
static void write_to_logfile(const char *buf);
static void stdout_handler(FdWatch *w);
static void sighup_handler(int signal);
static void sigterm_handler(int signal);
static bool open_logfile(void);


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

static char   	      	*logfile_name = NULL;
static char   	      	*config = NULL;
static int    	      	daemonize = 0;
static int    	      	logfd = -1;
static FdWatch	      	*stdin_watch = 0;
static FdWatch	      	*stdout_watch = 0;
static string         	tstamp_format;
static struct sigaction sighup_oldact, sigint_oldact, sigterm_oldact;
static volatile bool  	reopen_log = false;



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

  parse_arguments(argc, const_cast<const char **>(argv));

  struct sigaction act;
  act.sa_handler = sighup_handler;
  sigemptyset(&act.sa_mask);
  act.sa_flags = 0;
  if (sigaction(SIGHUP, &act, &sighup_oldact) == -1)
  {
    perror("sigaction");
    exit(1);
  }

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
  
  int pipefd[2] = {-1, -1};
  int noclose = 0;
  if (logfile_name != 0)
  {
      /* Open the logfile */
    if (!open_logfile())
    {
      exit(1);
    }

      /* Create a pipe to route stdout through */
    if (pipe(pipefd) == -1)
    {
      perror("pipe");
      exit(1);
    }
    stdout_watch = new FdWatch(pipefd[0], FdWatch::FD_WATCH_RD);
    stdout_watch->activity.connect(slot(&stdout_handler));

      /* Redirect stdout to the logpipe */
    if (close(STDOUT_FILENO) == -1)
    {
      perror("close(stdout)");
      exit(1);
    }
    if (dup2(pipefd[1], STDOUT_FILENO) == -1)
    {
      perror("dup2(stdout)");
      exit(1);
    }

      /* Redirect stderr to the logpipe */
    if (close(STDERR_FILENO) == -1)
    {
      perror("close(stderr)");
      exit(1);
    }
    if (dup2(pipefd[1], STDERR_FILENO) == -1)
    {
      perror("dup2(stderr)");
      exit(1);
    }

      /* Close stdin */
    close(STDIN_FILENO);
    
      /* Tell the daemon function call not to close the file descriptors */
    noclose = 1;
  }

  if (daemonize)
  {
    if (daemon(0, noclose) == -1)
    {
      perror("daemon");
      exit(1);
    }
  }

  const char *home_dir = getenv("HOME");
  if (home_dir == NULL)
  {
    home_dir = ".";
  }
  
  tstamp_format = "%c";

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
      cfg_filename = string(home_dir);
      cfg_filename += "/.remotetrxrc";
      if (!cfg.open(cfg_filename))
      {
	cfg_filename = "/etc/remotetrx.conf";
	if (!cfg.open(cfg_filename))
	{
	  cerr << "*** ERROR: Could not open configuration file. Tried:\n"
      	       << "\t" << home_dir << "/.svxlink/remotetrx.conf\n"
      	       << "\t" << home_dir << "/.remotetrxrc\n"
	       << "\t/etc/remotetrx.conf\n"
	       << "Possible reasons for failure are: None of the files exist,\n"
	       << "you do not have permission to read the file or there was a\n"
	       << "syntax error in the file\n";
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
      if (dirent->d_name[0] == '.') continue;
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
  
  cfg.getValue("GLOBAL", "TIMESTAMP_FORMAT", tstamp_format);
  
  cout << PROGRAM_NAME " v" REMOTE_TRX_VERSION " (" __DATE__
      	  ") starting up...\n";
  cout << "\nUsing configuration file: " << main_cfg_filename << endl;
  
  string value;
  if (cfg.getValue("GLOBAL", "CARD_SAMPLE_RATE", value))
  {
    int rate = atoi(value.c_str());
    if (rate == 8000)
    {
      AudioIO::setSampleRate(rate);
      AudioIO::setBlocksize(256);
      AudioIO::setBufferCount(2);
    }
    else if (rate == 16000)
    {
      AudioIO::setSampleRate(rate);
      AudioIO::setBlocksize(512);
      AudioIO::setBufferCount(2);
    }
    else if (rate == 48000)
    {
      AudioIO::setSampleRate(rate);
      AudioIO::setBlocksize(1024);
      AudioIO::setBufferCount(4);
    }
    else
    {
      cerr << "*** ERROR: Illegal sound card sample rate specified for "
      	      "config variable GLOBAL/CARD_SAMPLE_RATE. Valid rates are "
	      "8000, 16000 and 48000\n";
      exit(1);
    }
  }
  
  struct termios org_termios;
  if (logfile_name == 0)
  {
    struct termios termios;
    tcgetattr(STDIN_FILENO, &org_termios);
    termios = org_termios;
    termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &termios);

    stdin_watch = new FdWatch(STDIN_FILENO, FdWatch::FD_WATCH_RD);
    stdin_watch->activity.connect(slot(&stdinHandler));
  }
  
  NetTrxAdapterFactory net_trx_adapter_factory;

  TrxHandler trx_handler(cfg);
  if (trx_handler.initialize())
  {
    app.exec();
  }
  
  if (stdin_watch != 0)
  {
    delete stdin_watch;
    tcsetattr(STDIN_FILENO, TCSANOW, &org_termios);
  }

  if (stdout_watch != 0)
  {
    delete stdout_watch;
    close(pipefd[0]);
    close(pipefd[1]);
  }

  if (logfd != -1)
  {
    close(logfd);
  }
  
  if (sigaction(SIGHUP, &sighup_oldact, NULL) == -1)
  {
    perror("sigaction");
  }
  
  if (sigaction(SIGHUP, &sigterm_oldact, NULL) == -1)
  {
    perror("sigaction");
  }
  
  if (sigaction(SIGHUP, &sigint_oldact, NULL) == -1)
  {
    perror("sigaction");
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
  poptContext optCon;
  const struct poptOption optionsTable[] =
  {
    POPT_AUTOHELP
    {"logfile", 0, POPT_ARG_STRING, &logfile_name, 0,
	    "Specify the logfile to use (stdout and stderr)", "<filename>"},
    {"config", 0, POPT_ARG_STRING, &config, 0,
	    "Specify the configuration file to use", "<filename>"},
    /*
    {"int_arg", 'i', POPT_ARG_INT, &int_arg, 0,
	    "Description of int argument", "<an int>"},
    */
    {"daemon", 0, POPT_ARG_NONE, &daemonize, 0,
	    "Start " PROGRAM_NAME " as a daemon", NULL},
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

} /* parse_arguments */


static void stdinHandler(FdWatch *w)
{
  char buf[1];
  int cnt = ::read(STDIN_FILENO, buf, 1);
  if (cnt == -1)
  {
    fprintf(stderr, "*** error reading from stdin\n");
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


static void write_to_logfile(const char *buf)
{
  if (logfd == -1)
  {
    cout << buf;
    return;
  }
  
  const char *ptr = buf;
  while (*ptr != 0)
  {
    static bool print_timestamp = true;
    
    if (print_timestamp)
    {
      if (!tstamp_format.empty())
      {
	time_t epoch = time(NULL);
	struct tm *tm = localtime(&epoch);
	char tstr[256];
	size_t tlen = strftime(tstr, sizeof(tstr), tstamp_format.c_str(), tm);
	write(logfd, tstr, tlen);
	write(logfd, ": ", 2);
	print_timestamp = false;
      }

      if (reopen_log)
      {
	const char *reopen_txt = "SIGHUP received. Reopening logfile\n";
	write(logfd, reopen_txt, strlen(reopen_txt));
	open_logfile();
	reopen_log = false;
	print_timestamp = true;
	continue;
      }
    }

    int write_len = 0;
    char *nl = strchr(ptr, '\n');
    if (nl != 0)
    {
      write_len = nl-ptr+1;
      print_timestamp = true;
    }
    else
    {
      write_len = strlen(ptr);
    }
    write(logfd, ptr, write_len);
    ptr += write_len;
  }
} /* write_to_logfile */


static void stdout_handler(FdWatch *w)
{
  char buf[256];
  ssize_t len = read(w->fd(), buf, sizeof(buf)-1);
  if (len == -1)
  {
    return;
  }
  buf[len] = 0;
  
  write_to_logfile(buf);
  
} /* stdout_handler  */


void sighup_handler(int signal)
{
  if (logfile_name == 0)
  {
    write(STDOUT_FILENO, "Ignoring SIGHUP\n", 16);
    return;
  }
  
  write(STDOUT_FILENO, "SIGHUP received. Logfile reopened\n", 34);
  reopen_log = true;
    
} /* sighup_handler */


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
  write_to_logfile(msg.c_str());
  Application::app().quit();
} /* sigterm_handler */


bool open_logfile(void)
{
  if (logfd != -1)
  {
    close(logfd);
  }
  
  logfd = open(logfile_name, O_WRONLY | O_APPEND | O_CREAT, 00644);
  if (logfd == -1)
  {
    char str[256] = "open(\"";
    strcat(str, logfile_name);
    strcat(str, "\")");
    perror(str);
    return false;
  }

  return true;
  
} /* open_logfile */




/*
 * This file has not been truncated
 */

