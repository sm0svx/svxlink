/*
@file	 ModuleMetarInfo.cpp
@brief   gives out a METAR report
@author  Adi Bier / DL1HRC
@date	 2009-10-14

\verbatim
A module (plugin) to request the latest METAR (weather) information from
by using ICAO shortcuts.
Look at http://en.wikipedia.org/wiki/METAR for further information

Copyright (C) 2009  Tobias Blomberg / SM0SVX ( & Adi )

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

#include <cstdio>
#include <cstdlib>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <time.h>
#include <algorithm>
#include <regex.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncTcpClient.h>
#include <AsyncConfig.h>
#include <version/MODULE_METARINFO.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ModuleMetarInfo.h"



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
#define WINDVARIES 11
#define UTC 12
#define AUTO 13
#define QNH 14
#define RVR 15
#define WORDSNOEXT 16
#define WORDSEXT 17
#define ISPARTOFMILES 18
#define RUNWAY 19
#define FORECAST 20

#define END 50

#define NOTMEASURED 97
#define NOTACTUAL 98
#define INVALID 99


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


ModuleMetarInfo::ModuleMetarInfo(void *dl_handle, Logic *logic, const string& cfg_name)
  : Module(dl_handle, logic, cfg_name)
{
  cout << "\tModule MetarInfo v" MODULE_METARINFO_VERSION " starting...\n";

} /* ModuleMetarInfo */


ModuleMetarInfo::~ModuleMetarInfo(void)
{
   delete con;
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

  shdesig["l"] = "left";
  shdesig["r"] = "right";
  shdesig["c"] = "center";
  shdesig["ll"]= "left_out";
  shdesig["rr"]= "right_out";
  shdesig["m"] = "less_than";
  shdesig["p"] = "more_than";
  shdesig["d"] = "decreasing";
  shdesig["u"] = "increasing";
  shdesig["n"] = "no_distinct_tendency";
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
  const char *pos;
  std::string spos;
  StrList mtcmd;
  typedef map<char, std::string> Digits;
  Digits mypad;

  mypad['1'] = "1111111111";
  mypad['2'] = "2ABCCCCCCC";
  mypad['3'] = "3DEFFFFFFF";
  mypad['4'] = "4GHIIIIIII";
  mypad['5'] = "5JKLLLLLLL";
  mypad['6'] = "6MNOOOOOOO";
  mypad['7'] = "7PQRSSSSSS";
  mypad['8'] = "8TUVVVVVVV";
  mypad['9'] = "9WXYZZZZZZ";

  cout << "DTMF command received in module " << name() << ": " << cmd << endl;

  int icmd = atoi(cmd.c_str());

  if (cmd == "")
  {
     deactivateMe();
     return;
  }

  if (cmd == "0")                       // normal help
  {
     processEvent("say metarhelp");
     return;
  }

  else if (cmd == "01")                 // play configured airports
  {
     processEvent("say icao_available");

     tosay << "airports ";
     for (StrList::const_iterator it=aplist.begin(); it != aplist.end(); it++)
     {
        tosay << ++a << " " << *it << " ";
     }
     processEvent(tosay.str());
     tosay.str("");
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
        pos = (cmdit->substr(0,1)).c_str();
        spos= mypad[pos[0]];
        icao += spos.substr(cmdit->length(),1);
     }
  }

  // and the SVX-method
  else if (cmd.length() == 8 && cmd.find('*') == string::npos)
  {
     icao = "";
     for (a=0; a<8; a+=2)
     {
        pos = cmd.substr(a,1).c_str();
        spos= mypad[pos[0]];
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
     cout << "icao-code by dtmf-method: " << icao << "\n";
     openConnection();
  }
  else processEvent("say no_airport_defined");

} /* dtmfCmdReceived */


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
 *----------------------------------------------------------------------------
 * Method:    reportState
 * Purpose:   This function is called by the logic core when it wishes the
 *    	      module to report its state on the radio channel. Typically this
 *    	      is done when a manual identification has been triggered by the
 *    	      user by sending a "*".
 *    	      This function will only be called if this module is active.
 * Input:     None
 * Output:    None
 * Author:    Tobias Blomberg / SM0SVX
 * Created:   2005-08-28
 * Remarks:
 * Bugs:
 *----------------------------------------------------------------------------
 */
