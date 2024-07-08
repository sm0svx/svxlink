/**
@file	 MainWindow.cpp
@brief   Implementation class for the main window
@author  Tobias Blomberg
@date	 2003-03-09

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




/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <cassert>

#include <sigc++/sigc++.h>

#include <QStatusBar>
#include <QTimer>
#include <QMessageBox>
#include <QAction>
#include <QPixmap>
#include <QLabel>
#include <QToolTip>
#include <QPushButton>
#include <QInputDialog>
#include <QSplitter>
#include <QCloseEvent>
#undef emit


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <AsyncAudioInterpolator.h>
#include <EchoLinkDirectory.h>
#include <EchoLinkProxy.h>
#include <common.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "version/QTEL.h"
#include "Settings.h"
#include "EchoLinkDispatcher.h"
#include "ComDialog.h"
#include "MainWindow.h"
#include "MsgHandler.h"
#include "EchoLinkDirectoryModel.h"
#include "multirate_filter_coeff.h"


/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;
using namespace EchoLink;


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
 * Method:    MainWindow::MainWindow
 * Purpose:   Constructor
 * Input:     dir - An EchoLink directory server object
 * Output:    None
 * Author:    Tobias Blomberg
 * Created:   2003-03-09
 * Remarks:   
 * Bugs:      
 *------------------------------------------------------------------------
 */
MainWindow::MainWindow(void)
  : dir(0), refresh_call_list_timer(0), is_busy(false), msg_handler(0),
    msg_audio_io(0), prev_status(StationData::STAT_UNKNOWN), proxy(0)
{
  setupUi(this);
  
  station_view->addAction(connectionConnectToSelectedAction);
  QAction *separator = new QAction(this);
  separator->setSeparator(true);
  station_view->addAction(separator);
  station_view->addAction(addNamedStationToBookmarksAction);
  station_view->addAction(addSelectedToBookmarksAction);
  station_view->addAction(removeSelectedFromBookmarksAction);

  connect(directoryRefreshAction, SIGNAL(triggered()),
      	  this, SLOT(refreshCallList()));
  connect(connectionConnectToSelectedAction, SIGNAL(triggered()),
      	  this, SLOT(connectionConnectToSelectedActionActivated()));
  connect(connectionConnectToIpAction, SIGNAL(triggered()),
      	  this, SLOT(connectionConnectToIpActionActivated()));
  connect(addSelectedToBookmarksAction, SIGNAL(triggered()),
	  this, SLOT(addSelectedToBookmarks()));
  connect(removeSelectedFromBookmarksAction, SIGNAL(triggered()),
	  this, SLOT(removeSelectedFromBookmarks()));
  connect(addNamedStationToBookmarksAction, SIGNAL(triggered()),
	  this, SLOT(addNamedStationToBookmarks()));
  connect(settingsConfigureAction, SIGNAL(triggered()),
      	  this, SLOT(settings()));
  connect(helpAboutAction, SIGNAL(triggered()),
      	  this, SLOT(helpAbout()));

  connect(station_view_selector, SIGNAL(currentItemChanged(QListWidgetItem*,
						           QListWidgetItem*)),
	  this, SLOT(stationViewSelectorCurrentItemChanged(QListWidgetItem*,
						           QListWidgetItem*)));
  connect(station_view, SIGNAL(activated(const QModelIndex&)),
	  this, SLOT(stationViewDoubleClicked(const QModelIndex&)));
  connect(incoming_con_view, SIGNAL(itemSelectionChanged()),
      	  this, SLOT(incomingSelectionChanged()));
  connect(incoming_con_view, SIGNAL(itemActivated(QTreeWidgetItem*,int)),
      	  this, SLOT(acceptIncoming()));
  connect(incoming_clear_button, SIGNAL(clicked()),
      	  this, SLOT(clearIncomingList()));
  connect(incoming_accept_button, SIGNAL(clicked()),
      	  this, SLOT(acceptIncoming()));
  
  setupAudioParams();
  initEchoLink();

  status_indicator = new QLabel(statusBar());
  status_indicator->setPixmap(QPixmap(":/icons/images/offline_icon.xpm"));
  statusBar()->addPermanentWidget(status_indicator);
    
  is_busy = Settings::instance()->startAsBusy();
  directoryBusyAction->setChecked(is_busy);
  updateRegistration();
  connect(directoryBusyAction, SIGNAL(toggled(bool)),
      	  this, SLOT(setBusy(bool)));
  
  //statusBar()->message(tr("Getting calls from directory server..."));
  //dir->getCalls();
  refresh_call_list_timer = new QTimer(this);
  refresh_call_list_timer->start(
      1000 * 60 * Settings::instance()->listRefreshTime());
  connect(refresh_call_list_timer, SIGNAL(timeout()),
      this, SLOT(refreshCallList()));

  if (Settings::instance()->mainWindowSize().isValid())
  {
    resize(Settings::instance()->mainWindowSize());
    vsplitter->setSizes(Settings::instance()->vSplitterSizes());
    hsplitter->setSizes(Settings::instance()->hSplitterSizes());
  }
  
  Settings::instance()->configurationUpdated.connect(
      mem_fun(*this, &MainWindow::configurationUpdated));
  
  bookmark_model = new EchoLinkDirectoryModel(this);
  conf_model = new EchoLinkDirectoryModel(this);
  link_model = new EchoLinkDirectoryModel(this);
  repeater_model = new EchoLinkDirectoryModel(this);
  station_model = new EchoLinkDirectoryModel(this);
  updateBookmarkModel();
  station_view_selector->setCurrentRow(0);

  QList<int> sizes = Settings::instance()->stationViewColSizes();
  for (int i=0; i<sizes.size(); ++i)
  {
    station_view->setColumnWidth(i, sizes.at(i));
  }
  
  sizes = Settings::instance()->incomingViewColSizes();
  for (int i=0; i<sizes.size(); ++i)
  {
    incoming_con_view->setColumnWidth(i, sizes.at(i));
  }
  
  initMsgAudioIo();
} /* MainWindow::MainWindow */


