/**
@file	 Voter.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2005-04-18

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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Voter.h"
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

Voter::Voter(Config &cfg, const std::string& name)
  : Rx(cfg, name), active_rx(0), is_muted(true)
{

} /* Voter::Voter */


Voter::~Voter(void)
{
  list<Rx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    delete *it;
  }
  rxs.clear();
} /* Voter::~Voter */


bool Voter::initialize(void)
{
  string receivers;
  if (!cfg().getValue(name(), "RECEIVERS", receivers))
  {
    cerr << "*** ERROR: Config variable " << name() << "/RECEIVERS not set\n";
    return false;
  }

  string::iterator start(receivers.begin());
  for (;;)
  {
    string::iterator comma = find(start, receivers.end(), ',');
    string rx_name(start, comma);
    if (!rx_name.empty())
    {
      cout << "Adding receiver to Voter: " << rx_name << endl;
      Rx *rx = Rx::create(cfg(), rx_name);
      if ((rx == 0) || !rx->initialize())
      {
      	return false;
      }
      rx->mute(true);
      rx->squelchOpen.connect(slot(this, &Voter::satSquelchOpen));
      rx->audioReceived.connect(audioReceived.slot());
      rx->dtmfDigitDetected.connect(dtmfDigitDetected.slot());
      rx->toneDetected.connect(toneDetected.slot());
      
      rxs.push_back(rx);
    }
    if (comma == receivers.end())
    {
      break;
    }
    start = comma;
    ++start;
  }
  
  
  return true;
  
} /* Voter::initialize */


void Voter::mute(bool do_mute)
{
  if (do_mute == is_muted)
  {
    return;
  }
  
  list<Rx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    (*it)->mute(do_mute);
  }
  
  if (do_mute)
  {
    active_rx = 0;
  }
  
  is_muted = do_mute;
  
} /* Voter::mute */


bool Voter::squelchIsOpen(void) const
{
  return active_rx != 0;
} /* Voter::squelchIsOpen */


bool Voter::addToneDetector(int fq, int bw, int required_duration)
{
  bool success = true;
  list<Rx *>::iterator it;
  for (it=rxs.begin(); it!=rxs.end(); ++it)
  {
    success &= (*it)->addToneDetector(fq, bw, required_duration);
  }
  
  return success;
  
} /* Voter::addToneDetector */



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
void Voter::satSquelchOpen(bool is_open)
{
  //cout << "Voter::satSquelchOpen(" << (is_open ? "TRUE" : "FALSE") << ")\n";
  
  if (is_open)
  {
    assert(active_rx == 0);
    list<Rx *>::iterator it;
    for (it=rxs.begin(); it!=rxs.end(); ++it)
    {
      if ((*it)->squelchIsOpen())
      {
      	assert(active_rx == 0);
      	active_rx = *it;
      }
      else
      {
      	(*it)->mute(true);
      }
    }
    assert(active_rx != 0);
  }
  else
  {
    assert(active_rx != 0);
    list<Rx *>::iterator it;
    for (it=rxs.begin(); it!=rxs.end(); ++it)
    {
      if (active_rx != *it)
      {
      	(*it)->mute(false);
      }
    }
    active_rx = 0;
  }

  squelchOpen(is_open);

} /* Voter::satSquelchOpen */






/*
 * This file has not been truncated
 */

