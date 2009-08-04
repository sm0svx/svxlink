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


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Module.h>
#include <version/SVXLINK.h>
#include <AsyncTcpClient.h>
#include <AsyncConfig.h>



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



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

    typedef std::vector<std::string> StrList;
    StrList  aplist;

    Async::TcpClient *con;

    bool initialize(void);
    void activateInit(void);
    void deactivateCleanup(void);
    bool dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceived(const std::string& cmd);
    void squelchOpen(bool is_open);
    void allMsgsWritten(void);
    void reportState(void);
    void onDisconnected(Async::TcpClient::TcpConnection *con,
                        Async::TcpClient::DisconnectReason reason);
    void onConnected(void);
    void openConnection(void);
    int  onDataReceived(Async::TcpClient::TcpConnection *con, void *buf, int count);
    int  splitStr(StrList& L, const std::string& seq,
                  const std::string& delims);
    int isWind(std::string token);
    int isvalidUTC(std::string token);
    int isView(std::string token);
    bool isQnh(std::string token);
    bool isRVR(std::string token);
    bool isActualWX(std::string token);
    bool isIntoken(std::string token);
    bool validDp(std::string token);
    bool validTemp(std::string token);
    bool isWindVaries(std::string token);
    bool isInInch(std::string token);
    int ispObscurance(std::string token);
    void say(std::stringstream &tmp);

};  /* class ModuleMetarInfo */


//} /* namespace */

#endif /* MODULE_METAR_INCLUDED */



/*
 * This file has not been truncated
 */
