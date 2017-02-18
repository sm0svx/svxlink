/**
@file	 svxserver.cpp
@brief   Main file for the svxserver remote transceiver for SvxLink
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2017-01-17

This is the main file for the svxserver remote transceiver for the
SvxLink server. It is used to link in remote transceivers to the SvxLink
server core (e.g. via a TCP/IP network).

\verbatim
svxserver - A remote receiver for the SvxLink server
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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
#include <cassert>
#include <signal.h>
#include <termios.h>
#include <dirent.h>
#include <popt.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>
#include <pwd.h>
#include <grp.h>

#include <iostream>
#include <iomanip>
#include <cstring>
#include <cstdlib>
#include <vector>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncConfig.h>
#include <AsyncFdWatch.h>
#include <common.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXSERVER.h"
#include "server.h"


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

#define PROGRAM_NAME "svxserver"


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

static char             *pidfile_name = NULL;
static char             *logfile_name = NULL;
static char             *runasuser = NULL;
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
  memset(&act, '\0', sizeof(act));
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

  act.sa_handler = SIG_IGN;
  if (sigaction(SIGPIPE, &act, NULL) == -1)
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
    // must explicitly specify name space for ptr_fun() to avoid conflict
    // with ptr_fun() in /usr/include/c++/4.5/bits/stl_function.h
    stdout_watch->activity.connect(sigc::ptr_fun(&stdout_handler));

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

      /* Force stdout to line buffered mode */
    if (setvbuf(stdout, NULL, _IOLBF, 0) != 0)
    {
      perror("setlinebuf");
      exit(1);
    }

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

    int rc = fchown(logfd, passwd->pw_uid, passwd->pw_gid);
    if (rc != 0)
    {
      char str[256];
      sprintf(str, "chown returns:%d uid:%d gid:%d", rc, 
              passwd->pw_uid, passwd->pw_gid);
      perror(str);
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
    cfg_filename += "/.svxlink/svxserver.conf";
    if (!cfg.open(cfg_filename))
    {
      cfg_filename = "/etc/svxlink/svxserver.conf";
      if (!cfg.open(cfg_filename))
      {
	cfg_filename = "/etc/svxserver.conf";
	if (!cfg.open(cfg_filename))
	{
	  cerr << "*** ERROR: Could not open configuration file. Tried:\n"
      	       << "\t" << home_dir << "/.svxlink/svxserver.conf\n"
      	       << "\t/etc/svxlink/svxserver.conf\n"
	       << "\t/etc/svxserver.conf\n"
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

  cout << PROGRAM_NAME " v" SVXSERVER_VERSION " (" __DATE__
          ") Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX\n\n";
  cout << PROGRAM_NAME " comes with ABSOLUTELY NO WARRANTY. "
          "This is free software, and you are\n";
  cout << "welcome to redistribute it in accordance with the "
          "terms and conditions in the\n";
  cout << "GNU GPL (General Public License) version 2 or later.\n";

  cout << "\nUsing configuration file: " << main_cfg_filename << endl;


  struct termios org_termios;
  if (logfile_name == 0)
  {
    struct termios termios;
    tcgetattr(STDIN_FILENO, &org_termios);
    termios = org_termios;
    termios.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &termios);

    stdin_watch = new FdWatch(STDIN_FILENO, FdWatch::FD_WATCH_RD);
    // must explicitly specify name space for ptr_fun() to avoid conflict
    // with ptr_fun() in /usr/include/c++/4.5/bits/stl_function.h
    stdin_watch->activity.connect(sigc::ptr_fun(&stdinHandler));
  }

  SvxServer my_server(cfg);
  app.exec();

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
  ssize_t ret;
  while (*ptr != 0)
  {
    static bool print_timestamp = true;

    if (print_timestamp)
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
	struct tm *tm = localtime(&tv.tv_sec);
	char tstr[256];
	size_t tlen = strftime(tstr, sizeof(tstr), fmt.c_str(), tm);
	ret = write(logfd, tstr, tlen);
        assert(ret == static_cast<ssize_t>(tlen));
	ret = write(logfd, ": ", 2);
        assert(ret == 2);
	print_timestamp = false;
      }

      if (reopen_log)
      {
	const char *reopen_txt = "SIGHUP received. Reopening logfile\n";
        const size_t reopen_txt_len = strlen(reopen_txt);
	ret = write(logfd, reopen_txt, reopen_txt_len);
        assert(ret == static_cast<ssize_t>(reopen_txt_len));
	open_logfile();
	reopen_log = false;
	print_timestamp = true;
	continue;
      }
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
    assert(ret == static_cast<ssize_t>(write_len));
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
  ssize_t ret;

  if (logfile_name == 0)
  {
    ret = write(STDOUT_FILENO, "Ignoring SIGHUP\n", 16);
    assert(ret == 16);
    return;
  }

  ret = write(STDOUT_FILENO, "SIGHUP received. Logfile reopened\n", 34);
  assert(ret == 34);
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

