/**
@file	 DtmfDecoder.cpp
@brief   This file contains the base class for implementing a DTMF decoder
@author  Tobias Blomberg / SM0SVX
@date	 2008-02-04

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2008  Tobias Blomberg / SM0SVX

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

#include "DtmfDecoder.h"
#include "Dh1dmSwDtmfDecoder.h"
#include "SvxSwDtmfDecoder.h"
#include "S54sDtmfDecoder.h"
#include "AfskDtmfDecoder.h"
#include "PtyDtmfDecoder.h"


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

DtmfDecoder *DtmfDecoder::create(Rx *rx, Config &cfg, const string& name)
{
  DtmfDecoder *dec = 0;
  string type;
  cfg.getValue(name, "DTMF_DEC_TYPE", type);
  if (type == "INTERNAL")
  {
    dec = new SvxSwDtmfDecoder(cfg, name);
  }
  else if (type == "S54S")
  {
    dec = new S54sDtmfDecoder(cfg, name);
  }
  else if (type == "AFSK")
  {
    dec = new AfskDtmfDecoder(rx, cfg, name);
  }
  else if (type == "PTY")
  {
    dec = new PtyDtmfDecoder(cfg, name);
  }
  else if (type == "DH1DM")
  {
    dec = new Dh1dmSwDtmfDecoder(cfg, name);
  }
  else
  {
    cerr << "*** ERROR: Unknown DTMF decoder type \"" << type << "\" "
         << "specified for " << name << "/DTMF_DEC_TYPE. "
      	 << "Legal values are: \"NONE\", \"INTERNAL\", \"PTY\" or \"S54S\"\n";
  }
  
  return dec;
  
} /* DtmfDecoder::create */


bool DtmfDecoder::initialize(void)
{
  cfg().getValue(name(), "DTMF_HANGTIME", m_hangtime);
  return true;
} /* DtmfDecoder::initialize */


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

