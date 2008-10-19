/**
@file	 AudioEncoder.cpp
@brief   Base class for an audio decoder
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

#include "AsyncAudioEncoder.h"
#include "AsyncAudioEncoderRaw.h"
#include "AsyncAudioEncoderS16.h"
#include "AsyncAudioEncoderGsm.h"
#ifdef SPEEX_MAJOR
#include "AsyncAudioEncoderSpeex.h"
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

AudioEncoder *AudioEncoder::create(const std::string &name)
{
  if (name == "RAW")
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
  else
  {
    return 0;
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



/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */

