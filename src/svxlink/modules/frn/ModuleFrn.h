/**
@file	 ModuleFrn.h
@brief   A_brief_description_of_this_module
@author  Tobias Blomberg / SM0SVX
@date	 2005-08-28

\verbatim
A module (plugin) for the svxlink server, a multi purpose tranciever
frontend system.
Copyright (C) 2004-2005  Tobias Blomberg / SM0SVX

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


#ifndef MODULE_FRN_INCLUDED
#define MODULE_FRN_INCLUDED


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
#include "QsoFrn.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/
namespace Async
{
  class AudioSplitter;
  class AudioValve;
  class AudioSelector;
  class AudioFifo;
  class AudioJitterFifo;
};


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
class ModuleFrn : public Module
{
  public:
    ModuleFrn(void *dl_handle, Logic *logic, const std::string& cfg_name);
    ~ModuleFrn(void);
    const char *compiledForVersion(void) const { return SVXLINK_VERSION; }

  private:
    void moduleCleanup();
    bool initialize(void);
    void activateInit(void);
    void deactivateCleanup(void);
    bool dtmfDigitReceived(char digit, int duration);
    void dtmfCmdReceived(const std::string& cmd);
    //void dtmfCmdReceivedWhenIdle(const std::string &cmd);
    void squelchOpen(bool is_open);
    void allMsgsWritten(void);
    void reportState(void);

  private:
    QsoFrn *qso;
    Async::AudioValve       *audio_valve;
    Async::AudioSplitter    *audio_splitter;
    Async::AudioSelector    *audio_selector;
    Async::AudioJitterFifo  *audio_fifo;

};  /* class ModuleFrn */


//} /* namespace */

#endif /* MODULE_FRN_INCLUDED */



/*
 * This file has not been truncated
 */
