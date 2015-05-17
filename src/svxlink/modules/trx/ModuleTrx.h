/**
@file	 ModuleTrx.h
@brief   A module that provide access to a tranceiver
@author  Tobias Blomberg / SM0SVX
@date	 2015-05-16

\verbatim
A module (plugin) for the svxlink server, a multi purpose tranciever
frontend system.
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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


#ifndef MODULE_TRX_INCLUDED
#define MODULE_TRX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <Module.h>
#include <version/SVXLINK.h>



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

class Rx;
class Tx;


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
@brief	A module that provide access to a tranceiver
@author Tobias Blomberg
@date   2015-05-16
*/
class ModuleTrx : public Module
{
  public:
    ModuleTrx(void *dl_handle, Logic *logic, const std::string& cfg_name);
    ~ModuleTrx(void);
    const char *compiledForVersion(void) const { return SVXLINK_VERSION; }

  private:
    Rx *rx;
    Tx *tx;

    bool initialize(void);
    void activateInit(void);
    void deactivateCleanup(void);
    bool dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceived(const std::string& cmd);
    //void dtmfCmdReceivedWhenIdle(const std::string &cmd);
    //void squelchOpen(bool is_open);
    //void allMsgsWritten(void);

};  /* class ModuleTrx */


//} /* namespace */

#endif /* MODULE_TRX_INCLUDED */



/*
 * This file has not been truncated
 */
