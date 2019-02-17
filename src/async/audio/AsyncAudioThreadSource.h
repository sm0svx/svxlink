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
#include <condition_variable>
//#include <iostream>


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
     * @brief   No copy constructor
     */
    AudioThreadSource(const AudioThreadSource&) = delete;

    /**
     * @brief   Destructor
     */
    //~AudioThreadSource(void) {}

    /*
     * @brief   No assignment operator
     */
    AudioThreadSource& operator=(const AudioThreadSource&) = delete;

  protected:
    /**
     * @brief   Queue samples for delivery to the connected sink
     * @param   samples The buffer containing the samples
     * @param   count The number of samples in the buffer
     *
     * This function is used to queue audio for later delivery to the connected
     * audio sink. If a flush is pending, calling this function will cancel the
     * flush.
     */
    virtual int sinkWriteSamples(const float *samples, int count)
    {
      {
        std::lock_guard<std::mutex> lk(m_in_buf_mu);
        m_flush = false;
        m_in_buf.insert(m_in_buf.end(), samples, samples+count);
      }
      {
        std::lock_guard<Async::Mutex> lk(m_mu);
        Async::Application::app().runTask(
            sigc::mem_fun(*this, &AudioThreadSource::resumeOutput));
      }
      return count;
    }

    /**
     * @brief   Flush samples in the connected sink when the queue is empty
     *
     * This function is used to tell the connected sink to flush previously
     * written samples when the queue is empty. If the writeSamples function is
     * called before the queue is empty, the flush is cancelled.
     */
    virtual void sinkFlushSamples(void)
    {
      std::lock_guard<Async::Mutex> lk(m_mu);
      m_flush = true;
      Async::Application::app().runTask(
          sigc::mem_fun(*this, &AudioThreadSource::resumeOutput));
    }

    void waitForAllSamplesFlushed(void)
    {
      std::unique_lock<Async::Mutex> lk(m_mu);
      m_all_flushed_cond.wait(lk, [this]{ return m_all_flushed; });
    }

  private:
    Async::Mutex                  m_mu;
    std::mutex                    m_in_buf_mu;
    std::vector<float>            m_in_buf;
    std::vector<float>            m_out_buf;
    std::vector<float>::iterator  m_out_buf_it = m_out_buf.begin();
    bool                          m_flush = false;
    bool                          m_unflushed = false;
    bool                          m_all_flushed = true;
    std::condition_variable_any   m_all_flushed_cond;

    /**
     * @brief Resume audio output to the sink
     *
     * This function will be called when the registered audio sink is ready
     * to accept more samples. It is also called when more samples have been
     * written to the queue or if the flushSamples function is called.
     */
    virtual void resumeOutput(void)
    {
      for (;;)
      {
        {
          std::lock_guard<std::mutex> lk(m_in_buf_mu);
          if (m_out_buf.empty() && !m_in_buf.empty())
          {
            m_out_buf.swap(m_in_buf);
            m_out_buf_it = m_out_buf.begin();
          }
        }
        if (!m_out_buf.empty())
        {
          m_unflushed = true;
          int cnt = AudioSource::sinkWriteSamples(&(*m_out_buf_it),
              m_out_buf.end()-m_out_buf_it);
          if (cnt > 0)
          {
            m_out_buf_it += cnt;
            if (m_out_buf_it == m_out_buf.end())
            {
              m_out_buf.clear();
            }
          }
          else
          {
            break;
          }
        }
        else
        {
          if (m_flush)
          {
            {
              std::lock_guard<std::mutex> lk(m_in_buf_mu);
              m_flush = false;
            }
            if (m_unflushed)
            {
              m_unflushed = false;
              m_all_flushed = false;
              AudioSource::sinkFlushSamples();
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
    virtual void allSamplesFlushed(void)
    {
      m_all_flushed = true;
      m_all_flushed_cond.notify_all();
    }

};  /* class AudioThreadSource */


} /* namespace */

#endif /* ASYNC_AUDIO_THREAD_SOURCE_INCLUDED */

/*
 * This file has not been truncated
 */
