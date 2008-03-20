/**
@file	 Vox.cpp
@brief   Contains a class that implements voice operated transmission (VOX)
@author  Stuart Longland, Tobias Blomberg / SM0SVX
@date	 2008-03-07

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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
#include <cmath>
#include <cassert>

#include <qtimer.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Vox.h"



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

Vox::Vox(void)
  : threshold(0), vox_timer(0), vox_state(IDLE), enable(false)
{
  vox_timer = new QTimer;
  connect(vox_timer, SIGNAL(timeout()),
          this, SLOT(voxTimeout()));
} /* Vox::Vox */


Vox::~Vox(void)
{
  delete vox_timer;
} /* Vox::~Vox */


int Vox::writeSamples(const float *samples, int count)
{
  assert(count > 0);
  
  if (!enable)
  {
    return count;
  }
  
    // Calculate average level
  float sum = 0.0f;
  for (int i=0; i < count; i++)
  {
    float sample = samples[i];
    if (sample > 0)
    {
      sum += sample;
    }
    else
    {
      sum -= sample;
    }
  }

  float avg = sum / (float)count;
  int db_level = -60;
  if (avg > 1.0f)
  {
    db_level = 0;
  }
  else if (avg > 0.001f)
  {
    db_level = (int)(20.0f * log10f(avg));
  }
  levelChanged(db_level);

  if (db_level > threshold)
  {
    setState(ACTIVE);
  }
  else if (vox_state == ACTIVE)
  {
    setState(HANG);
  }

  return count;
    
} /* Vox::writeSamples */


void Vox::setThreshold(int threshold_db)
{
  if (threshold_db < -60)
  {
    threshold = -60;
  }
  else if (threshold_db > 0)
  {
    threshold = 0;
  }
  else
  {
    threshold = threshold_db;
  }
} /* Vox::setThreshold */


void Vox::setDelay(int delay_ms)
{
  if (delay_ms < 0)
  {
    delay = 0;
  }
  else
  {
    delay = delay_ms;
  }
} /* Vox::setDelay */


void Vox::setEnable(bool enable)
{
  this->enable = enable;
  if (!enable)
  {
    levelChanged(-60);
    setState(IDLE);
  }
} /* Vox::setEnable */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/**
 * @brief Called when the VOX delay time has expired
 */
void Vox::voxTimeout(void)
{
  setState(IDLE);
} /* Vox::voxTimeout */


/**
 * @brief Used to set the state of the VOX
 * @param new_state The new state to set (IDLE, ACTIVE or HANG)
 */
void Vox::setState(State new_state)
{
  if (new_state == vox_state)
  {
    return;
  }
  
  vox_state = new_state;
  switch (vox_state)
  {
    case Vox::IDLE:
    case Vox::ACTIVE:
      vox_timer->stop();
      break;
      
    case Vox::HANG:
      vox_timer->start(delay, true);
      break;
  }
  
  stateChanged(vox_state);
  
} /* Vox::setState */



/*
 * This file has not been truncated
 */