MainWindow::~MainWindow(void)
{
  delete msg_handler;
  msg_handler = 0;
  delete msg_audio_io;
  msg_audio_io = 0;
  Settings::instance()->setMainWindowSize(size());
  Settings::instance()->setHSplitterSizes(hsplitter->sizes());
  Settings::instance()->setVSplitterSizes(vsplitter->sizes());
  
  QList<int> sizes;
  int size;
  for (int i=0; (size = station_view->columnWidth(i)) > 0; ++i)
  {
    sizes << size;
  }
  Settings::instance()->setStationViewColSizes(sizes);

  sizes.clear();
  for (int i=0; (size = incoming_con_view->columnWidth(i)) > 0; ++i)
  {
    sizes << size;
  }
  Settings::instance()->setIncomingViewColSizes(sizes);
  
  delete dir;

  Dispatcher::deleteInstance();
} /* MainWindow::~MainWindow */



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


void MainWindow::incomingConnection(const IpAddress& remote_ip,
    const string& remote_call, const string& remote_name,
    const string& remote_priv)
{
  time_t t = time(0);
  struct tm tm;
  char time_str[16];
  strftime(time_str, sizeof(time_str), "%H:%M", localtime_r(&t, &tm));
  
  QList<QTreeWidgetItem*> items = incoming_con_view->findItems(
		QString::fromStdString(remote_call),
		Qt::MatchExactly);
  QTreeWidgetItem *item = 0;
  if (items.size() == 0)
  {
    QStringList values;
    values << QString::fromStdString(remote_call)
           << QString::fromStdString(remote_name)
           << QString(time_str);
    item = new QTreeWidgetItem(values);
    msg_audio_io->open(AudioIO::MODE_WR);
    msg_handler->playFile(Settings::instance()->connectSound().toStdString());
  }
  else
  {
    Q_ASSERT(items.size() == 1);
    item = items.at(0);
    int item_index = incoming_con_view->indexOfTopLevelItem(item);
    Q_ASSERT(item_index >= 0);
    incoming_con_view->takeTopLevelItem(item_index);
    item->setText(2, time_str);
  }
  incoming_con_view->insertTopLevelItem(0, item);
  incoming_con_view->setCurrentItem(item);
  incoming_con_param[QString::fromStdString(remote_call)] =
      QString::fromStdString(remote_priv);
  
} /* MainWindow::incomingConnection */


