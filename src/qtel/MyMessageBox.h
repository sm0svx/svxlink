/**
@file	MyMessageBox.h
@brief  A simple specialization of the QMessageBox to make it emit
      	a signal when it closes
@author Tobias Blomberg
@date	2003-04-21

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003  Tobias Blomberg / SM0SVX

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


#ifndef MY_MESSAGE_BOX_INCLUDED
#define MY_MESSAGE_BOX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <QMessageBox>


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
 * Defines & typedefs
 *
 ****************************************************************************/

/*
 *----------------------------------------------------------------------------
 * Macro:   
 * Purpose: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 * Bugs:    
 *----------------------------------------------------------------------------
 */


/*
 *----------------------------------------------------------------------------
 * Type:    
 * Purpose: 
 * Members: 
 * Input:   
 * Output:  
 * Author:  
 * Created: 
 * Remarks: 
 *----------------------------------------------------------------------------
 */


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

/*
 *----------------------------------------------------------------------------
 * Class:     MyMessageBox
 * Purpose:   MyMessageBox class
 * Inherits:  
 * Input:     
 * Output:    
 * Author:    
 * Created:   
 * Remarks:   
 * Bugs:      
 *----------------------------------------------------------------------------
 */   
class MyMessageBox : public QMessageBox
{
  Q_OBJECT
  
  public:
    MyMessageBox(const QString& caption, const QString& text)
      : QMessageBox(caption, text, QMessageBox::Critical, QMessageBox::Ok,
      	      	    Qt::NoButton, Qt::NoButton)
    {
      setAttribute(Qt::WA_DeleteOnClose);
    }
    
    virtual ~MyMessageBox(void)
    {
      closed();
    }
    
  signals:
    void closed(void);
  
    
  protected:
    
  private:
    
};  /* class MyMessageBox */


//} /* namespace */

#endif /* MY_MESSAGE_BOX_INCLUDED */



/*
 * This file has not been truncated
 */

