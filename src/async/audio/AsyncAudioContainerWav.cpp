/**
@file   AsyncAudioContainerWav.cpp
@brief  Handle WAV type audio container
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

#include <cstring>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <iostream>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioContainerWav.h"


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

AudioContainerWav::AudioContainerWav(void)
{
  m_block = new char[m_block_size];
  m_block_ptr = m_block;
} /* AudioContainerWav::AudioContainerWav */


AudioContainerWav::~AudioContainerWav(void)
{
  delete [] m_block;
  m_block = nullptr;
  m_block_ptr = nullptr;
} /* AudioContainerWav::~AudioContainerWav */


int AudioContainerWav::writeSamples(const float *samples, int count)
{
  //std::cout << "### AudioContainerWav::writeSamples: count=" << count
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
    m_block_ptr += store16bitValue(m_block_ptr, sample);

    if (m_block_ptr >= m_block+m_block_size)
    {
      writeBlock(reinterpret_cast<char*>(m_block), m_block_size);
      m_samples_written += m_block_size;
      m_block_ptr = m_block;
    }
  }
  return count;
} /* AudioContainerWav::writeSamples */


void AudioContainerWav::flushSamples(void)
{
  if (m_block_ptr > m_block)
  {
    writeBlock(reinterpret_cast<char*>(m_block), m_block_ptr - m_block);
    m_samples_written += m_block_ptr - m_block;
    m_block_ptr = m_block;
  }
} /* AudioContainerWav::flushSamples */


const char* AudioContainerWav::header(void)
{
  char *ptr = m_wave_header;

    // ChunkID
  ptr += storeBuf(ptr, "RIFF", 4);

    // ChunkSize
  if (m_realtime)
  {
    ptr += store32bitValue(ptr, 0xffffffff);
  }
  else
  {
    ptr += store32bitValue(ptr,
        WAVE_HEADER_SIZE - 8 + m_samples_written * NUM_CHANNELS * sizeof(short));
  }

    // Format
  ptr += storeBuf(ptr, "WAVE", 4);

    // Subchunk1ID
  ptr += storeBuf(ptr, "fmt ", 4);

    // Subchunk1Size
  ptr += store32bitValue(ptr, 16);

    // AudioFormat (PCM)
  ptr += store16bitValue(ptr, 1);

    // NumChannels
  ptr += store16bitValue(ptr, NUM_CHANNELS);

    // SampleRate
  ptr += store32bitValue(ptr, m_sample_rate);

    // ByteRate (sample rate * num channels * bytes per sample)
  ptr += store32bitValue(ptr, m_sample_rate * NUM_CHANNELS * sizeof(short));

    // BlockAlign (num channels * bytes per sample)
  ptr += store16bitValue(ptr, NUM_CHANNELS * sizeof(short));

    // BitsPerSample
  ptr += store16bitValue(ptr, 16);

    // Subchunk2ID
  ptr += storeBuf(ptr, "data", 4);

    // Subchunk2Size (num samples * num channels * bytes per sample)
  if (m_realtime)
  {
    ptr += store32bitValue(ptr, 0xffffffff - WAVE_HEADER_SIZE + 8);
  }
  else
  {
    ptr += store32bitValue(ptr,
        m_samples_written * NUM_CHANNELS * sizeof(short));
  }

  assert(ptr - m_wave_header == WAVE_HEADER_SIZE);

  return m_wave_header;
} /* AudioContainerWav::header */


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