void MainWindow::closeEvent(QCloseEvent *e)
{
  static int close_count = 0;
  
  if ((dir->status() != StationData::STAT_OFFLINE) && (close_count++ == 0))
  {
    statusBar()->showMessage(tr("Logging off from directory server..."));
    dir->makeOffline();
    QTimer::singleShot(5000, this, SLOT(forceQuit()));
    e->ignore();
  }
  else
  {
    e->accept();
  }
} /* MainWindow::closeEvent */


void MainWindow::serverError(const string& msg)
{
  cout << msg << endl;
  statusBar()->showMessage(msg.c_str(), 5000);
  server_msg_view->append(msg.c_str());
} /* MainWindow::serverError */


void MainWindow::statusChanged(StationData::Status status)
{
  switch (status)
  {
    case StationData::STAT_ONLINE:
      status_indicator->setPixmap(
	  QPixmap(":/icons/images/online_icon.xpm"));
      if (prev_status != StationData::STAT_BUSY)
      {
      	refreshCallList();
      }
      break;
      
    case StationData::STAT_BUSY:
      status_indicator->setPixmap(QPixmap(":/icons/images/busy_icon.xpm"));
      if (prev_status != StationData::STAT_ONLINE)
      {
      	refreshCallList();
      }
      break;
      
    case StationData::STAT_OFFLINE:
      status_indicator->setPixmap(QPixmap(":/icons/images/offline_icon.xpm"));
      close();
      break;
      
    case StationData::STAT_UNKNOWN:
      status_indicator->setPixmap(QPixmap(":/icons/images/offline_icon.xpm"));
      break;
      
  }
  
  prev_status = status;
  
} /* MainWindow::statusChanged */


void MainWindow::allMsgsWritten(void)
{
  //cout << "MainWindow::allMsgsWritten\n";
  msg_audio_io->close();  
} /* MainWindow::allMsgsWritten */


void MainWindow::initMsgAudioIo(void)
{
  delete msg_audio_io;
  delete msg_handler;
  
  msg_audio_io =
      new AudioIO(Settings::instance()->spkrAudioDevice().toStdString(), 0);

  msg_handler = new MsgHandler(INTERNAL_SAMPLE_RATE);
  msg_handler->allMsgsWritten.connect(
    mem_fun(*this, &MainWindow::allMsgsWritten));
  AudioSource *prev_src = msg_handler;

#if (INTERNAL_SAMPLE_RATE == 8000)  
  if (msg_audio_io->sampleRate() > 8000)
  {
      // Interpolate sample rate to 16kHz
    AudioInterpolator *i1 = new AudioInterpolator(2, coeff_16_8,
                                                  coeff_16_8_taps);
    prev_src->registerSink(i1, true);
    prev_src = i1;
  }
#endif

  if (msg_audio_io->sampleRate() > 16000)
  {
      // Interpolate sample rate to 48kHz
#if (INTERNAL_SAMPLE_RATE == 8000)
    AudioInterpolator *i2 = new AudioInterpolator(3, coeff_48_16_int,
                                                  coeff_48_16_int_taps);
#else
    AudioInterpolator *i2 = new AudioInterpolator(3, coeff_48_16,
                                                  coeff_48_16_taps);
#endif
    prev_src->registerSink(i2, true);
    prev_src = i2;
  }

  prev_src->registerSink(msg_audio_io);
  
} /* MainWindow::initMsgAudioIo */


