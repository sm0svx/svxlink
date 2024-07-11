/**
@file	 svxreflector.cpp
@brief   Main source file for the SvxReflector application
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-11

The SvxReflector application is a central hub used to connect multiple SvxLink
nodes together.

\verbatim
SvxReflector - An audio reflector for connecting SvxLink Servers
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

#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <grp.h>
#include <pwd.h>
#include <dirent.h>
#include <termios.h>
#include <errno.h>

#include <popt.h>
#include <sigc++/sigc++.h>

#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncFdWatch.h>
#include <AsyncConfig.h>
#include <config.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXREFLECTOR.h"
#include "Reflector.h"


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

#define PROGRAM_NAME "SvxReflector"


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

static void parse_arguments(int argc, const char **argv);
static void stdinHandler(FdWatch *w);
static void stdout_handler(FdWatch *w);
static void sighup_handler(int signal);
static void sigterm_handler(int signal);
static void handle_unix_signal(int signum);
static bool logfile_open(void);
static void logfile_reopen(const char *reason);
static bool logfile_write_timestamp(void);
static void logfile_write(const char *buf);
static void logfile_flush(void);


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

static char             *pidfile_name = NULL;
static char             *logfile_name = NULL;
static char             *runasuser = NULL;
static char   	      	*config = NULL;
static int    	      	daemonize = 0;
static int    	      	logfd = -1;
static FdWatch	      	*stdin_watch = 0;
static FdWatch	      	*stdout_watch = 0;
static string         	tstamp_format;


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
 * Created:   2017-02-11
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
int main(int argc, const char *argv[])
{
  setlocale(LC_ALL, "");

  CppApplication app;
  app.catchUnixSignal(SIGHUP);
  app.catchUnixSignal(SIGINT);
  app.catchUnixSignal(SIGTERM);
  app.unixSignalCaught.connect(sigc::ptr_fun(&handle_unix_signal));

  parse_arguments(argc, const_cast<const char **>(argv));

  int pipefd[2] = {-1, -1};
  int noclose = 0;
  if (logfile_name != 0)
  {
      /* Open the logfile */
    if (!logfile_open())
    {
      exit(1);
    }

      /* Create a pipe to route stdout through */
    if (pipe(pipefd) == -1)
    {
      perror("pipe");
      exit(1);
    }
    int flags = fcntl(pipefd[0], F_GETFL);
    if (flags == -1)
    {
      perror("fcntl(..., F_GETFL)");
      exit(1);
    }
    flags |= O_NONBLOCK;
    if (fcntl(pipefd[0], F_SETFL, flags) == -1)
    {
      perror("fcntl(..., F_SETFL)");
      exit(1);
    }
    stdout_watch = new FdWatch(pipefd[0], FdWatch::FD_WATCH_RD);
    stdout_watch->activity.connect(sigc::ptr_fun(&stdout_handler));

      /* Redirect stdout to the logpipe */
    if (dup2(pipefd[1], STDOUT_FILENO) == -1)
    {
      perror("dup2(stdout)");
      exit(1);
    }

      /* Redirect stderr to the logpipe */
    if (dup2(pipefd[1], STDERR_FILENO) == -1)
    {
      perror("dup2(stderr)");
      exit(1);
    }

      // We also need to close stdin but that is not a good idea since we need
      // the stdin filedescriptor to keep being allocated so that it is not
      // assigned to some other random filedescriptor allocation. That would
      // be very bad.
    int devnull = open("/dev/null", O_RDONLY);
    if (devnull == -1)
    {
      perror("open(/dev/null)");
      exit(1);
    }
    if (dup2(devnull, STDIN_FILENO) == -1)
    {
      perror("dup2(stdin)");
      exit(1);
    }
    close(devnull);

      /* Force stdout to line buffered mode */
    if (setvbuf(stdout, NULL, _IOLBF, 0) != 0)
    {
      perror("setlinebuf");
      exit(1);
    }

    atexit(logfile_flush);

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
    cfg_filename += "/.svxlink/svxreflector.conf";
    if (!cfg.open(cfg_filename))
    {
      cfg_filename = SVX_SYSCONF_INSTALL_DIR "/svxreflector.conf";
      if (!cfg.open(cfg_filename))
      {
        cerr << "*** ERROR: Could not open configuration file";
        if (errno != 0)
        {
          cerr << " (" << strerror(errno) << ")";
        }
        cerr << ".\n";
        cerr << "Tried the following paths:\n"
             << "\t" << home_dir << "/.svxlink/svxreflector.conf\n"
             << "\t" SVX_SYSCONF_INSTALL_DIR "/svxreflector.conf\n"
             << "Possible reasons for failure are: None of the files exist,\n"
             << "you do not have permission to read the file or there was a\n"
             << "syntax error in the file.\n";
        exit(1);
      }
    }
  }
  string main_cfg_filename(cfg_filename);

  std::string main_cfg_dir = ".";
  auto slash_pos = main_cfg_filename.rfind('/');
  if (slash_pos != std::string::npos)
  {
    main_cfg_dir = main_cfg_filename.substr(0, slash_pos);
  }

  string cfg_dir;
  if (cfg.getValue("GLOBAL", "CFG_DIR", cfg_dir))
  {
    if (cfg_dir[0] != '/')
    {
      cfg_dir = main_cfg_dir + "/" + cfg_dir;
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
      if ((dot == NULL) || (strcmp(dot, ".conf") != 0))
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

  cfg.getValue("GLOBAL", "TIMESTAMP_FORMAT", tstamp_format);

  //std::string pki_dir;
  //if (!cfg.getValue("GLOBAL", "CERT_PKI_DIR", pki_dir))
  //{
  //  pki_dir = main_cfg_dir + "/pki";
  //  cfg.setValue("GLOBAL", "CERT_PKI_DIR", pki_dir);
  //}

  cout << PROGRAM_NAME " v" SVXREFLECTOR_VERSION
          " Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX\n\n";
  cout << PROGRAM_NAME " comes with ABSOLUTELY NO WARRANTY. "
          "This is free software, and you are\n";
  cout << "welcome to redistribute it in accordance with the "
          "terms and conditions in the\n";
  cout << "GNU GPL (General Public License) version 2 or later.\n";

  cout << "\nUsing configuration file: " << main_cfg_filename << endl;

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

  Reflector ref;
  if (ref.initialize(cfg))
  {
    app.exec();
  }
  else
  {
    cerr << ":-(" << endl;
  }

  logfile_flush();

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

    default:
      break;
  }
} /* stdinHandler */


