#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncDnsLookup.h>

using namespace std;
using namespace Async;

class MyClass : public SigC::Object
{
  public:
    MyClass(void)
    {
      cout << "Starting query...\n";
      dns_lookup = new DnsLookup("ftp.sunet.se");
      dns_lookup->resultsReady.connect(slot(this, &MyClass::onResultsReady));
    }
    
    ~MyClass(void)
    {
      delete dns_lookup;
    }
    
    void onResultsReady(DnsLookup& dns)
    {
      cout << "Results received:\n";
      vector<IpAddress> addresses = dns.addresses();
      vector<IpAddress>::iterator it;
      for (it = addresses.begin(); it != addresses.end(); ++it)
      {
	cout << *it << endl;
      }
      Application::app().quit();
    }
    
  private:
    DnsLookup *dns_lookup;
  
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass dns;
  app.exec();
}
