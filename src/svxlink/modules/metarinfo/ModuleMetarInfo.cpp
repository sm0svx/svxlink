/*
@file	 ModuleMetarInfo.cpp
@brief   gives out a METAR report
@author  Steve Koehler / DH1DM & Adi Bier / DL1HRC
@date	 2018-03-10

\verbatim
A module (plugin) to request the latest METAR (weather) information from
by using ICAO shortcuts.
Look at http://en.wikipedia.org/wiki/METAR for further information

Copyright (C) 2009-2015 Tobias Blomberg / SM0SVX

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

#include <string.h>
#include <iostream>
#include <sstream>
#include <time.h>
#include <algorithm>
#include <queue>
#include <regex.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncTimer.h>
#include <AsyncFdWatch.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/MODULE_METARINFO.h"
#include "ModuleMetarInfo.h"
#include "common.h"


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

#define INTOKEN 1
#define TIME 2
#define ISMILE 3
#define IS1STPARTOFVIEW 4
#define ISVIEW 5
#define TEMPERATURE 6
#define CLOUDSVALID 7
#define WIND 8
#define VERTICALVIEW 9
#define ACTUALWX 10
#define VALUEVARIES 11
#define UTC 12
#define AUTO 13
#define QNH 14
#define RVR 15
#define WORDSNOEXT 16
#define WORDSEXT 17
#define ISPARTOFMILES 18
#define RUNWAY 19
#define FORECAST 20
#define SLP 21
#define PEAKWIND 22
#define ALLRWYSTATE 23

#define SNOWCLOSED 25
#define WINDSHIFT 26
#define AUTOTYPE 27
#define RMKVISIBILITY 28
#define NOSPECI 29
#define FROPA 30
#define LIGHTNING 31
#define VIRGA 32
#define RMK 33
#define DAYTEMPMAX 34
#define DAYTEMPMIN 35
#define FLIGHTLEVEL 36
#define WORDSINRMK 37
#define TEMPOOBSCURATION 38
#define TEMPINRMK 39
#define PRESSURETENDENCY 40
#define PRECIPITATION1 41
#define PRECIPITATION6 42
#define PRECIPITATION24 43
#define MINMAXTEMP 44
#define NOSEVEREWX 45
#define MAXTEMP 46
#define MINTEMP 47
#define CEILING 48
#define MAINTENANCE 49
#define PRECIPINRMK 50
#define CLOUDTYPE 51
#define QFEINRMK 52

#define END 60

#define NOTMEASURED 97
#define NOTACTUAL 98
#define INVALID 99


/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

class Http : public sigc::trackable
{
   CURLM* multi_handle; 
   Async::Timer update_timer;
   std::map<int, Async::FdWatch*> watch_map;
   std::queue<CURL*> url_queue;
   CURL* pending_curl;

  public:

   Http() : multi_handle(0), pending_curl(0)
   {
     multi_handle = curl_multi_init();
     long curl_timeout = -1;
     curl_multi_timeout(multi_handle, &curl_timeout);
     update_timer.setTimeout((curl_timeout >= 0) ? curl_timeout : 100);
     update_timer.setEnable(false);
     update_timer.expired.connect(mem_fun(*this, &Http::onTimeout));
   } /* Http */

   ~Http()
   {
     if (pending_curl)
       curl_easy_cleanup(pending_curl);
     ClearWatchMap();
     curl_multi_cleanup(multi_handle);
   } /* ~Http */

   // a signal when a metar has been available
   sigc::signal<void, std::string, size_t> metarInfo;

   // a signal when a metar has a timeout
   sigc::signal<void> metarTimeout;


   // update the html handler periodically
   void onTimeout(Async::Timer *timer)
   {
     int handle_count;
     curl_multi_perform(multi_handle, &handle_count);
     if (handle_count == 0) 
     {
       ClearWatchMap();
       curl_easy_cleanup(pending_curl);
       if (url_queue.empty())
       {
         pending_curl = 0;
         update_timer.setEnable(false);
       }
       else
       {
         pending_curl = url_queue.front();
         url_queue.pop();
         curl_multi_add_handle(multi_handle, pending_curl);
         update_timer.setEnable(true);
       }
     }
     UpdateWatchMap();
     update_timer.reset();
   } /* Update */

   void onActivity(Async::FdWatch *watch)
   {
     int handle_count;
     curl_multi_perform(multi_handle, &handle_count);
     if (handle_count == 0)
     {
       ClearWatchMap();
       curl_easy_cleanup(pending_curl);
       if (url_queue.empty())
       {
         pending_curl = 0;
         update_timer.setEnable(false);
       }
       else
       {
         pending_curl = url_queue.front();
         url_queue.pop();
         curl_multi_add_handle(multi_handle, pending_curl);
         UpdateWatchMap();
         update_timer.setEnable(true);
       }
     }
     update_timer.reset();
   } /* onActivity */

   static size_t callback(char *contents, size_t size, size_t nmemb,
                                       void *userp)
   {
     if (userp == NULL) return 0;
     size_t written = size * nmemb;
     std::string html((const char *)contents, written);
     static_cast<Http*>(userp)->metarInfo(html, html.size());
     return written;
   } /* callback */

   void AddRequest(const char* uri)
   {
     CURL* curl = curl_easy_init();
     curl_easy_setopt(curl, CURLOPT_URL, uri);
     curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &Http::callback);
     curl_easy_setopt(curl, CURLOPT_WRITEDATA, this);

     if (!pending_curl)
     {
       pending_curl = curl;
       curl_multi_add_handle(multi_handle, pending_curl);
       UpdateWatchMap();
       update_timer.reset();
       update_timer.setEnable(true);
     }
     else
     {
       url_queue.push(curl);
     }
   } /* AddRequest */

  private:

   void UpdateWatchMap()
   {
     fd_set fdread;
     fd_set fdwrite;
     fd_set fdexcep;
     int maxfd = -1;

     FD_ZERO(&fdread);
     FD_ZERO(&fdwrite);
     FD_ZERO(&fdexcep);
     curl_multi_fdset(multi_handle, &fdread, &fdwrite, &fdexcep, &maxfd);

     for (int fd = 0; fd <= maxfd; fd++) 
     {
       if (watch_map.find(fd) != watch_map.end())
         continue;
       if (FD_ISSET(fd, &fdread))
       {
         Async::FdWatch *watch = new Async::FdWatch(fd,Async::FdWatch::FD_WATCH_RD);
         watch->activity.connect(mem_fun(*this, &Http::onActivity));
         watch_map[fd] = watch;
       }
       if (FD_ISSET(fd, &fdwrite)) 
       {
         Async::FdWatch *watch = new Async::FdWatch(fd,Async::FdWatch::FD_WATCH_WR);
         watch->activity.connect(mem_fun(*this, &Http::onActivity));
         watch_map[fd] = watch;
       }
     }
   } /* UpdateWatchMap */

   void ClearWatchMap() {
     for (std::map<int, Async::FdWatch*>::iterator it = watch_map.begin();
          it != watch_map.end(); ++it)
     {
       delete it->second;
     }
     watch_map.clear();
   } /* ClearWatchMap */
};


/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



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

