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
#include <cassert>


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

class ToneDurationDet : public ToneDetector
{
  public:
    ToneDurationDet(int fq, int bw, int duration)
      : ToneDetector(fq, 8000 / bw), required_duration(duration)
    {
      timerclear(&activation_timestamp);
      ToneDetector::activated.connect(
      	      slot(this, &ToneDurationDet::toneActivated));
    }
    
    SigC::Signal1<void, int> detected;
    
  private:
    int     	    required_duration;
    struct timeval  activation_timestamp;
    
    void toneActivated(bool is_activated)
    {
      //printf("%d tone %s...\n", toneFq(),
      //	     is_activated ? "ACTIVATED" : "DEACTIVATED");
      if (is_activated)
      {
	gettimeofday(&activation_timestamp, NULL);
      }
      else
      {
      	assert(timerisset(&activation_timestamp));
	struct timeval tv, tv_diff;
	gettimeofday(&tv, NULL);
	timersub(&tv, &activation_timestamp, &tv_diff);
	long diff = tv_diff.tv_sec * 1000 + tv_diff.tv_usec / 1000;
	//printf("The %d tone was active for %ld milliseconds\n",
	//      	 toneFq(), diff);
	if (diff >= required_duration)
	{
	  detected(toneFq());
	}
      	timerclear(&activation_timestamp);
      }
    }
};




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
    dtmf_dec(0), ctcss_det(0), ctcss_fq(0), sql_is_open(false), serial(0),
    sql_pin(Serial::PIN_CTS), sql_pin_act_lvl(true)
{
  
} /* LocalRx::LocalRx */


