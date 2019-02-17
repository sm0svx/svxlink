#include <iostream>
#include <thread>
#include <chrono>

#include <AsyncCppApplication.h>
#include <AsyncMutex.h>
#include <AsyncTimer.h>

class MyThread
{
  public:
    MyThread(int id) : m_id(id) {}

      // Thread entrypoint
    void operator()(void)
    {
      int seq = 0;
      for (;;)
      {
        {
            // This lock is necessary to get exclusive access to the Async main
            // thread. If we do not do this (try commenting the row below out)
            // there will be many strange hard to diagnose errors like
            // misbehavior, segmentation faults and hangs.
          std::lock_guard<Async::Mutex> lk(m_mu);

            // We can now safely add a timer on the main Async thread
          Async::Timer *t = new Async::Timer(0);
          t->expired.connect(sigc::bind(
                sigc::mem_fun(*this, &MyThread::handleTimeout), seq++));
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
      }
    }

  private:
    Async::Mutex  m_mu;
    int           m_expected_seq = 0;
    int           m_id;

      // This function is executed on the main thread
    void handleTimeout(Async::Timer* t, int seq)
    {
      std::cout << "MyThread::handleTimeout called["
                << m_id << "]: seq=" << seq << std::endl;
      if (seq != m_expected_seq)
      {
        std::cout << "*** Out of sequence. Expected " << m_expected_seq
                  << " but got " << seq << std::endl;
      }
      m_expected_seq = seq + 1;
      delete t;
    }
};

int main(void)
{
  Async::CppApplication app;

    // Set up an Async timer to mess with the threads that also set up timers.
  Async::Timer t(1, Async::Timer::TYPE_PERIODIC);

    // Start a couple of threads
  MyThread m1(1);
  std::thread th1(std::ref(m1));
  MyThread m2(2);
  std::thread th2(std::ref(m2));

    // Enter the Async main loop
  app.exec();

  return 0;
}
