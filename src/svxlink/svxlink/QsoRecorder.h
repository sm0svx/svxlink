/**
@file	 QsoRecorder.h
@brief   The QSO recorder is used to write radio traffic to file
@author  Tobias Blomberg / SM0SVX
@date	 2009-06-06

\verbatim
SvxLink - A Multi Purpose Voice Services System for Ham Radio Use
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

#ifndef QSO_RECORDER_INCLUDED
#define QSO_RECORDER_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <string>


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

namespace Async
{
  class AudioSelector;
  class AudioRecorder;
  class Config;
  class Timer;
  class Exec;
};

class Logic;


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
@brief	The QSO recorder is used to write radio traffic to file
@author Tobias Blomberg / SM0SVX
@date   2009-06-06
*/
class QsoRecorder
{
  public:
    /**
     * @brief 	Constuctor
     * @param   rec_dir The path to the directory where the recordings
     *                  are placed
     */
    QsoRecorder(Logic *logic);

    /**
     * @brief 	Destructor
     */
    ~QsoRecorder(void);

    /**
     * @brief   Initialize the qso recorder
     * @param   cfg An already initialize config object
     * @param   name The name of the config section tp read config from
     * @return  Returns \rm true on success or else \em false
     */
    bool initialize(const Async::Config &cfg, const std::string &name);

    /**
     * @brief   Add an audio source to the QSO recorder
     * @param   source The audio source to add
     * @param   prio   The priority to set. Higher numbers give higher priority.
     */
    void addSource(Async::AudioSource *source, int prio);

    /**
     * @brief 	Enable or disable the recorder
     * @param 	enable Set to \em true to enable the recorder or \em false to
     *                 disable it
     */
    void setEnabled(bool enable);

    /**
     * @brief   Check if the recorder is enabled or not
     * @returns Returns \em true if the recorder is enabled or else \em false
     */
    bool isEnabled(void) const { return (recorder != 0); }

    /**
     * @brief   Set the maximum size of ech recorded file
     * @param   max_time 
     * @param   soft_time
     *
     * Use this function to limit the file size of the recordings. When the
     * soft limit is reached, the next flushSamples (e.g. squelch close) will
     * close the file
     */
    void setMaxChunkTime(unsigned max_time, unsigned soft_time=0);

    void setMaxRecDirSize(unsigned max_size);

    bool recorderIsActive(void) const { return (recorder != 0); }

  protected:

  private:
    class FileEncoder;

    Async::AudioSelector  *selector;
    Async::AudioRecorder  *recorder;
    std::string           rec_dir;
    unsigned              hard_chunk_limit;
    unsigned              soft_chunk_limit;
    unsigned              max_dirsize;
    bool                  default_active;
    Async::Timer          *tmo_timer;
    Logic                 *logic;
    Async::Timer          *qso_tmo_timer;
    unsigned              min_samples;
    std::string           encoder_cmd;

    QsoRecorder(const QsoRecorder&);
    QsoRecorder& operator=(const QsoRecorder&);
    void openNewFile(void);
    void openFile(void);
    void closeFile(void);
    void cleanupDirectory(void);
    void timerExpired(void);
    void checkTimeoutTimers(void);
    void handleEncoderPrintouts(const char *buf, int cnt);
    void encoderExited(FileEncoder *enc);
    void onError(void);

};  /* class QsoRecorder */


//} /* namespace */

#endif /* QSO_RECORDER_INCLUDED */



/*
 * This file has not been truncated
 */

