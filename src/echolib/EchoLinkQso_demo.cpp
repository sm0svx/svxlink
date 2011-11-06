#include <iostream>
#include <AsyncCppApplication.h>
#include <EchoLinkQso.h>

using namespace std;
using namespace Async;
using namespace EchoLink;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      qso = new Qso(IpAddress("127.0.0.1"), "MYCALL", "MyName", "A test Qso");
      if (!qso->initOk())
      {
	delete qso;
	cerr << "Creation of Qso failed\n";
	Application::app().quit();
	return;
      }
      qso->infoMsgReceived.connect(mem_fun(*this, &MyClass::onInfoMsgReceived));
      qso->stateChange.connect(mem_fun(*this, &MyClass::onStateChange));
      qso->connect();
    }
    
    ~MyClass(void)
    {
      delete qso;
    }
    
  private:
    Qso *qso;
    
    void onInfoMsgReceived(const string& msg)
    {
      cerr << "Info message received: " << msg << endl;
      qso->disconnect();
    }
    
    void onStateChange(Qso::State state)
    {
      cerr << "State changed to ";
      switch (state)
      {
	case Qso::STATE_DISCONNECTED:
	  cerr << "DISCONNECTED";
	  Application::app().quit();
	  break;
	case Qso::STATE_CONNECTING:
	  cerr << "CONNECTING";
	  break;
	case Qso::STATE_CONNECTED:
	  cerr << "CONNECTED";
	  break;
	default:
	  break;
      }
      cout << endl;
    }
};

int main(int argc, char **argv)
{
  CppApplication app; // or QtApplication
  MyClass my_class;
  app.exec();
}
