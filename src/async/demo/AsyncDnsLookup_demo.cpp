#include <iostream>
#include <AsyncCppApplication.h>
#include <AsyncDnsLookup.h>

using namespace Async;

class MyClass : public sigc::trackable
{
  public:
    MyClass(void)
    {
      dns.resultsReady.connect(mem_fun(*this, &MyClass::onResultsReady));

      dns.lookup("www.svxlink.org");
      //dns.lookup("www.svxlink.org", DnsLookup::Type::CNAME);
      //dns.lookup("185.199.110.153");
      //dns.lookup("185.199.110.153", DnsLookup::Type::PTR);
      //dns.lookup("153.110.199.185.in-addr.arpa.", DnsLookup::Type::PTR);

      //std::string srv = "_svxreflector._tcp.test.svxlink.org";
      //dns.addStaticResourceRecord(
      //    new DnsResourceRecordSRV(srv, 3600, 15, 10, 5304, "localhost."));
      //dns.lookup(srv, DnsLookup::Type::SRV);

      std::cout << "Starting " << dns.typeStr() << " record query for \""
                << dns.label() << "\"..." << std::endl;
    }

    void onResultsReady(DnsLookup& dns)
    {
      if (!dns.lookupFailed())
      {
          // Simple IP address lookup API
        std::cout << "IP addresses received:\n";
        for (auto& addr : dns.addresses())
        {
          std::cout << addr << std::endl;
        }
        std::cout << std::endl;

          // Access to all resource records
        std::cout << "All resource records received:\n";
        for (auto& rr : dns.resourceRecords())
        {
          std::cout << rr->toString() << std::endl;
        }
        std::cout << std::endl;

          // Access A records with full detail
        DnsResourceRecordA::List a_rrs;
        dns.resourceRecords(a_rrs);
        if (!a_rrs.empty())
        {
          std::cout << "A records received:\n";
          for (auto& rr : a_rrs)
          {
            std::cout << rr->name() << "\t" << rr->ttl() << "\t" << rr->ip()
                      << std::endl;
          }
          std::cout << std::endl;
        }

          // Access PTR records with full detail
        DnsResourceRecordPTR::List ptr_rrs;
        dns.resourceRecords(ptr_rrs);
        if (!ptr_rrs.empty())
        {
          std::cout << "PTR records received:\n";
          for (auto& rr : ptr_rrs)
          {
            std::cout << rr->name() << "\t" << rr->ttl() << "\t" << rr->dname()
                      << std::endl;
          }
          std::cout << std::endl;
        }

          // Access CNAME records with full detail
        DnsResourceRecordCNAME::List cname_rrs;
        dns.resourceRecords(cname_rrs);
        if (!cname_rrs.empty())
        {
          std::cout << "CNAME records received:\n";
          for (auto& rr : cname_rrs)
          {
            std::cout << rr->name() << "\t" << rr->ttl() << "\t" << rr->cname()
                      << std::endl;
          }
          std::cout << std::endl;
        }

          // Access SRV records with full detail
        DnsResourceRecordSRV::List srv_rrs;
        dns.resourceRecords(srv_rrs);
        if (!srv_rrs.empty())
        {
          std::cout << "SRV records received:\n";
          for (auto& rr : srv_rrs)
          {
            std::cout << rr->name() << "\t" << rr->ttl() << "\t" << rr->prio()
                      << " " << rr->weight() << " " << rr->port() << " "
                      << rr->target() << std::endl;
          }
        }
      }
      else
      {
        std::cout << "*** ERROR: The " << dns.typeStr()
                  << " record DNS lookup for " << dns.label()
                  << " failed" << std::endl;
      }

      Application::app().quit();
    }

  private:
    DnsLookup dns;
};

int main(int argc, char **argv)
{
  CppApplication app;
  MyClass dns;
  app.exec();
}
