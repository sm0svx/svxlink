/**
@file	 SigLevDet.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2006-05-07

A_detailed_description_for_this_file

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

#include <cmath>
//#include <iostream>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AudioFilter.h>
#include <SigCAudioSink.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "SigLevDet.h"



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

SigLevDet::SigLevDet(void)
{
  filter = new AudioFilter("HpBu4/3500");
  setHandler(filter);
  sigc_sink = new SigCAudioSink;
  sigc_sink->sigWriteSamples.connect(slot(this, &SigLevDet::processSamples));
  sigc_sink->sigFlushSamples.connect(
      slot(sigc_sink, &SigCAudioSink::allSamplesFlushed));
  sigc_sink->registerSource(filter);
} /* SigLevDet::SigLevDet */


SigLevDet::~SigLevDet(void)
{
  clearHandler();
  delete filter;
  delete sigc_sink;
} /* SigLevDet::~SigLevDet */


void SigLevDet::reset(void)
{
  filter->reset();
} /* SigLevDet::reset */



/****************************************************************************
 *
 * Protected member functions
 *
 ****************************************************************************/


/*
 *------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */






/****************************************************************************
 *
 * Private member functions
 *
 ****************************************************************************/


/*
 *----------------------------------------------------------------------------
 * Method:    
 * Purpose:   
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
int SigLevDet::processSamples(float *samples, int count)
{
  //cout << "SigLevDet::processSamples: count=" << count << "\n";
  double rms = 0.0;
  for (int i=0; i<count; ++i)
  {
    rms += pow(samples[i], 2);
  }
  last_siglev = sqrt(rms / count);
  
  return count;
  
} /* SigLevDet::processSamples */






/*
 * This file has not been truncated
 */

