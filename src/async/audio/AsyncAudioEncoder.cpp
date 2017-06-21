/**
@file	 AudioEncoder.cpp
@brief   Base class for an audio decoder
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-06

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2014 Tobias Blomberg / SM0SVX

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



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "AsyncAudioEncoder.h"
#include "AsyncAudioEncoderNull.h"
#include "AsyncAudioEncoderRaw.h"
#include "AsyncAudioEncoderS16.h"
#include "AsyncAudioEncoderGsm.h"
#include "AsyncAudioEncoderAmbe.h"
#ifdef SPEEX_MAJOR
#include "AsyncAudioEncoderSpeex.h"
#endif
#ifdef OPUS_MAJOR
#include "AsyncAudioEncoderOpus.h"
#endif


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

AudioEncoder *AudioEncoder::create(const std::string &name, const std::map<std::string,std::string> &options)
{
  if (name == "NULL")
  {
    return new AudioEncoderNull;
  }
  else if (name == "RAW")
  {
    return new AudioEncoderRaw;
  }
  else if (name == "S16")
  {
    return new AudioEncoderS16;
  }
  else if (name == "GSM")
  {
    return new AudioEncoderGsm;
  }
  else if (name == "AMBE")
  {
    return AudioCodecAmbe::create(options);
  }
#ifdef SPEEX_MAJOR
  else if (name == "SPEEX")
  {
    return new AudioEncoderSpeex(options);
  }
#endif
#ifdef OPUS_MAJOR
  else if (name == "OPUS")
  {
    return new AudioEncoderOpus(options);
  }
#endif
  else
  {
    return NULL;
  }
} /* AudioEncoder::create */



#if 0
AudioEncoder::AudioEncoder(void)
{

} /* AudioEncoder::AudioEncoder */


AudioEncoder::~AudioEncoder(void)
{

} /* AudioEncoder::~AudioEncoder */
#endif



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/

void AudioEncoder::setOptions(const Options &options) {
    for(Options::const_iterator it = options.begin(); it != options.end(); ++it) {
        setOption(it->first,it->second);
    }
}

/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