void MainWindow::setupAudioParams(void)
{
  int rate = Settings::instance()->cardSampleRate();
  if (rate == 48000)
  {
    AudioIO::setBlocksize(1024);
    AudioIO::setBlockCount(4);
  }
  else if (rate == 16000)
  {
    AudioIO::setBlocksize(512);
    AudioIO::setBlockCount(2);
  }
#if INTERNAL_SAMPLE_RATE <= 8000
  else if (rate == 8000)
  {
    AudioIO::setBlocksize(256);
    AudioIO::setBlockCount(2);
  }
#endif
  AudioIO::setSampleRate(rate);
  AudioIO::setChannels(1);
} /* MainWindow::setupAudioParams */


void MainWindow::initEchoLink(void)
{
  Settings *settings = Settings::instance();

    // First clean up any old instances
  Dispatcher::deleteInstance();
  delete dir;
  dir = 0;
  delete proxy;
  proxy = 0;

  if (settings->proxyEnabled())
  {
    proxy = new Proxy(
        settings->proxyServer().toStdString(),
        settings->proxyPort(),
        settings->callsign().toStdString(),
        settings->proxyPassword().toStdString());
    proxy->connect();
  }

  Async::IpAddress bind_ip;
  if (!settings->bindAddress().isEmpty())
  {
    bind_ip = Async::IpAddress(settings->bindAddress().toStdString());
  }

  vector<string> servers;
  SvxLink::splitStr(servers, settings->directoryServers().toStdString(), " ");
  dir = new Directory(
      servers,
      settings->callsign().toStdString(),
      settings->password().toStdString(),
      settings->location().toStdString(),
      bind_ip);
  dir->error.connect(mem_fun(*this, &MainWindow::serverError));
  dir->statusChanged.connect(mem_fun(*this, &MainWindow::statusChanged));
  dir->stationListUpdated.connect(
      mem_fun(*this, &MainWindow::callsignListUpdated));

  Dispatcher::setBindAddr(bind_ip);
  Dispatcher *disp = Dispatcher::instance();
  if (disp == 0)
  {
    fprintf(stderr, "Could not initialize network listen ports\n");
    return;
  }
  disp->incomingConnection.connect(
      mem_fun(*this, &MainWindow::incomingConnection));
} /* MainWindow::initEchoLink */


void MainWindow::updateBookmarkModel(void)
{
  list<StationData> bookmarks;
  QStringList callsigns = Settings::instance()->bookmarks();
  QStringList::iterator it;
  foreach (QString callsign, callsigns)
  {
    const StationData *station = dir->findCall(callsign.toStdString());
    if (station != 0)
    {
      bookmarks.push_back(*station);
    }
    else
    {
      StationData stn;
      stn.setCallsign(callsign.toStdString());
      stn.setStatus(StationData::STAT_OFFLINE);
      stn.setIp(Async::IpAddress("0.0.0.0"));
      bookmarks.push_back(stn);
    }
  }
  bookmark_model->updateStationList(bookmarks);
  
} /* MainWindow::updateBookmarkModel */



void MainWindow::stationViewSelectorCurrentItemChanged(QListWidgetItem *current,
						       QListWidgetItem *previous
						      )
{
  Q_UNUSED(previous);
  
  if (current == 0)
  {
    return;
  }
  
  QAbstractItemModel *model = 0;
  QItemSelectionModel *m = station_view->selectionModel();
  if (current->text() == tr("Bookmarks"))
  {
    model = bookmark_model;
  }
  else if (current->text() == tr("Conferences"))
  {
    model = conf_model;
  }
  else if (current->text() == tr("Links"))
  {
    model = link_model;
  }
  else if (current->text() == tr("Repeaters"))
  {
    model = repeater_model;
  }
  else if (current->text() == tr("Stations"))
  {
    model = station_model;
  }
  
  station_view->setModel(model);
  if (m != 0)
  {
    m->clear();
    delete m;
  }
  
  QItemSelectionModel *sm = station_view->selectionModel();
  if (sm != 0)
  {
    connect(sm, SIGNAL(selectionChanged(const QItemSelection&,
					const QItemSelection&)),
	    this, SLOT(stationViewSelectionChanged(const QItemSelection&,
						   const QItemSelection&)));
  }

} /* MainWindow::stationViewSelectorCurrentItemChanged */


