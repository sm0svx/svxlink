/**
@file	 AsyncAudioSelector.cpp
@brief   This file contains a class that is used to select one of many audio
      	 streams.
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2017 Tobias Blomberg / SM0SVX

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
#include <sigc++/sigc++.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>


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

class AudioSelector::Branch : public AudioSink, public sigc::trackable
{
  public:
    Branch(bool debug=false)
      : m_state(STATE_IDLE), m_prio(0), m_autoselect(false), m_flush_wait(true),
        m_debug(debug)
    {
    }

    State state(void) const { return m_state; }
    void setSelectionPrio(int prio) { m_prio = prio; }
    int selectionPrio(void) const { return m_prio; }
    void enableAutoSelect(void) { m_autoselect = true; }
    void disableAutoSelect(void) { m_autoselect = false; }
    bool autoSelectEnabled(void) const { return m_autoselect; }
    void setFlushWait(bool flush_wait) { m_flush_wait = flush_wait; }
    bool flushWait(void) const { return m_flush_wait; }

    virtual int writeSamples(const float *samples, int count)
    {
      setState(STATE_ACTIVE);
      int ret = sigWriteSamples(const_cast<float *>(samples), count);
      if (ret <= 0)
      {
        setState(STATE_STOPPED);
      }
      return ret;
    }

    virtual void flushSamples(void)
    {
      setState(STATE_FLUSHING);
      sigFlushSamples();
    }

    void resumeOutput(void) { sourceResumeOutput(); }

    void allSamplesFlushed(void)
    {
      setState(STATE_IDLE);
      sourceAllSamplesFlushed();
    }

    sigc::signal<int, float *, int> sigWriteSamples;
    sigc::signal<void>              sigFlushSamples;

  private:
    State m_state;
    int   m_prio;
    bool  m_autoselect;
    bool  m_flush_wait;
    bool  m_debug;

    void setState(State state)
    {
      if (state != m_state)
      {
        m_state = state;
        if (m_debug)
        {
          cout << "### AudioSelector::Branch::setState[" << this << "]: state="
               << AudioSelector::stateToString(m_state) << endl;
        }
      }
    }
}; /* class AudioSelector::Branch */



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
  : m_selected(0), m_state(STATE_IDLE), m_debug(false)
{
} /* AudioSelector::AudioSelector */


AudioSelector::~AudioSelector(void)
{
  selectSource(0);
  BranchMap::iterator it;
  for (it = m_branch_map.begin(); it != m_branch_map.end(); ++it)
  {
    delete (*it).second;
  }
} /* AudioSelector::~AudioSelector */


void AudioSelector::addSource(Async::AudioSource *source)
{
  assert(m_branch_map.find(source) == m_branch_map.end());
  Branch *branch = new Branch(m_debug);
  source->registerSink(branch);
  branch->sigWriteSamples.connect(
      sigc::bind<0>(mem_fun(*this, &AudioSelector::writeSamples), branch));
  branch->sigFlushSamples.connect(
      sigc::bind(mem_fun(*this, &AudioSelector::flushSamples), branch));
  m_branch_map[source] = branch;
} /* AudioSelector::addSource */


void AudioSelector::removeSource(AudioSource *source)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  if (branch == m_selected)
  {
    selectBranch(0);
  }
  // FIXME: Erasing a source from the list should be done from a timer
  m_branch_map.erase(source);
  assert(m_branch_map.find(source) == m_branch_map.end());
  delete branch;
} /* AudioSelector::removeSource */


void AudioSelector::setSelectionPrio(AudioSource *source, int prio)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  branch->setSelectionPrio(prio);
} /* AudioSelector::setAutoSelectPrio */


void AudioSelector::enableAutoSelect(AudioSource *source, int prio)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  branch->setSelectionPrio(prio);
  branch->enableAutoSelect();
} /* AudioSelector::enableAutoSelect */


void AudioSelector::disableAutoSelect(AudioSource *source)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  branch->disableAutoSelect();
  if (branch == m_selected)
  {
    selectBranch(0);
  }
} /* AudioSelector::disableAutoSelect */


bool AudioSelector::autoSelectEnabled(AudioSource *source)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  return branch->autoSelectEnabled();
} /* AudioSelector::autoSelectEnabled */


void AudioSelector::selectSource(AudioSource *source)
{
  Branch *branch = 0;
  if (source != 0)
  {
    BranchMap::iterator it = m_branch_map.find(source);
    assert(it != m_branch_map.end());
    branch = it->second;
  }
  selectBranch(branch);
} /* AudioSelector::selectSource */


