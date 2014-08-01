/**
@file	 AsyncAudioSelector.cpp
@brief   This file contains a class that is used to select one of many audio
      	 streams.
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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

class Async::AudioSelector::Branch : public AudioPassthrough
{
  public:
    Branch(AudioSelector *selector, AudioSource *source)
      : selector(selector), auto_select(false), prio(0)
    {
    }
    
    void setSelectionPrio(int prio)
    {
      this->prio = prio;
    }
    
    int selectionPrio(void) const
    {
      return prio;
    }
    
    void enableAutoSelect(void)
    {
      auto_select = true;
    }
    
    void disableAutoSelect(void)
    {
      auto_select = false;
      if (isSelected())
      {
      	selector->selectBranch(0);
      }
    }
    
    bool autoSelectEnabled(void) const
    {
      return auto_select;
    }
    
    bool isSelected(void) const
    {
      return selector->handler() == this;
    }
    
    virtual int writeSamples(const float *samples, int count)
    {
      if (auto_select && !isSelected())
      {
	Branch *selected_branch = dynamic_cast<Branch *>(selector->handler());
	assert(selected_branch != 0);
	if (selected_branch->selectionPrio() < prio)
	{
	  selector->selectBranch(this);
	}
      }
      return AudioPassthrough::writeSamples(samples, count);
    }    

    virtual void allSamplesFlushed(void)
    {
      if (auto_select && isSelected())
      {
      	selector->selectBranch(0);
      }
      AudioPassthrough::allSamplesFlushed();
    }
    
  
  private:
    AudioSelector *selector;
    bool auto_select;
    int prio;
    
}; /* class Async::AudioSelector::Branch */


class Async::AudioSelector::NullBranch : public Async::AudioSelector::Branch
{
  public:
    NullBranch(AudioSelector *selector)
      : Branch(selector, 0)
    {
    }
    void resumeOutput(void) {}
    void allSamplesFlushed(void) {}

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

AudioSelector::AudioSelector(void)
{
  null_branch = new NullBranch(this);
  null_branch->setSelectionPrio(-100000);
  setHandler(null_branch);
} /* AudioSelector::AudioSelector */


AudioSelector::~AudioSelector(void)
{
  //selectSource(0);
  clearHandler();
  BranchMap::iterator it;
  for (it = branch_map.begin(); it != branch_map.end(); ++it)
  {
    delete (*it).second;
  }
  delete null_branch;
} /* AudioSelector::~AudioSelector */


void AudioSelector::addSource(Async::AudioSource *source)
{
  assert(branch_map.find(source) == branch_map.end());
  Branch *branch = new Branch(this, source);
  source->registerSink(branch);
  branch_map[source] = branch;
} /* AudioSelector::addSource */


void AudioSelector::removeSource(AudioSource *source)
{
  assert(branch_map.find(source) != branch_map.end());
  Branch *branch = branch_map[source];
  if (branch == handler())
  {
    selectBranch(0);
  }
  // FIXME: Erasing a source from the list should be done from a timer
  branch_map.erase(source);
  assert(branch_map.find(source) == branch_map.end());
  delete branch;
  
} /* AudioSelector::removeSource */


void AudioSelector::setSelectionPrio(AudioSource *source, int prio)
{
  assert(branch_map.find(source) != branch_map.end());
  Branch *branch = branch_map[source];
  branch->setSelectionPrio(prio);
} /* AudioSelector::setAutoSelectPrio */


void AudioSelector::enableAutoSelect(AudioSource *source, int prio)
{
  assert(branch_map.find(source) != branch_map.end());
  Branch *branch = branch_map[source];
  branch->setSelectionPrio(prio);
  branch->enableAutoSelect();
} /* AudioSelector::enableAutoSelect */


void AudioSelector::disableAutoSelect(AudioSource *source)
{
  assert(branch_map.find(source) != branch_map.end());
  Branch *branch = branch_map[source];
  branch->disableAutoSelect();
} /* AudioSelector::disableAutoSelect */


bool AudioSelector::autoSelectEnabled(AudioSource *source)
{
  assert(branch_map.find(source) != branch_map.end());
  const Branch *branch = branch_map[source];
  return branch->autoSelectEnabled();
} /* AudioSelector::autoSelectEnabled */


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
  
  selectBranch(branch);
  
} /* AudioSelector::selectSource */



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

void AudioSelector::selectBranch(Branch *branch)
{
  //printf("AudioSelector::selectBranch: branch=%p\n", branch);
  
  clearHandler();

  if (branch == 0)
  {
    setHandler(null_branch);
    return;
  }
  
  setHandler(branch);

} /* AudioSelector::selectBranch */



/*
 * This file has not been truncated
 */
