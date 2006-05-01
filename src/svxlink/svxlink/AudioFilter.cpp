/**
@file	 AudioFilter.cpp
@brief   Contains a class for creating a wide range of audio filters
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

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

#include "AudioFilter.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;



/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define SAMPLE_RATE 8000


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

AudioFilter::AudioFilter(const string &filter_spec)
  : ff(0), ff_run(0), ff_buf(0), output_gain(1)
{
  char spec_buf[256];
  strncpy(spec_buf, filter_spec.c_str(), sizeof(spec_buf));
  spec_buf[sizeof(spec_buf) - 1] = 0;
  char *spec = spec_buf;
  char *fferr = fid_parse(SAMPLE_RATE, &spec, &ff);
  if (fferr != 0)
  {
    cerr << "***ERROR: Filter creation error: " << fferr << endl;
    exit(1);
  }
  ff_run = fid_run_new(ff, &ff_func);
  ff_buf = fid_run_newbuf(ff_run);
  
} /* AudioFilter::AudioFilter */


AudioFilter::~AudioFilter(void)
{
  if (ff != 0)
  {
    fid_run_freebuf(ff_buf);
    fid_run_free(ff_run);
    free(ff);
  }  
} /* AudioFilter::~AudioFilter */




/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


void AudioFilter::processSamples(float *dest, const float *src, int count)
{
  //cout << "AudioFilter::processSamples: len=" << len << endl;
  
  for (int i=0; i<count; ++i)
  {
    dest[i] = output_gain * ff_func(ff_buf, src[i]);
  }
} /* AudioFilter::writeSamples */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/




/*
 * This file has not been truncated
 */

