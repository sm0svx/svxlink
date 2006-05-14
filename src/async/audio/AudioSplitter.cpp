/**
@file	 AudioSplitter.cpp
@brief   A class that splits an audio stream into multiple streams
@author  Tobias Blomberg / SM0SVX
@date	 2005-05-05

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

#include <cassert>
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

#include "AudioSource.h"
#include "AudioSplitter.h"


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

namespace Async
{

class Branch : public AudioSource
{
  public:
    int   current_buf_pos;
    bool  is_flushed;
  
    Branch(AudioSplitter *splitter, AudioSink *sink)
      : current_buf_pos(0), is_flushed(true), is_stopped(false),
	splitter(splitter)
    {
      assert(registerSink(sink));
    } /* Branch */
    
    ~Branch(void)
    {
      unregisterSink();
    } /* ~Branch */
    
    int sinkWriteSamples(const float *samples, int len)
    {
      is_flushed = false;
      
      if (is_stopped)
      {
	return 0;
      }
      
      len = AudioSource::sinkWriteSamples(samples, len);
      is_stopped = (len == 0);
      current_buf_pos += len;
      
      return len;
      
    } /* sinkWriteSamples */
    
    void sinkFlushSamples(void)
    {
      AudioSource::sinkFlushSamples();
    } /* sinkFlushSamples */
    

  private:
    bool      	  is_stopped;
    AudioSplitter *splitter;
  
    virtual void resumeOutput(void)
    {
      is_stopped = false;
      splitter->branchResumeOutput();
    } /* resumeOutput */
    
    virtual void allSamplesFlushed(void)
    {
      is_flushed = true;
      splitter->branchAllSamplesFlushed();
    } /* allSamplesFlushed */

}; /* class Branch */

} /* namespace Async */


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
  : buf(0), buf_size(0), buf_len(0), do_flush(false), input_stopped(false),
    flushed_branches(0)
{
} /* AudioSplitter::AudioSplitter */


AudioSplitter::~AudioSplitter(void)
{
  delete buf;
  list<Async::Branch *>::iterator it;
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    delete (*it);
  }
} /* AudioSplitter::~AudioSplitter */


void AudioSplitter::addSink(AudioSink *sink)
{
  Branch *branch = new Branch(this, sink);
  branches.push_back(branch);
  if (do_flush)
  {
    branch->sinkFlushSamples();
  }
} /* AudioSplitter::addSink */


int AudioSplitter::writeSamples(const float *samples, int len)
{
  do_flush = false;
  
  if (len == 0)
  {
    return 0;
  }
  
  if (buf_len > 0)
  {
    input_stopped = true;
    return 0;
  }
  
  bool samples_written = false;
  
  list<Branch *>::iterator it;
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    (*it)->current_buf_pos = 0;
    int written = (*it)->sinkWriteSamples(samples, len);
    if (written != len)
    {
      if (buf_len == 0) // Only copy the buffer one time
      {
	if (buf_size < len)
	{
	  delete buf;
	  buf = new float[len];
	}
	memcpy(buf, samples, len * sizeof(*samples));
	buf_len = len;
      }
    }
    samples_written |= (written > 0);
  }
  
  writeFromBuffer();
  
  return len;
  
} /* AudioSplitter::writeSamples */


void AudioSplitter::flushSamples(void)
{
  if (do_flush)
  {
    return;
  }
  
  if (branches.empty())
  {
    sourceAllSamplesFlushed();
    return;
  }
  
  do_flush = true;
  flushed_branches = 0;
  
  if (buf_len > 0)
  {
    return;
  }
  
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
void AudioSplitter::writeFromBuffer(void)
{
  bool samples_written = true;
  bool all_written = (buf_len == 0);
  
  //cout << "samples_written=" << samples_written << "  all_written="
    //   << all_written << endl;
  
  while (samples_written && !all_written)
  {
    samples_written = false;
    all_written = true;
    list<Branch *>::iterator it;
    for (it = branches.begin(); it != branches.end(); ++it)
    {
      //cout << "(*it)->current_buf_pos=" << (*it)->current_buf_pos
	//   << "  buf_len=" << buf_len << endl;
      if ((*it)->current_buf_pos < buf_len)
      {
	int written = (*it)->sinkWriteSamples(buf+(*it)->current_buf_pos,
	      	      	      	      	      buf_len-(*it)->current_buf_pos);
	//cout << "written=" << written << endl;
	samples_written |= (written > 0);
	all_written &= ((*it)->current_buf_pos == buf_len);
      }
    }
    
    if (all_written)
    {
      buf_len = 0;
      if (do_flush)
      {
	flushAllBranches();
      }
    }
  }
} /* AudioSplitter::writeFromBuffer */


void AudioSplitter::flushAllBranches(void)
{
  list<Branch *>::iterator it;
  for (it = branches.begin(); it != branches.end(); ++it)
  {
    (*it)->sinkFlushSamples();
  }
} /* AudioSplitter::flushAllBranches */





void AudioSplitter::branchResumeOutput(void)
{
  writeFromBuffer();
  if (input_stopped && (buf_len == 0))
  {
    input_stopped = false;
    sourceResumeOutput();
  }
} /* AudioSplitter::branchResumeOutput */


void AudioSplitter::branchAllSamplesFlushed(void)
{
  cout << "AudioSplitter::branchAllSamplesFlushed: flushed_branches="
       << flushed_branches << endl;
  
  if (static_cast<unsigned>(++flushed_branches) == branches.size())
  {
    do_flush = false;
    sourceAllSamplesFlushed();
  }
} /* AudioSplitter::branchAllSamplesFlushed */



/*
 * This file has not been truncated
 */

