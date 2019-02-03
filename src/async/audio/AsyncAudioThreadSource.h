/**
@file   AsyncAudioThreadFifo.h
@brief  An audio fifo used to safely connect two different threads
@author Tobias Blomberg / SM0SVX
@date   2019-02-02

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

/** @example AudioThreadSource_demo.cpp
An example of how to use the Async::AudioThreadSource class
*/


#ifndef ASYNC_AUDIO_THREAD_SOURCE_INCLUDED
#define ASYNC_AUDIO_THREAD_SOURCE_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <vector>
#include <mutex>
#include <atomic>
#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncApplication.h>
#include <AsyncAudioSink.h>
#include <AsyncAudioSource.h>
#include <AsyncMutex.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2019-02-02

A_detailed_class_description

\include AudioThreadSource_demo.cpp
*/
class AudioThreadSource : public Async::AudioSource
{
  public:
    /**
     * @brief   Default constructor
     */
    AudioThreadSource(void) {}

    /**
     * @brief   Destructor
     */
    ~AudioThreadSource(void) {}

    /**
     * @brief   Write samples into this audio sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     * @return  Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    void writeSamples(const float *samples, int count)
    {
      {
        std::lock_guard<std::mutex> lk(in_buf_mu);
        flush = false;
        in_buf.insert(in_buf.end(), samples, samples+count);
      }
      {
        std::lock_guard<Async::Mutex> lk(m_mu);
        Async::Application::app().runTask(
            sigc::mem_fun(*this, &AudioThreadSource::resumeOutput));
      }
    }

    /**
     * @brief   Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    void flushSamples(void)
    {
      std::lock_guard<Async::Mutex> lk(m_mu);
      flush = true;
      Async::Application::app().runTask(
          sigc::mem_fun(*this, &AudioThreadSource::resumeOutput));
    }

  protected:

  private:
    Async::Mutex        m_mu;
    std::mutex          in_buf_mu;
    std::vector<float>  in_buf;
    std::vector<float>  out_buf;
    std::vector<float>::iterator out_buf_it = out_buf.begin();
    bool                flush = false;
    bool                m_unflushed = false;

    AudioThreadSource(const AudioThreadSource&);
    AudioThreadSource& operator=(const AudioThreadSource&);

    /**
     * @brief Resume audio output to the sink
     *
     * This function will be called when the registered audio sink is ready
     * to accept more samples.
     * This function is normally only called from a connected sink object.
     */
    virtual void resumeOutput(void)
    {
      //std::cout << "### resumeOutput" << std::endl;
      for (;;)
      {
        {
          std::lock_guard<std::mutex> lk(in_buf_mu);
          if (out_buf.empty() && !in_buf.empty())
          {
            out_buf.swap(in_buf);
            out_buf_it = out_buf.begin();
          }
        }
        if (!out_buf.empty())
        {
          m_unflushed = true;
          //std::cout << "### sinkWriteSamples" << std::endl;
          int cnt = sinkWriteSamples(&(*out_buf_it), out_buf.end()-out_buf_it);
          if (cnt > 0)
          {
            out_buf_it += cnt;
            if (out_buf_it == out_buf.end())
            {
              out_buf.clear();
            }
          }
          else
          {
            break;
          }
        }
        else
        {
          if (flush)
          {
            {
              std::lock_guard<std::mutex> lk(in_buf_mu);
              flush = false;
            }
            if (m_unflushed)
            {
              m_unflushed = false;
              sinkFlushSamples();
            }
          }
          break;
        }
      }
    }

    /**
     * @brief The registered sink has flushed all samples
     *
     * This function will be called when all samples have been flushed in the
     * registered sink.
     * This function is normally only called from a connected sink object.
     */
    virtual void allSamplesFlushed(void) {}

};  /* class AudioThreadSource */


} /* namespace */

#endif /* ASYNC_AUDIO_THREAD_SOURCE_INCLUDED */

/*
 * This file has not been truncated
 */
