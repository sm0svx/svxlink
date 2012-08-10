/**
@file	 FmsDecoder.cpp
@brief   This file contains the base class for implementing an Fms decoder
@author  Tobias Blomberg / SM0SVX & Christian Stussak (University of Halle)
         & Adi Bier / DL1HRC
@date	 2012-08-10

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2012  Tobias Blomberg / SM0SVX

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
#include <algorithm>
#include <string>

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

#include "FmsDecoder.h"
#include "SwFmsDecoder.h"



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

FmsDecoder *FmsDecoder::create(Config &cfg, const string& name)
{
  FmsDecoder *dec = 0;
  string type;

    // For later extensions we take the same structure from the dtmf stuff
    // to have the chance to connect a e.g. Fms hardware detector
  if (!cfg.getValue(name, "FMS_DEC_TYPE", type))
  {
    cerr << "*** ERROR: Config variable " << name << "/FMS_DEC_TYPE not "
      	 << "specified.\n";
    return 0;
  }

  std::transform(type.begin(), type.end(), type.begin(), ::toupper);

  if (type == "INTERNAL")
  {
    dec = new SwFmsDecoder(cfg, name);
  }
  else
  {
    cerr << "*** ERROR: Unknown Fms decoder type \"" << type << "\". "
      	 << "Legal values at the moment are: \"INTERNAL\"\n";
  }

  return dec;

} /* FmsDecoder::create */


bool FmsDecoder::initialize(void)
{
  return true;

} /* FmsDecoder::initialize */


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

