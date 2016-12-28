/**
@file	 echolib_test.h
@brief   A test application for the EchoLib library
@author  Tobias Blomberg
@date	 2003-03-08

This is a test application for the EchoLink library. It can be used to do a real
echolink Qso. It might not be very well tested though.
Try the "--help" switch for more info.

\verbatim
EchoLib - A library for EchoLink communication
Copyright (C) 2003  Tobias Blomberg / SM0SVX

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

/** @example Template_demo.cpp
An example of how to use the Template class
*/



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <popt.h>
#include <syslog.h>
#include <unistd.h>
#include <termios.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>

	
/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "EchoLinkProxy.h"
#include "EchoLinkDirectory.h"
#include "EchoLinkDispatcher.h"
#include "EchoLinkQsoTest.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define PROGRAM_NAME  "echolib_test"
#define SERVER_NAME   "server1.echolink.org"


typedef enum
{
  PS_START, PS_LOGON, PS_BUSY, PS_PRINT_CALLS, PS_PRINT_LINKS,
  PS_PRINT_REPEATERS, PS_PRINT_CONFERENCES, PS_PRINT_STATIONS,
  PS_CONNECT_TO_CALL, PS_CONNECT_TO_IP, PS_LOGOFF, PS_QUIT
} ProcessingStage;


/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

static void process_next_stage(void);
static void on_error_msg(const string& msg);
static void on_status_changed(StationData::Status status);
static void echolink_qso_done(EchoLinkQsoTest *con);
static void on_station_list_updated(void);
static void print_call_list(const list<StationData>& calls);
static void parse_arguments(int argc, const char **argv);


/****************************************************************************
 *
 * Global Variables
 *
 ****************************************************************************/
 


/****************************************************************************
 *
 * Private Global Variables
 *
 ****************************************************************************/

int verbose = 0;
int logon = 0;
int logoff = 0;
int make_busy = 0;
int get_calls = 0;
int print_links = 0;
int print_repeaters = 0;
int print_conferences = 0;
int print_stations = 0;
char *filter = 0;
const char *connect_to_call = 0;
const char *connect_to_ip = 0;
long vox_limit = -1;
int portbase = -1;
const char *proxy_host = 0;
int proxy_port = 8100;
const char *proxy_password = "";

Directory *dir = 0;
ProcessingStage processing_stage = PS_START;
char my_callsign[256];
char my_password[256];
char my_name[256];
char my_location[256];
char my_info[256];


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
 * Author:    
 * Created:   2003-03-08
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
int main( int argc, const char **argv )
{
  openlog(PROGRAM_NAME, LOG_PID, LOG_USER);
  parse_arguments(argc, argv);
  
  const char *home = getenv("HOME");
  if (home == 0)
  {
    home = ".";
  }
  string cfg_filename(string(home) + "/echolib_test.cfg");
    // Very simple config file reading. Ugly but sufficient...
  FILE *cfg = fopen(cfg_filename.c_str(), "r");
  if (cfg == 0)
  {
    fprintf(stderr, "Could not open config file: %s\n",
	cfg_filename.c_str());
    exit(1);
  }
  
  char *ret = fgets(my_callsign, sizeof(my_callsign), cfg);
  assert(ret != NULL);
  my_callsign[strlen(my_callsign)-1] = 0;
  
  ret = fgets(my_password, sizeof(my_password), cfg);
  assert(ret != NULL);
  my_password[strlen(my_password)-1] = 0;
  
  ret = fgets(my_name, sizeof(my_name), cfg);
  assert(ret != NULL);
  my_name[strlen(my_name)-1] = 0;
  
  ret = fgets(my_location, sizeof(my_location), cfg);
  assert(ret != NULL);
  my_location[strlen(my_location)-1] = 0;
  
  ret = fgets(my_info, sizeof(my_info), cfg);
  assert(ret != NULL);
  my_info[strlen(my_info)-1] = 0;
  
  fclose(cfg);

  CppApplication app;
   
  if (proxy_host != 0)
  {
    Proxy *proxy = new Proxy(proxy_host, proxy_port, my_callsign,
                             proxy_password);
    proxy->connect();
  }

  if (portbase != -1)
  {
    Dispatcher::setPortBase(portbase);
  }
  
  vector<string> servers;
  servers.push_back(SERVER_NAME);
  dir = new Directory(servers, my_callsign, my_password, my_location);
  dir->error.connect(sigc::ptr_fun(&on_error_msg));
  dir->statusChanged.connect(sigc::ptr_fun(&on_status_changed));
  dir->stationListUpdated.connect(sigc::ptr_fun(&on_station_list_updated));
    
  if (Dispatcher::instance() == 0)
  {
    cerr << "*** ERROR: Could not create EchoLink listener (Dispatcher)\n";
    exit(1);
  }
  // Handle incoming connections

  process_next_stage();
  
  app.exec();
  
  delete dir;
  
  closelog();
  
  return 0;
  
} /* main */







