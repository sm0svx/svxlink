/**
@file	 AsyncMsg.h
@brief   A message packing framework
@author  Tobias Blomberg / SM0SVX
@date	 2017-02-25

This is a message packing framework for use when for example sending network
messages or storing structured binary data to a file. Messages are defined as
classes which can be packed (serialized) and unpacked (deserialized) to/from a
stream. This is a simple example class:

\code{.cpp}
class MsgBase : public Async::Msg
{
  public:
    int         a;
    std::string str;

    ASYNC_MSG_MEMBERS(a, str)
};
\endcode

The most common types may be packed, like number types, std::string,
std::vector, std::map. Adding new types are rather simple. This is an example
implementing support for the std::pair type:

\code{.cpp}
namespace Async
{
  template <typename First, typename Second>
  class MsgPacker<std::pair<First, Second> >
  {
    public:
      static bool pack(std::ostream& os, const std::pair<First, Second>& p)
      {
        return MsgPacker<First>::pack(os, p.first) &&
               MsgPacker<Second>::pack(os, p.second);
      }
      static size_t packedSize(const std::pair<First, Second>& p)
      {
        return MsgPacker<First>::packedSize(p.first) +
               MsgPacker<Second>::packedSize(p.second);
      }
      static bool unpack(std::istream& is, std::pair<First, Second>& p)
      {
        return MsgPacker<First>::unpack(is, p.first) &&
               MsgPacker<Second>::unpack(is, p.second);
        return true;
      }
  };
};
\endcode

Inheritance is also possible. If you want the base class to also be packed when
the derived class is packed, use the ASYNC_MSG_DERIVED_FROM macro. The use of
the std::pair extension is also demonstrated here.

\code{.cpp}
class MsgDerived : public MsgBase
{
  public:
    float                       f;
    std::vector<int>            vec;
    std::pair<int, std::string> p;

    ASYNC_MSG_DERIVED_FROM(MsgBase)
    ASYNC_MSG_MEMBERS(f, vec, p)
};
\endcode

These classes can be used in the usual way but with the extra property that
they can be serialized/deserialized to/from a stream.

\code{.cpp}
MsgDerived d1;
d1.a = 42;
d1.str = "Fourtytwo";
d1.f = 3.14;
d1.vec.push_back(4711);
d1.p.first = 100;
d1.p.second = "THE STRING";
std::stringstream ss;
d1.pack(ss);

MsgDerived d2;
d2.unpack(ss);
\endcode

For a working example, have a look at the demo application,
\ref AsyncMsg_demo.cpp.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2023 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
\endverbatim
*/

/** @example AsyncMsg_demo.cpp
An example of how to use the AsyncMsg class
*/


#ifndef ASYNC_MSG_INCLUDED
#define ASYNC_MSG_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <istream>
#include <ostream>
#include <vector>
#include <array>
#include <set>
#include <map>
#include <limits>
#include <endian.h>
#include <stdint.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Forward declarations
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Namespace
 *
 ****************************************************************************/