void ModuleMetarInfo::reportState(void)
{
    stringstream ss;
    ss << "status_report ";
    processEvent(ss.str());

} /* reportState */

/*
* establish a tcp-connection to the METAR-Server
*/
void ModuleMetarInfo::openConnection(void)
{
  std::string server = "weather.noaa.gov";

  con = new TcpClient(server, 80);
  con->connected.connect(slot(*this, &ModuleMetarInfo::onConnected));
  con->disconnected.connect(slot(*this, &ModuleMetarInfo::onDisconnected));
  con->dataReceived.connect(slot(*this, &ModuleMetarInfo::onDataReceived));
  con->connect();

} /* openConnection */


int ModuleMetarInfo::onDataReceived(TcpConnection *con, void *buf, int count)
{
   std::string current;
   std::string tempstr;
   std::stringstream temp;
   StrList values;
   bool is_false = false;
   bool endflag = false;
   bool isrvr = false;
   float temp_view = 0;
   unsigned int found;
   int metartoken;

   char *metarinput = static_cast<char *>(buf);
   string html(metarinput, metarinput + count);

   metartoken = 0;            // start with UTC

   // This is a MEATAR-report:
   //
   // 2009/04/07 13:20
   // FBJW 071300Z 09013KT 9999 FEW030 29/15 Q1023 RMK ...
   //
   // don't worry, it's always the same structure...
   cout << html; // debug

   // \n -> <SPACE>
   while ((found = html.find('\n')) != string::npos) html[found] = ' ';

   temp << "airport " << icao;
   processEvent(temp.str());
   temp.str("");

   if (html.find("404 Not Found") != string::npos)
   {
      temp << "no_such_airport";
      say(temp);
      return -1;
   }
   else
   {
      temp << "Airport";
      say(temp);
   }

   // check if METAR is actual
   if (!isvalidUTC(html.substr(0,16)))
   {
      temp << "metar_not_valid";
      say(temp);
      return -1;
   }

   splitStr(values, html, " ");
   StrList::const_iterator it = values.begin();
   it++;
   it++;
   it++;

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
            temp << "metreport_time " << splitStrAll(current.substr(2,4));
            break;

         case AUTO:
          //  temp << " automatic_station ";
            break;

         case WIND:
            isWind(tempstr, current);
            temp << tempstr;
            break;

         case WINDVARIES:
            isWindVaries(tempstr, current);
            temp << tempstr;
            break;

         case IS1STPARTOFVIEW:
            temp_view = atof(current.c_str());
            break;

         case ISPARTOFMILES:
            if (isPartofMiles(tempstr, current))
            {
              temp_view += atof(tempstr.c_str());
              temp << " visibility " << flsplitStr(temp_view) << " miles";
            }
            break;

         case ISVIEW:
            if (isView(tempstr, current))
              temp << " visibility " << tempstr;
            break;

         case WORDSEXT:
            temp << " " << current << longmsg;
            break;

         case WORDSNOEXT:
            temp << " " << current << " ";
            break;

         case CLOUDSVALID:
            if (ispObscurance(tempstr, current))
            {
              if (!is_false)     // only once
              {
                 temp << " clouds";
                 is_false = true;
              }
              temp << " " << tempstr;
            }
            break;

         case VERTICALVIEW:
            if (isVerticalView(tempstr, current))
               temp << " " << tempstr;
            break;

         case ACTUALWX:
            if (isActualWX(tempstr, current))
               temp << " " << tempstr;
            break;

         case RVR:
            if (isRVR(tempstr, current))
            {
              if (!isrvr)
              {
                  temp << " rvr";
                  isrvr = true;
              }
              temp << tempstr;
            }
            break;

         case TEMPERATURE:
            if (validTemp(tempstr, current))
              temp << " " << tempstr;
            if (validDp(tempstr, current))
              temp << " " << tempstr;
            break;

         case QNH:
            isQnh(tempstr, current);
            temp << tempstr;
            is_false = false;
            break;

         case RUNWAY:
            isRunway(tempstr, current);
            temp << tempstr;
            break;

         case TIME:
            isTime(tempstr, current);
            temp << tempstr;
            break;

         case FORECAST:
            temp << " trend " << current << "_long ";
            break;

         case INVALID:
           // it++;
            break;

         case END:
            endflag = true;
            break;

         default:
          //  it++;
            break;
     }
     it++;
   }
   say(temp);
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
    mre["^[0-9]{3}v[0-9]{3}$"]                       = WINDVARIES;
    mre["^(m)?(//|[0-9]{2})/(m)?(//|[0-9]{2})$"]     = TEMPERATURE;
    mre["cavok"]                                     = WORDSEXT;
    mre["(becmg|nosig)"]                             = FORECAST;
    mre["^(all|ws|clr|rwy|skc|nsc|tempo)$"]          = WORDSNOEXT;
    mre["^(fm|tl)([0-9]{4})$"]                       = TIME;
    mre["^((few|sct|bkn|ovc)[0-9]{3})(///)?(cb|tcu)?"] = CLOUDSVALID;
    mre["r[0-9]{2}(ll|l|c|r|rr)?/(p|m)?([0-9]{4})(v(p|m)[0-9]{4})?(u|d|n)?(ft)?"] = RVR;
    mre["^vv[0-9]{3}$"]                              = VERTICALVIEW;
    mre["^(\\+|\\-|vc|re)?([bdfimprstv][a-z]){1,2}$"]= ACTUALWX;
    mre["^rwy[0-9]{2}(ll|l|c|r|rr)?$"]               = RUNWAY;
    mre["^[1-9]$"]                                   = IS1STPARTOFVIEW;
    mre["rmk"]                                       = END;
    mre["auto"]                                      = AUTO;

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


