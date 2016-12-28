/**
@file	 EchoLinkDirectoryModel.h
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

/** @example EchoLinkDirectoryModel_demo.cpp
An example of how to use the EchoLinkDirectoryModel class
*/


#ifndef ECHOLINK_DIRECTORY_MODEL_INCLUDED
#define ECHOLINK_DIRECTORY_MODEL_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <QList>
#include <QAbstractItemModel>


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
@brief	A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2011-06-18

A_detailed_class_description

\include EchoLinkDirectoryModel_demo.cpp
*/
class EchoLinkDirectoryModel : public QAbstractItemModel
{
  Q_OBJECT
    
  public:
    /**
     * @brief 	Default constuctor
     */
    EchoLinkDirectoryModel(QObject *parent = 0);
  
    /**
     * @brief 	Destructor
     */
    ~EchoLinkDirectoryModel(void);
  
    /**
     * @brief 	A_brief_member_function_description
     * @param 	param1 Description_of_param1
     * @return	Return_value_of_this_member_function
     */
    void updateStationList(const std::list<EchoLink::StationData> &stn_list);
    
    QModelIndex index(int row, int column,
			      const QModelIndex &parent = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &index) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant headerData(int section, Qt::Orientation orientation,
				int role = Qt::DisplayRole) const;
    
    QVariant data(const QModelIndex &index,
			  int role = Qt::DisplayRole) const;

    bool removeRows(int row, int count,
		    const QModelIndex &parent = QModelIndex());

  protected:
    
  private:
    QList<EchoLink::StationData> stations;
    
    EchoLinkDirectoryModel(const EchoLinkDirectoryModel&);
    EchoLinkDirectoryModel& operator=(const EchoLinkDirectoryModel&);
    
};  /* class EchoLinkDirectoryModel */


//} /* namespace */

#endif /* ECHOLINK_DIRECTORY_MODEL_INCLUDED */



/*
 * This file has not been truncated
 */