namespace Async
{


/****************************************************************************
 *
 * Forward declarations of classes inside of the declared namespace
 *
 ****************************************************************************/

  

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

/**
 * @brief   Define which class is the baseclass
 * @param   BASE_CLASS The name of the baseclass
 *
 * Use this macro in a class definition to define which class is the base
 * class. Multiple inheritance is not supported.
 */
#define ASYNC_MSG_DERIVED_FROM(BASE_CLASS) \
    bool packParent(std::ostream& os) const \
    { \
      return BASE_CLASS::pack(os); \
    } \
    size_t packedSizeParent(void) const \
    { \
      return BASE_CLASS::packedSize(); \
    } \
    bool unpackParent(std::istream& is) \
    { \
      return BASE_CLASS::unpack(is); \
    }

/**
 * @brief   Define which members of the class that should be packed
 *
 * Use this macro to define which members of the class that should be packed.
 * Variables not listed here will not be included in the serialized version of
 * the class.
 */
#define ASYNC_MSG_MEMBERS(...) \
    bool pack(std::ostream& os) const override \
    { \
      return packParent(os) && Msg::pack(os, __VA_ARGS__); \
    } \
    size_t packedSize(void) const override \
    { \
      return packedSizeParent() + Msg::packedSize(__VA_ARGS__); \
    } \
    bool unpack(std::istream& is) override \
    { \
      return unpackParent(is) && Msg::unpack(is, __VA_ARGS__); \
    }

/**
 * @brief   Specify that this class have no members to pack
 *
 * If the class have no members to pack, this macro must be used.
 */
#define ASYNC_MSG_NO_MEMBERS \
    bool pack(std::ostream& os) const override \
    { \
      return packParent(os); \
    } \
    size_t packedSize(void) const override { return packedSizeParent(); } \
    bool unpack(std::istream& is) override \
    { \
      return unpackParent(is); \
    }


/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/



/****************************************************************************
 *
 * Class definitions
 *
 ****************************************************************************/

template <typename T>
class MsgPacker
{
  public:
    static bool pack(std::ostream& os, const T& val) { return val.pack(os); }
    static size_t packedSize(const T& val) { return val.packedSize(); }
    static bool unpack(std::istream& is, T& val) { return val.unpack(is); }
};

template <>
class MsgPacker<char>
{
  public:
    static bool pack(std::ostream& os, char val)
    {
      //std::cout << "pack<char>("<< int(val) << ")" << std::endl;
      return os.write(&val, 1).good();
    }
    static size_t packedSize(const char& val) { return sizeof(char); }
    static bool unpack(std::istream& is, char& val)
    {
      is.read(&val, 1);
      //std::cout << "unpack<char>(" << int(val) << ")" << std::endl;
      return is.good();
    }
};

template <typename T>
class Packer64
{
  public:
    static bool pack(std::ostream& os, const T& val)
    {
      //std::cout << "pack<64>(" << val << ")" << std::endl;
      Overlay o;
      o.val = val;
      o.uval = htobe64(o.uval);
      return os.write(o.buf, sizeof(T)).good();
    }
    static size_t packedSize(const T& val) { return sizeof(T); }
    static bool unpack(std::istream& is, T& val)
    {
      Overlay o;
      is.read(o.buf, sizeof(T));
      o.uval = be64toh(o.uval);
      val = o.val;
      //std::cout << "unpack<64>(" << val << ")" << std::endl;
      return is.good();
    }
  private:
    union Overlay
    {
      char buf[sizeof(T)];
      uint64_t uval;
      T val;
    };
};
template <> class MsgPacker<uint64_t> : public Packer64<uint64_t> {};
template <> class MsgPacker<int64_t> : public Packer64<int64_t> {};
template <> class MsgPacker<double> : public Packer64<double> {};

template <typename T>
class Packer32
{
  public:
    static bool pack(std::ostream& os, const T& val)
    {
      //std::cout << "pack<32>(" << val << ")" << std::endl;
      Overlay o;
      o.val = val;
      o.uval = htobe32(o.uval);
      return os.write(o.buf, sizeof(T)).good();
    }
    static size_t packedSize(const T& val) { return sizeof(T); }
    static bool unpack(std::istream& is, T& val)
    {
      Overlay o;
      is.read(o.buf, sizeof(T));
      o.uval = be32toh(o.uval);
      val = o.val;
      //std::cout << "unpack<32>(" << val << ")" << std::endl;
      return is.good();
    }
  private:
    union Overlay
    {
      char buf[sizeof(T)];
      uint32_t uval;
      T val;
    };
};
template <> class MsgPacker<uint32_t> : public Packer32<uint32_t> {};
template <> class MsgPacker<int32_t> : public Packer32<int32_t> {};
template <> class MsgPacker<float> : public Packer32<float> {};

template <typename T>
class Packer16
{
  public:
    static bool pack(std::ostream& os, const T& val)
    {
      //std::cout << "pack<16>(" << val << ")" << std::endl;
      Overlay o;
      o.val = val;
      o.uval = htobe16(o.uval);
      return os.write(o.buf, sizeof(T)).good();
    }
    static size_t packedSize(const T& val) { return sizeof(T); }
    static bool unpack(std::istream& is, T& val)
    {
      Overlay o;
      is.read(o.buf, sizeof(T));
      o.uval = be16toh(o.uval);
      val = o.val;
      //std::cout << "unpack<16>(" << val << ")" << std::endl;
      return is.good();
    }
  private:
    union Overlay
    {
      char buf[sizeof(T)];
      uint16_t uval;
      T val;
    };
};
template <> class MsgPacker<uint16_t> : public Packer16<uint16_t> {};
template <> class MsgPacker<int16_t> : public Packer16<int16_t> {};

template <typename T>
class Packer8
{
  public:
    static bool pack(std::ostream& os, const T& val)
    {
      //std::cout << "pack<8>(" << int(val) << ")" << std::endl;
      return os.write(reinterpret_cast<const char*>(&val), sizeof(T)).good();
    }
    static size_t packedSize(const T& val) { return sizeof(T); }
    static bool unpack(std::istream& is, T& val)
    {
      is.read(reinterpret_cast<char*>(&val), sizeof(T));
      //std::cout << "unpack<8>(" << int(val) << ")" << std::endl;
      return is.good();
    }
};
template <> class MsgPacker<uint8_t> : public Packer8<uint8_t> {};
template <> class MsgPacker<int8_t> : public Packer8<int8_t> {};

template <>
class MsgPacker<std::string>
{
  public:
    static bool pack(std::ostream& os, const std::string& val)
    {
      //std::cout << "pack<string>(" << val << ")" << std::endl;
      if (val.size() > std::numeric_limits<uint16_t>::max())
      {
        return false;
      }
      uint16_t str_len(val.size());
      return MsgPacker<uint16_t>::pack(os, str_len) &&
             os.write(val.c_str(), val.size());
    }
    static size_t packedSize(const std::string& val)
    {
      return sizeof(uint16_t) + val.size();
    }
    static bool unpack(std::istream& is, std::string& val)
    {
      uint16_t str_len;
      if (MsgPacker<uint16_t>::unpack(is, str_len))
      {
        if (str_len > std::numeric_limits<uint16_t>::max())
        {
          return false;
        }
        char buf[str_len];
        if (is.read(buf, str_len))
        {
          val.assign(buf, str_len);
          //std::cout << "unpack<string>(" << val << ")" << std::endl;
          return true;
        }
      }
      return false;
    }
};

template <typename I>
class MsgPacker<std::vector<I>>
{
  public:
    static bool pack(std::ostream& os, const std::vector<I>& vec)
    {
      //std::cout << "pack<vector>(" << vec.size() << ")" << std::endl;
      if (vec.size() > std::numeric_limits<uint16_t>::max())
      {
        return false;
      }
      MsgPacker<uint16_t>::pack(os, vec.size());
      for (const auto& item : vec)
      {
        if (!MsgPacker<I>::pack(os, item))
        {
          return false;
        }
      }
      return true;
    }
    static size_t packedSize(const std::vector<I>& vec)
    {
      size_t size = sizeof(uint16_t);
      for (const auto& item : vec)
      {
        size += MsgPacker<I>::packedSize(item);
      }
      return size;
    }
    static bool unpack(std::istream& is, std::vector<I>& vec)
    {
      uint16_t vec_size;
      MsgPacker<uint16_t>::unpack(is, vec_size);
      if (vec_size > std::numeric_limits<uint16_t>::max())
      {
        return false;
      }
      //std::cout << "unpack<vector>(" << vec_size << ")" << std::endl;
      vec.resize(vec_size);
      for (auto& item : vec)
      {
        if (!MsgPacker<I>::unpack(is, item))
        {
          return false;
        }
      }
      return true;
    }
};

template <typename I>
class MsgPacker<std::set<I>>
{
  public:
    static bool pack(std::ostream& os, const std::set<I>& s)
    {
      //std::cout << "pack<set>(" << s.size() << ")" << std::endl;
      if (s.size() > std::numeric_limits<uint16_t>::max())
      {
        return false;
      }
      if (!MsgPacker<uint16_t>::pack(os, s.size()))
      {
        return false;
      }
      for (const auto& item : s)
      {
        if (!MsgPacker<I>::pack(os, item))
        {
          return false;
        }
      }
      return true;
    }
    static size_t packedSize(const std::set<I>& s)
    {
      size_t size = sizeof(uint16_t);
      for (const auto& item : s)
      {
        size += MsgPacker<I>::packedSize(item);
      }
      return size;
    }
    static bool unpack(std::istream& is, std::set<I>& s)
    {
      uint16_t set_size;
      if (!MsgPacker<uint16_t>::unpack(is, set_size))
      {
        return false;
      }
      if (set_size > std::numeric_limits<uint16_t>::max())
      {
        return false;
      }
      //std::cout << "unpack<set>(" << set_size << ")" << std::endl;
      s.clear();
      for (int i=0; i<set_size; ++i)
      {
        I val;
        if (!MsgPacker<I>::unpack(is, val))
        {
          return false;
        }
        s.insert(val);
      }
      return true;
    }
};

template <typename Tag, typename Value>
class MsgPacker<std::map<Tag,Value>>
{
  public:
    static bool pack(std::ostream& os, const std::map<Tag, Value>& m)
    {
      //std::cout << "pack<map>(" << m.size() << ")" << std::endl;
      if (m.size() > std::numeric_limits<uint16_t>::max())
      {
        return false;
      }
      MsgPacker<uint16_t>::pack(os, m.size());
      for (const auto& item : m)
      {
        MsgPacker<Tag>::pack(os, item.first);
        MsgPacker<Value>::pack(os, item.second);
      }
      return true;
    }
    static size_t packedSize(const std::map<Tag, Value>& m)
    {
      size_t size = sizeof(uint16_t);
      for (const auto& item : m)
      {
        size += (MsgPacker<Tag>::packedSize(item.first) +
                 MsgPacker<Value>::packedSize(item.second));
      }
      return size;
    }
    static bool unpack(std::istream& is, std::map<Tag,Value>& m)
    {
      uint16_t map_size;
      MsgPacker<uint16_t>::unpack(is, map_size);
      if (map_size > std::numeric_limits<uint16_t>::max())
      {
        return false;
      }
      //std::cout << "unpack<map>(" << map_size << ")" << std::endl;
      m.clear();
      for (int i=0; i<map_size; ++i)
      {
        Tag tag;
        Value val;
        MsgPacker<Tag>::unpack(is, tag);
        MsgPacker<Value>::unpack(is, val);
        m[tag] = val;
      }
      return true;
    }
};

template <typename T, size_t N>
class MsgPacker<std::array<T, N>>
{
  public:
    static bool pack(std::ostream& os, const std::array<T, N>& vec)
    {
      for (const auto& item : vec)
      {
        if (!MsgPacker<T>::pack(os, item))
        {
          return false;
        }
      }
      return true;
    }
    static size_t packedSize(const std::array<T, N>& vec)
    {
      size_t size = 0;
      for (const auto& item : vec)
      {
        size += MsgPacker<T>::packedSize(item);
      }
      return size;
    }
    static bool unpack(std::istream& is, std::array<T, N>& vec)
    {
      for (auto& item : vec)
      {
        if (!MsgPacker<T>::unpack(is, item))
        {
          return false;
        }
      }
      return true;
    }
};

template <typename T, size_t N> class MsgPacker<T[N]>
{
  public:
    static bool pack(std::ostream& os, const T (&vec)[N])
    {
      for (const auto& item : vec)
      {
        if (!MsgPacker<T>::pack(os, item))
        {
          return false;
        }
      }
      return true;
    }
    static size_t packedSize(const T (&vec)[N])
    {
      size_t size = 0;
      for (const auto& item : vec)
      {
        size += MsgPacker<T>::packedSize(item);
      }
      return size;
    }
    static bool unpack(std::istream& is, T (&vec)[N])
    {
      for (auto& item : vec)
      {
        if (!MsgPacker<T>::unpack(is, item))
        {
          return false;
        }
      }
      return true;
    }
};


/**
@brief	Base class for all messages
@author Tobias Blomberg / SM0SVX
@date   2017-02-25
*/
class Msg
{
  public:
    virtual ~Msg(void) {}

