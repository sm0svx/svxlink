/**
@file	 Vox.h
@brief   Contains a class that implements voice operated transmission (VOX)
@author  Stuart Longland, Tobias Blomberg / SM0SVX
@date	 2008-03-07

\verbatim
Qtel - The Qt EchoLink client
Copyright (C) 2003-2008 Tobias Blomberg / SM0SVX

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


#ifndef VOX_INCLUDED
#define VOX_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <QObject>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncAudioSink.h>


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

class QTimer;


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
@brief	Implements voice operated transmission (VOX)
@author Stuart Longland, Tobias Blomberg / SM0SVX
@date   2008-03-07

This class implements the logic for a voice operated transmission control
(VOX).
*/
class Vox : public QObject, public Async::AudioSink
{
  Q_OBJECT
  
  public:
    typedef enum
    {
      IDLE, ACTIVE, HANG
    } State;
    
    /**
     * @brief 	Default constuctor
     */
    Vox(void);
  
    /**
     * @brief 	Destructor
     */
    virtual ~Vox(void);
    
    /**
     * @brief 	Get the VOX state
     * @return	Returns the current state of the VOX
     */
    State state(void) const { return m_vox_state; }
    
    /**
     * @brief 	Check if the VOX is enabled or not
     * @return	Returns \em true if the VOX is enabled or else \em false
     */
    bool enabled(void) const { return m_enabled; }
    
    /**
     * @brief 	Get the VOX threshold
     * @return	Returns the current VOX threshold
     */
    int threshold(void) const { return m_threshold; }
    
    /**
     * @brief 	Get the VOX delay
     * @return	Returns the current VOX delay
     */
    int delay(void) const { return m_delay; }
    
    
    /**
     * @brief 	Write samples into this audio sink
     * @param 	samples The buffer containing the samples
     * @param 	count The number of samples in the buffer
     * @return	Returns the number of samples that has been taken care of
     *
     * This function is used to write audio into this audio sink. If it
     * returns 0, no more samples should be written until the resumeOutput
     * function in the source have been called.
     * This function is normally only called from a connected source object.
     */
    virtual int writeSamples(const float *samples, int count);
    
    /**
     * @brief 	Tell the sink to flush the previously written samples
     *
     * This function is used to tell the sink to flush previously written
     * samples. When done flushing, the sink should call the
     * sourceAllSamplesFlushed function.
     * This function is normally only called from a connected source object.
     */
    virtual void flushSamples(void) { sourceAllSamplesFlushed(); }

      
  signals:
    /**
     * @brief 	A signal that is emitted when the input level has changed
     * @param 	level_db The level in dB [-60,0]
     */
    void levelChanged(int level_db);

    /**
     * @brief 	A signal that is emitted when the VOX state has changed
     * @param 	new_state The new state of the VOX (IDLE, ACTIVE or HANG)
     */
    void stateChanged(Vox::State new_state);


  public slots:    
    /**
     * @brief 	Set the VOX threshold level
     * @param 	threshold_db The threshold in dB [-60,0]
     */
    void setThreshold(int threshold_db);

    /**
     * @brief 	Set the VOX delay
     * @param 	delay_ms The VOX delay in milliseconds
     */
    void setDelay(int delay_ms);

    /**
     * @brief 	Enable/disable the VOX
     * @param 	enable Set to \em true to enable the VOX or \em false to
     *	      	       disable it
     */
    void setEnabled(bool enable);
    
    
  protected:
    
  private:
    int  	  m_threshold;
    int       	  m_delay;
    QTimer    	  *m_vox_timer;
    State     	  m_vox_state;
    bool      	  m_enabled;
    
    Vox(const Vox&);
    Vox& operator=(const Vox&);
    void setState(State new_state);


  private slots:
    void voxTimeout(void);
    
};  /* class Vox */


//} /* namespace */

#endif /* VOX_INCLUDED */



/*
 * This file has not been truncated
 */

