/*
@file	 ModuleMetarInfo.cpp
@brief   gives out a METAR report
@author  Adi Bier / DL1HRC
@date	 2009-04-14

\verbatim
A module (plugin) to request the latest METAR (weather) information from
predefined airports.
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

#define ISPARTOFMILE 1
#define ISKMETER  2
#define ISMORE 3
#define ISMETERS 4
#define ISMILES 5
#define ISNDV 6
#define IS1STPARTOFMILE 7
#define ISVALID 1

#define NOTMEASURED 1
#define CLOUDSVALID 2

#define DEGKTS 1
#define DEGKTSG 2
#define VRBKTS 3
#define CALM 4
#define MPS 5
#define VRBMPS 6

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
     longmsg = "_long";  // taking "cavok_long" instead of "cavok"
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
  int a = 1;

  cout << "DTMF command received in module " << name() << ": " << cmd << endl;

  int icmd = atoi(cmd.c_str());

  if (cmd == "")
  {
     deactivateMe();
     return;
  }

  if (cmd == "0")
  {
      processEvent("say metarhelp");
  }

  if (cmd == "A" || cmd == "0")
  {
      processEvent("say icao_available");
      for (StrList::const_iterator it = aplist.begin(); it != aplist.end(); it++)
      {
         tosay << "report_cfg " << a++ << " " << *it;
         say(tosay);
      }
  }
  else if (icmd <= (int)aplist.size() && icmd > 0)
  {
       icao = aplist[icmd - 1];
       openConnection();
  }
  else processEvent("no_airport_defined");

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
   std::stringstream temp;
   StrList values;
   StrList trvr;
   bool is_false = false;
   float temp_view = 0;
   unsigned int found;

   enum Mtoken {utc, intoken, autom, cor, windvaries, view, actualWX, dewpt,
        pobscurance, skycond, cavok, qnh, wind, rvr, trend, rmk, temperature,
        additional};
   Mtoken Metartoken;

   char *metarinput = static_cast<char *>(buf);
   string html(metarinput, metarinput + count);

   Metartoken = utc;            // start with UTC

   // This is a MEATAR-report:
   //
   // 2009/04/07 13:20
   // FBJW 071300Z 09013KT 9999 FEW030 29/15 Q1023 RMK ...
   //
   // don't worry, it's always the same structure...
   cout << html; // debug

   temp << "airport " << icao;
   say(temp);

   // \n -> <SPACE>
   while ((found = html.find('\n')) != string::npos) html[found] = ' ';

   // check if METAR is actual
   if (isvalidUTC(html.substr(0,16)) == INVALID)
   {
      processEvent("metar_not_valid");
      return -1;
   }

   splitStr(values, html, " ");
   StrList::const_iterator it = values.begin();
   Metartoken = intoken;    // begin of metar is "METAR" in some cases

   while (it != values.end()) {

     current = *it;

     // == current.tolower()
     transform(current.begin(),current.end(),current.begin(),
               (int(*)(int))tolower);

     switch (Metartoken)
     {
       case intoken:
         if (isIntoken(current))
         {
             // nothing to do at the moment
         }
         Metartoken = utc;
         it++;
         it++;
         it++;
         break;

       case utc:
         if (isvalidUTC(current))
         {
            temp << "metreport_time " << current.substr(2,4);
            say(temp);
            it++;
         }
         Metartoken = autom;
         break;

       case autom:
         if (current == "auto")
         {
             // report automatic station??
             // maybe later...
             it++;
         }
         Metartoken = cor;
         break;

       case cor:
         Metartoken = wind;
         break;

       // wind is given as follows:
       // 17005KT -> 170 deg, 5 kts
       // 17011G21KT -> 170 deg, 11kts, gusts 21kts
       // 00000KT -> wind calm
       // VRB04KT -> wind variable, 4kts
       case wind:
         switch (isWind(current))
         {
           // wind degrees and knots
           case DEGKTS:
             temp << "wind_degrees " << current.substr(0,3) << " "
                  << current.substr(3,2);
             say(temp);
             it++;
           break;

           // wind degrees, knots and gusts
           case DEGKTSG:
             temp << "wind_degrees " << current.substr(0,3) << " "
                  << current.substr(3,2);
             say(temp);

             temp << "gusts_up " << current.substr(6,2);
             say(temp);
             it++;
           break;

           // wind variable and knots
           case VRBKTS:
             temp << "wind_variable_kts " << current.substr(3,2);
             say(temp);
             it++;
           break;

           // wind variable and meter per second
           case VRBMPS:
             temp << "wind_variable_mps " << current.substr(3,2);
             say(temp);
             it++;
           break;

           // wind in meter per second
           case MPS:
             temp << "wind_mps " << current.substr(0,3) << " "
                  << current.substr(3,2);
             say(temp);
             it++;
           break;

           // wind calm
           case CALM:
             processEvent("wind_calm");
             it++;
           break;
         }
         Metartoken = windvaries;
         break; // wind

       case windvaries:
         // check if wind varies
         if (isWindVaries(current))
         {
            temp << "wind_varies_from " << current.substr(0,3) << " "
                 << current.substr(4,3);
            say(temp);
            it++;
         }
         Metartoken = cavok;
         break;

       case cavok:
         // cavok == ceiling and visibility OK
         if (current == "cavok") {
            temp << "say " << current << longmsg;
            say(temp);
            it++;
            Metartoken = rvr;
         }
         else Metartoken = view;
         break;

       // view is given in meters Statue and/or parts of them3#
       // 9999 means visibility > 10km
       // 5000 means visibility == 5km
       // 2400 means visibility == 2400m
       // 1 3/4SM means visibility 1,75SM
       // ...
       case view:
         switch (isView(current)) {
           case ISPARTOFMILE:
             temp << "visibility ";
             if (current.substr(0,1) == "m") temp << "M"; // less_than
             if (current.find("1/8",0) != string::npos) temp_view += 0.125;
             if (current.find("1/4",0) != string::npos) temp_view += 0.25;
             if (current.find("3/8",0) != string::npos) temp_view += 0.375;
             if (current.find("1/2",0) != string::npos) temp_view += 0.5;
             if (current.find("5/8",0) != string::npos) temp_view += 0.625;
             if (current.find("3/4",0) != string::npos) temp_view += 0.75;
             if (current.find("7/8",0) != string::npos) temp_view += 0.875;
             temp << temp_view << " miles";
             say(temp);
             it++;
             break;

           case ISNDV:        // warning from automatic stations
                              // -> no directional variation
             if (current.substr(0,4) == "9999")
             {
                 temp << "more_than_10 km";
                 say(temp);
             }
             else
             {
                temp << "visibility " << atoi(current.substr(0,4).c_str())
                     << " meters";
                say(temp);
             }
             processEvent("say ndv");
             it++;
             break;

           case ISKMETER:  // given in km
             temp << "visibility " << atoi(current.substr(0,4).c_str()) << " km";
             say(temp);
             it++;
             break;

           case ISMORE:   // 9999 means "more than 10km"
             temp << "more_than_10 km";
             say(temp);
             it++;
             break;

           case ISMETERS:  // given in meters
             //check if between 5000 and 9998 m
             temp << "visibility ";
             if (atoi(current.c_str()) > 4999)
             {
               temp << atoi(current.c_str())/1000 << " km";
             }
             else
             {
               temp << atoi(current.c_str()) << " meters";
             }
             say(temp);
             current.erase(0,4);

             // the rest should be N, NE,E, SE, S, ...
             if (current.length() > 0)
             {
                temp << "say dir_" << current;
                say(temp);
             }
             it++;
             break;

           case ISMILES:
             temp << "visibility " << atoi(current.c_str()) << " miles";
             say(temp);
             it++;
             break;

           // in America, f.e. "1 3/8SM"
           case IS1STPARTOFMILE:
             temp_view = atof(current.c_str());
             it++;
             break;

           case INVALID:
             Metartoken = rvr;
             break;
         }

         break;

       case rvr:
         // Runway visual range, f.e. R21L/3000V5000
         if (isRVR(current))
         {
            if (!is_false)
            {
              processEvent("say rvr"); // only once
              is_false = true;
            }

            temp << "runway " << current.substr(1,2);
            switch (current.substr(3,1).c_str()[0])
            {
              case 'l':  // RWY21L  -> 2 1 left
              case 'r':  // RWY12R  -> 1 2 right
              case 'c':  // RWY22C  -> 2 2 center
              temp << current.substr(3,1);
            }
            say(temp);

            // RVR visibility...
            splitStr(trvr, current, "/");

            if (trvr[1].find("v",0))
            {
              temp << "rvr_varies_from ";
              switch ((trvr[1].substr(0,1).c_str())[0])
              {
              case 'm':
                 temp << "m " << atoi(trvr[1].substr(1,4).c_str());
                 trvr[1].erase(0,1);
                 break;
              case 'p':
                 temp << "p " << atoi(trvr[1].substr(1,4).c_str());
                 trvr[1].erase(0,1);
                 break;
              default:
                 temp << "0 " << atoi(trvr[1].substr(0,4).c_str());
              }

              say(temp);
              trvr[1].erase(0,5);
            }

            // catches the 1st character
            // indicates if more or less
            switch ((trvr[1].substr(0,1).c_str())[0])
            {
              case 'm':
                processEvent("say less_than");
                trvr[1].erase(0,1);
                break;
              case 'p':
                processEvent("say more_than");
                trvr[1].erase(0,1);
            }

            temp << "rvr " << atoi(trvr[1].substr(0,4).c_str());

            if (trvr[1].substr(trvr[1].length()-2,2) == "ft") temp << " feet";
            else temp << " meter";

            say(temp);

            if (trvr[1].find("d",0)) processEvent("say decreasing");
            if (trvr[1].find("u",0)) processEvent("say increasing");
            if (trvr[1].find("n",0)) processEvent("say no_distinct_tendency");
            it++;
         }
         else
         {
           is_false = false;
           Metartoken = actualWX;
         }
         break;

       // precipitation
       case actualWX:
         if (isActualWX(current))
         {
           // intensity or proximity
           // light intensity or proximity
           if (current.substr(0,1) == "-") {
             processEvent("say light");
             current.erase(0,1);
           }
           // heavy
           else if (current.substr(0,1) == "+")
           {
             processEvent("say heavy");
             current.erase(0,1);
           }
           // in vicinity
           else if (current.substr(0,2) == "vc")
           {
             processEvent("say vc");
             current.erase(0,2);
           }
           else processEvent("say moderate");

           temp << "say ";

           // descriptor + precipitation
           if (current.length() == 2)
           {
              temp << current;
              say(temp);
           }
           else
           {
             // when showers (SHRA) it should be sad as.... "rain shower"
             // not "shower rain"
             if (current.find("sh") != string::npos)
             {
                temp << current.substr(2,current.length());
                say(temp);
                temp << "say " << current.substr(0,2);
                say(temp);
             }
             else
             {
                temp << current.substr(0,2);
                say(temp);
                temp << "say " << current.substr(2,current.length());
                say(temp);
             }
           }
           it++;
         }
         else Metartoken = skycond;
         break;

      // sky condition
       case skycond:
         if (current == "ncd" || current == "nsc" ||
             current == "clr" || current == "skc" ||
             current == "nsw")
         {
            temp << "say " << current;
            say(temp);
            it++;
         }
         // vertical view
         else if (current.find("vv",0) != string::npos &&
                  current.find("//",0) == string::npos)
         {
           temp << "vertical_view " << atoi(current.erase(0,2).c_str())*100;
           say(temp);
           it++;
         }
         else Metartoken = pobscurance;
         break;

       // cloud layers
       case pobscurance:
         switch (ispObscurance(current))
         {
           case CLOUDSVALID:
             if (!is_false)
             {
               processEvent("say clouds");  // say clouds only once
               is_false = true;
             }

             temp << "clouds " << current.substr(0,3)
                  << " " << atoi(current.substr(3,3).c_str()) * 100;  // altitude

             say(temp);

             current.erase(0,6);
             if (current.length() > 0)
             {
                temp << "say " << current << longmsg;
                say(temp);
             }
             it++;
            break;

           case NOTMEASURED:
             it++;
            break;

           case INVALID:
             Metartoken = temperature;
            break;
         }
         break;

       // temperature + dewpoint
       case temperature:
         if (validTemp(current))
         {
           temp << "temperature ";
           if (current.substr(0,1) == "m") temp << "-"
                << atoi(current.substr(1,2).c_str());
           else temp << atoi(current.substr(0,2).c_str());
           say(temp);
         }

         // dewpoint
         if (validDp(current))
         {
           temp << "dewpoint ";
           if (current.substr(current.length()-3,1) == "m")
             temp << "-" << atoi(current.substr(current.length()-2,2).c_str());
           else temp << atoi(current.substr(current.length()-2,2).c_str());

           say(temp);
         }
         it++;
         Metartoken = qnh;
         break;

       // qnh given in hPa or inches
       case qnh:
         if (isQnh(current))
         {
           temp << "qnh ";
           if (isInInch(current))
              temp << current.substr(1,2) << "." << current.substr(3,2)
                   << " inches";
           else temp << current.substr(1,4) << " hPa";
           say(temp);
           it++;
         }
         Metartoken = additional;
         break;

       case additional:
         // wind shear
         if (current.find("ws",0) != string::npos)
         {
           processEvent("say wind_shear");
           it++;
         }
         // recent obcurance or precipitation
         else if (current.find("re",0) != string::npos)
         {
           processEvent("say recent");
           temp << "say " << current.substr(2,2);
           say(temp);
           it++;
         }
     /*    else if (current == "tempo")
         {
             processEvent("say temporary");
             it++;
         } */
         else if (current == "all")
         {
             processEvent("say all");
             it++;
         }
         else if (current.find("rwy",0) != string::npos)
         {
           current.erase(0,3);
           if (current.length() == 0) processEvent("say runways");
           else
           {
              temp << "runway " << current;
              say(temp);
           }
           it++;
         }
         else Metartoken = trend;
         break;

       // trend?
       case trend:
         if (current == "nosig")
         {
            processEvent("say trend");
            temp << "say nosig" << longmsg;
            say(temp);
            it++;
         }
         Metartoken = rmk;
         break;

       // remarks later
       case rmk:
         if (current == "rmk")
         {
           // later...
           // processEvent("say remark");
         }
         it++;
         break;

       default:
         it++;
         break;
     }
   }
   return 1;
}


