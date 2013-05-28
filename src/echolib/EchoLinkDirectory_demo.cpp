#include <iostream>
#include <iomanip>
#include <cstdlib>
#include <AsyncCppApplication.h>
#include <EchoLinkDirectory.h>

using namespace std;
using namespace Async;
using namespace EchoLink;

class MyClass : public sigc::trackable
{
  public:
    MyClass(const string& mycall, const string& mypass) : mycall(mycall)
    {
      vector<string> servers;
      servers.push_back("servers.echolink.org");
      dir = new Directory(servers, mycall, mypass, "Testing...");
      dir->statusChanged.connect(mem_fun(*this, &MyClass::onStatusChanged));
      dir->stationListUpdated.connect(
	  mem_fun(*this, &MyClass::onStationListUpdated));
      dir->error.connect(mem_fun(*this, &MyClass::onError));
      dir->makeBusy();	// Set status busy so noone think we are really online
    }
    
    ~MyClass(void)
    {
      delete dir;
    }
    
  private:
    string    	mycall;
    Directory * dir;
    
    void onStatusChanged(StationData::Status status)
    {
      cerr << "Status changed to " << StationData::statusStr(status) << endl;
      if (status == StationData::STAT_BUSY)
      {
	dir->getCalls();
      }
      else
      {
	Application::app().quit();
      }
    }
    
    void onStationListUpdated(void)
    {
      const list<StationData>& stations = dir->stations();
      list<StationData>::const_iterator it;
      for (it = stations.begin(); it != stations.end(); ++it)
      {
	cerr << *it << endl;
      }
      
      cerr << endl << "Message:" << endl;
      cerr << dir->message() << endl;
      
      const StationData *mydata = dir->findCall(mycall);
      cerr << endl << "My station data:" << endl << *mydata << endl;
      dir->makeOffline();
    }
    
    void onError(const string& msg)
    {
      cerr << "ERROR: " << msg << endl;
      Application::app().quit();
    }
};

int main(int argc, char **argv)
{
  CppApplication app; // or QtApplication
  if (argc < 3)
  {
    cerr << "Usage: EchoLinkDispatcher_demo <callsign> <password>\n";
    exit(1);
  }
  MyClass my_class(argv[1], argv[2]);
  app.exec();
}
