/**
@file	 SettingsDialog.h
@brief   Wrapper for the SettingsDialog
@author  Tobias Blomberg / SM0SVX
@date	 2010-05-01

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2015 Tobias Blomberg / SM0SVX

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

#ifndef SETTINGS_DIALOG_INCLUDED
#define SETTINGS_DIALOG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <QFileDialog>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <EchoLinkStationData.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "ui_SettingsDialogBase.h"


/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

//namespace MyNameSpace
//{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

  

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

/**
@brief  Wrapper for the SettingsDialog
@author Tobias Blomberg / SM0SVX
@date   2010-05-01
*/
class SettingsDialog : public QDialog, public Ui::SettingsDialog
{
  Q_OBJECT
  
  public:
    SettingsDialog(void) 
    { 
      setupUi(this);
      my_callsign->setMaxLength(EchoLink::StationData::MAXCALL);
      my_location->setMaxLength(EchoLink::StationData::MAXDESC);
#if INTERNAL_SAMPLE_RATE == 8000
      card_sample_rate->addItem("8000");
#endif
      card_sample_rate->addItem("16000");
      card_sample_rate->addItem("48000");
    }

  private slots:
    void browseConnectSound(void)
    {
      QString s = QFileDialog::getOpenFileName(
			this,
			tr("Choose a connect sound file"),
			connect_sound->text(),
			tr("Raw Sound Files (*.raw)"));
      if (!s.isNull())
      {
	connect_sound->setText(s);
      }
    }
};


//} /* namespace */

#endif /* SETTINGS_DIALOG_INCLUDED */



/*
 * This file has not been truncated
 */

