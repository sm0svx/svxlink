/**
@file	 MainWindow.cpp
@brief   Implementation class for the main window
@author  Tobias Blomberg
@date	 2003-03-09

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2009 Tobias Blomberg / SM0SVX

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

#include <qlistview.h>
#include <qtextbrowser.h>
#include <qstatusbar.h>
#include <qtimer.h>
#include <qmessagebox.h>
#include <qaction.h>
#include <qpixmap.h>
#include <qlabel.h>
#include <qtooltip.h>
#include <qpushbutton.h>
#include <qinputdialog.h>
#include <qsplitter.h>
#undef emit

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncIpAddress.h>
#include <EchoLinkDirectory.h>


/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

#include "images/offline_icon.xpm"
#include "images/online_icon.xpm"
#include "images/busy_icon.xpm"

#include "Settings.h"
#include "EchoLinkDispatcher.h"
#include "ComDialog.h"
#include "MainWindow.h"
#include "MsgHandler.h"


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
MainWindow::MainWindow(Directory &dir)
  : dir(dir), refresh_call_list_timer(0), is_busy(false), audio_io(0),
    prev_status(StationData::STAT_UNKNOWN)
{
  connect(explorerView, SIGNAL(currentChanged(QListViewItem*)),
      	  this, SLOT(explorerViewClicked(QListViewItem*)));
  connect(stationView, SIGNAL(doubleClicked(QListViewItem*)),
      	  this, SLOT(stationViewDoubleClicked(QListViewItem*)));
  connect(stationView, SIGNAL(selectionChanged(void)),
      	  this, SLOT(stationViewSelectionChanged(void)));
  connect(stationView, SIGNAL(returnPressed(QListViewItem*)),
      	  this, SLOT(stationViewDoubleClicked(QListViewItem*)));
  connect(directoryRefreshAction, SIGNAL(activated()),
      	  this, SLOT(refreshCallList()));
  connect(incoming_con_view, SIGNAL(selectionChanged(QListViewItem*)),
      	  this, SLOT(incomingSelectionChanged(QListViewItem*)));
  connect(incoming_con_view, SIGNAL(doubleClicked(QListViewItem*)),
      	  this, SLOT(acceptIncoming()));
  connect(incoming_con_view, SIGNAL(returnPressed(QListViewItem*)),
      	  this, SLOT(acceptIncoming()));
  connect(incoming_clear_button, SIGNAL(clicked()),
      	  this, SLOT(clearIncomingList()));
  connect(incoming_accept_button, SIGNAL(clicked()),
      	  this, SLOT(acceptIncoming()));
  
  AudioIO::setChannels(1);
  
  audio_io = new AudioIO(Settings::instance()->audioDevice().latin1(), 0);
  
  dir.error.connect(slot(*this, &MainWindow::serverError));
  dir.statusChanged.connect(slot(*this, &MainWindow::statusChanged));
  dir.stationListUpdated.connect(
      slot(*this, &MainWindow::callsignListUpdated));
  
  EchoLink::Dispatcher *disp = EchoLink::Dispatcher::instance();
  disp->incomingConnection.connect(slot(*this, &MainWindow::incomingConnection));

  status_indicator = new QLabel(statusBar());
  status_indicator->setPixmap(QPixmap(offline_icon));
  statusBar()->addWidget(status_indicator, 0, TRUE);
    
  is_busy = Settings::instance()->startAsBusy();
  directoryBusyAction->setOn(is_busy);
  updateRegistration();
  connect(reinterpret_cast<QObject *>(directoryBusyAction),
      	  SIGNAL(toggled(bool)),
      	  this, SLOT(setBusy(bool)));
  
  //statusBar()->message(trUtf8("Getting calls from directory server..."));
  //dir.getCalls();
  refresh_call_list_timer = new QTimer(this, "refresh_call_list_timer");
  refresh_call_list_timer->start(
      1000 * 60 * Settings::instance()->listRefreshTime());
  QObject::connect(refresh_call_list_timer, SIGNAL(timeout()),
      this, SLOT(refreshCallList()));
  
  QListViewItem *item = explorerView->findItem(
      MainWindowBase::trUtf8("Bookmarks"), 0);
  assert(item != 0);
  explorerView->setSelected(item, TRUE);
  
  incoming_con_view->setSorting(-1);
  
  station_view_popup = new QPopupMenu(stationView);
  station_view_popup_add =
      station_view_popup->insertItem(trUtf8("Add to bookmarks"), this,
      SLOT(addSelectedToBookmarks()));
  station_view_popup_remove =
      station_view_popup->insertItem(trUtf8("Remove from bookmarks"), this,
      SLOT(removeSelectedFromBookmarks()));
  station_view_popup_add_named =
      station_view_popup->insertItem(trUtf8("Add named station..."), this,
      SLOT(addNamedStationToBookmarks()));
  connect(
      stationView,
      SIGNAL(rightButtonClicked(QListViewItem*, const QPoint&, int)),
      this,
      SLOT(stationViewRightClicked(QListViewItem*, const QPoint&, int)));
  
  if (!Settings::instance()->mainWindowSize().isNull())
  {
    resize(Settings::instance()->mainWindowSize());
    vsplitter->setSizes(Settings::instance()->vSplitterSizes());
    hsplitter->setSizes(Settings::instance()->hSplitterSizes());
  }
  
  Settings::instance()->configurationUpdated.connect(
      slot(*this, &MainWindow::configurationUpdated));
  
  fileQuitAction->setAccel(
      QKeySequence(trUtf8("Ctrl+Q", "fileQuitAction")));
  directoryRefreshAction->setAccel(
      QKeySequence(trUtf8("F5", "directoryRefreshAction")));
  directoryBusyAction->setAccel(
      QKeySequence(trUtf8("Ctrl+B", "directoryBusyAction")));
  
  msg_audio_io = new AudioIO(Settings::instance()->audioDevice().latin1(), 0);  
  msg_handler = new MsgHandler("/", msg_audio_io->sampleRate());
  msg_handler->allMsgsWritten.connect(slot(*this, &MainWindow::allMsgsWritten));
  msg_audio_io->registerSource(msg_handler);
} /* MainWindow::MainWindow */


