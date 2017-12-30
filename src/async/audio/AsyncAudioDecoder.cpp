/**
@file	 AsyncAudioDecoder.cpp
@brief   Base class of an audio decoder
@author  Tobias Blomberg / SM0SVX
@date	 2008-10-06

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

#include "AsyncAudioDecoder.h"
#include "AsyncAudioDecoderDummy.h"
#include "AsyncAudioDecoderNull.h"
#include "AsyncAudioDecoderRaw.h"
#include "AsyncAudioDecoderS16.h"
#include "AsyncAudioDecoderGsm.h"
#ifdef SPEEX_MAJOR
#include "AsyncAudioDecoderSpeex.h"
#endif
#ifdef OPUS_MAJOR
#include "AsyncAudioDecoderOpus.h"
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

bool AudioDecoder::isAvailable(const std::string &name)
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
} /* AudioDecoder::isAvailable */


AudioDecoder *AudioDecoder::create(const std::string &name)
{
  if (name == "NULL")
  {
    return new AudioDecoderNull;
  }
  else if (name == "DUMMY")
  {
    return new AudioDecoderDummy;
  }
  else if (name == "RAW")
  {
    return new AudioDecoderRaw;
  }
  else if (name == "S16")
  {
    return new AudioDecoderS16;
  }
  else if (name == "GSM")
  {
    return new AudioDecoderGsm;
  }
#ifdef SPEEX_MAJOR
  else if (name == "SPEEX")
  {
    return new AudioDecoderSpeex;
  }
#endif
#ifdef OPUS_MAJOR
  else if (name == "OPUS")
  {
    return new AudioDecoderOpus;
  }
#endif
  else
  {
    return 0;
  }
}


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