/****************************************************************************
 *
 * Functions
 *
 ****************************************************************************/

static void process_next_stage(void)
{
  processing_stage = static_cast<ProcessingStage>(processing_stage + 1);
  
  switch (processing_stage)
  {
    case PS_START:
      process_next_stage();
      break;

    case PS_LOGON:
      if (logon)
      {
	if (verbose) cout << "Logging on...\n";
	dir->makeOnline();
      }
      else
      {
	process_next_stage();
      }
      break;
  
    case PS_BUSY:
      if (make_busy)
      {
	if (verbose) cout << "Setting status to busy...\n";
	dir->makeBusy();
      }
      else
      {
	process_next_stage();
      }
      break;
  
    case PS_PRINT_CALLS:
      if (get_calls)
      {
	if (verbose) cout << "Getting calls...\n";
	dir->getCalls();
      }
      else
      {
	process_next_stage();
      }
      break;
  
    case PS_PRINT_LINKS:
      if (print_links)
      {
	if (verbose) cout << "Getting calls (links)...\n";
	dir->getCalls();
      }
      else
      {
	process_next_stage();
      }
      break;
  
    case PS_PRINT_REPEATERS:
      if (print_repeaters)
      {
	if (verbose) cout << "Getting calls (repeaters)...\n";
	dir->getCalls();
      }
      else
      {
	process_next_stage();
      }
      break;
      
    case PS_PRINT_CONFERENCES:
      if (print_conferences)
      {
	if (verbose) cout << "Getting calls (conferences)...\n";
	dir->getCalls();
      }
      else
      {
	process_next_stage();
      }
      break;

    case PS_PRINT_STATIONS:
      if (print_stations)
      {
	if (verbose) cout << "Getting calls (stations)...\n";
	dir->getCalls();
      }
      else
      {
	process_next_stage();
      }
      break;

    case PS_CONNECT_TO_CALL:
      if (connect_to_call != 0)
      {
	if (verbose) cout << "Getting calls (connect_to)...\n";
	dir->getCalls();
      }
      else
      {
	process_next_stage();
      }
      break;

    case PS_CONNECT_TO_IP:
      if (connect_to_ip != 0)
      {
	StationData *station = new StationData;
	station->setCallsign(connect_to_ip);
	station->setIp(IpAddress(connect_to_ip));

	EchoLinkQsoTest *echolink_qso =
	    new EchoLinkQsoTest(my_callsign, my_name, my_info, station);
	if (!echolink_qso->initOk())
	{
	  cerr << "ERROR: Could not create connection to " << connect_to_ip
	       << endl;
	  delete echolink_qso;
	  process_next_stage();
	  break;
	}
	echolink_qso->setVoxLimit(vox_limit);
	echolink_qso->done.connect(sigc::ptr_fun(&echolink_qso_done));
      }
      else
      {
	process_next_stage();
      }
      break;
      
    case PS_LOGOFF:
      if (logoff)
      {
	if (verbose) cout << "Logging off...\n";
	dir->makeOffline();
      }
      else
      {
	process_next_stage();
      }
      break;
  
    case PS_QUIT:
      Application::app().quit();
      break;
    
    /*
    default:
      process_next_stage();
      break;
    */
  }
  
} /* process_next_stage */


static void on_error_msg(const string& msg)
{
  cout << "*** ERROR: " << msg << endl;
  Application::app().quit();
} /* on_error_msg */


static void on_status_changed(StationData::Status status)
{
  if (verbose)
  {
    cout << "Status changed to " << dir->statusStr() << endl;
  }
  
  process_next_stage();
  
} /* on_status_changed */


static void echolink_qso_done(EchoLinkQsoTest *con)
{
  delete con;
  process_next_stage();
} /* echolink_qso_done */


static void on_station_list_updated(void)
{
  if (verbose)
  {
    /*
    list<string> message = dir->message();
    list<string>::const_iterator it;
    for (it=message.begin(); it!=message.end(); ++it)
    {
      cout << *it << endl;
    }
    */
    cout << dir->message();
  }
  
  switch (processing_stage)
  {
    case PS_PRINT_CALLS:
      print_call_list(dir->links());
      print_call_list(dir->repeaters());
      print_call_list(dir->conferences());
      print_call_list(dir->stations());
      process_next_stage();
      break;
      
    case PS_PRINT_LINKS:
      print_call_list(dir->links());
      process_next_stage();
      break;
      
    case PS_PRINT_REPEATERS:
      print_call_list(dir->repeaters());
      process_next_stage();
      break;
      
    case PS_PRINT_CONFERENCES:
      print_call_list(dir->conferences());
      process_next_stage();
      break;
      
    case PS_PRINT_STATIONS:
      print_call_list(dir->stations());
      process_next_stage();
      break;
      
    case PS_CONNECT_TO_CALL:
    {
      const StationData *station = dir->findCall(connect_to_call);
      if (station != 0)
      {
	EchoLinkQsoTest *echolink_qso =
	    new EchoLinkQsoTest(my_callsign, my_name, my_info, station);
	if (!echolink_qso->initOk())
	{
	  cerr << "ERROR: Could not create connection to " << my_callsign
	       << endl;
	  delete echolink_qso;
	  process_next_stage();
	  break;
	}
	echolink_qso->done.connect(sigc::ptr_fun(&echolink_qso_done));
      }
      else
      {
      	cerr << "ERROR: Could not find call " << connect_to_call << endl;
	process_next_stage();
      }
      break;
    }
    
    default:
      process_next_stage();
      break;
  }
  
} /* on_station_list_updated */


