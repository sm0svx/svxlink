/**
@file	 ModuleMetarInfo.h
@brief   A_brief_description_of_this_module
@author  Adi Bier / DL1HRC
@date	 2009-04-28

\verbatim
A module (plugin) to request the latest METAR (weather) information from
predefined airports.
Look at http://en.wikipedia.org/wiki/METAR for further information

Copyright (C) 2004-2009  Tobias Blomberg / SM0SVX

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


#ifndef MODULE_METAR_INCLUDED
#define MODULE_METAR_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>
#include <vector>
#include <list>
#include <map>
#include <iostream>
#include <curl/curl.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Module.h>
#include <AsyncConfig.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/SVXLINK.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

namespace Async {
  class Timer;
};


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief	A_brief_description_of_this_class
@author Tobias Blomberg
@date   2005-08-28
*/
class ModuleMetarInfo : public Module
{
  public:
    ModuleMetarInfo(void *dl_handle, Logic *logic, const std::string& cfg_name);
    ~ModuleMetarInfo(void);
    const char *compiledForVersion(void) const { return SVXLINK_VERSION; }

  protected:
    virtual void resumeOutput(void);
    virtual void allSamplesFlushed(void);
    virtual int writeSamples(const float *samples, int count);
    virtual void flushSamples(void);

  private:
    std::string icao;
    std::string icao_default;
    std::string longmsg;

    bool remarks;
    bool debug;

    typedef std::map<std::string,std::string> DescList;
    DescList shdesig;

    typedef std::vector<std::string> StrList;
    StrList  aplist;

    typedef std::map<std::string, std::string> Repdefs;
    Repdefs repstr;

    std::string html;
    std::string type;
    std::string server;
    std::string link;
    CURL *http_handle; 
    CURLM *multi_handle;

    bool initialize(void);
    void activateInit(void);
    void deactivateCleanup(void);
    bool dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceived(const std::string& cmd);
    void dtmfCmdReceivedWhenIdle(const std::string& cmd);
    void squelchOpen(bool is_open);
    void allMsgsWritten(void);
    void openConnection(void);
    void onTimeout(void);
    std::string getSlp(std::string token);
    std::string getTempTime(std::string token);
    std::string getTempinRmk(std::string token);
    std::string getPressureinRmk(std::string token);
    std::string getPrecipitationinRmk(std::string token);
    std::string getTemp(std::string token);
    std::string getLightning(std::string token);
    std::string getPrecipitation(std::string token);
    std::string getCloudType(std::string token);
    void isRwyState(std::string &retval, std::string token);
    void onData(std::string metarinput, size_t count);
    int  splitEmptyStr(StrList& L, const std::string& seq);
    bool isWind(std::string &retval, std::string token);
    bool isvalidUTC(std::string utctoken);
    int checkToken(std::string token);
    bool rmatch(std::string tok, std::string token, regex_t *re);
    bool checkDirection(std::string &retval, std::string token);
    bool getRmkVisibility(std::string &retval, std::string token);
    void isTime(std::string &retval, std::string token);
    bool isRunway(std::string &retval, std::string token);
    void isPartofMiles(std::string &retval, std::string token);
    bool isView(std::string &retval, std::string token);
    bool isQnh(std::string &retval, std::string token);
    bool isRVR(std::string &retval, std::string token);
    bool isActualWX(std::string &retval, std::string token);
    void isVerticalView(std::string &retval, std::string token);
    void validDp(std::string &tempstr, std::string token);
    void validTemp(std::string &tempstr, std::string token);
    void isValueVaries(std::string &tempstr, std::string token);
    bool ispObscurance(std::string &tempstr, std::string token);
    bool getPeakWind(std::string &retval, std::string token);
    void say(std::stringstream &tmp);
    int handleMetar(std::string input);
    std::string getXmlParam(std::string token, std::string input);
};  /* class ModuleMetarInfo */


//} /* namespace */

#endif /* MODULE_METAR_INCLUDED */



/*
 * This file has not been truncated
 */
