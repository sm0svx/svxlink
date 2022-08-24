#include <iostream>
#include <cstdlib>
#include <AsyncCppApplication.h>
#include <EchoLinkDispatcher.h>

using namespace std;
using namespace Async;
using namespace EchoLink;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      if (Dispatcher::instance() == 0)
      {
	cerr << "Could not create EchoLink listener (Dispatcher) object\n";
	exit(1);
      }

      Dispatcher::instance()->incomingConnection.connect(mem_fun(*this,
	  &MyClass::onIncomingConnection));
    }
    
  private:
    void onIncomingConnection(const IpAddress& ip, const string& callsign,
      	      	      	      const string& name, const string& priv)
    {
      cerr << "Incoming connection from " << ip << ": " << callsign
      	   << " (" << name << ")\n";
      // Find out the station data by using the Directory class
      // Create a new Qso object to accept the connection
    }
};

int main(int argc, char **argv)
{
  CppApplication app; // or QtApplication
  MyClass my_class;
  app.exec();
}
