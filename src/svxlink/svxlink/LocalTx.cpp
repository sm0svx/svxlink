/**
@file	 LocalTx.cpp
@brief   Implements a local transmitter
@author  Tobias Blomberg / SM0SVX
@date	 2004-03-21

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003 Tobias Blomberg / SM0SVX

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
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include <iostream>
#include <cstdio>
#include <algorithm>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioIO.h>
#include <AsyncConfig.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "LocalTx.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;



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

LocalTx::LocalTx(Config& cfg, const string& name)
  : Tx(name), name(name), cfg(cfg), audio_io(0), is_transmitting(false),
    serial_fd(-1), txtot(0), tx_timeout_occured(false), tx_timeout(0),
    tx_delay(0)
{

} /* LocalTx::LocalTx */


LocalTx::~LocalTx(void)
{
  transmit(false);
  
  delete txtot;
  
  if (serial_fd != -1)
  {
    ::close(serial_fd);
    serial_fd = -1;
  }
  
  delete audio_io;
} /* LocalTx::~LocalTx */


bool LocalTx::initialize(void)
{
  string audio_dev;
  if (!cfg.getValue(name, "AUDIO_DEV", audio_dev))
  {
    cerr << "*** ERROR: Config variable " << name << "/AUDIO_DEV not set\n";
    return false;
  }
  
  string ptt_port;
  if (!cfg.getValue(name, "PTT_PORT", ptt_port))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PORT not set\n";
    return false;
  }
  
  string ptt_pin_str;
  if (!cfg.getValue(name, "PTT_PIN", ptt_pin_str))
  {
    cerr << "*** ERROR: Config variable " << name << "/PTT_PIN not set\n";
    return false;
  }
  if (ptt_pin_str == "RTS")
  {
    ptt_pin = TIOCM_RTS;
  }
  else if (ptt_pin_str == "DTR")
  {
    ptt_pin = TIOCM_DTR;
  }
  else
  {
    cerr << "*** ERROR: Accepted values for config variable "
      	 << name << "/PTT_PIN are \"RTS\" and \"DTR\"\n";
    return false;
  }
  
  string tx_timeout_str;
  if (cfg.getValue(name, "TIMEOUT", tx_timeout_str))
  {
    tx_timeout = 1000 * atoi(tx_timeout_str.c_str());
  }
  
  string tx_delay_str;
  if (cfg.getValue(name, "TX_DELAY", tx_delay_str))
  {
    tx_delay = max(0, min(atoi(tx_delay_str.c_str()), 1000));
  }
  
  
  serial_fd = ::open(ptt_port.c_str(), O_RDWR);
  if(serial_fd == -1)
  {
    perror("open serial port");
    return false;
  }
  
  int pin = ptt_pin;
  if (ioctl(serial_fd, TIOCMBIC, &pin) == -1)
  {
    ::close(serial_fd);
    serial_fd = -1;
    perror("ioctl");
    return false;
  }
  
  audio_io = new AudioIO(audio_dev);
  audio_io->writeBufferFull.connect(transmitBufferFull.slot());
  audio_io->allSamplesFlushed.connect(allSamplesFlushed.slot());
  
  return true;
  
} /* LocalTx::initialize */


void LocalTx::transmit(bool do_transmit)
{
  if (do_transmit == is_transmitting)
  {
    return;
  }
  
  cout << "Turning the transmitter " << (do_transmit ? "ON" : "OFF") << endl;
  
  is_transmitting = do_transmit;
  
  if (do_transmit)
  {
    if (!audio_io->open(AudioIO::MODE_WR))
    {
      cerr << "*** ERROR: Could not open audio device for transmitter \""
      	   << name << "\"\n";
      is_transmitting = false;
      return;
    }
    if ((txtot == 0) && (tx_timeout > 0))
    {
      txtot = new Timer(tx_timeout);
      txtot->expired.connect(slot(this, &LocalTx::txTimeoutOccured));
    }
    
    if (tx_delay > 0)
    {
      short samples[8000];
      memset(samples, 0, sizeof(samples));
      transmitAudio(samples, 8000 * tx_delay / 1000);
      flushSamples();
    }
    
    transmitBufferFull(false);
  }
  else
  {
    audio_io->close();
    delete txtot;
    txtot = 0;
    tx_timeout_occured = false;
  }
  
  int pin = ptt_pin;
  if (ioctl(serial_fd, (is_transmitting && !tx_timeout_occured)
      	      	      	? TIOCMBIS : TIOCMBIC, &pin) == -1)
  {
     perror("ioctl");
  }
  
} /* LocalTx::transmit */


int LocalTx::transmitAudio(short *samples, int count)
{
  if (!is_transmitting)
  {
    transmitBufferFull(true);
    return 0;
  }
  
  return audio_io->write(samples, count);
  
} /* LocalTx::transmitAudio */


int LocalTx::samplesToWrite(void)
{
  return audio_io->samplesToWrite();
} /* LocalTx::samplesToWrite */


void LocalTx::flushSamples(void)
{
  audio_io->flushSamples();
} /* LocalTx::flushSamples */


bool LocalTx::isFlushing(void) const
{
  return audio_io->isFlushing();
} /* LocalTx::isFlushing */



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
void LocalTx::txTimeoutOccured(Timer *t)
{
  delete txtot;
  txtot = 0;
  
  if (tx_timeout_occured)
  {
    return;
  }
  
  cerr << "*** ERROR: The transmitter have been active for too long. Turning "
      	  "it off...\n";
  
  int pin = ptt_pin;
  if (ioctl(serial_fd, TIOCMBIC, &pin) == -1)
  {
     perror("ioctl");
  }
  
  tx_timeout_occured = true;
  txTimeout();
} /* LocalTx::txTimeoutOccured */









/*
 * This file has not been truncated
 */

