#include <iostream>
#include "AsyncMsg.h"

#include <sstream>
#include <fstream>
#include <iterator>
#include <algorithm>
#include <cmath>
#include <stdlib.h>
#include <cstring>


class MsgOne : public Async::Msg
{
  public:
    uint64_t                      a;
    char                          b;
    double                        f;
    std::string                   str;
    std::vector<int>              ivec;
    std::vector<uint8_t>          bvec;
    std::vector<float>            fvec;
    std::vector<std::string>      svec;
    std::map<std::string, float>  sfmap;
    std::array<int, 8>            iarr;
    char                          carr[8];

    //ASYNC_MSG_DERIVED_FROM(Msg)
    ASYNC_MSG_MEMBERS(a, b, str, f, ivec, svec, sfmap, fvec, iarr, carr)
};


class MsgTwo : public Async::Msg
{
  public:
    MsgOne one;
    int    i;

    //ASYNC_MSG_DERIVED_FROM(MsgOne)
    ASYNC_MSG_MEMBERS(one, i)
};


int main(void)
{
  MsgOne one;
  one.a = 0xffffffffffffffff;
  one.b = 43;
  one.f = M_PI;
  one.str = "Hello World";
  one.ivec.push_back(1);
  one.ivec.push_back(2);
  one.ivec.push_back(3);
  one.bvec.push_back(1);
  one.bvec.push_back(2);
  one.bvec.push_back(3);
  one.svec.push_back("One");
  one.svec.push_back("Two");
  one.svec.push_back("Three");
  one.sfmap["Ett"] = 1.1;
  one.sfmap["Två"] = 2.2;
  one.sfmap["Tre"] = 3.3;
  one.fvec.push_back(1.2);
  one.fvec.push_back(3.4);
  one.fvec.push_back(5.6);
  one.iarr = {1,2,3,4};
  strcpy(one.carr, "Hello");

  MsgTwo mt;
  mt.one.a = 45;
  mt.i = 42;
  //*static_cast<MsgOne*>(&mt) = one;
  mt.one = one;

  //std::stringstream ss;
  std::ofstream os;
  os.open("m2.msg", std::ofstream::binary);
  if (!os)
  {
    std::cerr << "Could not open file for output\n";
    exit(1);
  }
  //if (!mt.MsgOne::pack(os) || !mt.pack(os))
  Async::Msg &m = mt;
  if (!m.pack(os))
  {
    std::cerr << "*** ERROR: Packing failed\n";
    return 1;
  }
  std::cout << std::endl;
  os.close();

  MsgTwo two;
  //ss.write("\x00\x01\x00\x00", 4);
  std::ifstream is;
  is.open("m2.msg", std::ifstream::binary);
  if (!is)
  {
    std::cerr << "Could not open file for input\n";
    exit(1);
  }
  //if (!two.MsgOne::unpack(is) || !two.unpack(is))
  if (!two.unpack(is))
  {
    std::cout << "*** ERROR: Unpacking failed\n";
    return 1;
  }
  is.close();
  std::cout << "two.one.a=0x" << std::hex << two.one.a
            << std::dec << std::endl;
  std::cout << "two.one.b=" << +two.one.b << std::endl;
  std::cout << "two.one.f=" << two.one.f << std::endl;
  std::cout << "two.one.str=" << two.one.str << std::endl;
  std::cout << "two.one.ivec=";
  copy(two.one.ivec.begin(), two.one.ivec.end(),
       std::ostream_iterator<int>(std::cout, " "));
  std::cout << std::endl;
  std::cout << "two.one.svec=";
  copy(two.one.svec.begin(), two.one.svec.end(),
       std::ostream_iterator<std::string>(std::cout, " "));
  std::cout << std::endl;
  std::cout << "two.one.fvec=";
  copy(two.one.fvec.begin(), two.one.fvec.end(),
       std::ostream_iterator<float>(std::cout, " "));
  std::cout << std::endl;
  std::cout << "two.one.sfmap:" << std::endl;
  for (const auto& item : two.one.sfmap)
  {
    std::cout << "  " << item.first << "=" << item.second << std::endl;
  }
  std::cout << "two.one.iarr=";
  copy(two.one.iarr.begin(), two.one.iarr.end(),
       std::ostream_iterator<int>(std::cout, " "));
  std::cout << std::endl;
  std::cout << "two.one.carr=" << two.one.carr << std::endl;
  std::cout << "two.i=" << two.i << std::endl;

  return 0;
} /* main */

