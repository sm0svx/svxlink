/**
@file	 EchoLinkDirectoryModel.cpp
@brief   A_brief_description_for_this_file
@author  Tobias Blomberg / SM0SVX
@date	 2011-06-18

A_detailed_description_for_this_file

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2010 Tobias Blomberg / SM0SVX

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

#include <algorithm>
#include <iostream>

#include <QtAlgorithms>
#include <QtGlobal>


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

#include "EchoLinkDirectoryModel.h"



/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
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

EchoLinkDirectoryModel::EchoLinkDirectoryModel(QObject *parent)
  : QAbstractItemModel(parent)
{
  
} /* EchoLinkDirectoryModel::EchoLinkDirectoryModel */


EchoLinkDirectoryModel::~EchoLinkDirectoryModel(void)
{
  
} /* EchoLinkDirectoryModel::~EchoLinkDirectoryModel */


void EchoLinkDirectoryModel::updateStationList(
				    const list<StationData> &stn_list)
{
#if QT_VERSION >= 0x050e00
  QList<StationData> updated_stations(stn_list.begin(), stn_list.end());
  std::stable_sort(updated_stations.begin(), updated_stations.end());
#else
  QList<StationData> updated_stations =
      QList<StationData>::fromStdList(stn_list);
  qStableSort(updated_stations);
#endif
  
  //cout << "### updated_stations=" << updated_stations.count() << endl;  
  
  int row = 0;
  while (!updated_stations.isEmpty() && (row < stations.count()))
  {
    const StationData &updated_stn = updated_stations.first();
    StationData &stn = stations[row];
    if (updated_stn.callsign() == stn.callsign())
    {
      if (updated_stn.description() != stn.description())
      {
	QModelIndex stn_index = index(row, 1);
	stn.setDescription(updated_stn.description());
	dataChanged(stn_index, stn_index);
      }
      if (updated_stn.status() != stn.status())
      {
	QModelIndex stn_index = index(row, 2);
	stn.setStatus(updated_stn.status());
	dataChanged(stn_index, stn_index);
      }
      if (updated_stn.time() != stn.time())
      {
	QModelIndex stn_index = index(row, 3);
	stn.setTime(updated_stn.time());
	dataChanged(stn_index, stn_index);
      }
      if (updated_stn.id() != stn.id())
      {
	QModelIndex stn_index = index(row, 4);
	stn.setId(updated_stn.id());
	dataChanged(stn_index, stn_index);
      }
      if (updated_stn.ip() != stn.ip())
      {
	QModelIndex stn_index = index(row, 5);
	stn.setIp(updated_stn.ip());
	dataChanged(stn_index, stn_index);
      }
      row += 1;
      updated_stations.removeFirst();
    }
    else if (updated_stn.callsign() < stn.callsign())
    {
      //cout << "### Inserting 1 row starting at row " << row << endl;
      beginInsertRows(QModelIndex(), row, row);
      stations.insert(row, updated_stn);
      endInsertRows();
      row += 1;
      updated_stations.removeFirst();
    }
    else
    {
      removeRows(row, 1);
    }
  }
  
  if (!updated_stations.isEmpty())
  {
    int first = stations.count();
    int last = stations.count()+updated_stations.count()-1;
    //cout << "### Inserting " << last-first+1 << " rows starting at row " << row << endl;
    beginInsertRows(QModelIndex(), first, last);
    //stations.append(updated_stations);
    copy(updated_stations.begin(), updated_stations.end(),
         back_inserter(stations));
    endInsertRows();
  }
  else if (row < stations.count())
  {
    removeRows(row, stations.count()-row);
  }
  
  //cout << "### stations=" << rowCount() << endl;
  
} /* EchoLinkDirectoryModel::updateStationList */


QModelIndex EchoLinkDirectoryModel::index(int row, int column,
					  const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  if ((row >= 0) && (row < rowCount()) &&
      (column >= 0) && (column < columnCount()))
  {
    return createIndex(row, column);
  }
  
  return QModelIndex();
} /* EchoLinkDirectoryModel::index */


QModelIndex EchoLinkDirectoryModel::parent(const QModelIndex &index) const
{
  Q_UNUSED(index);
  return QModelIndex();
} /* EchoLinkDirectoryModel::parent */


int EchoLinkDirectoryModel::rowCount(const QModelIndex &parent) const
{
  if (parent.isValid())
  {
    return 0;
  }
  
  return stations.count();
} /* EchoLinkDirectoryModel::rowCount */


int EchoLinkDirectoryModel::columnCount(const QModelIndex &parent) const
{
  Q_UNUSED(parent);
  return 6;
} /* EchoLinkDirectoryModel::columnCount */


QVariant EchoLinkDirectoryModel::headerData(int section,
					    Qt::Orientation orientation,
					    int role) const
{
  if ((orientation == Qt::Horizontal) && (role == Qt::DisplayRole))
  {
    switch (section)
    {
      case 0:
	return tr("Callsign");
      case 1:
	return tr("Location/Description");
      case 2:
	return tr("Status");
      case 3:
	return tr("Local Time");
      case 4:
	return tr("Node ID");
      case 5:
	return tr("IP Address");
      default:
	return QVariant();
    }
  }
  
  return QAbstractItemModel::headerData(section, orientation, role);
} /* EchoLinkDirectoryModel::headerData */


QVariant EchoLinkDirectoryModel::data(const QModelIndex &index, int role) const
{
  if (!index.isValid())
  {
    return QVariant();
  }
  
  if (role == Qt::DisplayRole)
  {
    const StationData &stn = stations.at(index.row());
    switch (index.column())
    {
      case 0:
	return QString::fromStdString(stn.callsign());
      case 1:
	return QString::fromStdString(stn.description());
      case 2:
	return QString::fromStdString(stn.statusStr());
      case 3:
	return QString::fromStdString(stn.time());
      case 4:
	if (stn.id() == -1)
	{
	  return QVariant();
	}
	return QString::number(stn.id());
      case 5:
	if (stn.ipStr() == "0.0.0.0")
	{
	  return QVariant();
	}
	return QString::fromStdString(stn.ipStr());
      default:
	return QVariant();
    }
  }
  
  return QVariant();
} /* EchoLinkDirectoryModel::data */


bool EchoLinkDirectoryModel::removeRows(int row, int count,
					const QModelIndex &parent)
{
  if ((row < 0) || (count < 1) || (row+count > rowCount()) ||
      (parent.isValid()))
  {
    return false;
  }
  
  //cout << "### Removing " << count << " rows starting at row " << row << endl;
  beginRemoveRows(QModelIndex(), row, row+count-1);
  QList<StationData>::iterator begin(stations.begin()+row);
  stations.erase(begin, begin+count);
  endRemoveRows();
  
  return true;
} /* EchoLinkDirectoryModel::removeRows */


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

