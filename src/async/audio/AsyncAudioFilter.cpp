/**
@file	 AsyncAudioFilter.cpp
@brief   Contains a class for creating a wide range of audio filters
@author  Tobias Blomberg / SM0SVX
@date	 2006-04-23

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

#include <cstring>
#include <cstdlib>
#include <cmath>
#include <locale>


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

extern "C" {
#include "fidlib.h"
};

#include "AsyncAudioFilter.h"



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

namespace Async
{
  class FidVars
  {
    public:
      FidFilter 	*ff;
      FidRun    	*run;
      FidFunc   	*func;
      void      	*buf;

      FidVars(void) : ff(0), run(0), func(0), buf(0) {}
  };
};


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

AudioFilter::AudioFilter(int sample_rate)
  : sample_rate(sample_rate), fv(0), output_gain(1.0f)
{

} /* AudioFilter::AudioFilter */


AudioFilter::AudioFilter(const string &filter_spec, int sample_rate)
  : sample_rate(sample_rate), fv(0), output_gain(1.0f)
{
  if (!parseFilterSpec(filter_spec))
  {
    cerr << "***ERROR: Filter creation error: " << error_str << endl;
    exit(1);
  }
} /* AudioFilter::AudioFilter */


AudioFilter::~AudioFilter(void)
{
  deleteFilter();
} /* AudioFilter::~AudioFilter */


bool AudioFilter::parseFilterSpec(const std::string &filter_spec)
{
  deleteFilter();

  fv = new FidVars;
  
  char spec_buf[256];
  strncpy(spec_buf, filter_spec.c_str(), sizeof(spec_buf));
  spec_buf[sizeof(spec_buf) - 1] = 0;
  char *spec = spec_buf;
  char *old_locale = setlocale(LC_ALL, "C");
  char *fferr = fid_parse(sample_rate, &spec, &fv->ff);
  setlocale(LC_ALL, old_locale);
  if (fferr != 0)
  {
    error_str = fferr;
    free(fferr);
    deleteFilter();
    return false;
  }
  fv->run = fid_run_new(fv->ff, &fv->func);
  fv->buf = fid_run_newbuf(fv->run);
  return true;
} /* AudioFilter::parseFilterSpec */


void AudioFilter::setOutputGain(float gain_db)
{
  output_gain = powf(10.0f, gain_db / 20.0f);
} /* AudioFilter::setOutputGain */


void AudioFilter::reset(void)
{
  fid_run_zapbuf(fv->buf);
} /* AudioFilter::reset */



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
    dest[i] = output_gain * fv->func(fv->buf, src[i]);
  }
} /* AudioFilter::writeSamples */




/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/

void AudioFilter::deleteFilter(void)
{
  if (fv != 0)
  {
    if (fv->ff != 0)
    {
      fid_run_freebuf(fv->buf);
      fid_run_free(fv->run);
      free(fv->ff);
    }
    delete fv;
    fv = 0;
  }
} /* AudioFilter::deleteFilter */



/*
 * This file has not been truncated
 */

