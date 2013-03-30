/**
@file	 EchoLinkQsoTest.h
@brief   This file contains a test object for an EchoLink Qso.
@author  Tobias Blomberg
@date	 2003-03-16

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


#ifndef ECHOLINK_QSO_TEST_INCLUDED
#define ECHOLINK_QSO_TEST_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncCppApplication.h>
#include <AsyncFdWatch.h>
#include <AsyncAudioIO.h>
#include <EchoLinkDirectory.h>
#include <EchoLinkQso.h>


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

namespace Async
{
  class SigCAudioSink;
  class SigCAudioSource;
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

class EchoLinkQsoTest : public EchoLink::Qso
{
  public:
    EchoLinkQsoTest(const std::string& callsign, const std::string& name,
      	const std::string& info, const EchoLink::StationData *station);
    ~EchoLinkQsoTest(void);
    
    void setVoxLimit(long limit) { vox_limit = limit; }
    long voxLimit(void) const { return vox_limit; }
    
    sigc::signal<void, EchoLinkQsoTest*> done;
    
  protected:
    
  private:
    const EchoLink::StationData * station;
    struct termios    	      	  org_termios;
    Async::FdWatch *   	      	  stdin_watch;
    bool      	      	      	  chat_mode;
    Async::AudioIO *  	      	  audio_io;
    bool      	      	      	  is_transmitting;
    bool      	      	      	  full_duplex;
    long      	      	      	  vox_limit;
    Async::SigCAudioSink      	  *sigc_sink;
    //Async::SigCAudioSource        *sigc_src;
  
    void stdinHandler(Async::FdWatch *watch);
    void printPrompt(void);
    void handleChatMode(void);
    void chatMsg(const std::string& msg);
    void infoMsg(const std::string& msg);
    void onStateChange(Qso::State state);
    int micAudioRead(float *buf, int len);
    
};  /* class EchoLinkQsoTest */


//} /* namespace */

#endif /* ECHOLINK_QSO_TEST_INCLUDED */



/*
 * This file has not been truncated
 */