MainWindow::~MainWindow(void)
{
  delete msg_handler;
  msg_handler = 0;
  delete audio_io;
  audio_io = 0;
  delete msg_audio_io;
  msg_audio_io = 0;
  Settings::instance()->setMainWindowSize(size());
  Settings::instance()->setHSplitterSizes(hsplitter->sizes());
  Settings::instance()->setVSplitterSizes(vsplitter->sizes());
  dir.makeOffline();
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
  struct tm *tm = localtime(&t);
  char time_str[16];
  strftime(time_str, sizeof(time_str), "%H:%M", tm);
  
  QListViewItem *item = incoming_con_view->findItem(remote_call.c_str(), 0);
  if (item == 0)
  {
    item = new QListViewItem(incoming_con_view, remote_call.c_str(),
	remote_name.c_str(), time_str);
    
    msg_audio_io->open(AudioIO::MODE_WR);
      // FIXME: Hard coded filename
    msg_handler->playFile(Settings::instance()->connectSound().latin1());
  }
  else
  {
    incoming_con_view->takeItem(item);
    incoming_con_view->insertItem(item);
    item->setText(2, time_str);
  }
  incoming_con_view->setSelected(item, TRUE);
  incoming_con_param[remote_call] = remote_priv;
  
} /* MainWindow::incomingConnection */


