/**
@file	 EchoLinkQsoTest.cpp
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




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <termios.h>
#include <unistd.h>

#include <cstdio>
#include <cmath>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>
#include <AsyncCppApplication.h>
#include <AsyncAudioDevice.h>
#include <AsyncSigCAudioSink.h>
#include <AsyncSigCAudioSource.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "EchoLinkQsoTest.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
using namespace Async;
using namespace EchoLink;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



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
 * Public member functions
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
EchoLinkQsoTest::EchoLinkQsoTest(const string& callsign, const string& name,
    const string& info, const StationData *station)
  : Qso(station->ip(), callsign, name, info),
    station(station), chat_mode(false), is_transmitting(false),
    vox_limit(-1), sigc_sink(0) /*, sigc_src(0)*/
{
  cout << "Call        : " << station->callsign() << endl;
  cout << "Description : " << station->description() << endl;
  cout << "IP-address  : " << station->ipStr() << endl;
  cout << "Time        : " << station->time() << endl;
  cout << "Status      : " << station->statusStr() << endl;
  
    /* Turn off line buffering and echo for stdin */
  struct termios termios;
  tcgetattr(STDIN_FILENO, &org_termios);
  termios = org_termios;
  termios.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &termios);
  
  stdin_watch = new FdWatch(STDIN_FILENO, FdWatch::FD_WATCH_RD);
  stdin_watch->activity.connect(mem_fun(*this, &EchoLinkQsoTest::stdinHandler));
  
  //sigc_src = new SigCAudioSource;
  
  AudioDevice::setSampleRate(8000);
  audio_io = new AudioIO("alsa:default", 0);
  full_duplex = audio_io->isFullDuplexCapable();
  if (full_duplex)
  {
    audio_io->open(AudioIO::MODE_RDWR);
  }
  else
  {
    audio_io->open(AudioIO::MODE_WR);
  }
  audio_io->registerSource(this);
  
  sigc_sink = new SigCAudioSink;
  audio_io->registerSink(sigc_sink);
  sigc_sink->sigWriteSamples.connect(
      mem_fun(*this, &EchoLinkQsoTest::micAudioRead));
  
  chatMsgReceived.connect(mem_fun(*this, &EchoLinkQsoTest::chatMsg));
  infoMsgReceived.connect(mem_fun(*this, &EchoLinkQsoTest::infoMsg));
  stateChange.connect(mem_fun(*this, &EchoLinkQsoTest::onStateChange));
  //audioReceived.connect(mem_fun(*sigc_src, &SigCAudioSource::writeSamples));
  
  cout << string("Audio device is ") << (full_duplex ? "" : "NOT ")
      << "full duplex capable\n";

  printPrompt();
  
} /* EchoLinkQsoTest::EchoLinkQsoTest */


EchoLinkQsoTest::~EchoLinkQsoTest(void)
{
  //delete sigc_src;
  delete sigc_sink;
  delete stdin_watch;
  tcsetattr(STDIN_FILENO, TCSANOW, &org_termios);  
} /* EchoLinkQsoTest::~EchoLinkQsoTest */



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






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


void EchoLinkQsoTest::handleChatMode(void)
{
  static string buf;
  
  int ch = getchar();
  
  if (ch == '\n')
  {
    //cout << "Sending: " << buf << endl;
    sendChatData(buf);
    buf = "";
    putchar('\n');
  }
  else if ((ch == '\033') || (ch == EOF))  // ESC or CTRL-D pressed
  {
    cout << "Exiting chat mode\n";
    printPrompt();
    chat_mode = false;
  }
  else if (ch >= 0)	// Add character to chat buffer
  {
    buf.push_back(ch);
    putchar(ch);
  }
  
  fflush(stdout);
  
} /* EchoLinkQsoTest::handleChatMode */


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
void EchoLinkQsoTest::stdinHandler(FdWatch *watch)
{
  if (chat_mode)
  {
    handleChatMode();
    return;
  }
  
  char cmd = toupper(getchar());
  cout << cmd << endl;
  switch (cmd)
  {
    case 'C':
      cout << "Connecting to " << station->ipStr() << endl;
      connect();
      break;

    case 'D':
      cout << "Disconnecting from " << station->ipStr() << endl;
      disconnect();
      break;

    case ' ':
      if (is_transmitting)
      {
	cout << "Turning off transmitter\n";
	if (!full_duplex)
	{
	  audio_io->close();
	  audio_io->open(AudioIO::MODE_WR);
	}
      }
      else
      {
	cout << "Turning on transmitter\n";
	if (!full_duplex)
	{
	  audio_io->close();
	  audio_io->open(AudioIO::MODE_RD);
	}
      }
      is_transmitting = !is_transmitting;
      break;

    case 'T':
      cout << "Entering chat (talk) mode. ENTER=Send data  ESC=exit\n";
      chat_mode = true;
      return;

    case 'Q':
      done(this);
      return;
    
    case '\n':
      break;

    default:
      cout << "Unknown command: " << cmd << endl;
      break;
  }
  
  printPrompt();  
}


void EchoLinkQsoTest::printPrompt(void)
{
  cout << endl;
  cout << "C=Connect  D=Disconnect  <space>=Toggle TX/RX  T=Talk  Q=Quit"
       << endl;
  cout << station->callsign() << "> ";
  cout.flush();
} /* EchoLinkQsoTest::printPrompt */


void EchoLinkQsoTest::chatMsg(const string& msg)
{
  cout << msg << endl;
} /* EchoLinkQsoTest::chatMsg */


void EchoLinkQsoTest::infoMsg(const string& msg)
{
  cout << "------------ INFO ------------\n";
  cout << msg << endl;
  cout << "------------------------------\n";
} /* EchoLinkQsoTest::infoMsg */


void EchoLinkQsoTest::onStateChange(Qso::State state)
{
  cout << "EchoLoink QSO state changed to ";  
  switch (state)
  {
    case Qso::STATE_DISCONNECTED:
      cout << "DISCONNECTED\n";
      break;
    case Qso::STATE_CONNECTING:
      cout << "CONNECTING\n";
      break;
    case Qso::STATE_CONNECTED:
      cout << "CONNECTED: ";
      cout << remoteCallsign() << " " << remoteName() << endl;
      break;
    case Qso::STATE_BYE_RECEIVED:
      cout << "BYE_RECEIVED\n";
      break;
    default:
      cout << "?\n";
      break;
  }
} /* EchoLinkQsoTest::onStateChange */


int EchoLinkQsoTest::micAudioRead(float *buf, int len)
{
  if (is_transmitting)
  {
    double average = 0;
    for(int i=0; i<len; i++)
    {
      average += fabs(buf[i] * 32767);
    }
    average /= 320.0;
    if (average > vox_limit)
    {
      writeSamples(buf, len);
    }
  }
  
  return len;
  
} /* EchoLinkTest::micAudioRead */


/*
 * This file has not been truncated
 */