/*
 *----------------------------------------------------------------------------
 * Function:  print_call_list
 * Purpose:   Print the contents of the supplied list of calls
 * Input:     calls - The list to print
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-03-09
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
static void print_call_list(const list<StationData>& calls)
{
  list<StationData>::const_iterator iter;
  for (iter=calls.begin(); iter!=calls.end(); ++iter)
  {
    if ((filter == 0) || (strstr(iter->callsign().c_str(), filter) != 0))
    printf("%-*s %-4s %5s %-30s %6d %s\n",
      	StationData::MAXCALL, iter->callsign().c_str(),
	iter->statusStr().c_str(), iter->time().c_str(),
	iter->description().c_str(), iter->id(), iter->ipStr().c_str());
  }  
} /* print_call_list */


/*
 *----------------------------------------------------------------------------
 * Function:  parse_arguments
 * Purpose:   Parse the command line arguments.
 * Input:     argc  - Number of arguments in the command line
 *    	      argv  - Array of strings with the arguments
 * Output:    Returns 0 if all is ok, otherwise -1.
 * Author:    Tobias Blomberg
 * Created:   2000-06-13
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
static void parse_arguments( int argc, const char **argv )
{
  poptContext optCon;
  const struct poptOption optionsTable[] =
  {
    POPT_AUTOHELP
    {"filter", 'f', POPT_ARG_STRING, &filter, 0,
	    "Filter out calls containing filter string", "<call filter>"},
    {"call", 'c', POPT_ARG_STRING, &connect_to_call, 0,
	    "Connect to the given station", "<callsign>"},
    {"ip", 'i', POPT_ARG_STRING, &connect_to_ip, 0,
	    "Connect to the given host (IP-address or hostname)", "<host>"},
    {"verbose", 'v', POPT_ARG_NONE, &verbose, 0,
	    "Print some informational messages", NULL},
    {"logon", 0, POPT_ARG_NONE, &logon, 0,
	    "Send logon request", NULL},
    {"logoff", 0, POPT_ARG_NONE, &logoff, 0,
	    "Send logoff request", NULL},
    {"busy", 0, POPT_ARG_NONE, &make_busy, 0,
	    "Send set busy request", NULL},
    {"calls", 0, POPT_ARG_NONE, &get_calls, 0,
	    "Get and print all active calls", NULL},
    {"links", 0, POPT_ARG_NONE, &print_links, 0,
	    "Get and print all links", NULL},
    {"repeaters", 0, POPT_ARG_NONE, &print_repeaters, 0,
	    "Get and print all repeaters", NULL},
    {"conferences", 0, POPT_ARG_NONE, &print_conferences, 0,
	    "Get and print all conferences", NULL},
    {"stations", 0, POPT_ARG_NONE, &print_stations, 0,
	    "Get and print all stations", NULL},
    {"vox", 0, POPT_ARG_INT, &vox_limit, 0,
	    "The VOX limit to use", "<limit>"},
    {"portbase", 0, POPT_ARG_INT, &portbase, 0,
	    "The UDP port base to use", "<port base>"},
    {"proxyhost", 'p', POPT_ARG_STRING, &proxy_host, 0,
            "Use EchoLink Proxy server", "<hostname>"},
    {"proxyport", 0, POPT_ARG_INT, &proxy_port, 0,
	    "Use EchoLink proxy TCP port (default 8100)", "<port>"},
    {"proxypasswd", 'p', POPT_ARG_STRING, &proxy_password, 0,
            "Use EchoLink Proxy server password", "<password>"},
    {NULL, 0, 0, NULL, 0}
  };
  int err;
  const char *arg = NULL;
  int argcnt = 0;
  
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

  if (filter != 0)
  {
    char *str = filter;
    while (*str)
    {
      *str = toupper(*str);
      ++str;
    }
  }
  
    /* Parse arguments that do not begin with '-' (leftovers) */
  arg = poptGetArg(optCon);
  while (arg != NULL)
  {
    printf("arg %2d      = %s\n", ++argcnt, arg);
    arg = poptGetArg(optCon);
  }

  poptFreeContext(optCon);

}



/*
 * This file has not been truncated
 */

