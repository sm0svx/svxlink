/**
@file	 AsyncAudioSelector.cpp
@brief   This file contains a class that is used to select one of many audio
      	 streams.
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2006 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioPassthrough.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioSelector.h"



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

static class NullSource : public AudioSource
{
  public:
    void resumeOutput(void) {}
    void allSamplesFlushed(void) {}
    
} null_source;


class Async::AudioSelector::Branch : public AudioPassthrough
{
  public:
    Branch(AudioSelector *selector, AudioSource *source)
    {
      assert(registerSource(source));
    }
    
}; /* class Branch */



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

AudioSelector::AudioSelector(void)
{
  setHandler(&null_source);
} /* AudioSelector::AudioSelector */


AudioSelector::~AudioSelector(void)
{
  selectSource(0);
  BranchMap::iterator it;
  for (it = branch_map.begin(); it != branch_map.end(); ++it)
  {
    delete (*it).second;
  }
} /* AudioSelector::~AudioSelector */


void AudioSelector::addSource(Async::AudioSource *source)
{
  assert(branch_map.find(source) == branch_map.end());
  branch_map[source] = new Branch(this, source);
} /* AudioSelector::addSource */


void AudioSelector::selectSource(AudioSource *source)
{
  Branch *branch = 0;
  
  if (source != 0)
  {
    assert(branch_map.find(source) != branch_map.end());
    branch = branch_map[source];

    if (branch == handler())
    {
      return;
    }
  }
    
  clearHandler();

  if (branch == 0)
  {
    setHandler(&null_source);
    return;
  }
  
  setHandler(branch);
} /* AudioSelector::selectSource */



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







/*
 * This file has not been truncated
 */