//
int ModuleMetarInfo::isvalidUTC(std::string token)
{

   time_t rawtime;
   struct tm mtime;              // time of METAR
   struct tm *utc;               // actual time as UTC
   double diff;

   if (token.length() != 16) return INVALID;

   rawtime = time(NULL);
   utc = gmtime(&rawtime);

   mtime.tm_sec  = 0;
   mtime.tm_min  = atoi(token.substr(14,2).c_str());
   mtime.tm_hour = atoi(token.substr(11,2).c_str()) + 1; // why??
   mtime.tm_mday = atoi(token.substr(8,2).c_str());
   mtime.tm_mon  = atoi(token.substr(5,2).c_str()) - 1;
   mtime.tm_year = atoi(token.substr(0,4).c_str()) - 1900;

   diff = difftime(mktime(utc),mktime(&mtime));

   if (diff > 3720) return INVALID;

   return ISVALID;
} /* isvalidUTC */


int ModuleMetarInfo::isView(std::string token)
{
   if (token.length() == 1 && atoi(token.c_str()) > 0 &&
            atoi(token.c_str()) < 10 ) return IS1STPARTOFMILE;
   else if (token.find("sm", 0) != string::npos &&
            token.find("/",0) != string::npos) return ISPARTOFMILE;
   else if (token.find("km", 0) != string::npos) return ISKMETER;
   else if (token == "9999") return ISMORE;
   else if (token.find("ndv", 3) != string::npos
       && atoi(token.substr(0,4).c_str()) > 1000) return ISNDV;
   else if (token.length() > 4  && token.length() < 7
             && atoi(token.substr(0,4).c_str()) > 1000) return ISMETERS;
   else if (token.length() == 4 && atoi(token.substr(0,4).c_str()) > 1000)
       return ISMETERS;
   else if (token.find("sm",0) != string::npos ) return ISMILES;

   return INVALID;
} /* isView */


