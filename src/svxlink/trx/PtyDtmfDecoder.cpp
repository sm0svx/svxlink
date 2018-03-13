/**
@file	 PtyDtmfDecoder.cpp
@brief   This file contains a class that add support for the Pty interface
@author  Tobias Blomberg / SM0SVX & Adi Bier / DL1HRC
@date	 2014-03-21

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
Copyright (C) 2004-2014  Tobias Blomberg / SM0SVX

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

#include "RefCountingPty.h"
#include "PtyDtmfDecoder.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace sigc;
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

PtyDtmfDecoder::PtyDtmfDecoder(Config &cfg, const string &name)
  : HwDtmfDecoder(cfg, name), pty(0)
{

} /* PtyDtmfDecoder::PtyDtmfDecoder */


PtyDtmfDecoder::~PtyDtmfDecoder(void)
{
  if (pty != 0) pty->destroy();
  pty = 0;
} /* PtyDtmfDecoder::~PtyDtmfDecoder */


bool PtyDtmfDecoder::initialize(void)
{
  if (!HwDtmfDecoder::initialize())
  {
    return false;
  }

  string dtmf_pty;
  if (!cfg().getValue(name(), "DTMF_PTY", dtmf_pty))
  {
    cerr << "*** ERROR: Config variable " << name()
      	 << "/DTMF_PTY not specified\n";
    return false;
  }

  pty = RefCountingPty::instance(dtmf_pty);
  if (pty == 0)
  {
    return false;
  }
  pty->dataReceived.connect(
      sigc::mem_fun(*this, &PtyDtmfDecoder::dataReceived));
  return true;
} /* PtyDtmfDecoder::initialize */


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

void PtyDtmfDecoder::dataReceived(const void *buf, size_t count)
{
  const char *ptr = reinterpret_cast<const char*>(buf);
  for (size_t i=0; i<count; ++i)
  {
    const char &cmd = *ptr++;
    if (cmd == ' ')
    {
      digitIdle(); // DTMF digit deactivated
    }
    else if (((cmd >= '0') && (cmd <= '9')) ||
        ((cmd >= 'A') && (cmd <= 'D')) ||
        (cmd == '*') ||
        (cmd == '#'))
    {
      digitActive(cmd); // DTMF digit activated
    }
    else if (cmd == 'E')
    {
      digitActive('*'); // DTMF digit E==* activated
    }
    else if (cmd == 'F')
    {
      digitActive('#'); // DTMF digit F==# activated
    }
    /*
    else
    {
      cerr << "*** WARNING: Illegal DTMF PTY command received: " << cmd << endl;
    }
    */
  }
} /* PtyDtmfDecoder::dataReceived */



/*
 * This file has not been truncated
 */

