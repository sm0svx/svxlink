/**
@file	 AsyncExec.h
@brief   Execute external commands
@author  Tobias Blomberg / SM0SVX
@date	 2013-10-26

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2013 Tobias Blomberg / SM0SVX

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

/** @example AsyncExec_demo.cpp
An example of how to use the Exec class
*/


#ifndef ASYNC_EXEC_INCLUDED
#define ASYNC_EXEC_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <unistd.h>
#include <signal.h>
#include <sigc++/sigc++.h>

#include <vector>
#include <string>
#include <map>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncFdWatch.h>


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

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

class Timer;
  

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
@brief	Execute external commands
@author Tobias Blomberg / SM0SVX
@date   2013-10-26

This class is used to execute external commands. It essentially wraps the
exec system call together with commonly used infrastructure in a convenient
class.

This class depends on the SIGCHLD UNIX signal so it must not be used by
another part of the application.

\include AsyncExec_demo.cpp
*/
class Exec : public sigc::trackable
{
  public:
    using Environment = std::vector<std::pair<std::string,std::string>>;

    /**
     * @brief 	Default constructor
     */
    explicit Exec(const std::string &cmdline="");
  
    /**
     * @brief 	Destructor
     */
    ~Exec(void);
  
    /**
     * @brief 	Set the command line to use
     * @param 	cmdline The command line to run
     *
     * This function can be used to set the command line before the run
     * method is called. The command line can also be set directly in the
     * constructor. It's possible to append more arguments to the command
     * line using the appendArgument method.
     */
    void setCommandLine(const std::string &cmdline);

    /**
     * @brief   Get the command name for the command
     * @returns Returns the command name
     */
    const std::string &command(void) const { return args[0]; }

    /**
     * @brief   Append a command line argument to a command
     * @param   arg The command line argument to add
     */
    void appendArgument(const std::string &arg);

    /**
     * @brief   Clear the environment
     *
     * This function is used to clear the environment. It must be called before
     * calling run(). It will clear both the environment inherited from the
     * parent process as well as any environment variables added using the
     * addEnvironmentVar(s) functions.
     */
    void clearEnvironment(void);

    /**
     * @brief   Add an additional environment variable
     * @param   name The name of the environment variable
     * @param   val  The value of the environment variable
     *
     * This function is used to add a variable to the environment for the
     * process to be executed. It must be done before calling run().
     */
    void addEnvironmentVar(const std::string& name, const std::string& val);

    /**
     * @brief   Add multiple environment variables
     * @param   env The environment variables to add
     *
     * This function is used to add multiple variables to the environment for
     * the process to be executed. It must be done before calling run().
     */
    void addEnvironmentVars(const Environment& env);

    /**
     * @brief   Modify the nice value for the child subprocess
     * @param   inc How much to increase the nice value
     * @returns Returns \em true on success or else \em false
     *
     * This function will modify the nice value of the subprocess. A positive
     * value will increase the nice value (lower priority) and a negative value
     * will decrease the nice value (higher priority). This function may be
     * called both before and after the run method.
     */
    bool nice(int inc=10);

    /**
     * @brief   Set a timeout on the allowed runtime for the subprocess
     * @param   time_s The timeout time in seconds
     *
     * Use this method to limit the maximum runtime for the subprocess. If the
     * process runs for longer than the specified time, a SIGTERM will be sent
     * to it. If the subprocess has not exited withing ten seconds, a SIGKILL
     * will be sent to the subprocess.
     */
    void setTimeout(int time_s);

    /**
     * @brief   Run the command
     * @returns Returns \em true on success or \em false otherwise
     *
     * This method is used to run the command specified using the constructor,
     * setCommandLine and appendArgument. This function will return success as
     * long as the fork call succeeds. If the command cannot be run for some
     * reason, this function will still return success. Errors that occurr
     * after the fork call will be handled through the "exited" signal. If the
     * command cannot run for some reason, the exit code will be 255.
     */
    bool run(void);

    /**
     * @brief   Write data to stdin on the subprocess
     * @param   buf The buffer to write data from
     * @param   cnt The number of bytes to write
     * @returns Returns \em true on success or \em false otherwise
     */
    bool writeStdin(const char *buf, int cnt);

