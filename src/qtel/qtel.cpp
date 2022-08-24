/**
@file	 qtel.cpp
@brief   An EchoLink client implemented in Qt
@author  Tobias Blomberg
@date	 2003-03-09

This is an EchoLink client implemented in Qt using the classes from EchoLib to
create a fully working EchoLink client.

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2009 Tobias Blomberg

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

#include <cstdio>
#include <cstdlib>

#include <sigc++/sigc++.h>

#include <QApplication>
#include <QMessageBox>
#include <QTranslator>
#include <QTextCodec>
#include <QLocale>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <config.h>

#include <AsyncQtApplication.h>
#include <AsyncTcpClient.h>

#include <EchoLinkDirectory.h>
#include <EchoLinkDispatcher.h>

	
/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "Settings.h"
#include "MainWindow.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace Async;
using namespace EchoLink;


/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

#define PROGRAM_NAME "qtel"

	

/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Global Variables
 *
 ****************************************************************************/
 


/****************************************************************************
 *
 * Private Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * MAIN
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Function:  main
 * Purpose:   Start everything...
 * Input:     argc  - The number of arguments passed to this program
 *    	      	      (including the program name).
 *    	      argv  - The arguments passed to this program. argv[0] is the
 *    	      	      program name.
 * Output:    Return 0 on success, else non-zero.
 * Author:    Tobias Blomberg
 * Created:   2003-03-09
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */
int main(int argc, char **argv)
{
  QtApplication app(argc, argv);
  
  QTranslator translator;
  translator.load(QString("qtel_%1").arg(QLocale::system().name()),
      SHARE_INSTALL_PREFIX "/qtel/translations");
  app.installTranslator(&translator);

  QCoreApplication::setOrganizationName("SvxLink");
  QCoreApplication::setOrganizationDomain("svxlink.org");
  QCoreApplication::setApplicationName("Qtel");
	
  Settings *settings = Settings::instance();
  bool cfg_ok = false;
  while (!cfg_ok)
  {
    settings->readSettings();
    if (settings->callsign().isEmpty() || settings->password().isEmpty() ||
	settings->name().isEmpty())
    {
      int button = QMessageBox::critical(0, "Bad configuration",
	  "There are one or more configuration items that have not been "
	  "filled in. Please fill in the missing items.",
	  "Configure", "Quit");
      if (button == 1)
      {
      	return 1;
      }
    }
    else
    {
      cfg_ok = true;
    }
  }
   
  MainWindow mainWindow;
  mainWindow.show();
  app.exec();
  
  delete Settings::instance();
  
  return 0;
}



/****************************************************************************
 *
 * Functions
 *
 ****************************************************************************/



/*
 * This file has not been truncated
 */
