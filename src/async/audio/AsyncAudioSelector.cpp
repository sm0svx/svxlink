/**
@file	 AsyncAudioSelector.cpp
@brief   This file contains a class that is used to select one of many audio
      	 streams.
@author  Tobias Blomberg / SM0SVX
@date	 2006-08-01

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2019 Tobias Blomberg / SM0SVX

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

class Async::AudioSelector::Branch : public AudioSink
{
  public:
    Branch(AudioSelector *selector)
      : m_selector(selector), m_auto_select(false), m_prio(0),
        m_stream_state(STATE_IDLE), m_flush_wait(true)
    {
      assert(selector != 0);
    }

    StreamState streamState(void) const { return m_stream_state; }
    void setSelectionPrio(int prio) { m_prio = prio; }
    int selectionPrio(void) const { return m_prio; }
    void enableAutoSelect(void) { m_auto_select = true; }
    bool autoSelectEnabled(void) const { return m_auto_select; }
    void setFlushWait(bool flush_wait) { m_flush_wait = flush_wait; }
    bool flushWait(void) const { return m_flush_wait; }

    void disableAutoSelect(void)
    {
      m_auto_select = false;
      if (isSelected())
      {
        m_selector->selectHighestPrioActiveBranch(true);
      }
    }

    bool isSelected(void) const
    {
      return (m_selector->selectedBranch() == this);
    }

    virtual int writeSamples(const float *samples, int count)
    {
      assert(count > 0);
      m_stream_state = STATE_WRITING;
      if (m_auto_select && !isSelected())
      {
	const Branch *selected_branch = m_selector->selectedBranch();
	if ((selected_branch == 0) ||
            (selected_branch->selectionPrio() < m_prio))
	{
	  m_selector->selectBranch(this);
	}
      }
      int ret(count);
      if (isSelected())
      {
        ret = m_selector->branchWriteSamples(samples, count);
        if (ret == 0)
        {
          m_stream_state = STATE_STOPPED;
        }
      }
      return ret;
    }

    virtual void flushSamples(void)
    {
      switch (m_stream_state)
      {
        case STATE_IDLE:
          sourceAllSamplesFlushed();
          break;

        case STATE_WRITING:
        case STATE_STOPPED:
          if (isSelected())
          {
            m_stream_state = STATE_FLUSHING;
            m_selector->branchFlushSamples();
          }
          else
          {
            m_stream_state = STATE_IDLE;
            sourceAllSamplesFlushed();
          }
          break;

        case STATE_FLUSHING:
          break;
      }
    }

    void resumeOutput(void)
    {
      if (m_stream_state == STATE_STOPPED)
      {
        m_stream_state = STATE_WRITING;
        sourceResumeOutput();
      }
    }

    void allSamplesFlushed(void)
    {
      if (m_stream_state == STATE_FLUSHING)
      {
        m_stream_state = STATE_IDLE;
        if (m_auto_select)
        {
          m_selector->selectBranch(0);
        }
        sourceAllSamplesFlushed();
      }
    }

    void unselect(void)
    {
      switch (m_stream_state)
      {
        case STATE_IDLE:
        case STATE_WRITING:
          break;

        case STATE_STOPPED:
          m_stream_state = STATE_WRITING;
          sourceResumeOutput();
          break;

        case STATE_FLUSHING:
          m_stream_state = STATE_IDLE;
          sourceAllSamplesFlushed();
          break;
      }
    }

  private:
    AudioSelector * m_selector;
    bool            m_auto_select;
    int             m_prio;
    StreamState     m_stream_state;
    bool            m_flush_wait;

}; /* class Async::AudioSelector::Branch */


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
  : m_selected_branch(0), m_stream_state(STATE_IDLE)
{
} /* AudioSelector::AudioSelector */


AudioSelector::~AudioSelector(void)
{
  //selectSource(0);
  BranchMap::iterator it;
  for (it = m_branch_map.begin(); it != m_branch_map.end(); ++it)
  {
    delete (*it).second;
  }
} /* AudioSelector::~AudioSelector */


void AudioSelector::addSource(Async::AudioSource *source)
{
  assert(source != 0);
  assert(m_branch_map.find(source) == m_branch_map.end());
  Branch *branch = new Branch(this);
  source->registerSink(branch);
  m_branch_map[source] = branch;
} /* AudioSelector::addSource */


void AudioSelector::removeSource(AudioSource *source)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  m_branch_map.erase(it);
  assert(m_branch_map.find(source) == m_branch_map.end());
  if (branch == selectedBranch())
  {
    selectHighestPrioActiveBranch(true);
  }
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
} /* AudioSelector::disableAutoSelect */


