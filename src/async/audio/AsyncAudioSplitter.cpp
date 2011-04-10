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
      : stream_state(STREAM_IDLE), is_enabled(true),
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
      
      if (enabled)
      {
        if (stream_state == STREAM_ACTIVE)
        {
          AudioSource::sinkAvailSamples();
        }
      }
      else
      {
        switch (stream_state)
        {
          case STREAM_FLUSHING:
	    stream_state = STREAM_IDLE;
	    splitter->branchAllSamplesFlushed();
	    break;
	  case STREAM_ACTIVE:
	    AudioSource::sinkFlushSamples();
	    break;
          case STREAM_IDLE:
            break;
	}
      }
    } /* setEnabled */
    
    int sinkWriteSamples(const float *samples, int len)
    {
      stream_state = STREAM_ACTIVE;
      
      if (is_enabled)
      {
      	len = AudioSource::sinkWriteSamples(samples, len);
      }
      
      return len;
      
    } /* sinkWriteSamples */

    void sinkAvailSamples(void)
    {
      stream_state = STREAM_ACTIVE;
      
      if (is_enabled)
      {
      	AudioSource::sinkAvailSamples();
      }
    } /* sinkAvailSamples */

    void sinkFlushSamples(void)
    {
      if (is_enabled)
      {
      	stream_state = STREAM_FLUSHING;
      	AudioSource::sinkFlushSamples();
      }
      else
      {
      	stream_state = STREAM_IDLE;
      	splitter->branchAllSamplesFlushed();
      }
    } /* sinkFlushSamples */

    bool enabled() const { return is_enabled; }
    bool empty() const { return fifo.empty(); }
    int spaceAvail() const { return fifo.spaceAvail(); }

    AudioSink *sink(void) const { return fifo.sink(); }
    bool sinkManaged(void) const { return fifo.sinkManaged(); }
    
    void clearFifo(void) { fifo.clear(); }

  private:
    typedef enum
    {
      STREAM_IDLE, STREAM_ACTIVE, STREAM_FLUSHING
    } StreamState;
    StreamState   stream_state;
    bool      	  is_enabled;
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
      if (is_enabled && (stream_state == STREAM_FLUSHING))
      {
        stream_state = STREAM_IDLE;
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
 
  bool enabled = false;
  bool empty = false;
  int len = count;

  list<Branch *>::const_iterator it;

  /* To avoid buffer latency accumulation, at least one branch buffer */
  /* has to be completely empty. */
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    Branch *branch = *it;
    if (branch->enabled())
    {
      enabled = true;
      empty |= branch->empty();
    }
  }

  /* No branch is enabled. */
  if (!enabled)
  {
    return count;
  }

  /* No enabled branch FIFO is empty. */
  if (!empty)
  {
    return 0;
  }

  /* Determine the least common available buffer space */
  /* of all branch buffers. */
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    Branch *branch = *it;
    if (branch->enabled())
    {
      if (branch->spaceAvail() == 0)
      {
        /* There's at least one empty and one full branch FIFO. */
        /* This indicates that the maximum allowed sample rate */
        /* unbalance has been reached. The only thing we can do now */
        /* is to throw away all samples from the full branch FIFO. */
        branch->clearFifo();
      }
      len = min(len, branch->spaceAvail());
    }
  }

  /* Write samples into all branches, we already assured that */
  /* there is sufficient branch buffer space available */
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    (*it)->sinkWriteSamples(samples, len);
  }
  
  return len;

} /* AudioSplitter::writeSamples */


void AudioSplitter::availSamples(void)
{
  is_flushing = false;
  
  list<Branch *>::const_iterator it;
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    (*it)->sinkAvailSamples();
  }
} /* AudioSplitter::availSamples */


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