void MainWindow::closeEvent(QCloseEvent *e)
{
  static int close_count = 0;
  
  if ((dir.status() != StationData::STAT_OFFLINE) && (close_count++ == 0))
  {
    statusBar()->message(trUtf8("Logging off from directory server..."));
    dir.makeOffline();
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
  statusBar()->message(msg.c_str(), 5000);
  server_msg_view->append(msg.c_str());
} /* MainWindow::serverError */


void MainWindow::statusChanged(StationData::Status status)
{
  switch (status)
  {
    case StationData::STAT_ONLINE:
      status_indicator->setPixmap(
	  QPixmap(const_cast<const char **>(online_icon)));
      if (prev_status != StationData::STAT_BUSY)
      {
      	refreshCallList();
      }
      break;
      
    case StationData::STAT_BUSY:
      status_indicator->setPixmap(QPixmap(busy_icon));
      if (prev_status != StationData::STAT_ONLINE)
      {
      	refreshCallList();
      }
      break;
      
    case StationData::STAT_OFFLINE:
      status_indicator->setPixmap(QPixmap(offline_icon));
      close();
      break;
      
    case StationData::STAT_UNKNOWN:
      status_indicator->setPixmap(QPixmap(offline_icon));
      break;
      
  }
  
  prev_status = status;
  
} /* MainWindow::statusChanged */


void MainWindow::allMsgsWritten(void)
{
  cout << "MainWindow::allMsgsWritten\n";
  msg_audio_io->flushSamples();
} /* MainWindow::allMsgsWritten */


void MainWindow::allSamplesFlushed(void)
{
  cout << "MainWindow::allSamplesFlushed\n";
  msg_audio_io->close();  
} /* MainWindow::allSamplesFlushed */




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
void MainWindow::initialize(void)
{
} /* MainWindow::initialize */


void MainWindow::explorerViewClicked(QListViewItem* item)
{
  list<StationData> station_list;
  
  //stationView->setUpdatesEnabled(FALSE);
  
  if (item == 0)
  {
    //stationView->clear();
  }
  else if (item->text(0) == MainWindowBase::trUtf8("Bookmarks"))
  {
    //stationView->clear();
    QStringList bookmarks = Settings::instance()->bookmarks();
    QStringList::iterator it;
    for (it = bookmarks.begin(); it != bookmarks.end(); ++it)
    {
      const StationData *station = dir.findCall((*it).latin1());
      if (station != 0)
      {
	station_list.push_back(*station);
      }
      else
      {
	StationData stn;
	stn.setCallsign((*it).latin1());
	stn.setStatus(StationData::STAT_OFFLINE);
	stn.setIp(Async::IpAddress("0.0.0.0"));
	station_list.push_back(stn);
      }
    }
  }
  else if (item->text(0) == MainWindowBase::trUtf8("Links"))
  {
    station_list = dir.links();
  }
  else if (item->text(0) == MainWindowBase::trUtf8("Repeaters"))
  {
    station_list = dir.repeaters();
  }
  else if (item->text(0) == MainWindowBase::trUtf8("Conferences"))
  {
    station_list = dir.conferences();
  }
  else if (item->text(0) == MainWindowBase::trUtf8("Stations"))
  {
    station_list = dir.stations();
  }
  
  stationView->clear();
  list<StationData>::const_iterator iter;
  for (iter=station_list.begin(); iter!=station_list.end(); ++iter)
  {
    char id_str[256] = "";
    if (iter->id() != -1)
    {
      sprintf(id_str, "%d", iter->id());
    }
    new QListViewItem(stationView, iter->callsign().c_str(),
	      	      iter->description().c_str(), iter->statusStr().c_str(),
	      	      iter->time().c_str(), id_str);
  }
  
  if (stationView->firstChild() != 0)
  {
    QListViewItem *selected_item = stationView->findItem(select_map[item], 0);
    if (selected_item == 0)
    {
      selected_item = stationView->firstChild();
      select_map[item] = selected_item->text(0);
    }
    if (selected_item != 0)
    {
      stationView->setSelected(selected_item, true);
      stationView->ensureItemVisible(selected_item);
      QRect rect = stationView->itemRect(selected_item);
      int target_top = stationView->viewport()->height() / 2 -
	  selected_item->height() / 2;
      if (rect.top() > target_top)
      {
	stationView->scrollBy(0, rect.top() - target_top);
      }
    }
  }
  
  //stationView->setUpdatesEnabled(TRUE);
  //stationView->update();
  
} /* MainWindow::explorerViewClicked */


void MainWindow::stationViewDoubleClicked(QListViewItem *item)
{
  ComDialog *com_dialog = new ComDialog(audio_io, dir, item->text(0), "?");
  com_dialog->show();
} /* MainWindow::stationViewDoubleClicked */


void MainWindow::stationViewSelectionChanged(void)
{
  QListViewItem *explorer_item = explorerView->selectedItem();
  QListViewItem *item = stationView->selectedItem();
  if (explorer_item != 0)
  {
    select_map[explorer_item] = (item != 0) ? item->text(0) : "";
  }
  connectionConnectToSelectedAction->setEnabled(item != 0);
} /* MainWindow::stationViewSelectionChanged */


void MainWindow::callsignListUpdated(void)
{
  /*
  QString selected_callsign;
  QListViewItem *item = stationView->selectedItem();
  if (item != 0)
  {
    selected_callsign = item->text(0);
  }
  */
  
  QListViewItem *item = explorerView->selectedItem();
  if (item != 0)
  {
    explorerViewClicked(item);
  }
  
  /*
  if (!selected_callsign.isEmpty())
  {
    item = stationView->findItem(selected_callsign, 0);
    if (item != 0)
    {
      stationView->setSelected(item, true);
      stationView->ensureItemVisible(item);
    }
  }
  */
  
  statusBar()->message(trUtf8("Station list has been refreshed"), 5000);
  
  const string &msg = dir.message();
  if (msg != old_server_msg)
  {
    server_msg_view->append(msg.c_str());
    old_server_msg = msg;
  }
    
} /* MainWindow::callsignListUpdated */


void MainWindow::refreshCallList(void)
{
  statusBar()->message(trUtf8("Refreshing station list..."));
  dir.getCalls();
} /* MainWindow::refreshCallList */


void MainWindow::updateRegistration(void)
{
  if (is_busy)
  {
    dir.makeBusy();
  }
  else
  {
    dir.makeOnline();
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


void MainWindow::incomingSelectionChanged(QListViewItem *item)
{
  incoming_accept_button->setEnabled(item != 0);
} /* MainWindow::incomingSelectionChanged */


void MainWindow::clearIncomingList(void)
{
  incoming_con_view->clear();
  incoming_accept_button->setEnabled(FALSE);
} /* MainWindow::clearIncomingList */


void MainWindow::acceptIncoming(void)
{
  QListViewItem *item = incoming_con_view->selectedItem();
  assert(item != 0);
  incoming_accept_button->setEnabled(FALSE);
  ComDialog *com_dialog = new ComDialog(audio_io, dir, item->text(0),
      item->text(1));
  com_dialog->show();
  com_dialog->acceptConnection();
  com_dialog->setRemoteParams(incoming_con_param[item->text(0)]);
  incoming_con_view->takeItem(item);
  delete item;
} /* MainWindow::acceptIncoming */


void MainWindow::stationViewRightClicked(QListViewItem *item,
    const QPoint &point, int column)    
{
  item = explorerView->selectedItem();
  if ((item == 0) || (stationView->selectedItem() == 0))
  {
    station_view_popup->setItemEnabled(station_view_popup_add, FALSE);
    station_view_popup->setItemEnabled(station_view_popup_remove, FALSE);
  }
  else if (item->text(0) == MainWindowBase::trUtf8("Bookmarks"))
  {
    station_view_popup->setItemEnabled(station_view_popup_add, FALSE);
    station_view_popup->setItemEnabled(station_view_popup_remove, TRUE);
  }
  else
  {
    station_view_popup->setItemEnabled(station_view_popup_add, TRUE);
    station_view_popup->setItemEnabled(station_view_popup_remove, FALSE);
  }
  station_view_popup->popup(point);
} /* MainWindow::stationViewRightClicked */


void MainWindow::addSelectedToBookmarks(void)
{
  QListViewItem *item = stationView->selectedItem();
  assert(item != 0);
  QStringList bookmarks = Settings::instance()->bookmarks();
  if (bookmarks.find(item->text(0)) == bookmarks.end())
  {
    bookmarks.append(item->text(0));
    Settings::instance()->setBookmarks(bookmarks);
  }
} /* MainWindow::addSelectedToBookmarks */


void MainWindow::removeSelectedFromBookmarks(void)
{
  QListViewItem *item = stationView->selectedItem();
  assert(item != 0);
  QStringList bookmarks = Settings::instance()->bookmarks();
  QStringList::iterator it = bookmarks.find(item->text(0));
  if (it != bookmarks.end())
  {
    bookmarks.remove(it);
    Settings::instance()->setBookmarks(bookmarks);
    item = explorerView->selectedItem();
    if (item != 0)
    {
      explorerViewClicked(item);
    }    
  }
} /* MainWindow::removeSelectedFromBookmarks */


void MainWindow::addNamedStationToBookmarks(void)
{
  QString call = QInputDialog::getText(trUtf8("Qtel - Add station..."),
      trUtf8("Enter callsign of the station to add"));
  
  if (!call.isEmpty())
  {
    call = call.upper();
    QStringList bookmarks = Settings::instance()->bookmarks();
    if (bookmarks.find(call) == bookmarks.end())
    {
      bookmarks.append(call);
      Settings::instance()->setBookmarks(bookmarks);
      QListViewItem *item = explorerView->selectedItem();
      if ((item != 0) && (item->text(0) == MainWindowBase::trUtf8("Bookmarks")))
      {
	explorerViewClicked(item);
      }
    }
  }
  
} /* MainWindow::addNamedStationToBookmarks */


void MainWindow::configurationUpdated(void)
{
  dir.setServer(Settings::instance()->directoryServer().latin1());
  dir.setCallsign(Settings::instance()->callsign().latin1());
  dir.setPassword(Settings::instance()->password().latin1());
  dir.setDescription(Settings::instance()->location().latin1());
  updateRegistration();
  
  refresh_call_list_timer->changeInterval(
      1000 * 60 * Settings::instance()->listRefreshTime());
  
  delete audio_io;
  audio_io = new AudioIO(Settings::instance()->audioDevice().latin1(), 0);

} /* MainWindow::configurationChanged */


void MainWindow::connectionConnectToIpActionActivated(void)
{
  bool ok;
  QString remote_host = QInputDialog::getText(
	    trUtf8("Qtel: Connect to IP"),
	    trUtf8("Enter an IP address or hostname:"),
	    QLineEdit::Normal, Settings::instance()->connectToIp(), &ok, this);
  if (ok)
  {
    Settings::instance()->setConnectToIp(remote_host);
    if (!remote_host.isEmpty())
    {
      ComDialog *com_dialog = new ComDialog(audio_io, dir, remote_host);
      com_dialog->show();
    }
  }
} /* MainWindow::connectionConnectToIpActionActivated */


void MainWindow::connectionConnectToSelectedActionActivated(void)
{
  QListViewItem *item = stationView->selectedItem();
  if (item != 0)
  {
    ComDialog *com_dialog = new ComDialog(audio_io, dir, item->text(0), "?");
    com_dialog->show();
  }
} /* MainWindow::connectionConnectToSelectedActionActivated */



/*
 * This file has not been truncated
 */

