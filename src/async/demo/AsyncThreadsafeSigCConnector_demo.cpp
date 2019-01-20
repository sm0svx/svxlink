#include <iostream>
#include <chrono>
#include <thread>
#include <atomic>

#include <AsyncCppApplication.h>
#include <AsyncTimer.h>
#include <AsyncThreadsafeSigCConnector.h>

std::atomic<bool> done(false);

struct MyThread
{
  void operator()(int n)
  {
    while (!done)
    {
      sig(n);
      std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
  }
  sigc::signal<void, int> sig;
};

struct MyHandler : public sigc::trackable
{
  void mySlot(int n)
  {
    std::cout << "MyHandler::mySlot: " << n << std::endl;
  }
};

void timeout(Async::Timer* t)
{
  std::cout << "--- TIMEOUT ---" << std::endl;
}

void handle_signal(int signum)
{
  Async::Application::app().quit();
}

int main(void)
{
    // Create application object and catch Ctrl-C (SIGINT)
  Async::CppApplication app;
  app.catchUnixSignal(SIGINT);
  app.unixSignalCaught.connect(std::ptr_fun(handle_signal));

    // Create a thread and handler object then connect a signal from the thread
    // object to a member function in the handler object
  MyThread th;
  MyHandler h;
  Async::ThreadsafeSigCConnector<void, int> con(
      th.sig, sigc::mem_fun(h, &MyHandler::mySlot));

    // Create an Async::Timer which show that the main thread is alive
  Async::Timer tmo(1000, Async::Timer::TYPE_PERIODIC);
  tmo.expired.connect(sigc::ptr_fun(timeout));

    // Start the thread
  std::thread t(th, 42);

    // Execute the main thread
  app.exec();

    // Tell the thread that we are done then join it when it exits
  done = true;
  t.join();
}
