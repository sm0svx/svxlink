/**
@file	 LocalRx.cpp
@brief   A receiver class to handle local receivers
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

#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncAudioIO.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "DtmfDecoder.h"
#include "ToneDetector.h"
#include "Vox.h"
#include "LocalRx.h"



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

LocalRx::LocalRx(Config &cfg, const std::string& name)
  : Rx(name), cfg(cfg), name(name), audio_io(0), is_muted(true), vox(0),
    dtmf_dec(0), det_1750(0), req_1750_duration(0)
{
  
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  delete det_1750;
  delete dtmf_dec;
  delete audio_io;
  delete vox;
} /* LocalRx::~LocalRx */


bool LocalRx::initialize(void)
{
  string audio_dev;
  if (!cfg.getValue(name, "AUDIO_DEV", audio_dev))
  {
    return false;
  }
  
  string vox_filter_depth;
  if (!cfg.getValue(name, "VOX_FILTER_DEPTH", vox_filter_depth))
  {
    return false;
  }
  
  string vox_limit;
  if (!cfg.getValue(name, "VOX_LIMIT", vox_limit))
  {
    return false;
  }
  
  string vox_hangtime;
  if (!cfg.getValue(name, "VOX_HANGTIME", vox_hangtime))
  {
    return false;
  }
  
  vox = new Vox(atoi(vox_filter_depth.c_str()));
  vox->squelchOpen.connect(squelchOpen.slot());
  vox->setVoxLimit(atoi(vox_limit.c_str()));
  vox->setHangtime(atoi(vox_hangtime.c_str())*8);
  
  audio_io = new AudioIO(audio_dev);
  audio_io->audioRead.connect(slot(vox, &Vox::audioIn));
  vox->audioOut.connect(audioReceived.slot());
  
  dtmf_dec = new DtmfDecoder;
  audio_io->audioRead.connect(slot(dtmf_dec, &DtmfDecoder::processSamples));
  dtmf_dec->digitDetected.connect(dtmfDigitDetected.slot());
  
  return true;
  
} /* LocalRx:initialize */


void LocalRx::mute(bool do_mute)
{
  if (do_mute == is_muted)
  {
    return;
  }
  
  if (do_mute)
  {
    audio_io->close();
  }
  else
  {
    if (!audio_io->open(AudioIO::MODE_RD))
    {
      cerr << "*** Error: Could not open audio device for receiver \""
      	   << name << "\"\n";
      return;
    }
  }

  is_muted = do_mute;

} /* LocalRx::mute */


bool LocalRx::squelchIsOpen(void) const
{
  return vox->isUp();
} /* LocalRx::squelchIsOpen */


bool LocalRx::detect1750(int required_duration)
{
  det_1750 = new ToneDetector(1750, 330); // FIXME: Determine optimum N
  det_1750->activated.connect(slot(this, &LocalRx::activated1750));
  req_1750_duration = required_duration;
  audio_io->audioRead.connect(slot(det_1750, &ToneDetector::processSamples));

  return true;
  
} /* LocalRx::detect1750 */



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


void LocalRx::activated1750(bool is_activated)
{
  //printf("1750 %s...\n", is_activated ? "ACTIVATED" : "DEACTIVATED");
  if (is_activated)
  {
    gettimeofday(&det_1750_timestamp, NULL);
  }
  else
  {
    struct timeval tv, tv_diff;
    gettimeofday(&tv, NULL);
    timersub(&tv, &det_1750_timestamp, &tv_diff);
    long diff = tv_diff.tv_sec * 1000 + tv_diff.tv_usec / 1000;
    //printf("The 1750 tone was active for %ld milliseconds\n", diff);
    if (diff >= req_1750_duration)
    {
      detected1750();
    }
  }
  
} /* LocalRx::activated1750 */






/*
 * This file has not been truncated
 */

