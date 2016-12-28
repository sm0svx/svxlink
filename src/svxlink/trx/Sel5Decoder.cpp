/**
@file	 Sel5Decoder.cpp
@brief   This file contains the base class for implementing a Sel5 decoder
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2010-03-09

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2010  Tobias Blomberg / SM0SVX

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

#include "Sel5Decoder.h"
#include "SwSel5Decoder.h"



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

Sel5Decoder *Sel5Decoder::create(Config &cfg, const string& name)
{
  Sel5Decoder *dec = 0;

    // For later extensions we take the same structure from the dtmf stuff
    // to have the chance to connect a e.g. ZVEI1 hardware detector
  string type;
  cfg.getValue(name, "SEL5_DEC_TYPE", type);
  if (type == "INTERNAL")
  {
    dec = new SwSel5Decoder(cfg, name);
  }
  else
  {
    cerr << "*** ERROR: Unknown Sel5 decoder type \"" << type
         << "\" specified for " << name << "/SEL5_DEC_TYPE. "
      	 << "Legal values are: \"NONE\" or \"INTERNAL\"\n";
  }

  return dec;

} /* Sel5Decoder::create */


bool Sel5Decoder::initialize(void)
{
  return true;
} /* Sel5Decoder::initialize */


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

