#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncAtTimer.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      timer.setExpireOffset(100);
      timer.expired.connect(mem_fun(*this, &MyClass::onTimerExpired));
      timeoutNextMinute();
      timer.start();
    }
    
    ~MyClass(void)
    {
    }

  private:
    AtTimer timer;
    
    void timeoutNextMinute(void)
    {
      struct timeval tv;
      gettimeofday(&tv, NULL);
      struct tm tm;
      localtime_r(&tv.tv_sec, &tm);
      tm.tm_min += 1;
      tm.tm_sec = 0;
      char timebuf[256];
      cout << "Setting timer to expire at " << asctime_r(&tm, timebuf);
      timer.setTimeout(tm);
    }

    void onTimerExpired(AtTimer *t)
    {
      struct timeval now;
      gettimeofday(&now, NULL);
      struct tm tm;
      char timebuf[256];
      cout << "Timer expired at (+" << now.tv_usec / 1000 << "msec) "
           << asctime_r(localtime_r(&now.tv_sec, &tm), timebuf) << endl;
      timeoutNextMinute();
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