std::string desc[] = {
          "vcbldu", "vcblsa", "vcblsn", "bcfg", "vcpo",
          "bldu", "blsa", "blpy", "blsn", "fzbr",
          "vcbr", "tsgr", "vcts", "drdu", "drsa",
          "drsn","fzfg",  "fzdz", "fzra", "prfg",
          "mifg", "shra","shsn",  "shpe", "shpl",
          "shgs", "shgr", "vcfg", "vcfc", "vcss",
          "vcds", "tsra", "tspe", "tspl", "tssn",
          "vcsh", "br", "du", "dz", "ds",
          "fg",   "fc", "fu", "gs", "gr",
          "hz",   "ic", "pe", "pl", "po",
          "ra",   "fz", "sn", "sg", "sq",
          "sa",   "ss", "ts", "va", "py",
          "sh"};

std::string clouds[] = {"acc", "ac", "as", "cbmam", "cb",
                        "cc", "cf",  "ci", "cs", "cu",
                        "tcu", "ns", "sc", "sf", "st"};

/****************************************************************************
 *
 * Pure C-functions
 *
 ****************************************************************************/


extern "C" {
  Module *module_init(void *dl_handle, Logic *logic, const char *cfg_name)
  {
    return new ModuleMetarInfo(dl_handle, logic, cfg_name);
  }
} /* extern "C" */



/****************************************************************************
 *
 * Public member functions
 *
 ****************************************************************************/


ModuleMetarInfo::ModuleMetarInfo(void *dl_handle, Logic *logic,
                                 const string& cfg_name)
  : Module(dl_handle, logic, cfg_name), remarks(false), debug(false)
{
  cout << "\tModule MetarInfo v" MODULE_METARINFO_VERSION " starting...\n";

} /* ModuleMetarInfo */


ModuleMetarInfo::~ModuleMetarInfo(void)
{

} /* ~ModuleMetarInfo */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:
 * Purpose:
 * Input:
 * Output:
 * Author:
 * Created:
 * Remarks:
 * Bugs:
 *------------------------------------------------------------------------
 */

void ModuleMetarInfo::resumeOutput(void)
{

} /* ModuleMetarInfo::resumeOutput */


void ModuleMetarInfo::allSamplesFlushed(void)
{

} /* ModuleMetarInfo::allSamplesFlushed */


int ModuleMetarInfo::writeSamples(const float *samples, int count)
{
  return count;
} /* ModuleMetarInfo::writeSamples */


void ModuleMetarInfo::flushSamples(void)
{
  sourceAllSamplesFlushed();
} /* ModuleMetarInfo::flushSamples */



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    initialize
 * Purpose:   Called by the core system right after the object has been
 *    	      constructed. As little of the initialization should be done in
 *    	      the constructor. It's easier to handle errors here.
 * Input:     None
 * Output:    Return \em true on success or else \em false should be returned
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:   The base class initialize method must be called from here.
 * Bugs:
 *----------------------------------------------------------------------------
 */
bool ModuleMetarInfo::initialize(void)
{

  string value;
  StrList apset;
  std::string tp;
  html = "";

  repstr["shra"] = "ra sh ";
  repstr["shsn"] = "sn sh ";
  repstr["shpl"] = "pl sh ";
  repstr["shpe"] = "pe sh ";
  repstr["shgr"] = "gr sh ";
  repstr["bcfg"] = "fg bc ";

  shdesig["l"] = "left";
  shdesig["r"] = "right";
  shdesig["c"] = "center";
  shdesig["ll"]= "left_out";
  shdesig["rr"]= "right_out";
  shdesig["m"] = "less_than";
  shdesig["p"] = "more_than";
  shdesig["d"] = "decreasing";
  shdesig["u"] = "increasing";
  shdesig["n"] = "ndt";
  shdesig["vc"]= "vicinity";
  shdesig["re"]= "recent";
  shdesig["-"] = "light";
  shdesig["+"] = "heavy";
  shdesig["fm"]= "from";
  shdesig["tl"]= "until";

  if (!Module::initialize())
  {
    return false;
  }

  if (!cfg().getValue(cfgName(), "AIRPORTS", value))
  {
      cout << "*** ERROR: Config variable " << cfgName()
           << "/AIRPORTS not set or wrong, example: AIRPORTS=EDDP,EDDS,EDDB\n";
      return false;
  }

  // split the line
  splitStr(apset, value, ",");

  for (StrList::const_iterator it = apset.begin(); it != apset.end(); it++)
  {
     tp = *it;
     if (tp.length() != 4)
     {
        cout << "*** ERROR: Config variable " << cfgName()
             << "/AIRPORTS: "<< tp << " is not valid.\n";
        return false;
     }
     // all to upcase
     transform(tp.begin(),tp.end(),tp.begin(),(int(*)(int))toupper);
     aplist.push_back(tp);
  }

  if (cfg().getValue(cfgName(), "STARTDEFAULT", value))
  {
     if (value.length() == 4) icao_default = value;
     else
     {
        cout << "**** WARNING: Config variable " << cfgName()
             << "/STARTDEFAULT: " << value << " is not valid.\n";
        return false;
     }
  }

  // still  development
  if (cfg().getValue(cfgName(), "REMARKS", value))
  {
    remarks = true;
  }

  if (cfg().getValue(cfgName(), "DEBUG", value))
  {
    debug = true;
  }
  
  if (!cfg().getValue(cfgName(), "TYPE", type))
  {
    cout << "**** WARNING: Config variable " << cfgName() 
         << "/TYPE is not set.\n";
    return false;
  }

  if (type != "TXT" && type != "XML")
  {
    cout << "**** WARNING: Config variable " << cfgName() 
         << "/TYPE: " << type << " is not valid.\n";
    return false;
  }

  if (!cfg().getValue(cfgName(), "SERVER", server))
  {
    cout << "**** WARNING: Config variable " << cfgName() 
         << "/SERVER: " << server << " is not set.\n";
    return false;      
  }

  cfg().getValue(cfgName(), "LINK", link);

  // long messages or short messages
  // nosig -> "nosig"  == short message
  //       -> "no significant change" == long message
  if (cfg().getValue(cfgName(), "LONGMESSAGES", value))
  {
     longmsg = "_long ";  // taking "cavok_long" instead of "cavok"
  }

  return true;

} /* initialize */


/*
 *----------------------------------------------------------------------------
 * Method:    activateInit
 * Purpose:   Called by the core system when this module is activated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleMetarInfo::activateInit(void)
{
  if (icao_default.length() == 4)
  {
      icao = icao_default;
      openConnection();
  }
} /* activateInit */