void AudioSelector::setFlushWait(AudioSource *source, bool flush_wait)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  branch->setFlushWait(flush_wait);
} /* AudioSelector::setFlushWait */


void AudioSelector::resumeOutput(void)
{
  if (m_state == STATE_STOPPED)
  {
    setState(STATE_ACTIVE);
  }
  if (m_selected != 0)
  {
    m_selected->resumeOutput();
  }
} /* AudioSelector::resumeOutput */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioSelector::allSamplesFlushed(void)
{
  if (m_selected != 0)
  {
    m_selected->allSamplesFlushed();
  }
  if (m_state == STATE_FLUSHING)
  {
    setState(STATE_IDLE);
    selectHighestPrioActiveBranch();
  }
} /* AudioSelector::allSamplesFlushed */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

const char *AudioSelector::stateToString(State state)
{
  switch (state)
  {
    case STATE_IDLE:
      return "IDLE";
      break;
    case STATE_ACTIVE:
      return "ACTIVE";
      break;
    case STATE_STOPPED:
      return "STOPPED";
      break;
    case STATE_FLUSHING:
      return "FLUSHING";
      break;
    default:
      return "?";
      break;
  }
} /* AudioSelector::stateToString */


int AudioSelector::writeSamples(Branch *branch, float *samples, int count)
{
  /*
  cout << "### AudioSelector::writeSamples: "
       << "branch=" << branch << " autoSelect=" << branch->autoSelectEnabled()
       << " m_selected=" << m_selected
       << endl;
  */
  if ((branch != m_selected) && branch->autoSelectEnabled() &&
      ((m_selected == 0) || (m_selected->state() == STATE_IDLE) ||
       (branch->selectionPrio() > m_selected->selectionPrio())))
  {
    selectBranch(branch);
  }
  if (branch == m_selected)
  {
    setState(STATE_ACTIVE);
    int ret = sinkWriteSamples(samples, count);
    if (ret <= 0)
    {
      setState(STATE_STOPPED);
    }
    return ret;
  }
  return count;
} /* AudioSelector::writeSamples */


void AudioSelector::flushSamples(Branch *branch)
{
  if (branch == m_selected)
  {
    if (branch->flushWait())
    {
      setState(STATE_FLUSHING);
      sinkFlushSamples();
    }
    else
    {
      selectHighestPrioActiveBranch();
      if (m_selected == branch)
      {
        sinkFlushSamples();
      }
      else
      {
        branch->allSamplesFlushed();
      }
    }
    return;
  }
  branch->allSamplesFlushed();
} /* AudioSelector::flushSamples */


void AudioSelector::selectHighestPrioActiveBranch(void)
{
  Branch *new_branch = 0;
  for (BranchMap::iterator it = m_branch_map.begin();
       it != m_branch_map.end(); ++it)
  {
    Branch *b = (*it).second;
    assert(b->state() != STATE_STOPPED);
    if ((b->autoSelectEnabled()) && (b->state() == STATE_ACTIVE))
    {
      if ((new_branch == 0) ||
          (b->selectionPrio() > new_branch->selectionPrio()))
      new_branch = b;
    }
  }
  if (new_branch != 0)
  {
    selectBranch(new_branch);
  }
} /* AudioSelector::selectHighestPrioActiveBranch */


void AudioSelector::selectBranch(Branch *branch)
{
  if (m_selected == branch)
  {
    return;
  }

  if (m_debug)
  {
    cout << "### AudioSelector::selectBranch: " << branch << endl;
  }

  if (m_selected != 0)
  {
    Branch *selected = m_selected;
    m_selected = 0;

    switch (selected->state())
    {
      case STATE_STOPPED:
        selected->resumeOutput();
        break;
      case STATE_FLUSHING:
        selected->allSamplesFlushed();
        break;
      default:
        break;
    }
  }

  switch (m_state)
  {
    case STATE_ACTIVE:
    case STATE_STOPPED:
      if ((branch == 0) || (branch->state() == STATE_IDLE))
      {
        setState(STATE_FLUSHING);
        sinkFlushSamples();
      }
      break;
    default:
      break;
  }

  m_selected = branch;
} /* AudioSelector::selectBranch */


void AudioSelector::setState(State state)
{
  if (state != m_state)
  {
    m_state = state;
    if (m_debug)
    {
      cout << "### AudioSelector::setState: state="
           << stateToString(m_state) << endl;
    }
  }
} /* AudioSelector::setState */


/*
 * This file has not been truncated
 */
