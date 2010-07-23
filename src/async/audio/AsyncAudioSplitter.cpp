/**
@file	 AsyncAudioSplitter.cpp
@brief   A class that splits an audio stream into multiple streams
@author  Tobias Blomberg / SM0SVX
@date	 2005-05-05

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2007  Tobias Blomberg / SM0SVX

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

#include <cassert>
#include <cstring>
#include <iostream>


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

#include "AsyncAudioSource.h"
#include "AsyncAudioSplitter.h"
#include "AsyncAudioFifo.h"


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

class Async::AudioSplitter::Branch : public AudioSource
{
  public:
    Branch(AudioSplitter *splitter, AudioSink *sink, bool managed)
      : is_idle(true), is_enabled(true), is_flushing(false),
        fifo(1024), splitter(splitter)
    {
      /* Note: The FIFO does not add significant audio latency, as long */
      /*       as the sample acceptance rates of the splitter branches */
      /*       do not differ too much. */
      assert(registerSink(&fifo));
      assert(fifo.registerSink(sink, managed));
    }
    
    void setEnabled(bool enabled)
    {
      if (enabled == is_enabled)
      {
      	return;
      }
      is_enabled = enabled;
      
      if (!enabled)
      {
	if (is_flushing)
	{
	  is_flushing = false;
	  splitter->branchAllSamplesFlushed();
	}
	else if (!is_idle)
      	{
	  AudioSource::sinkFlushSamples();
	}
      }
    } /* setEnabled */
    
    int sinkWriteSamples(const float *samples, int len)
    {
      is_idle = false;
      is_flushing = false;
      
      if (is_enabled)
      {
      	len = AudioSource::sinkWriteSamples(samples, len);
      }
      
      return len;
      
    } /* sinkWriteSamples */
    
    void sinkFlushSamples(void)
    {
      if (is_enabled)
      {
      	is_flushing = true;
      	AudioSource::sinkFlushSamples();
      }
      else
      {
      	is_idle = true;
      	splitter->branchAllSamplesFlushed();
      }
    } /* sinkFlushSamples */

    bool enabled() const { return is_enabled; }
    bool empty() const { return fifo.empty(); }
    int spaceAvail() const { return fifo.spaceAvail(); }


  private:
    bool          is_idle;
    bool      	  is_enabled;
    bool      	  is_flushing;
    AudioFifo     fifo;
    AudioSplitter *splitter;
  
    virtual void requestSamples(int count)
    {
      if (is_enabled)
      {
      	splitter->branchRequestSamples(count);
      }
    } /* requestSamples */
    
    virtual void allSamplesFlushed(void)
    {
      bool was_flushing = is_flushing;
      is_flushing = false;
      is_idle = true;
      if (is_enabled && was_flushing)
      {
      	splitter->branchAllSamplesFlushed();
      }
    } /* allSamplesFlushed */

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

AudioSplitter::AudioSplitter(void)
  : is_flushing(false), flushed_branches(0)
{
} /* AudioSplitter::AudioSplitter */


AudioSplitter::~AudioSplitter(void)
{
  removeAllSinks();
} /* AudioSplitter::~AudioSplitter */


void AudioSplitter::addSink(AudioSink *sink, bool managed)
{
  Branch *branch = new Branch(this, sink, managed);
  branches.push_back(branch);
  if (is_flushing)
  {
    branch->sinkFlushSamples();
  }
} /* AudioSplitter::addSink */


void AudioSplitter::removeSink(AudioSink *sink)
{
  list<Branch *>::iterator it = branches.begin();
  while (it != branches.end())
  {
    if ((*it)->sink() == sink)
    {
      if ((*it)->sinkManaged())
      {
      	delete (*it)->sink();
      }
      else
      {
      	(*it)->unregisterSink();
      }

      Branch *branch = *it;
      it = branches.erase(it);
      delete branch;

      break;
    }
    else
    {
      ++it;
    }
  }
} /* AudioSplitter::removeSink */


void AudioSplitter::removeAllSinks(void)
{
  list<Branch *>::const_iterator it;
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    delete (*it);
  }
  branches.clear();
} /* AudioSplitter::removeAllSinks */


void AudioSplitter::enableSink(AudioSink *sink, bool enable)
{
  list<Branch *>::const_iterator it;
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    if ((*it)->sink() == sink)
    {
      (*it)->setEnabled(enable);
      break;
    }
  }
} /* AudioSplitter::enableSink */


int AudioSplitter::writeSamples(const float *samples, int count)
{
  is_flushing = false;
 
  int enabled = 0;
  int empty = 0;
  int len = count;

  list<Branch *>::const_iterator it;

  /* To avoid buffer latency accumulation, at least one branch buffer */
  /* has to be completely empty. Furthermore, we determine the least */
  /* available buffer space of all branch buffers. */
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    Branch *branch = *it;
    if (branch->enabled())
    {
      enabled++;
      empty += branch->empty() ? 1 : 0;
      len = min(len, branch->spaceAvail());
    }
  }

  if (enabled == 0)
  {
    return count;
  }

  if ((empty == 0) || (len == 0))
  {
    return 0;
  }

  /* Write samples into all branches, we already assured that */
  /* there is sufficient branch buffer space available */
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    (*it)->sinkWriteSamples(samples, len);
  }
  
  return len;

} /* AudioSplitter::writeSamples */


void AudioSplitter::flushSamples(void)
{
  if (is_flushing)
  {
    return;
  }
  
  if (branches.empty())
  {
    sourceAllSamplesFlushed();
    return;
  }
  
  is_flushing = true;
  flushed_branches = 0;
  flushAllBranches();
  
} /* AudioSplitter::flushSamples */




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


void AudioSplitter::flushAllBranches(void)
{
  list<Branch *>::const_iterator it;
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    (*it)->sinkFlushSamples();
  }
} /* AudioSplitter::flushAllBranches */


void AudioSplitter::branchRequestSamples(int count)
{
  sourceRequestSamples(count);
} /* AudioSplitter::branchRequestSamples */


void AudioSplitter::branchAllSamplesFlushed(void)
{
  //cout << "AudioSplitter::branchAllSamplesFlushed: flushed_branches="
  //     << flushed_branches << endl;
  
  if (static_cast<unsigned>(++flushed_branches) == branches.size())
  {
    is_flushing = false;
    sourceAllSamplesFlushed();
  }
} /* AudioSplitter::branchAllSamplesFlushed */


/*
 * This file has not been truncated
 */

