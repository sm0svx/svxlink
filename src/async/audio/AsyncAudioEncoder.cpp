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
#include "AsyncAudioEncoderDummy.h"
#include "AsyncAudioEncoderNull.h"
#include "AsyncAudioEncoderRaw.h"
#include "AsyncAudioEncoderS16.h"
#include "AsyncAudioEncoderGsm.h"
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

bool AudioEncoder::isAvailable(const std::string &name)
{
  return (name == "NULL") || (name == "RAW") || (name == "S16") ||
         (name == "GSM") ||
#ifdef SPEEX_MAJOR
         (name == "SPEEX") ||
#endif
#ifdef OPUS_MAJOR
         (name == "OPUS") ||
#endif
         (name == "DUMMY");
} /* AudioEncoder::isAvailable */


AudioEncoder *AudioEncoder::create(const std::string &name)
{
  if (name == "NULL")
  {
    return new AudioEncoderNull;
  }
  else if (name == "DUMMY")
  {
    return new AudioEncoderDummy;
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
#ifdef SPEEX_MAJOR
  else if (name == "SPEEX")
  {
    return new AudioEncoderSpeex;
  }
#endif
#ifdef OPUS_MAJOR
  else if (name == "OPUS")
  {
    return new AudioEncoderOpus;
  }
#endif
  else
  {
    return 0;
  }
} /* AudioEncoder::create */


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

