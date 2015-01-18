/**
@file	 Vox.cpp
@brief   Contains a class that implements voice operated transmission (VOX)
@author  Stuart Longland, Tobias Blomberg / SM0SVX
@date	 2008-03-07

\verbatim
Qtel - The Qt EchoLink client
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



/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cmath>
#include <cassert>

#include <QTimer>


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
  : m_threshold(0), m_delay(0), m_vox_timer(0), m_vox_state(IDLE),
    m_enabled(false)
{
  m_vox_timer = new QTimer;
  connect(m_vox_timer, SIGNAL(timeout()),
          this, SLOT(voxTimeout()));
} /* Vox::Vox */


Vox::~Vox(void)
{
  delete m_vox_timer;
} /* Vox::~Vox */


int Vox::writeSamples(const float *samples, int count)
{
  assert(count > 0);
  
  if (!m_enabled)
  {
    return count;
  }
  
    // Calculate DC offset
  float dc_offset = 0.0f;
  for (int i=0; i < count; i++)
  {
    dc_offset += samples[i] / count;
  }

    // Calculate absolute average level sans offset
  float avg = 0.0f;
  for (int i=0; i < count; i++)
  {
    float sample = (samples[i] - dc_offset) / count;
    if (sample > 0)
    {
      avg += sample;
    }
    else
    {
      avg -= sample;
    }
  }

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

  if (db_level > m_threshold)
  {
    setState(ACTIVE);
  }
  else if (m_vox_state == ACTIVE)
  {
    setState(HANG);
  }

  return count;
    
} /* Vox::writeSamples */


void Vox::setThreshold(int threshold_db)
{
  if (threshold_db < -60)
  {
    m_threshold = -60;
  }
  else if (threshold_db > 0)
  {
    m_threshold = 0;
  }
  else
  {
    m_threshold = threshold_db;
  }
} /* Vox::setThreshold */


void Vox::setDelay(int delay_ms)
{
  if (delay_ms < 0)
  {
    m_delay = 0;
  }
  else
  {
    m_delay = delay_ms;
  }
} /* Vox::setDelay */


void Vox::setEnabled(bool enable)
{
  m_enabled = enable;
  if (!m_enabled)
  {
    levelChanged(-60);
    setState(IDLE);
  }
} /* Vox::setEnabled */



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
  if (new_state == m_vox_state)
  {
    return;
  }
  
  m_vox_state = new_state;
  switch (m_vox_state)
  {
    case Vox::IDLE:
    case Vox::ACTIVE:
      m_vox_timer->stop();
      break;
      
    case Vox::HANG:
      m_vox_timer->start(m_delay);
      break;
  }
  
  stateChanged(m_vox_state);
  
} /* Vox::setState */



/*
 * This file has not been truncated
 */