static void stdout_handler(FdWatch *w)
{
  ssize_t len =  0;
  do
  {
    char buf[256];
    len = read(w->fd(), buf, sizeof(buf)-1);
    if (len > 0)
    {
      buf[len] = 0;
      logfile_write(buf);
    }
  } while (len > 0);
} /* stdout_handler  */


static void sighup_handler(int signal)
{
  if (logfile_name == 0)
  {
    cout << "Ignoring SIGHUP\n";
    return;
  }
  logfile_reopen("SIGHUP received");
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
      signame = "???";
      break;
  }
  string msg("\n");
  msg += signame;
  msg += " received. Shutting down application...\n";
  logfile_write(msg.c_str());
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


static bool logfile_open(void)
{
  if (logfd != -1)
  {
    close(logfd);
  }

  logfd = open(logfile_name, O_WRONLY | O_APPEND | O_CREAT, 00644);
  if (logfd == -1)
  {
    ostringstream ss;
    ss << "open(\"" << logfile_name << "\")";
    perror(ss.str().c_str());
    return false;
  }

  return true;

} /* logfile_open */


static void logfile_reopen(const char *reason)
{
  logfile_write_timestamp();
  string msg(reason);
  msg += ". Reopening logfile\n";
  if (write(logfd, msg.c_str(), msg.size()) == -1) {}

  logfile_open();

  logfile_write_timestamp();
  msg = reason;
  msg += ". Logfile reopened\n";
  if (write(logfd, msg.c_str(), msg.size()) == -1) {}
} /* logfile_reopen */


static bool logfile_write_timestamp(void)
{
  if (!tstamp_format.empty())
  {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    string fmt(tstamp_format);
    const string frac_code("%f");
    size_t pos = fmt.find(frac_code);
    if (pos != string::npos)
    {
      stringstream ss;
      ss << setfill('0') << setw(3) << (tv.tv_usec / 1000);
      fmt.replace(pos, frac_code.length(), ss.str());
    }
    struct tm tm;
    char tstr[256];
    size_t tlen = strftime(tstr, sizeof(tstr), fmt.c_str(),
                           localtime_r(&tv.tv_sec, &tm));
    ssize_t ret = write(logfd, tstr, tlen);
    if (ret != static_cast<ssize_t>(tlen))
    {
      return false;
    }
    ret = write(logfd, ": ", 2);
    if (ret != 2)
    {
      return false;
    }
  }
  return true;
} /* logfile_write_timestamp */


static void logfile_write(const char *buf)
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
    ssize_t ret;

    if (print_timestamp)
    {
      if (!logfile_write_timestamp())
      {
        logfile_reopen("Write error");
        return;
      }
      print_timestamp = false;
    }

    size_t write_len = 0;
    const char *nl = strchr(ptr, '\n');
    if (nl != 0)
    {
      write_len = nl-ptr+1;
      print_timestamp = true;
    }
    else
    {
      write_len = strlen(ptr);
    }
    ret = write(logfd, ptr, write_len);
    if (ret != static_cast<ssize_t>(write_len))
    {
      logfile_reopen("Write error");
      return;
    }
    ptr += write_len;
  }
} /* logfile_write */


static void logfile_flush(void)
{
  cout.flush();
  cerr.flush();
  if (stdout_watch != 0)
  {
    stdout_handler(stdout_watch);
  }
} /*  logfile_flush */


/*
 * This file has not been truncated
 */

