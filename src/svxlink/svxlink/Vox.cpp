/**
@file	 Vox.cpp
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

#include "Vox.h"



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
Vox::Vox(int buf_len)
  : buf(0), buf_size(buf_len), head(0), sum(0), up_limit(0), down_limit(0),
    samples_since_up(0), is_up(false)
{
  buf = new short[buf_len];
  for (int i=0; i<buf_len; ++i)
  {
    buf[i] = 0;
  }
} /* Vox::Vox */


Vox::~Vox(void)
{
  delete [] buf;
  buf = 0;
} /* Vox::~Vox */


void Vox::setVoxLimit(short limit)
{
  up_limit = limit * buf_size;
  down_limit = limit * buf_size;
} /* Vox::setVoxLimit */


void Vox::setHangtime(int hangtime)
{
  this->hangtime = (hangtime >= 0) ? hangtime : 0;
} /* Vox::setHangtime */


int Vox::audioIn(short *samples, int count)
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
      samples_since_up = 0;
      if (!is_up)
      {
      	is_up = true;
      	squelchOpen(true);
      }
    }
    else if (is_up && (sum < down_limit))
    {
      if (++samples_since_up > hangtime)
      {
      	is_up = false;
      	squelchOpen(false);
      }
    }
  }
  
  if (is_up)
  {
    return audioOut(samples, count);
  }
  
  return count;
  
} /* Vox::audioIn */



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







/*
 * This file has not been truncated
 */

