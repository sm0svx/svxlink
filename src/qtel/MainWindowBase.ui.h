/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

#include <qmessagebox.h>

#include <version/QTEL.h>

#include "Settings.h"

void MainWindowBase::editUndo()
{

}

void MainWindowBase::editRedo()
{

}

void MainWindowBase::editCut()
{

}

void MainWindowBase::editCopy()
{

}

void MainWindowBase::editPaste()
{

}

void MainWindowBase::editFind()
{

}

void MainWindowBase::helpIndex()
{

}

void MainWindowBase::helpContents()
{

}

void MainWindowBase::helpAbout()
{
    QMessageBox::about(this, trUtf8("About Qtel"),
             trUtf8("Qtel v") + QTEL_VERSION +
	trUtf8("- Qt EchoLink client.\n") +
	"\n" +
	trUtf8("Author") + ": SM0SVX - Tobias Blomberg");
}

void MainWindowBase::settings()
{
    Settings::instance()->showDialog();    
}


