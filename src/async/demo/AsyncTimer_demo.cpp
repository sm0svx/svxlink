#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncTimer.h>

using namespace std;
using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void) : count(0)
    {
      timer = new Timer(1000, Timer::TYPE_PERIODIC);
      timer->expired.connect(mem_fun(*this, &MyClass::onTimerExpired));
    }
    
    ~MyClass(void)
    {
      delete timer;
    }

  private:
    Timer * timer;
    int     count;
    
    void onTimerExpired(Timer *t)
    {
      if (++count == 3)
      {
	Application::app().quit();
      }
      cout << "Timer expired " << count << "...\n";
    }
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass my_class;
  app.exec();
}