void MainWindow::stationViewDoubleClicked(const QModelIndex &index)
{
  connectionConnectToSelectedActionActivated();
} /* MainWindow::stationViewDoubleClicked */


void MainWindow::stationViewSelectionChanged(const QItemSelection &current,
					     const QItemSelection &previous)
{
  Q_UNUSED(previous);
  
  QModelIndexList indexes = current.indexes();
  bool item_selected = ((indexes.count() > 0) && indexes.at(0).isValid() &&
                        (indexes.at(0).column() == 0));
  
  connectionConnectToSelectedAction->setEnabled(item_selected);
  addSelectedToBookmarksAction->setEnabled(item_selected);
  removeSelectedFromBookmarksAction->setEnabled(item_selected);
  
} /* MainWindow::stationViewSelectionChanged */


void MainWindow::callsignListUpdated(void)
{
  updateBookmarkModel();
  
  conf_model->updateStationList(dir->conferences());
  link_model->updateStationList(dir->links());
  repeater_model->updateStationList(dir->repeaters());
  station_model->updateStationList(dir->stations());
  
  statusBar()->showMessage(tr("Station list has been refreshed"), 5000);
  
  const string &msg = dir->message();
  if (msg != old_server_msg)
  {
    server_msg_view->append(msg.c_str());
    old_server_msg = msg;
  }
    
} /* MainWindow::callsignListUpdated */


void MainWindow::refreshCallList(void)
{
  if (dir->status() >= StationData::STAT_ONLINE)
  {
    statusBar()->showMessage(tr("Refreshing station list..."));
    dir->getCalls();
  }
} /* MainWindow::refreshCallList */


void MainWindow::updateRegistration(void)
{
  if (is_busy)
  {
    dir->makeBusy();
  }
  else
  {
    dir->makeOnline();
  }
} /* MainWindow::updateRegistration */


void MainWindow::setBusy(bool busy)
{
  is_busy = busy;
  updateRegistration();
} /* MainWindow::setBusy */


void MainWindow::forceQuit(void)
{
  close();
} /* MainWindow::forceQuit */


void MainWindow::incomingSelectionChanged(void)
{
  incoming_accept_button->setEnabled(incoming_con_view->selectedItems().size() > 0);
} /* MainWindow::incomingSelectionChanged */


void MainWindow::clearIncomingList(void)
{
  incoming_con_view->clear();
  incoming_accept_button->setEnabled(false);
} /* MainWindow::clearIncomingList */


void MainWindow::acceptIncoming(void)
{
  QList<QTreeWidgetItem*> items = incoming_con_view->selectedItems();
  Q_ASSERT(items.size() == 1);
  QTreeWidgetItem *item = items.at(0);
  incoming_accept_button->setEnabled(false);
  ComDialog *com_dialog = new ComDialog(*dir, item->text(0), item->text(1));
  com_dialog->show();
  com_dialog->acceptConnection();
  com_dialog->setRemoteParams(incoming_con_param[item->text(0)]);
  int item_index = incoming_con_view->indexOfTopLevelItem(item);
  Q_ASSERT(item_index >= 0);
  incoming_con_view->takeTopLevelItem(item_index);
  delete item;
} /* MainWindow::acceptIncoming */


void MainWindow::addSelectedToBookmarks(void)
{
  QModelIndexList indexes = station_view->selectionModel()->selectedIndexes();
  if ((indexes.count() <= 0) || (indexes.at(0).column() != 0))
  {
    return;
  }
  
  QString callsign = indexes.at(0).data().toString();
  QStringList bookmarks = Settings::instance()->bookmarks();
  if (!callsign.isEmpty() && !bookmarks.contains(callsign))
  {
    bookmarks.append(callsign);
    Settings::instance()->setBookmarks(bookmarks);
  }
  updateBookmarkModel();

} /* MainWindow::addSelectedToBookmarks */


