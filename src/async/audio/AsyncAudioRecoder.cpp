/**
@file	 AsyncAudioRecoder.cpp
@brief   Base class of an audio Recoder
@author  Tobias Blomberg / SM0SVX
@date	 2017-03-10

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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

#include "AsyncAudioRecoder.h"
#include "AsyncAudioRecoderDV3k.h"



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

AudioRecoder *AudioRecoder::create(const std::string &name)
{
  if (name == "DV3k")
  {
    return new AudioRecoderDV3k;
  }
  else if (name == "SwDsd")
  {
//    return new AudioRecoderSwDsd;
  }

  return 0;
}


#if 0
AudioRecoder::AudioRecoder(void)
{

} /* AudioRecoder::AudioRecoder */


AudioRecoder::~AudioRecoder(void)
{

} /* AudioRecoder::~AudioRecoder */


void AudioRecoder::resumeOutput(void)
{

} /* AudioRecoder::resumeOutput */
#endif



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

#if 0
void AudioRecoder::allSamplesFlushed(void)
{

} /* AudioRecoder::allSamplesFlushed */
#endif



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

