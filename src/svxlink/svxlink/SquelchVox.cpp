/**
@file	 SquelchVox.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg
@date	 2004-02-15

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003  Tobias Blomberg

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

#include <stdio.h>
#include <cstdlib>
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

#include "SquelchVox.h"



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
SquelchVox::SquelchVox(void)
  : buf(0), buf_size(0), head(0), sum(0), up_limit(0), down_limit(0)
{
} /* SquelchVox::SquelchVox */


SquelchVox::~SquelchVox(void)
{
  delete [] buf;
  buf = 0;
} /* SquelchVox::~SquelchVox */


bool SquelchVox::initialize(Config& cfg, const string& rx_name)
{
  if (!Squelch::initialize(cfg, rx_name))
  {
    return false;
  }
  
  string value;
  if (!cfg.getValue(rx_name, "VOX_FILTER_DEPTH", value))
  {
    cerr << "*** ERROR: Config variable " << rx_name
      	 << "/VOX_FILTER_DEPTH not set\n";
    return false;
  }
  buf_size = 8000 * atoi(value.c_str()) / 1000;
  buf = new short[buf_size];
  for (int i=0; i<buf_size; ++i)
  {
    buf[i] = 0;
  }

  if (!cfg.getValue(rx_name, "VOX_LIMIT", value))
  {
    cerr << "*** ERROR: Config variable " << rx_name << "/VOX_LIMIT not set\n";
    return false;
  }
  setVoxLimit(atoi(value.c_str()));

  return true;
  
} /* SquelchVox::initialize */


void SquelchVox::setVoxLimit(short limit)
{
  up_limit = limit * buf_size;
  down_limit = limit * buf_size;
} /* SquelchVox::setVoxLimit */



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
int SquelchVox::processSamples(short *samples, int count)
{
  for (int i=0; i<count; ++i)
  {
    sum -= buf[head];
    buf[head] = abs(samples[i]);
    sum += buf[head];
    head = (head >= buf_size-1) ? 0 : head + 1;
    if (sum >= up_limit)
    {
      //printf("sum=%ld\n", sum);
      setOpen(true);
    }
    else if (isOpen() && (sum < down_limit))
    {
      setOpen(false);
    }
  }
  
  return count;
  
} /* SquelchVox::processSamples */








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







/*
 * This file has not been truncated
 */