    bool packParent(std::ostream&) const { return true; }
    size_t packedSizeParent(void) const { return 0; }
    bool unpackParent(std::istream&) { return true; }

    virtual bool pack(std::ostream&) const { return true; }
    virtual size_t packedSize(void) const { return 0; }
    virtual bool unpack(std::istream&) { return true; }

    template <typename T>
    bool pack(std::ostream& os, const T& val) const
    {
      return MsgPacker<T>::pack(os, val);
    }
    template <typename T>
    size_t packedSize(const T& val) const
    {
      return MsgPacker<T>::packedSize(val);
    }
    template <typename T>
    bool unpack(std::istream& is, T& val) const
    {
      return MsgPacker<T>::unpack(is, val);
    }

    template <typename T1, typename T2, typename... Args>
    bool pack(std::ostream& os, const T1& v1, const T2& v2,
              const Args&... args) const
    {
      return pack(os, v1) && pack(os, v2, args...);
    }
    template <typename T1, typename T2, typename... Args>
    size_t packedSize(const T1& v1, const T2& v2, const Args&... args) const
    {
      return packedSize(v1) + packedSize(v2, args...);
    }
    template <typename T1, typename T2, typename... Args>
    bool unpack(std::istream& is, T1& v1, T2& v2, Args&... args)
    {
      return unpack(is, v1) && unpack(is, v2, args...);
    }
}; /* class Msg */


} /* namespace */

#endif /* ASYNC_MSG_INCLUDED */


/*
 * This file has not been truncated
 */