bool ModuleMetarInfo::isInInch(std::string token)
{
   if (token.substr(0,1) == "a") return true;
   return false;
} /* isInInch */


int ModuleMetarInfo::isWind(std::string token)
{
   if (token.length() < 7 || token.find("//") != string::npos)
          return INVALID;

   if (token == "00000kt") return CALM;
   if (token.substr(0,3) == "vrb" && token.find("kt")) return VRBKTS;
   if (token.substr(0,3) == "vrb" && token.find("mps")) return VRBMPS;
   if (token.find("kt")  != string::npos && token.substr(5,1) == "g")
       return DEGKTSG;
   if (token.find("kt")  != string::npos) return DEGKTS;
   if (token.find("mps") != string::npos) return MPS;
   return INVALID;
} /* isWind */


bool ModuleMetarInfo::isQnh(std::string token)
{
    if (token.length() != 5) return false;
    if ((token.substr(0,1) == "a" || token.substr(0,1) == "q")
         && token.find("/",1) == string::npos) return true;
    return false;
} /* isQnh */


bool ModuleMetarInfo::validTemp(std::string token)
{
   if (token.length() < 5 || token.length() > 8 ) return false;
   if (token.find("/",0) != string::npos
     && token.substr(0,2) != "//"
     && token.substr(1,2) != "//") return true;
   return false;
} /* validTemp */