/*
 *----------------------------------------------------------------------------
 * Method:    deactivateCleanup
 * Purpose:   Called by the core system when this module is deactivated.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:   Do NOT call this function directly unless you really know what
 *    	      you are doing. Use Module::deactivate() instead.
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleMetarInfo::deactivateCleanup(void)
{
} /* deactivateCleanup */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfDigitReceived
 * Purpose:   Called by the core system when a DTMF digit has been
 *    	      received. This function will only be called if the module
 *    	      is active.
 * Input:     digit   	- The DTMF digit received (0-9, A-D, *, #)
 *            duration	- The length in milliseconds of the received digit
 * Output:    Return true if the digit is handled or false if not
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
bool ModuleMetarInfo::dtmfDigitReceived(char digit, int duration)
{

  cout << "DTMF digit received in module " << name() << ": " << digit << endl;

  return false;

} /* dtmfDigitReceived */


/*
 *----------------------------------------------------------------------------
 * Method:    dtmfCmdReceived
 * Purpose:   Called by the core system when a DTMF command has been
 *    	      received. A DTMF command consists of a string of digits ended
 *    	      with a number sign (#). The number sign is not included in the
 *    	      command string. This function will only be called if the module
 *    	      is active.
 * Input:     cmd - The received command.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2004-03-07
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleMetarInfo::dtmfCmdReceived(const string& cmd)
{
  stringstream tosay;
  int a = 0;
  int offset;
  //const char *pos;
  //std::string spos;
  StrList mtcmd;
  typedef map<char, std::string> Digits;
  Digits mypad;

  mypad['0'] = "0000000000";
  mypad['1'] = "1111111111";
  mypad['2'] = "2ABCCCCCCC";
  mypad['3'] = "3DEFFFFFFF";
  mypad['4'] = "4GHIIIIIII";
  mypad['5'] = "5JKLLLLLLL";
  mypad['6'] = "6MNOOOOOOO";
  mypad['7'] = "7PQRSSSSSS";
  mypad['8'] = "8TUVVVVVVV";
  mypad['9'] = "9WXYZZZZZZ";
  mypad['A'] = "AAAAAAAAAA";
  mypad['B'] = "BBBBBBBBBB";
  mypad['C'] = "CCCCCCCCCC";
  mypad['D'] = "DDDDDDDDDD";

  cout << "DTMF command received in module " << name() << ": " << cmd << endl;

  int icmd = atoi(cmd.c_str());

  if (cmd == "")
  {
     deactivateMe();
     return;
  }

  if (cmd == "0")                       // normal help
  {
     tosay << "say metarhelp";
     say(tosay);
     return;
  }

  else if (cmd == "01")                 // play configured airports
  {
     tosay << "icao_available";
     say(tosay);

     tosay << "airports ";
     for (StrList::const_iterator it=aplist.begin(); it != aplist.end(); it++)
     {
        tosay << ++a << " " << *it << " ";
     }
     say(tosay);
     return;
  }

  // here we have the star method
  else if (cmd.length() > 4 && cmd.find('*') != string::npos)
  {
     icao = "";
     a = 0;
     splitEmptyStr(mtcmd, cmd);
     for (StrList::const_iterator cmdit = mtcmd.begin();
        cmdit != mtcmd.end(); cmdit++)
     {
        //pos = (cmdit->substr(0,1)).c_str();
        //spos= mypad[pos[0]];
        string spos = mypad[(*cmdit)[0]];
        icao += spos.substr(cmdit->length(),1);
     }
  }

  // and the SVX-method
  else if (cmd.length() == 8 && cmd.find('*') == string::npos)
  {
     icao = "";
     for (a=0; a<8; a+=2)
     {
        //pos = cmd.substr(a,1).c_str();
        //spos= mypad[pos[0]];
        string spos = mypad[cmd[a]];
        offset = atoi(cmd.substr(a+1,1).c_str());
        icao += spos.substr(offset,1);
     }
  }

  // icao from predefined list
  else if (icmd <= (int)aplist.size() && icmd > 0)
  {
     icao = aplist[icmd - 1];
     openConnection();
     return;
  }

  // request icao-matarinfo
  if (icao.length() == 4)
  {
     if (debug) cout << "icao-code by dtmf-method: " << icao << endl;
     openConnection();
  }
  else
  {
      tosay << "no_airport_defined";
      say(tosay);
  }

} /* dtmfCmdReceived */



void ModuleMetarInfo::dtmfCmdReceivedWhenIdle(const string& cmd)
{
  std::cout << "dtmfCmdReceivedWhenIdle\n";
  dtmfCmdReceived(cmd);
} /* dtmfCmdReceivedWhenIdle */



/*
 *----------------------------------------------------------------------------
 * Method:    squelchOpen
 * Purpose:   Called by the core system when the squelch open or close.
 * Input:     is_open - Set to \em true if the squelch is open or \em false
 *    	      	      	if it's not.
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleMetarInfo::squelchOpen(bool is_open)
{

} /* squelchOpen */


/*
 *----------------------------------------------------------------------------
 * Method:    allMsgsWritten
 * Purpose:   Called by the core system when all announcement messages has
 *    	      been played. Note that this function also may be called even
 *    	      if it wasn't this module that initiated the message playing.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleMetarInfo::allMsgsWritten(void)
{

} /* allMsgsWritten */


/*
* establish a https-connection to the METAR-Server
* using curl library
*/
void ModuleMetarInfo::openConnection()
{

  Http *http = new Http();

  std::string path = server;
              path += link;
              path += icao;

  http->AddRequest(path.c_str());
  cout << path << endl;
  http->metarInfo.connect(mem_fun(*this, &ModuleMetarInfo::onData));
  http->metarTimeout.connect(mem_fun(*this, &ModuleMetarInfo::onTimeout));

} /* openConnection */


void ModuleMetarInfo::onTimeout(void)
{
  stringstream temp;
  temp << "metar_not_valid";
  say(temp);
} /* ModuleMetarInfo::onTimeout */


void ModuleMetarInfo::onData(std::string metarinput, size_t count)
{
  std::string metar = "";
  html += metarinput;

  // switching between the newer xml-service by aviationweather and the old 
  // noaa.gov version. With the standard TXT format anybody will be able to 
  // create it's own METAR report from it's own weather station

  if (type == "XML")
  {
    if (debug)
    {
      cout << "requesting XML metar version from " << server << "/" << endl;
    }

    if (html.find("<data num_results=\"0\" />") != string::npos)
    {
      stringstream temp;
      cout << "Metar information not available" << endl;
      temp << "metar_not_valid";
      say(temp);
      html = "";
      return;
    }

    // check day and time, if not in limit throw information away
    // e.g.: 2016-08-10T08:20:00Z
    std::string met_utc = getXmlParam("observation_time", html);

    // look for raw metar data
    metar = getXmlParam("raw_text", html);

    if (metar.length() > 0)
    {
      html = "";
      if (debug)
      {
        cout << "XML-METAR: " << metar << endl;
      }

      if (met_utc.length() == 20 && !isvalidUTC(met_utc))
      {
        stringstream temp;
        cout << "Metar information outdated" << endl;
        temp << "metar_not_valid";
        say(temp);
        return;
      }
    }
  }
  // the TXT version of METAR
  else 
  {
    // This is a MEATAR-report:
    //
    // 2009/04/07 13:20
    // FBJW 071300Z 09013KT 9999 FEW030 29/15 Q1023 RMK ...

    size_t found;
    StrList values;
    std::stringstream temp;
    splitStr(values, html, "\n");
    metar = values.back();  // contains the METAR

    if (debug)
    {
      cout << "TXT-METAR: " << metar << endl;
    }

    values.pop_back();
    std::string metartime = values.back();  // and the time at UTC

    // split \n -> <SPACE>
    while ((found = html.find('\n')) != string::npos) html[found] = ' ';

    if (html.find("404 Not Found") != string::npos)
    {
      cout << "ERROR 404 from webserver -> no such airport\n";
      temp << "no_such_airport";
      say(temp);
      return;
    }

    // check if METAR is actual
    if (!isvalidUTC(metartime.substr(0,16)))
    {
      temp << "metar_not_valid";
      say(temp);
      return;
    }
  }

  handleMetar(metar);
} /* onDataReceived */


std::string ModuleMetarInfo::getXmlParam(std::string token, std::string input)
{
  std::string start = "<";
  std::string stop = "</";
  start += token;
  start += ">";
  stop += token;
  stop += ">";

  size_t an, en;

  an = input.find(start);
  en = input.find(stop);

  if (an != std::string::npos && en != std::string::npos)
  {
     an += token.length() + 2;
     return input.substr(an, en - an);
  }

  return "";
} /* getXmlParam */


int ModuleMetarInfo::handleMetar(std::string input)
{
   std::string current;
   std::string tempstr;
   std::stringstream temp;
   StrList values;
   bool is_false = false;
   bool endflag = false;
   bool nceiling = false;
   float temp_view = 0;
   int metartoken;

   metartoken = 0;

   // This is a raw MEATAR-report:
   //
   // FBJW 071300Z 09013KT 9999 FEW030 29/15 Q1023 RMK ...
   //
   temp << "metar \"" << input << "\"";
   say(temp);

   temp << "airports " << icao;
   say(temp);

   processEvent("say airport");

   splitStr(values, input, " ");
   StrList::iterator it = values.begin();

   while (it != values.end() && !endflag) {

     current = *it;

     // == current.tolower()
     transform(current.begin(),current.end(),current.begin(),
               (int(*)(int))tolower);

     metartoken = checkToken(current);

     switch (metartoken)
     {

         case INTOKEN:
            break;

         case UTC:
            temp << "metreport_time " << current.substr(2,4);
            say(temp);
            break;

         case AUTO:
          //  temp << " automatic_station ";
            break;

         case WIND:
            if (isWind(tempstr, current))
            {
              temp << "wind " << tempstr;
              say(temp);
            }
            break;

         case VALUEVARIES:
            isValueVaries(tempstr, current);
            if (!nceiling)
               temp << "windvaries " << tempstr;
            else
            {
               temp << "ceilingvaries " << tempstr;
               nceiling = false;
            }
            say(temp);
            nceiling = false;
            break;

         case IS1STPARTOFVIEW:
            temp_view = atof(current.c_str());
            break;

         case ISPARTOFMILES:
            isPartofMiles(tempstr, current);
              temp_view += atof(tempstr.c_str());
            temp << "visibility " << temp_view << " unit_miles";
            say(temp);
            break;

         case ISVIEW:
            if (isView(tempstr, current))
            {
              temp << "visibility " << tempstr;
              say(temp);
            }
            break;

         case WORDSEXT:
            temp << "say " << current << longmsg;
            say(temp);
            break;

         case WORDSNOEXT:
            temp << "say " << current;
            say(temp);
            break;

         case CLOUDSVALID:
            if (ispObscurance(tempstr, current))
            {
              if (!is_false)     // only once
              {
                 processEvent("say clouds");
                 is_false = true;
              }
              temp << "clouds " << tempstr;
              say(temp);
            }
            break;

         case VERTICALVIEW:
            isVerticalView(tempstr, current);
            temp << "ceiling " << tempstr;
            say(temp);
            break;

         case ACTUALWX:
            if (isActualWX(tempstr, current))
            {
               temp << "actualWX " << tempstr;
               say(temp);
            }
            break;

         case RVR:
            if (isRVR(tempstr, current))
            {
               temp << "rvr " << tempstr;
               say(temp);
              }
            break;

         case TEMPERATURE:
            validTemp(tempstr, current);
            temp << "temperature " << tempstr;
            say(temp);
            validDp(tempstr, current);
            temp << "dewpoint " << tempstr;
            say(temp);
            break;

         case QNH:
            isQnh(tempstr, current);
            temp << tempstr;
            say(temp);
            is_false = false;
            break;

         case RUNWAY:
            isRunway(tempstr, current);
            temp << "runway " << tempstr;
            say(temp);
            break;

         case ALLRWYSTATE:
            isRwyState(tempstr, current);
            temp << "runwaystate " << tempstr;
            say(temp);
            break;

         case TIME:
            isTime(tempstr, current);
            temp << "time " << tempstr;
            say(temp);
            break;

         case FORECAST:
            temp << "trend " << current << longmsg;
            say(temp);
            break;

         case RMK:
            if (!remarks)
                  endflag = true;
            else
            {
                temp << "remarks";
                say(temp);
            }
            break;

         case SLP:
            temp << "slp " << getSlp(current);
            say(temp);
            break;

         case SNOWCLOSED:
            processEvent("snowclosed");
            break;

         case PEAKWIND:
            it++;
            current = *it;
            if (getPeakWind(tempstr, current))
            {
               temp << "peakwind " << tempstr;
               say(temp);
            }
            break;

         case NOSPECI:
            processEvent("nospeci");
            break;

         case WINDSHIFT:
            it++;
            current = *it;
            temp << "windshift " << current;
            say(temp);
            break;

         case AUTOTYPE:
            temp << "say " << current << longmsg;
            say(temp);
            break;

         case RMKVISIBILITY:
            it++;
            current = *it;
/*          temp << "rmk_visibility ";
            // check if a direction is given?
            if (checkDirection(tempstr, current))
            {
              temp << "dir_" << tempstr << " ";
              it++;
              current = *it;
            }

            if (getRmkVisibility(tempstr, current))
            {
               temp << tempstr;
               say(temp);
            }
*/
            break;

         case FROPA:
            break;

         case LIGHTNING:
            temp << "ltg " << getLightning(current);
            say(temp);
            break;

         case VIRGA:
            break;

         case CEILING:
            nceiling = true;
            break;

         case DAYTEMPMAX:
            temp << "max_daytemp " << getTempTime(current);
            say(temp);
            break;

         case DAYTEMPMIN:
            temp << "min_daytemp " << getTempTime(current);
            say(temp);
            break;

         case FLIGHTLEVEL:
            current.erase(0,2);
            temp << "flightlevel " << current;
            say(temp);
            break;

         case WORDSINRMK:
            temp << "say " << current;
            say(temp);
            break;

         case TEMPOOBSCURATION:
            temp << "tempo_obscuration " << current.substr(-4,2)
                 << " " << current.substr(-2,2);
            say(temp);
            break;

         case TEMPINRMK:
            temp << "rmk_tempdew " << getTempinRmk(current);
            say(temp);
            break;

         case MINMAXTEMP:
            temp << "rmk_minmaxtemp " << getTempinRmk(current);
            say(temp);
            break;

         case MINTEMP:
            temp << "rmk_mintemp " << getTemp(current);
            say(temp);
            break;

         case MAXTEMP:
            temp << "rmk_maxtemp " << getTemp(current);
            say(temp);
            break;

         case PRESSURETENDENCY:
            temp << "rmk_pressure " << getPressureinRmk(current);
            say(temp);
            break;

         case PRECIPITATION1:
            temp << "rmk_precipitation 1 " << getPrecipitationinRmk(current);
            say(temp);
            break;

         case PRECIPITATION6:
            temp << "rmk_precipitation 6 " << getPrecipitationinRmk(current);
            say(temp);
            break;

         case PRECIPITATION24:
            temp << "rmk_precipitation 24 " << getPrecipitationinRmk(current);
            say(temp);
            break;

         case PRECIPINRMK:
            cout << "PRECIPINRMK\n";
            temp << "rmk_precip " << getPrecipitation(current);
            say(temp);
            break;

         case NOSEVEREWX:
            temp << "say " << current;
            say(temp);
            break;

         case CLOUDTYPE:
            temp << "cloudtypes" << getCloudType(current);
            say(temp);
            break;

         case QFEINRMK:
            temp << "qfe " << current.erase(0,3);
            say(temp);
            break;

         case MAINTENANCE:
            temp << "say maintenance_needed";
            say(temp);
            break;

         case INVALID:
            break;

         case END:
            endflag = true;
            break;

         default:
            break;
     }
  //   cout << current << endl;
     it++;
   }
   return 1;
}


// here we check the current METAR-token with regex
// function, it returns the type (temperature, dewpoint, clouds, ...)
int ModuleMetarInfo::checkToken(std::string token)
{
    regex_t re;
    int retvalue = INVALID;
    typedef std::map<std::string, int> Mregex;
    Mregex mre;

    map<string, int>::iterator rt;

    mre["^[0-9]/[0-9]sm$"]                           = ISPARTOFMILES;
    mre["^(a|q)([0-9]{4})$"]                         = QNH;
    mre["^([0-9]{3}|vrb)([0-9]{2}g)?([0-9]{2})(kt|mph|mps|kph)"] = WIND; // wind
    mre["^[0-9]{4}(ndv|n|ne|e|se|s|sw|w|nw)?$"]      = ISVIEW;   // view
    mre["^[0-9]{6}z"]                                = UTC;
    mre["^[0-9]{1,2}sm$"]                            = ISVIEW;
    mre["^[0-9]{3}v[0-9]{3}$"]                         = VALUEVARIES;
    mre["^(m)?(//|[0-9]{2})/(m)?(//|[0-9]{2})$"]     = TEMPERATURE;
    mre["^(cavok|tcu)$"]                               = WORDSEXT;
    mre["(becmg|nosig)"]                             = FORECAST;
    mre["^(all|ws|clr|rwy|skc|nsc|tempo|ocnl|frq|nsw|cons)$"] = WORDSNOEXT;
    mre["^(fm|tl|at)([0-9]{4})z$"]                     = TIME;
    mre["^((few|sct|bkn|ovc)[0-9]{3})(///)?(ac|acc|as|cb|cbmam|cc|cf|ci|cs|cu|tcu|ns|sc|sf|st)?"] = CLOUDSVALID;
    mre["^r[0-3][0-9](ll|l|c|r|rr)?/(p|m)?([0-9]{4})(v(p|m)[0-9]{4})?(u|d|n)?(ft)?$"] = RVR;
    mre["^r[0-8][0-9](ll|l|c|r|rr)?/([0-9]|/|c)([1259]|/|l)([0-9]|/|r)([0-9]|/|d)([0-9]|/){2}$"] = ALLRWYSTATE;
    mre["^vv[0-9]{3}$"]                              = VERTICALVIEW;
    mre["^(\\+|\\-|vc|re)?([bdfimprstv][a-z]){1,2}$"]= ACTUALWX;
    mre["^rwy[0-9]{2}(ll|l|c|r|rr)?$"]               = RUNWAY;
    mre["^cig$"]                                       = CEILING;
    mre["^[1-9]$"]                                   = IS1STPARTOFVIEW;
    mre["^rmk$"]                                       = RMK;
    mre["^slp"]                                        = SLP;
    mre["^snoclo$"]                                    = SNOWCLOSED;
    mre["^wnd$"]                                       = PEAKWIND;
    mre["^auto$"]                                      = AUTO;
    mre["^nospeci$"]                                   = NOSPECI;
    mre["^ao[1|2]$"]                                   = AUTOTYPE;
    mre["^wshft$"]                                     = WINDSHIFT;
    mre["^vis$"]                                       = RMKVISIBILITY;
    mre["^fropa"]                                      = FROPA;
    mre["^ltg[ciag]{2,8}$"]                            = LIGHTNING;
    mre["^virga$"]                                     = VIRGA;
    mre["^tx(m)?[0-9]{2}/[0-9]{2}z$"]                  = DAYTEMPMAX;
    mre["^tn(m)?[0-9]{2}/[0-9]{2}z$"]                  = DAYTEMPMIN;
    mre["^fl[0-9]?[0-9]{2}$"]                          = FLIGHTLEVEL;
    mre["^tempo[0-9]{4}$"]                             = TEMPOOBSCURATION;
    mre["^t[0-1][0-9]{3}[0-1][0-9]{3}$"]               = TEMPINRMK;
    mre["^1[0-9]{4}$"]                                 = MAXTEMP;
    mre["^2[0-9]{4}$"]                                 = MINTEMP;
    mre["^4[0-9]{8}$"]                                 = MINMAXTEMP;
    mre["^5[0-9]{4}$"]                                 = PRESSURETENDENCY;
    mre["^p(cpn)?[0-9]{4}$"]                           = PRECIPITATION1;
    mre["^6[0-9]{4}$"]                                 = PRECIPITATION6;
    mre["^7[0-9]{4}$"]                                 = PRECIPITATION24;
    mre["^(tsno|fzrano)$"]                             = NOSEVEREWX;
    mre["^qfe[0-9]{3}\\.[0-9]$"]                       = QFEINRMK;
    mre["^[\\$]$"]                                     = MAINTENANCE;
    mre["^[a-z]{2,4}(b|e)([0-9]{2}){1,2}(e[0-9]{2,4})?$"]  = PRECIPINRMK;
    mre["^((ac|acc|as|cb|cbmam|cc|cf|ci|cs|cu|tcu|ns|sc|sf|st)[1-8]){1,4}$"] = CLOUDTYPE;
    mre["^(mar|alqds|mod|twr|sfc|dsnt|lan|loc|fir|presrr|presfr|abv|agl|btn|cld|cot|nil|obs|obsc|stnr|turb|valid|wkn|wspd|ltg|wx)$"] = WORDSINRMK;

    for (rt = mre.begin(); rt != mre.end(); rt++)
    {
       if (rmatch(token, rt->first, &re))
       {
           retvalue = rt->second;
           break;
       }
    }
    regfree(&re);

    return retvalue;
} /* checkToken */


/*
Cloud-ground 	CG 	Lightning between cloud and ground.
In-cloud 	IC 	Lightning within the cloud.
Cloud-cloud 	CC 	Streaks of lightning reaching from one cloud to another.
Cloud-air 	CA 	Streaks of lightning which pass from a cloud to the air, but do not strike the ground.
*/
std::string ModuleMetarInfo::getLightning(std::string token)
{
    stringstream ss;
    unsigned int a;

    for (a=0; a<token.length(); a+=2)
    {
       ss << "ltg_" << token.substr(a,2) << " ";
    }

    return ss.str();
} /* getLightning */


std::string ModuleMetarInfo::getCloudType(std::string token)
{
   stringstream ss;
   int a;

   while (token.length() > 0)
   {
     for (a=0; a<15; a++)
     {
        if (token.find(clouds[a],0) != string::npos)
        {
           ss << " cld_" << clouds[a] << " ";
           token.erase(0,clouds[a].length());
           ss << token.substr(0,1);
           token.erase(0,1);
        }
     }
   }

   return ss.str();
} /* getCloudType */


std::string ModuleMetarInfo::getTempinRmk(std::string token)
{
   stringstream ss;

   (token.substr(1,1) == "1") ? ss << "-" :ss << "";
   ss << atoi(token.substr(2,2).c_str()) << "." << token.substr(4,1);
   (token.substr(5,1) == "1") ? ss << " -" :ss << " ";
   ss << atoi(token.substr(6,2).c_str()) << "." << token.substr(8,1);

   return ss.str();
} /* getTemoinRmk */


bool ModuleMetarInfo::getPeakWind(std::string &retval, std::string token)
{
   stringstream ss;
   StrList tlist;

   // Format: PK WND dddff(f)/(hh)mm
   if (token.length() < 8 || token.length() > 11) return false;

   splitStr(tlist, token, "/");
   ss << tlist[0].substr(0,3) << " ";   // direction
   ss << tlist[0].substr(3,2) << " ";   // velocity

   if (tlist[1].length() == 4)
   {
       ss << tlist[1].substr(0,2) << " " << tlist[1].substr(2,2);
   }
   else
   {
       ss << "XX " << tlist[1].substr(0,2);
   }
   retval = ss.str();

   return true;
} /* getPeakWind */


bool ModuleMetarInfo::getRmkVisibility(std::string &retval, std::string token)
{
   stringstream ss;
   // to do
   return true;
} /* getRmkVisibility */


std::string ModuleMetarInfo::getTemp(std::string token)
{
   stringstream ss;
   (token.substr(1,1) == "1") ? ss << "-" :ss << "";
   ss << atoi(token.substr(2,2).c_str()) << "." << token.substr(4,1);

   return ss.str();
} /* getTemp */


// RMK section, Characteristics of Barometer Tendency
std::string ModuleMetarInfo::getPressureinRmk(std::string token)
{
   stringstream ss;
   typedef map<char, std::string> Digits;
   Digits ptrend;

   ptrend['0'] = "increasing then decreasing";
   ptrend['1'] = "increasing then steady";
   ptrend['2'] = "increasing steadily or unsteadily";
   ptrend['3'] = "decreasing or steady then increasing or increasing more_rapidly";
   ptrend['4'] = "steady";
   ptrend['5'] = "decreasing then increasing";
   ptrend['6'] = "decreasing then steady or decreasing slowly";
   ptrend['7'] = "decreasing steadily or unsteadily";
   ptrend['8'] = "steady or increasing then decreasing or decreasing more_rapidly";
   ptrend['9'] = "not reported";
   ptrend['/'] = "not reported";

   ss << atoi(token.substr(2,2).c_str()) << "." << token.substr(4,1) << " "
      << ptrend[token.substr(1,1)[0]];
   return ss.str();
} /* getPressureinRmk */


std::string ModuleMetarInfo::getPrecipitationinRmk(std::string token)
{
   stringstream ss;
   ss << atoi(token.substr(1,2).c_str()) << "." << token.substr(3,2);
   return ss.str();
} /* getPrecipitationinRmk */


std::string ModuleMetarInfo::getTempTime(std::string token)
{
    stringstream ss;

    token.erase(0,2);
    if (token.substr(0,1) == "m")
    {
       ss << "-";
       token.erase(0,1);
    }
    ss << atoi(token.substr(0,2).c_str()) << " "
       << token.substr(3,2);
    return ss.str();
} /* getTempTime */


// sea level pressure
// Format: SLPppp
std::string ModuleMetarInfo::getSlp(std::string token)
{
    stringstream ss;

    (atoi(token.substr(3,1).c_str()) > 6) ? ss << "9" : ss << "10";
    ss << token.substr(3,2) << "." << token.substr(5,1);
    return ss.str();
} /* getSlp */


void ModuleMetarInfo::isRwyState(std::string &retval, std::string token)
{
   typedef map<char, std::string> Digits;
   Digits rwydeposit, contamination;

   typedef map<std::string, std::string> Rwydefs;
   Rwydefs friction, deposit;

   std::map <string, string>::iterator tt;
   stringstream ss;

   StrList tlist;

   deposit["//"] = "deposit_depth not reported";
   deposit["92"] = "deposit_depth 10 unit_cms";
   deposit["93"] = "deposit_depth 15 unit_cms";
   deposit["94"] = "deposit_depth 20 unit_cms";
   deposit["95"] = "deposit_depth 25 unit_cms";
   deposit["96"] = "deposit_depth 30 unit_cms";
   deposit["97"] = "deposit_depth 35 unit_cms";
   deposit["98"] = "deposit_depth 40 unit_cms";
   deposit["99"] = "runways not operational";

   friction["//"] = "breaking_action not reported";
   friction["91"] = "breaking_action poor";
   friction["92"] = "breaking_action medium_poor";
   friction["93"] = "breaking_action medium";
   friction["94"] = "breaking_action medium_good";
   friction["95"] = "breaking_action good";
   friction["96"] = "breaking_action good";
   friction["97"] = "breaking_action good";
   friction["98"] = "breaking_action good";
   friction["99"] = "breaking_action figures_unreliable";

   rwydeposit['0'] = "clear_and_dry";
   rwydeposit['1'] = "damp";
   rwydeposit['2'] = "wet_or_water_patches";
   rwydeposit['3'] = "rime_or_frost_covered";
   rwydeposit['4'] = "dry sn";
   rwydeposit['5'] = "wet sn";
   rwydeposit['6'] = "slush";
   rwydeposit['7'] = "ic";
   rwydeposit['8'] = "compacted_or_rolled sn";
   rwydeposit['9'] = "frozen_ruts_or_ridges";
   rwydeposit['/'] = "deposit not reported";

   contamination['1'] = "contamination less_or_equal 10 percent";
   contamination['2'] = "contamination 11 to 25 percent";
   contamination['5'] = "contamination 26 to 50 percent";
   contamination['9'] = "contamination 51 to 100 percent";
   contamination['/'] = "contamination not reported";

   // Runway designator
   int trwy = atoi(token.substr(1,2).c_str());  //RXXxxxxxx
   if (trwy < 50)
   {
       ss << "runway " << token.substr(1,2);
       splitStr(tlist, token, "/");

       if (tlist[0].length() > 3)
       {
           tlist[0].erase(0,3);
           ss << " " << shdesig[tlist[0]];
           token.erase(0, tlist[0].length());
       }
   }
   else if (trwy < 88) ss << "runway " << (trwy-50) << " right ";
   else if (trwy == 88) ss << "all runways ";

   token.erase(0,3);

   if (token.find("clrd") != string::npos)
   {
      ss << " clrd";
      retval = ss.str();
      return;
   }

   // runway deposit type
   ss << " " << rwydeposit[token.substr(1,1)[0]];

   // runway contamination
   ss << " " << contamination[token.substr(2,1)[0]];

   // depth of deposit
   if (atoi(token.substr(3,1).c_str()) > 91 || token.substr(3,2) == "//")
   {
      tt = deposit.find(token.substr(3,2));
      ss << " " << tt->second << " ";
   }
   else
   {
       ss << " deposit_depth ";
       if (atoi(token.substr(3,2).c_str()) == 0)
       {
           ss << "less_than 1 unit_mm ";
       }
       else
       {
           ss << " " << atoi(token.substr(3,2).c_str()) << " unit_mms ";
       }
   }

   // friction coeffizient
   if (atoi(token.substr(5,2).c_str()) > 90 || token.substr(5,2) == "//")
   {
       tt = friction.find(token.substr(5,2));
       ss << " " << tt->second;
   }
   else
   {
       ss << " friction_coefficient 0." << atoi(token.substr(5,2).c_str());
   }

   retval = ss.str();
} /* isRwyState */


/* beginning an ending of Precipitation */
std::string ModuleMetarInfo::getPrecipitation(std::string token)
{

   // example: SHRABO5E30SHSNB20E55
   stringstream ss;
   std::string tstr;
   std::string newstr = "";
   size_t found;
   string::size_type pos = 0;
   string::size_type len;

   std::map <string, string>::iterator tt;

   // first detect the type of precipitation and separate it
   // SHRABO5E30SHSNB20E55 -> SH RA BO5E30 SH SN B20E55
   for (short a=0; a < 61; a++)
   {
      found = token.find(desc[a],0);
      if (found != string::npos)
      {
          tt = repstr.find(desc[a]);
          if (tt != repstr.end())
          {
              tstr = tt->second;
          }
          else
          {
            tstr = desc[a] + " ";
          }
          token.replace(found, desc[a].length(), tstr);
          tstr = "";
      }
   }


   // then we catch the rest by detecting the beginning (b) or ending (e) of
   // precipitation
   // SH RA BO5E30 SH SN B20E55 -> SH RA began_at 05 endet_at 30 sh sn began_at ...
   len = token.size();
   while (pos < len)
   {
      if ((token.substr(pos,1)[0] == 'b' || token.substr(pos,1)[0] == 'e') &&
            token.substr(pos+1,1)[0] >= '0' && token.substr(pos+1,1)[0] <= '9')
      {
          (token.substr(pos,1)[0] == 'b') ? newstr += " began_at " : newstr += " ended_at ";
          newstr += token.substr(pos+1, 2) + " ";
          pos += 3;
      }
      else
      {
         newstr += token[pos];
         pos++;
      }
   }

   return newstr;
} /* getRecipitation */


bool ModuleMetarInfo::isActualWX(std::string &retval, std::string token)
{
  stringstream ss;
   std::map <string, string>::iterator tt;

   // +SHRA means "heavy rain shower"
   if (token.substr(0,1) == "+")
   {
      ss << "heavy ";
      token.erase(0,1);
   }
   // -RA means "light rain"
   else if (token.substr(0,1) == "-")
   {
      ss << "light ";
      token.erase(0,1);
   }
   // vc -> in the vicinity
   else if (token.substr(0,2) == "vc")
   {
      ss << "vicinity ";
      token.erase(0,2);
   }
   // re -> recent
   else if (token.substr(0,2) == "re")
   {
      ss << "recent ";
      token.erase(0,2);
   }
   else ss << "moderate ";  // RA -> moderate rain

   for (short a = 0; a < 61; a++)
   {
      if (token.find(desc[a],0) != string::npos)
      {
         if (token.length() == 2)
         {
             ss << token;
             retval = ss.str();
             return true;
         }
         else  // e.g. SHRA, SHSN ...
         {
            tt = repstr.find(desc[a]);
            if (tt != repstr.end())
            {
                ss << tt->second;
            }
            else
            {
                ss << token.substr(0,2) << " " << token.substr(2,2);
            }
            retval = ss.str();
            return true;
         }
      }
   }
   return false;
} /* isActualWX */


// needed by regex
bool ModuleMetarInfo::rmatch(std::string tok, std::string pattern, regex_t *re)
{
  int status;

  if (( status = regcomp(re, pattern.c_str(), REG_EXTENDED)) != 0 )
  {
    return false;
  }

  bool success = (regexec(re, tok.c_str(), 0, NULL, 0) == 0);
  regfree(re);
  return success;

} /* rmatch */


bool ModuleMetarInfo::isvalidUTC(std::string utctoken)
{

   // token e.g.: 2016-08-10T08:20:00Z
   if (utctoken.length() < 16)
   {
     return false;
   }

   time_t rawtime;
   struct tm mtime;          // time of METAR
   struct tm *utc;           // actual time as UTC
   double diff;

   rawtime = time(NULL);
   utc = gmtime(&rawtime);

   mtime.tm_sec  = 0;
   mtime.tm_min  = atoi(utctoken.substr(14,2).c_str());
   mtime.tm_hour = atoi(utctoken.substr(11,2).c_str());
   mtime.tm_mday = atoi(utctoken.substr(8,2).c_str());
   mtime.tm_mon  = atoi(utctoken.substr(5,2).c_str()) - 1;
   mtime.tm_year = atoi(utctoken.substr(0,4).c_str()) - 1900;

   diff = difftime(mktime(utc),mktime(&mtime));

   if (debug)
   {
     cout << "UTC: " << utc->tm_hour << ":" << utc->tm_min << ":"
          << utc->tm_sec << " daytime saving:" << utc->tm_isdst
          << " vs " << mtime.tm_hour << ":" << mtime.tm_min
          << ":" << mtime.tm_sec << endl;
   }
   if (diff > 7200) return false;

   return true;
} /* isvalidUTC */


bool ModuleMetarInfo::isView(std::string &retval, std::string token)
{
   stringstream ss;

   // view given in km
   if (token.find("km", 0) != string::npos)
   {
      ss << token.substr(0,token.find("km")) << " unit_kms";
      token.erase(0,token.find("km")+2);
   }

   // view more than 10km
   else if (token.substr(0,4) == "9999")  // -> more than 10 km
   {
      ss << "more_than 10 unit_kms";
      token.erase(0,4);
   }

   else if (token.substr(0,4) == "0000")  // -> less than 50m
   {
      ss << "less_than 50 unit_meters";
      token.erase(0,4);
   }

   else if (token.find("sm",0) != string::npos )  // unit are statue miles
   {
      ss << token.substr(0,token.find("sm")) << " unit_mile";
      if (atoi(token.substr(0,token.find("sm")).c_str()) != 1) ss << "s";
      token.erase(0,token.find("sm")+2);  // deletes all to "sm"
   }

   // if less than 5000 m the visibility will be give out in meter, above
   // in kilometers
   else if (token.length() >= 4 && atoi(token.substr(0,4).c_str()) > 4999)
   {
      ss << atoi(token.substr(0,4).c_str())/1000 << " unit_kms";
      token.erase(0,4);
   }
   else if (token.length() >= 4 && atoi(token.substr(0,4).c_str()) < 5000
                                 && atoi(token.substr(0,4).c_str()) > 1 )
   {
      ss << atoi(token.substr(0,4).c_str()) << " unit_meters";
      token.erase(0,4);
   }
   else return false;

   // appendix ndv -> "no directional variation"
   if (token.find("ndv",0) != string::npos)
   {
      ss << " ndv";
      token.erase(0,3);
   }

   if (!token.empty())   // rest is a direction like N or NE or S ,...
   {
     ss << " dir_" << token;
   }

   retval = ss.str();
   return true;
} /* isView */

// That's why I love SI-units!
void ModuleMetarInfo::isPartofMiles(std::string &retval, std::string token)
{

   if (token.find("1/16",0)!= string::npos) retval = "0.0625";
   if (token.find("1/8",0) != string::npos) retval = "0.125";
   if (token.find("3/16",0)!= string::npos) retval = "0.1875";
   if (token.find("1/4",0) != string::npos) retval = "0.25";
   if (token.find("3/8",0) != string::npos) retval = "0.375";
   if (token.find("5/16",0)!= string::npos) retval = "0.3125";
   if (token.find("1/2",0) != string::npos) retval = "0.5";
   if (token.find("5/8",0) != string::npos) retval = "0.625";
   if (token.find("3/4",0) != string::npos) retval = "0.75";
   if (token.find("7/8",0) != string::npos) retval = "0.875";

} /* isPartofMile */


bool ModuleMetarInfo::isWind(std::string &retval, std::string token)
{
   stringstream ss;
   std::string unit;

   // detect the unit
   if (token.substr(token.length()-2,2) == "kt") unit = "unit_kts";
   else if (token.substr(token.length()-3,3) == "mps") unit = "unit_mps";
   else if (token.substr(token.length()-3,3) == "mph") unit = "unit_mph";
   else if (token.substr(token.length()-3,3) == "kph") unit = "unit_kph";
   else return false;

   // wind is calm
   if (token.substr(0,5) == "00000") ss << "calm";

   // wind is variable
   else if (token.substr(0,3) == "vrb")
   {
      ss << "variable " << token.substr(3,2) << " " << unit;
   }
   // degrees and velocity
   else
   {
      ss << token.substr(0,3) << " " << token.substr(3,2) << " " << unit;
   }

   // do we have gusts?
   if (token.find("g",3) != string::npos)
   {
      ss << " " << token.substr(token.length()-4,2) << " " << unit;
   }

   retval = ss.str();
   return true;
} /* isWind */


bool ModuleMetarInfo::isQnh(std::string &retval, std::string token)
{
    stringstream ss;

    // inches or hPa?
    switch (token.substr(0,1).c_str()[0]) {

      case 'q':
        ss << "qnh " << atoi(token.substr(1,4).c_str());
        break;

      case 'a':
        ss << "altimeter "<< token.substr(1,2) << "." << token.substr(3,2);
        break;

      default:
        return false;
    }
    retval = ss.str();
    return true;
} /* isQnh */


void ModuleMetarInfo::isValueVaries(std::string &retval, std::string token)
{
   stringstream ss;

   ss << token.substr(0,3) << " " << token.substr(4,3);
   retval = ss.str();
} /* isValueVaries */


void ModuleMetarInfo::isTime(std::string &retval, std::string token)
{
   stringstream ss;
   std::map <string, string>::iterator tt;

   tt = shdesig.find(token.substr(0,2));  // fm -> from,  tl -> until
   ss << tt->second;
   ss << " " << token.substr(2,4);
   retval = ss.str();
} /* isTime */


void ModuleMetarInfo::validTemp(std::string &retval, std::string token)
{
   stringstream ss;

   // temp is not measured
   if (token.substr(0,2) == "//")
   {
       ss << "not";
   }
   else
   {
   if (token.substr(0,1) == "m")
   {
        ss << "-";
      token.erase(0,1);
   }
     ss << atoi(token.substr(0,2).c_str());
   }
   retval = ss.str();
} /* validTemp */


void ModuleMetarInfo::validDp(std::string &retval, std::string token)
{
   stringstream ss;

   // dewpoint is not reported?
   if (token.substr(token.length()-2,2) == "//")
   {
       ss << "not";
   }
   else
   {
     if (token.substr(token.length()-3,1) == "m") ss << "-";
     ss << atoi(token.substr(token.length()-2,2).c_str());
   }
   retval = ss.str();
} /* validDp */


bool ModuleMetarInfo::isRunway(std::string &retval, std::string token)
{
   stringstream ss;
   std::map <string, string>::iterator it;

   ss << token.substr(3,2);
   token.erase(0,5);

   if (token.length() > 0)
   {
     it = shdesig.find(token);
     ss << " " << it->second;
   }

   retval = ss.str();
   return true;
}


bool ModuleMetarInfo::isRVR(std::string &retval, std::string token)
{
   stringstream ss;
   StrList tlist;
   std::map <string, string>::iterator it;
   std::string unit;

   // RVR examples:
   //
   // R32/5000           -> RWY 3 2 5000 meter
   // R32L/M5000         -> RWY 3 2 left less than 5000m
   // R33LL/M5000VP6000N -> RWY 3 3 left out varies from less than 5000m to
   //                       more than 6000m no distinct tendency
   // R22C/M5000VP7000FTU-> RWY 22 central varies from less than 5000ft to
   //                       more than 7000ft upcoming

   // check unit, feet or meter
   if (token.find("ft") != string::npos)
      unit = " unit_feet ";
   else unit = " unit_meters ";

   int str_size = splitStr(tlist, token, "/");

   // now we have
   // tlist[0]:    R32L
   // tlist[1]:    M5000V6000U

   // we handle the first part, the runway
   ss << tlist[0].substr(1,2) << " ";
   tlist[0].erase(0,3);

   // searches for "ll", "l", "c" ... "rr"
   it = shdesig.find(tlist[0]);
   if (it != shdesig.end())
   {
     ss << it->second << " ";
   }

   ss << "rvr ";

   // visibility
   if (tlist[1].find("v") != string::npos)  // we have varying visibility?
   {
      ss << "varies_from ";
      it = shdesig.find(tlist[1].substr(0,1));
      if (it != shdesig.end())
      {
        ss << it->second << " ";
        tlist[1].erase(0,1);
      }
      // visibility in meter
      ss << atoi(tlist[1].substr(0,4).c_str()) << unit << "to ";
      tlist[1].erase(0,5);
   }

   // p or m -> more_than, less_than
   it = shdesig.find(tlist[1].substr(0,1));
   if (it != shdesig.end())
   {
      ss << it->second << " ";
      tlist[1].erase(0,1);
   }

   // visibility in meter
   ss << atoi(tlist[1].substr(0,4).c_str()) << unit;
   tlist[1].erase(0,4);

   // do we have "u", "d"... ??
   if (tlist[1].length() > 0)
   {
     ss << shdesig[(tlist[1].substr(0,1))];
   }

   // is special cases we have /U or /D
   // no standard
   if (str_size == 3)
   {
     ss << shdesig[(tlist[2].substr(0,1))];
   }

   retval = ss.str();
   return true;
} /* isRVR */


void ModuleMetarInfo::isVerticalView(std::string &retval, std::string token)
{
    stringstream ss;

    // VV010  -> vertical view = 1000ft
    ss << atoi(token.substr(2,3).c_str()) * 100;
    retval = ss.str();
} /* isVerticalView */


bool ModuleMetarInfo::ispObscurance(std::string &retval, std::string token)
{
  stringstream ss;

  // e.g. SCT/// -> not measured
  if (token.find("///") != string::npos && token.length() == 6 ) return false;

  // cloud layer
  ss << token.substr(0,3) << " ";
  token.erase(0,3);

  ss << atoi(token.substr(0,3).c_str()) * 100;
  token.erase(0,3);

  if (token.length() > 0 && token.find("/") == string::npos)
  {
     ss << " cld_" << token << longmsg; // cb or tcu
  }

  retval = ss.str();
  return true;
} /* ispObscurance */


void ModuleMetarInfo::say(stringstream &tmp)
{
   if (debug) cout << tmp.str() << endl;  // debug
   processEvent(tmp.str());
   tmp.str("");
} /* say */


// special split function
int ModuleMetarInfo::splitEmptyStr(StrList& L, const string& seq)
{
  L.clear();
  const string delims = "*";
  string str, laststr;
  int a;
  string::size_type pos = 0;
  string::size_type len = seq.size();
  while (pos < len)
  {
      // Init/clear the token buffer
    str = "";
    a = 0;

      // remove any delimiters including optional (white)spaces
    while ((delims.find(seq[pos]) != string::npos) && (pos < len))
    {
      pos++;
      a++;
    }

      // Save token data
    while ((delims.find(seq[pos]) == string::npos) && (pos < len))
    {
      str += seq[pos++];
    }

    while (a > 1)
    {
       L.push_back(laststr);
       a--;
    }
    if (!str.empty()) {
       L.push_back(str);
       laststr = str;
    }
  }

  return L.size();

} /* ModuleMetarInfo::splitStr */


/*
 * This file has not been truncated
 */