void MainWindow::removeSelectedFromBookmarks(void)
{
  QModelIndexList indexes = station_view->selectionModel()->selectedIndexes();
  if ((indexes.count() <= 0) || !indexes.at(0).isValid() ||
      (indexes.at(0).column() != 0))
  {
    return;
  }
  
  QString callsign = indexes.at(0).data().toString();
  QStringList bookmarks = Settings::instance()->bookmarks();
  if (!callsign.isEmpty() && bookmarks.contains(callsign))
  {
    bookmarks.removeAll(callsign);
    Settings::instance()->setBookmarks(bookmarks);
    updateBookmarkModel();
  }
} /* MainWindow::removeSelectedFromBookmarks */


void MainWindow::addNamedStationToBookmarks(void)
{
  QString call = QInputDialog::getText(this, tr("Qtel - Add station..."),
      tr("Enter callsign of the station to add"));
  
  if (!call.isEmpty())
  {
    call = call.toUpper();
    QStringList bookmarks = Settings::instance()->bookmarks();
    if (!bookmarks.contains(call))
    {
      bookmarks.append(call);
      Settings::instance()->setBookmarks(bookmarks);
      updateBookmarkModel();
    }
  }
  
} /* MainWindow::addNamedStationToBookmarks */


void MainWindow::configurationUpdated(void)
{
  vector<string> servers;
  SvxLink::splitStr(servers,
                    Settings::instance()->directoryServers().toStdString(),
                    " ");
  dir->setServers(servers);
  dir->setCallsign(Settings::instance()->callsign().toStdString());
  dir->setPassword(Settings::instance()->password().toStdString());
  dir->setDescription(Settings::instance()->location().toStdString());
  
  refresh_call_list_timer->setInterval(
      1000 * 60 * Settings::instance()->listRefreshTime());

  setupAudioParams();
  initMsgAudioIo();
  initEchoLink();
  updateRegistration();
} /* MainWindow::configurationChanged */


void MainWindow::connectionConnectToIpActionActivated(void)
{
  bool ok;
  QString remote_host = QInputDialog::getText(
	    this,
	    tr("Qtel: Connect to IP"),
	    tr("Enter an IP address or hostname:"),
	    QLineEdit::Normal, Settings::instance()->connectToIp(), &ok);
  if (ok)
  {
    Settings::instance()->setConnectToIp(remote_host);
    if (!remote_host.isEmpty())
    {
      ComDialog *com_dialog = new ComDialog(*dir, remote_host);
      com_dialog->show();
    }
  }
} /* MainWindow::connectionConnectToIpActionActivated */


void MainWindow::connectionConnectToSelectedActionActivated(void)
{
  QModelIndexList indexes = station_view->selectionModel()->selectedIndexes();
  if ((indexes.count() <= 0) || (indexes.at(0).column() != 0))
  {
    return;
  }
  
  QString callsign = indexes.at(0).data().toString();
  if (!callsign.isEmpty())
  {
    ComDialog *com_dialog = new ComDialog(*dir, callsign, "?");
    com_dialog->show();
  }
} /* MainWindow::connectionConnectToSelectedActionActivated */


void MainWindow::settings(void)
{
  Settings::instance()->showDialog();
} /* MainWindow::settings */


void MainWindow::helpAbout(void)
{
    QMessageBox::about(this, tr("About Qtel"),
        tr("Qtel v") + QTEL_VERSION +
        tr(" - Qt EchoLink client.\n") +
        "\n" +
        tr("Copyright (C) 2003-2024 Tobias Blomberg / SM0SVX\n\n"
           "Qtel comes with ABSOLUTELY NO WARRANTY. "
           "This is free software, and you "
           "are welcome to redistribute it in accordance with the "
           "terms and conditions in "
           "the GNU GPL (General Public License) version 2 or later."));
} /* MainWindow::helpAbout */



/*
 * This file has not been truncated
 */