LocalRx::~LocalRx(void)
{
  delete serial;
  delete ctcss_det;
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
  
  string sql_port;
  string sql_pin_str;
  if ((sql_up_det == SQL_DET_SERIAL) || (sql_down_det == SQL_DET_SERIAL))
  {
    if (!cfg.getValue(name, "SQL_PORT", sql_port))
    {
      cerr << "*** ERROR: Config variable " << name << "/SQL_PORT not set\n";
      return false;
    }
    
    if (!cfg.getValue(name, "SQL_PIN", sql_pin_str))
    {
      cerr << "*** ERROR: Config variable " << name << "/SQL_PIN not set\n";
      return false;
    }
    string::iterator colon;
    colon = find(sql_pin_str.begin(), sql_pin_str.end(), ':');
    if ((colon == sql_pin_str.end()) || (colon + 1 == sql_pin_str.end()))
    {
      cerr << "*** ERROR: Illegal format for config variable " << name
      	   << "/SQL_PIN. Should be PINNAME:LEVEL\n";
      return false;
    }
    string pin_str(sql_pin_str.begin(), colon);
    string pin_act_lvl_str(colon + 1, sql_pin_str.end());
    if (pin_str == "CTS")
    {
      sql_pin = Serial::PIN_CTS;
    }
    else if (pin_str == "DSR")
    {
      sql_pin = Serial::PIN_DSR;
    }
    else if (pin_str == "DCD")
    {
      sql_pin = Serial::PIN_DCD;
    }
    else if (pin_str == "RI")
    {
      sql_pin = Serial::PIN_RI;
    }
    else
    {
      cerr << "*** ERROR: Illegal pin name for config variable " << name
      	   << "/SQL_PIN. Should be CTS, DSR, DCD or RI.\n";
      return false;
    }
    if (pin_act_lvl_str == "SET")
    {
      sql_pin_act_lvl = true;
    }
    else if (pin_act_lvl_str == "CLEAR")
    {
      sql_pin_act_lvl = false;
    }
    else
    {
      cerr << "*** ERROR: Illegal pin level for config variable " << name
      	   << "/SQL_PIN. Should be SET or CLEAR.\n";
      return false;
    }
  }
    
  if ((sql_up_det == SQL_DET_SERIAL) || (sql_down_det == SQL_DET_SERIAL))
  {
    serial = new Serial(sql_port);
    if (!serial->open())
    {
      delete serial;
      serial = 0;
      return false;
    }
    sql_pin_poll_timer = new Timer(100, Timer::TYPE_PERIODIC);
    sql_pin_poll_timer->expired.connect(slot(this, &LocalRx::sqlPinPoll));
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


bool LocalRx::addToneDetector(int fq, int bw, int required_duration)
{
  //printf("Adding tone detector with fq=%d  bw=%d  req_dur=%d\n",
  //    	 fq, bw, required_duration);
  ToneDurationDet *det = new ToneDurationDet(fq, bw, required_duration);
  assert(det != 0);
  det->detected.connect(toneDetected.slot());
  audio_io->audioRead.connect(slot(det, &ToneDurationDet::processSamples));
  
  return true;

} /* LocalRx::addToneDetector */



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
  else if (sql_det_str == "SERIAL")
  {
    return SQL_DET_SERIAL;
  }

  return SQL_DET_UNKNOWN;
  
} /* LocalRx::sqlDetStrToEnum */


int LocalRx::audioRead(short *samples, int count)
{
  if (sql_is_open && !is_muted)
  {
    short *filtered = new short[count];
    for (int i=0; i<count; ++i)
    {
      filtered[i] = samples[i];
    }
    highpassFilter(filtered, count);
    count = audioReceived(filtered, count);
    delete [] filtered;
  }
  
  return count;
  
} /* LocalRx::audioRead */


void LocalRx::sqlPinPoll(Timer *t)
{
  if (is_muted)
  {
    return;
  }
  
  bool is_set;
  if (!serial->getPin(sql_pin, is_set))
  {
    perror("getPin");
    return;
  }
  bool is_activated = (is_set == sql_pin_act_lvl);
  
  if (is_activated == sql_is_open)
  {
    return;
  }
  
  //printf("Serial squelch %s...\n", is_activated ? "ACTIVATED" : "DEACTIVATED");
  if (is_activated && (sql_up_det == SQL_DET_SERIAL))
  {
    sql_is_open = true;
    squelchOpen(true);
  }
  else if (!is_activated && (sql_down_det == SQL_DET_SERIAL))
  {
    sql_is_open = false;
    squelchOpen(false);
  }
  
} /* LocalRx::sqlPinPoll */



/* Digital filter designed by mkfilter/mkshape/gencode   A.J. Fisher
   Command line: /www/usr/fisher/helpers/mkfilter -Bu -Hp -o 4 \
      	      	  -a 3.7500000000e-02 0.0000000000e+00 -l

      filtertype  = Butterworth
      passtype	  = Highpass
      ripple  	  = 
      order   	  = 4
      samplerate  = 8000
      corner1 	  = 400
      corner2 	  = 
      adzero  	  = 
      logmin  	  = -60


*/

#define GAIN   1.361640944e+00


void LocalRx::highpassFilter(short *samples, int count)
{ 
  for (int i=0; i<count; ++i)
  {
    xv[0] = xv[1]; xv[1] = xv[2]; xv[2] = xv[3]; xv[3] = xv[4]; 
    xv[4] = static_cast<float>(samples[i]) / 32768 / GAIN;
    yv[0] = yv[1]; yv[1] = yv[2]; yv[2] = yv[3]; yv[3] = yv[4]; 
    yv[4] = (xv[0] + xv[4]) - 4 * (xv[1] + xv[3]) + 6 * xv[2]
                   + (-0.5393551283 * yv[0]) + (2.4891382938 * yv[1])
                   + (-4.3370618174 * yv[2]) + (3.3849727283 * yv[3]);
    /*
    if (yv[4] > 1.0)
    {
      printf("*** Distorsion: %.1f\n", yv[4]);
    }
    */
    samples[i] = static_cast<short>(32767 * yv[4]);
  }
} /* LocalRx::highpassFilter */



/*
 * This file has not been truncated
 */

