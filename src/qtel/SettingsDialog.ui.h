/****************************************************************************
** ui.h extension file, included from the uic-generated form implementation.
**
** If you wish to add, delete or rename slots use Qt Designer which will
** update this file, preserving your code. Create an init() slot in place of
** a constructor, and a destroy() slot in place of a destructor.
*****************************************************************************/

#include <qfiledialog.h>
#include <EchoLinkStationData.h>

void SettingsDialog::init()
{
    my_callsign->setMaxLength(
     EchoLink::StationData::MAXCALL);
    my_location->setMaxLength(
     EchoLink::StationData::MAXDESC);
}


void SettingsDialog::browseConnectSound()
{
    QString s = QFileDialog::getOpenFileName(
                    connect_sound->text(),
                    tr("Raw Sound Files (*.raw)"),
                    this,
                    "choose connect sound browse dialog",
                    tr("Choose a connect sound file"));
    if (!s.isNull())
    {
        connect_sound->setText(s);
    }
}