bool ModuleMetarInfo::isActualWX(std::string &retval, std::string token)
{
  stringstream ss;
  std::string desc[] = {
          "bcfg", "bldu", "blsa", "blpy", "blsn", "fzbr",
          "vcbr", "tsgr", "vcts", "drdu", "drsa", "drsn",
          "fzfg", "fzdz", "fzra", "prfg", "mifg", "shra",
          "shsn", "shpe", "shpl", "shgs", "shgr", "vcfg",
          "vcfc", "vcss", "vcds", "tsra", "tspe", "tspl",
          "tssn", "vcsh", "vcpo", "vcbldu","vcblsa","vcblsn",
          "br", "du", "dz", "ds", "fg", "fc", "fu", "gs",
          "gr", "hz", "ic", "pe", "pl", "po", "ra", "sn",
          "sg", "sq", "sa", "ss", "ts", "va", "py", "sh"};

   if (token.substr(0,1) == "+")
   {
      ss << "heavy ";
      token.erase(0,1);
   }
   else if (token.substr(0,1) == "-")
   {
      ss << "light ";
      token.erase(0,1);
   }
   else if (token.substr(0,2) == "vc")
   {
      ss << "vicinity ";
      token.erase(0,2);
   }
   else if (token.substr(0,2) == "re")
   {
      ss << "recent ";
      token.erase(0,2);
   }
   else ss << "moderate ";

   for (short a=0; a < 54; a++)
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
            if (token.find("sh") != string::npos) {
                ss << token.substr(2,2) << " " << token.substr(0,2);
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


bool ModuleMetarInfo::rmatch(std::string tok, std::string pattern, regex_t *re)
{
    int status;

    if (( status = regcomp(re, pattern.c_str(), REG_EXTENDED)) != 0 )
        return false;

    if (regexec(re, tok.c_str(), 0, NULL, 0) == 0) return true;
    else return false;
} /* rmatch */


bool ModuleMetarInfo::isvalidUTC(std::string token)
{

   time_t rawtime;
   struct tm mtime;              // time of METAR
   struct tm *utc;               // actual time as UTC
   double diff;

   rawtime = time(NULL);
   utc = gmtime(&rawtime);

   mtime.tm_sec  = 0;
   mtime.tm_min  = atoi(token.substr(14,2).c_str());
   mtime.tm_hour = atoi(token.substr(11,2).c_str()) + 1; // why??
   mtime.tm_mday = atoi(token.substr(8,2).c_str());
   mtime.tm_mon  = atoi(token.substr(5,2).c_str()) - 1;
   mtime.tm_year = atoi(token.substr(0,4).c_str()) - 1900;

   diff = difftime(mktime(utc),mktime(&mtime));

   if (diff > 3720) return false;

   return true;
} /* isvalidUTC */


bool ModuleMetarInfo::isView(std::string &retval, std::string token)
{
   stringstream ss;

   // view given in km
   if (token.find("km", 0) != string::npos)
   {
      ss << splitStrAll(token.substr(0,token.find("km"))) << " km";
      token.erase(0,token.find("km")+2);
   }

   // view more than 10km
   else if (token.substr(0,4) == "9999")  // -> more than 10 km
   {
      ss << "more_than 1 0 km";
      token.erase(0,4);
   }

   else if (token.substr(0,4) == "0000")  // not measured
   {
      token.erase(0,4);
      return false;
   }

   else if (token.find("sm",0) != string::npos )  // unit are statue miles
   {
      ss << splitStrAll(token.substr(0,token.find("sm"))) << " miles";
      token.erase(0,token.find("sm")+2);  // deletes all to "sm"
   }

   // if less than 5000 m the visibility will be give out in meters, above
   // in kilometers
   else if (token.length() >= 4 && atoi(token.substr(0,4).c_str()) > 4999)
   {
      ss << atoi(token.substr(0,4).c_str())/1000 << " km";
      token.erase(0,4);
   }
   else if (token.length() >= 4 && atoi(token.substr(0,4).c_str()) < 5000
                                 && atoi(token.substr(0,4).c_str()) > 1 )
   {
      ss << atoi(token.substr(0,4).c_str()) << " meters";
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
bool ModuleMetarInfo::isPartofMiles(std::string &retval, std::string token)
{
   stringstream ss;

   if (token.find("1/8",0) != string::npos) ss << "0.125";
   if (token.find("1/4",0) != string::npos) ss << "0.25";
   if (token.find("3/8",0) != string::npos) ss << "0.375";
   if (token.find("1/2",0) != string::npos) ss << "0.5";
   if (token.find("5/8",0) != string::npos) ss << "0.625";
   if (token.find("3/4",0) != string::npos) ss << "0.75";
   if (token.find("7/8",0) != string::npos) ss << "0.875";

   retval = ss.str();
   return true;
} /* isPartofMile */


int ModuleMetarInfo::isWind(std::string &retval, std::string token)
{
   stringstream ss;
   std::string unit;

   ss << "wind ";

   // detect the unit
   if (token.substr(token.length()-2,2) == "kt") unit = "kts";
   else if (token.substr(token.length()-3,3) == "mps") unit = "mps";
   else if (token.substr(token.length()-3,3) == "mph") unit = "mph";
   else if (token.substr(token.length()-3,3) == "kph") unit = "kph";
   else return false;

   // wind is calm
   if (token.substr(0,5) == "00000") ss << "calm";

   // wind is variable
   else if (token.substr(0,3) == "vrb")
   {
      ss << "variable " << splitStrAll(token.substr(3,2)) << unit;
   }
   // degrees and velocity
   else
   {
      ss << chHoundr(token.substr(0,3)) << " degrees " <<
            splitStrAll(token.substr(3,2)) << unit;
   }

   // do we have gusts?
   if (token.find("g",3) != string::npos)
   {
      ss << " gusts_up " << splitStrAll(token.substr(token.length()-4,2)) << unit;
   }

   retval = ss.str();
   return true;
} /* isWind */


bool ModuleMetarInfo::isQnh(std::string &retval, std::string token)
{
    stringstream ss;

    // inches or hPa?
    ss << " qnh ";
    switch (token.substr(0,1).c_str()[0]) {

      case 'q':
        ss << splitStrAll(token.substr(1,4)) << "hPa";
        break;

      case 'a':
        ss << splitStrAll(token.substr(1,2)) << ". "
           << splitStrAll(token.substr(3,2)) << "inches";
        break;

      default:
        return false;
    }
    retval = ss.str();
    return true;
} /* isQnh */


bool ModuleMetarInfo::isWindVaries(std::string &retval, std::string token)
{
   stringstream ss;

   ss << " wind varies_from " << chHoundr(token.substr(0,3))
      << "degrees to " << chHoundr(token.substr(4,3)) << "degrees ";

   retval = ss.str();
   return true;
} /* isWindVaries */


bool ModuleMetarInfo::isTime(std::string &retval, std::string token)
{
   stringstream ss;
   std::map <string, string>::iterator tt;

   tt = shdesig.find(token.substr(0,2));  // fm -> from,  tl -> until
   ss << tt->second;
   ss << " " << splitStrAll(token.substr(2,4));
   retval = ss.str();
   return true;
} /* isTime */


bool ModuleMetarInfo::validTemp(std::string &retval, std::string token)
{
   stringstream ss;

   // temp is not measured
   if (token.substr(0,2) == "//") return false;

   ss << "temperature ";
   if (token.substr(0,1) == "m")
   {
      ss << "minus ";
      token.erase(0,1);
   }
   ss << splitStrAll(chNumber(token.substr(0,2))) << "degrees";
   retval = ss.str();
   return true;
} /* validTemp */


bool ModuleMetarInfo::validDp(std::string &retval, std::string token)
{
   stringstream ss;

   // dewpoint is not measured
   if (token.substr(token.length()-2,2) == "//") return false;

   ss << "dewpoint ";
   if (token.substr(token.length()-3,1) == "m") ss << "minus ";

   ss << splitStrAll(chNumber(token.substr(token.length()-2,2)))
      << "degrees";
   retval = ss.str();
   return true;
} /* validDp */


bool ModuleMetarInfo::isRunway(std::string &retval, std::string token)
{
   stringstream ss;
   std::map <string, string>::iterator it;

   ss << " runway " << splitStrAll(token.substr(3,2));
   token.erase(0,5);

   if (token.length() > 0)
   {
     it = shdesig.find(token);
     ss << it->second << " ";
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
   // R22C/M5000VP7000FTU-> RWY 22 center varies from less than 5000ft to
   //                       more than 7000ft upcoming

   // check unit, feet or meter
   if (token.find("ft") != string::npos)
      unit = " feet ";
   else unit = " meters ";

   splitStr(tlist, token, "/");

   // now we have
   // tlist[0]:    R32L
   // tlist[1]:    M5000V6000U

   // we handle the first part, the runway
   ss << " runway " << splitStrAll(tlist[0].substr(1,2));
   tlist[0].erase(0,3);

   // searches for "ll", "l", "c" ... "rr"
   it = shdesig.find(tlist[0]);
   if (it != shdesig.end())
   {
     ss << it->second << " ";
   }

   // visibility
   if (tlist[1].find("v"))  // we have varying visibility?
   {
      ss << "varies_from ";
      it = shdesig.find(tlist[1].substr(0,1));
      if (it != shdesig.end())
      {
        ss << it->second << " ";
        tlist[1].erase(0,1);
      }
      // visibility in meter
      ss << chNumber(tlist[1].substr(0,4)) << unit << "to ";
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
   ss << chNumber(tlist[1].substr(0,4)) << unit;
   tlist[1].erase(0,4);

   // do we have "u", "d"... ??
   if (tlist[1].length() > 0)
   {
     ss << shdesig[(tlist[1].substr(0,1))];
   }

   retval = ss.str();
   return true;
} /* isRVR */


bool ModuleMetarInfo::isVerticalView(std::string &retval, std::string token)
{
    stringstream ss;

    // VV010  -> vertical view = 1000ft
    ss << " vertical_view " <<
          atoi(token.substr(2,3).c_str()) * 100 << " feet ";
    retval = ss.str();
    return true;
} /* isVerticalView */


bool ModuleMetarInfo::ispObscurance(std::string &retval, std::string token)
{
  stringstream ss;

  // e.g. SCT/// -> not measured
  if (token.find("///") != string::npos && token.length() == 6 ) return false;

  // cloud layer
  ss << token.substr(0,3) << " ";
  token.erase(0,3);

  ss << atoi(token.substr(0,3).c_str()) * 100 << " feet ";
  token.erase(0,3);

  if (token.length() > 0 && token.find("/") == string::npos)
  {
     ss << token << longmsg; // cb or tcu
  }

  retval = ss.str();
  return true;
} /* ispObscurance */


void ModuleMetarInfo::onConnected(void)
{
   char getpath[55];
   sprintf(getpath, "GET /pub/data/observations/metar/stations/%s.TXT\n",
           icao.c_str());
   if (!con->isConnected()) return;
   con->write(getpath, 55);
} /* onConnected */


void ModuleMetarInfo::onDisconnected(TcpConnection *con,
                     TcpClient::DisconnectReason reason)
{

} /* onDisconnect */


void ModuleMetarInfo::say(stringstream &tmp)
{
   std::stringstream tsay;
   tsay << "say " << tmp.str();
   cout << tsay.str() << "\n";  // debug
   processEvent(tsay.str());
   tmp.str("");
} /* say */


string ModuleMetarInfo::splitStrAll(const string seq)
{
   std::stringstream outStr;
   string::size_type pos = 0;
   while (pos < seq.size())
   {
      outStr << seq[pos] << " ";
      pos++;
   }
   return outStr.str();
} /* splitStrAll */


string ModuleMetarInfo::chNumber(const string seq)
{
    stringstream ss;
    ss << atoi(seq.c_str());
    return ss.str();
} /* chNumber */


string ModuleMetarInfo::chHoundr(const string seq)
{
   if (seq.substr(1,2) == "00")
     return seq + " ";
   return splitStrAll(seq);
} /* chHoundr */


string ModuleMetarInfo::flsplitStr(float val)
{
    stringstream ss;
    ss << val;
    return splitStrAll(ss.str());
} /* flsplitStr */


int ModuleMetarInfo::splitStr(StrList& L, const string& seq,
      const string& delims)
{
  L.clear();

  string str;
  string::size_type pos = 0;
  string::size_type len = seq.size();
  while (pos < len)
  {
      // Init/clear the token buffer
    str = "";

      // remove any delimiters including optional (white)spaces
    while ((delims.find(seq[pos]) != string::npos) && (pos < len))
    {
      pos++;
    }

      // leave if @eos
    if (pos == len)
    {
      return L.size();
    }

      // Save token data
    while ((delims.find(seq[pos]) == string::npos) && (pos < len))
    {
      str += seq[pos++];
    }

      // put valid str buffer into the supplied list
    if (!str.empty())
    {
      L.push_back(str);
    }
  }

  return L.size();

} /* ModuleMetarInfo::splitStr */


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