bool AudioSelector::autoSelectEnabled(const AudioSource *source) const
{
  BranchMap::const_iterator it = m_branch_map.find(
      const_cast<AudioSource*>(source));
  assert(it != m_branch_map.end());
  const Branch *branch = (*it).second;
  return branch->autoSelectEnabled();
} /* AudioSelector::autoSelectEnabled */


void AudioSelector::selectSource(AudioSource *source)
{
  Branch *branch = 0;
  if (source != 0)
  {
    BranchMap::iterator it = m_branch_map.find(source);
    assert(it != m_branch_map.end());
    branch = (*it).second;
  }
  selectBranch(branch);
} /* AudioSelector::selectSource */


AudioSource *AudioSelector::selectedSource(void) const
{
  for (BranchMap::const_iterator it = m_branch_map.begin();
       it != m_branch_map.end(); ++it)
  {
    if (it->second == selectedBranch())
    {
      return it->first;
    }
  }
  return 0;
} /* AudioSelector::selectedSource */


void AudioSelector::setFlushWait(AudioSource *source, bool flush_wait)
{
  BranchMap::iterator it = m_branch_map.find(source);
  assert(it != m_branch_map.end());
  Branch *branch = (*it).second;
  branch->setFlushWait(flush_wait);
} /* AudioSelector::setFlushWait */


void AudioSelector::resumeOutput(void)
{
  if (m_stream_state == STATE_STOPPED)
  {
    m_stream_state = STATE_WRITING;
    assert(m_selected_branch != 0);
    m_selected_branch->resumeOutput();
  }
} /* AudioSelector::resumeOutput */


/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioSelector::allSamplesFlushed(void)
{
  if (m_stream_state == STATE_FLUSHING)
  {
    m_stream_state = STATE_IDLE;
    if (m_selected_branch != 0)
    {
      m_selected_branch->allSamplesFlushed();
    }
  }
} /* AudioSelector::allSamplesFlushed */


/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

int AudioSelector::branchWriteSamples(const float *samples, int count)
{
  m_stream_state = STATE_WRITING;
  int ret = sinkWriteSamples(samples, count);
  assert(ret >= 0);
  if (ret == 0)
  {
    m_stream_state = STATE_STOPPED;
  }
  return ret;
} /* AudioSelector::branchWriteSamples */


void AudioSelector::branchFlushSamples(void)
{
  assert(m_selected_branch != 0);
  Branch *flusher = m_selected_branch;
  if (!m_selected_branch->flushWait())
  {
    selectHighestPrioActiveBranch(false);
  }

  if (m_selected_branch == flusher)
  {
    switch (m_stream_state)
    {
      case STATE_IDLE:
        m_selected_branch->allSamplesFlushed();
        break;

      case STATE_WRITING:
      case STATE_STOPPED:
        m_stream_state = STATE_FLUSHING;
        sinkFlushSamples();
        break;

      case STATE_FLUSHING:
        break;
    }
  }
} /* AudioSelector::branchFlushSamples */


void AudioSelector::selectBranch(Branch *branch)
{
  if (branch == m_selected_branch)
  {
    return;
  }

  Branch *prev_branch = m_selected_branch;
  m_selected_branch = branch;
  if (prev_branch != 0)
  {
    prev_branch->unselect();
  }

  assert((m_selected_branch == 0) ||
         (m_selected_branch->streamState() == STATE_IDLE) ||
         (m_selected_branch->streamState() == STATE_WRITING));

  switch (m_stream_state)
  {
    case STATE_IDLE:
    case STATE_FLUSHING:
      break;

    case STATE_WRITING:
    case STATE_STOPPED:
      if ((m_selected_branch == 0) ||
          (m_selected_branch->streamState() == STATE_IDLE))
      {
        m_stream_state = STATE_FLUSHING;
        sinkFlushSamples();
      }
      break;
  }
} /* AudioSelector::selectBranch */


void AudioSelector::selectHighestPrioActiveBranch(bool clear_if_no_active)
{
  Branch *new_branch = 0;
  for (BranchMap::iterator it = m_branch_map.begin();
       it != m_branch_map.end(); ++it)
  {
    Branch *branch = (*it).second;
    if (branch->autoSelectEnabled())
    {
      if (((branch->streamState() == STATE_WRITING) ||
           (branch->streamState() == STATE_STOPPED)) &&
          ((new_branch == 0) ||
          (branch->selectionPrio() > new_branch->selectionPrio())))
      {
        new_branch = branch;
      }
    }
  }
  if ((new_branch != 0) || clear_if_no_active)
  {
    selectBranch(new_branch);
  }
} /* AudioSelector::selectHighestPrioActiveBranch */


/*
 * This file has not been truncated
 */
