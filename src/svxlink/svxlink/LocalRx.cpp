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
    dtmf_dec(0), det_1750(0), req_1750_duration(0), ctcss_det(0), ctcss_fq(0),
    sql_is_open(false)
{
  
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  delete ctcss_det;
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
    cerr << "*** ERROR: Config variable " << name << "/AUDIO_DEV not set\n";
    return false;
  }
  
  string sql_up_det_str;
  if (!cfg.getValue(name, "SQL_UP_DET", sql_up_det_str))
  {
    cerr << "*** ERROR: Config variable " << name << "/SQL_UP_DET not set\n";
    return false;
  }
  sql_up_det = sqlDetStrToEnum(sql_up_det_str);
  if (sql_up_det == SQL_DET_UNKNOWN)
  {
    cerr << "*** ERROR: Config variable " << name << "/SQL_UP_DET is set to "
      	 << "an illegal value. Valid values are: VOX, CTCSS\n";
    return false;
  }
  
  string sql_down_det_str;
  if (!cfg.getValue(name, "SQL_DOWN_DET", sql_down_det_str))
  {
    cerr << "*** ERROR: Config variable " << name << "/SQL_DOWN_DET not set\n";
    return false;
  }
  sql_down_det = sqlDetStrToEnum(sql_down_det_str);
  if (sql_down_det == SQL_DET_UNKNOWN)
  {
    cerr << "*** ERROR: Config variable " << name << "/SQL_DOWN_DET is set to "
      	 << "an illegal value. Valid values are: VOX, CTCSS\n";
    return false;
  }

  string vox_filter_depth;
  string vox_limit;
  string vox_hangtime;
  if ((sql_up_det == SQL_DET_VOX) || (sql_down_det == SQL_DET_VOX))
  {
    if (!cfg.getValue(name, "VOX_FILTER_DEPTH", vox_filter_depth))
    {
      cerr << "*** ERROR: Config variable " << name
      	   << "/VOX_FILTER_DEPTH not set\n";
      return false;
    }

    if (!cfg.getValue(name, "VOX_LIMIT", vox_limit))
    {
      cerr << "*** ERROR: Config variable " << name << "/VOX_LIMIT not set\n";
      return false;
    }

    if (!cfg.getValue(name, "VOX_HANGTIME", vox_hangtime))
    {
      cerr << "*** ERROR: Config variable " << name << "/VOX_HANGTIME not set\n";
      return false;
    }
  }
    
  if ((sql_up_det == SQL_DET_CTCSS) || (sql_down_det == SQL_DET_CTCSS))
  {
    string ctcss_fq_str;
    if (cfg.getValue(name, "CTCSS_FQ", ctcss_fq_str))
    {
      ctcss_fq = atoi(ctcss_fq_str.c_str());
    }
    if (ctcss_fq <= 0)
    {
      cerr << "*** ERROR: Config variable " << name << "/CTCSS_FQ not set or "
      	   << "or is set to an illegal value\n";
      return false;
    }
  }
    
  audio_io = new AudioIO(audio_dev);
  audio_io->audioRead.connect(slot(this, &LocalRx::audioRead));
  
  if ((sql_up_det == SQL_DET_VOX) || (sql_down_det == SQL_DET_VOX))
  {
    vox = new Vox(atoi(vox_filter_depth.c_str()));
    vox->squelchOpen.connect(slot(this, &LocalRx::voxSqlOpen));
    vox->setVoxLimit(atoi(vox_limit.c_str()));
    vox->setHangtime(atoi(vox_hangtime.c_str())*8);
    audio_io->audioRead.connect(slot(vox, &Vox::audioIn));
  }
    
  if ((sql_up_det == SQL_DET_CTCSS) || (sql_down_det == SQL_DET_CTCSS))
  {
    ctcss_det = new ToneDetector(ctcss_fq, 2000); // FIXME: Determine optimum N
    ctcss_det->activated.connect(slot(this, &LocalRx::activatedCtcss));
    audio_io->audioRead.connect(slot(ctcss_det, &ToneDetector::processSamples));
  }
    
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
  return sql_is_open;
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


void LocalRx::voxSqlOpen(bool is_open)
{
  if (is_muted)
  {
    return;
  }
  
  //printf("Vox squelch is %s...\n", is_open ? "OPEN" : "CLOSED");
  if (is_open && (sql_up_det == SQL_DET_VOX))
  {
    sql_is_open = true;
    squelchOpen(true);
  }
  else if (!is_open && (sql_down_det == SQL_DET_VOX))
  {
    sql_is_open = false;
    squelchOpen(false);
  }
} /* LocalRx::voxSqlOpen */


void LocalRx::activatedCtcss(bool is_activated)
{
  if (is_muted)
  {
    return;
  }
  
  //printf("Ctcss %s...\n", is_activated ? "ACTIVATED" : "DEACTIVATED");
  if (is_activated && (sql_up_det == SQL_DET_CTCSS))
  {
    sql_is_open = true;
    squelchOpen(true);
  }
  else if (!is_activated && (sql_down_det == SQL_DET_CTCSS))
  {
    sql_is_open = false;
    squelchOpen(false);
  }
} /* LocalRx::activatedCtcss */


LocalRx::SqlDetType LocalRx::sqlDetStrToEnum(const string& sql_det_str)
{
  if (sql_det_str == "VOX")
  {
    return SQL_DET_VOX;
  }
  else if (sql_det_str == "CTCSS")
  {
    return SQL_DET_CTCSS;
  }

  return SQL_DET_UNKNOWN;
  
} /* LocalRx::sqlDetStrToEnum */


int LocalRx::audioRead(short *samples, int count)
{
  if (sql_is_open && !is_muted)
  {
    return audioReceived(samples, count);
  }
  
  return count;
  
} /* LocalRx::audioRead */



/*
 * This file has not been truncated
 */