bool ModuleMetarInfo::isWindVaries(std::string token)
{
    if (token.length() != 7) return false;
    if (token.substr(3,1) == "v") return true;
    return false;
} /* isWindVaries */


bool ModuleMetarInfo::validDp(std::string token)
{
   if (token.length() < 5 || token.length() > 7 ) return false;
   if (token.find("/",0) != string::npos
       && token.substr(token.length()-2,2) != "//")
       return true;
   return false;
} /* validDp */


bool ModuleMetarInfo::isRVR(std::string token)
{
   if (token.length() < 8 ) return false;
   if (token.substr(0,1) == "r" && token.find("/",3) != string::npos )
      return true;
  return false;
} /* isRVR */


int ModuleMetarInfo::ispObscurance(std::string token)
{
  if (token.find("000") != string::npos || token.find("///") != string::npos)
    return NOTMEASURED;

  if ((token.find("few", 0) != string::npos ||
       token.find("sct", 0) != string::npos ||
       token.find("bkn", 0) != string::npos ||
       token.find("ovc", 0) != string::npos))
       return CLOUDSVALID;

  return INVALID;
} /* ispObscurance */


bool ModuleMetarInfo::isIntoken(std::string token)
{
    // comes later
    return false;
} /* isIntoken */


bool ModuleMetarInfo::isActualWX(std::string token)
{
  std::string desc[] = {
          "bcfg", "bldu", "blsa", "blpy", "blsn", "fzbr",
          "vcbr", "tsgr", "vcts", "drdu", "drsa", "drsn",
          "fzfg", "fzdz", "fzra", "prfg", "mifg", "shra",
          "shsn", "shpe", "shpl", "shgs", "shgr", "vcfg",
          "vcfc", "vcss", "vcds", "tsra", "tspe", "tspl",
          "tssn", "vcsh", "vcpo", "vcbldu","vcblsa","vcblsn",
          "br", "du", "dz", "ds", "fg", "fc", "fu", "gs",
          "gr", "hz", "ic", "pe", "pl", "po", "ra", "sn",
          "sg", "sq", "sa", "ss", "ts", "va", "py"};

   for (short a=0; a < 59; a++)
   {
      if (token.find(desc[a],0) != string::npos) return true;
   }
   return false;
} /* isActualWX */


void ModuleMetarInfo::onConnected(void)
{
   char getpath[55];
   sprintf(getpath, "GET /pub/data/observations/metar/stations/%s.TXT\n",
           icao.c_str());
   con->write(getpath, 55);
} /* onConnect */


void ModuleMetarInfo::onDisconnected(TcpConnection *con,
                     TcpClient::DisconnectReason reason)
{

} /* onDisconnect */


void ModuleMetarInfo::say(stringstream &tmp)
{
   processEvent(tmp.str());
   tmp.str("");
} /* say */


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


/*
 * This file has not been truncated
 */
