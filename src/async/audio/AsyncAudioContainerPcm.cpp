/**
@file   AsyncAudioContainerPcm.cpp
@brief  Handle PCM type audio container
@author Tobias Blomberg / SM0SVX
@date   2020-02-29

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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

#include "AsyncAudioContainerPcm.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

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

AudioContainerPcm::AudioContainerPcm(void)
{
  m_block.reserve(m_block_size);
} /* AudioContainerPcm::AudioContainerPcm */


AudioContainerPcm::~AudioContainerPcm(void)
{
} /* AudioContainerPcm::~AudioContainerPcm */


int AudioContainerPcm::writeSamples(const float *samples, int count)
{
  //std::cout << "### AudioContainerPcm::writeSamples: count=" << count
  //          << std::endl;
  if (count <= 0)
  {
    return -1;
  }

  for (int i=0; i<count; ++i)
  {
    int16_t sample;
    if (samples[i] > 1.0f)
    {
      sample = 32767;
    }
    else if (samples[i] < -1.0f)
    {
      sample = -32767;
    }
    else
    {
      sample = samples[i] * 32767.0f;
    }
    m_block.push_back(sample);

    if (m_block.size() >= m_block_size)
    {
      writeBlock(reinterpret_cast<char*>(m_block.data()),
          sizeof(int16_t) * m_block.size());
      m_block.clear();
    }
  }
  return count;
} /* AudioContainerPcm::writeSamples */


void AudioContainerPcm::flushSamples(void)
{
  //std::cout << "### AudioContainerPcm::flushSamples" << std::endl;
  if (m_block.size() > 0)
  {
    writeBlock(reinterpret_cast<char*>(m_block.data()),
        sizeof(int16_t) * m_block.size());
    m_block.clear();
  }
} /* AudioContainerPcm::flushSamples */


//void AudioContainerOpus::endStream(void)
//{
//  flushSamples();
//} /* AudioContainerOpus::endStream */


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

/*
 * This file has not been truncated
 */
