#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncExec.h>
#include <AsyncTimer.h>

using namespace std;
using namespace Async;

void handleOutput(const char *buf, int cnt)
{
  cout << buf;
}

void handleExit(Exec *exec)
{
  cout << "Exited(\"" << exec->command() << "\"): ";
  if (exec->ifExited())
  {
    cout << "exit_status=" << exec->exitStatus();
  }
  else if (exec->ifSignaled())
  {
    cout << "term_sig=" << exec->termSig();
  }
  cout << endl;
}

int main()
{
  CppApplication app;

    // Start a "cat" process and then run some text through it
  Exec cat("/bin/cat -n");
  cat.stdoutData.connect(sigc::ptr_fun(handleOutput));
  cat.stderrData.connect(sigc::ptr_fun(handleOutput));
  cat.exited.connect(sigc::bind(sigc::ptr_fun(handleExit), &cat));
  cat.nice();
  cat.run();
  cat.writeStdin("Hello, Exec!\n");
  cat.writeStdin("This is a test\n");
  cat.closeStdin();

    // Try to run a command that does not exist
  Exec xyz("/bin/xyz");
  xyz.exited.connect(sigc::bind(sigc::ptr_fun(handleExit), &xyz));
  xyz.run();

    // Start a sleep but set a timeout that kills it before exit
  Exec kill("/bin/tail");
  kill.exited.connect(sigc::bind(sigc::ptr_fun(handleExit), &kill));
  kill.setTimeout(1);
  kill.run();

    // Sleep for four seconds then quit application
  Exec sleep("/bin/sleep 2");
  sleep.exited.connect(sigc::bind(sigc::ptr_fun(handleExit), &sleep));
  sleep.exited.connect(mem_fun(app, &CppApplication::quit));
  sleep.run();

  app.exec();

  return 0;
}