    /**
     * @brief   Write data to stdin on the subprocess
     * @param   str The string buffer to write
     * @returns Returns \em true on success or \em false otherwise
     */
    bool writeStdin(const std::string &str);

    /**
     * @brief   Send a UNIX signal to the subprocess
     * @param   sig The UNIX signal to send
     * @returns Returns \em true on success or \em false otherwise
     */
    bool kill(int sig=SIGTERM);

    /**
     * @brief   Close the stdin pipe to the subprocess
     * @returns Returns \em true on success or \em false otherwise
     *
     * This method is used to close the pipe from the parent process to the
     * subprocess. This will indicate to the subprocess that the parent
     * process is done sending data to it.
     */
    bool closeStdin(void);

    /**
     * @brief   Check if the subprocess exited in a normal way
     * @returns Returns \em true if it was a normal exit or \em false otherwise
     *
     * This function may only be called after the process has exited. This is
     * indicated by the "exited" signal.
     */
    bool ifExited(void) const;

    /**
     * @brief   Check if the subprocess exited due to receiving a UNIX signal
     * @returns Returns \em true if it was a signal exit or \em false otherwise
     *
     * This function may only be called after the process has exited. This is
     * indicated by the "exited" signal.
     */
    bool ifSignaled(void) const;

    /**
     * @brief   Read the exit code of the subprocess
     * @returns Returns the exit code (0-255) of the subprocess
     *
     * This function may only be called after the process has exited. This is
     * indicated by the "exited" signal.
     * The returned value is only meaningful if the ifExited function returns
     * \em true.
     */
    int exitStatus(void) const;

    /**
     * @brief   Read the UNIX signal number that caused the subprocess to stop
     * @returns Returns the UNIX signal number
     *
     * This function may only be called after the process has exited. This is
     * indicated by the "exited" signal.
     * The returned value is only meaningful if the ifSignaled function returns
     * \em true.
     */
    int termSig(void) const;

    /**
     * @brief   A signal that is emitted when the subprocess write to stdout
     * @param   buf The buffer containing the data
     * @param   cnt The number of valid bytes in the buffer
     *
     * This signal is emitted when a subprocess write to its stdout. The data
     * will be zero terminated.
     */
    sigc::signal<void, const char *, int> stdoutData;

    /**
     * @brief   A signal that is emitted when the subprocess write to stderr
     * @param   buf The buffer containing the data
     * @param   cnt The number of valid bytes in the buffer
     *
     * This signal is emitted when a subprocess write to its stderr. The data
     * will be zero terminated.
     */
    sigc::signal<void, const char *, int> stderrData;

    /**
     * @brief   A signal that is emitted when the subprocess close its stdout
     */
    sigc::signal<void> stdoutClosed;

    /**
     * @brief   A signal that is emitted when the subprocess close its stderr
     */
    sigc::signal<void> stderrClosed;

    /**
     * @brief   A signal that is emitted when the subprocess exits
     *
     * This signal will be emitted when the subprocess exits. After that the
     * methods ifExited, ifSignaled, exitStatus and termSig may be used to
     * find out what caused the subprocess to exit.
     */
    sigc::signal<void> exited;
    
  protected:
    
  private:
    typedef std::map<pid_t, Exec*> ExecMap;

    static ExecMap            execs;
    static int                sigchld_pipe[2];
    static Async::FdWatch     *sigchld_watch;
    static struct sigaction   old_sigact;

    std::vector<std::string>  args;
    std::vector<std::string>  env;
    pid_t                     pid;
    Async::FdWatch            *stdout_watch;
    Async::FdWatch            *stderr_watch;
    int                       stdin_fd;
    int                       status;
    int                       nice_value;
    Async::Timer              *timeout_timer;
    bool                      pending_term;
    bool                      clear_env = false;

    static void handleSigChld(int signal_number, siginfo_t *info,
                              void *context);
    static void sigchldReceived(void);

    Exec(const Exec&);
    Exec& operator=(const Exec&);
    void stdoutActivity(Async::FdWatch *w);
    void stderrActivity(Async::FdWatch *w);
    void subprocessExited(void);
    void handleTimeout(void);
    
};  /* class Exec */


} /* namespace */

#endif /* ASYNC_EXEC_INCLUDED */



/*
 * This file has not been truncated
 */
