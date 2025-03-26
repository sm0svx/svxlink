/**
@file   AsyncSslExtX509SubjectAltName.h
@brief  Represent the X.509 Subject Alternative Name extension
@author Tobias Blomberg / SM0SVX
@date   2022-05-27

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2003-2025 Tobias Blomberg / SM0SVX

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

/** @example AsyncSslX509_demo.cpp
An example of how to use the SslX509ExtSubjectAltName class
*/

#ifndef ASYNC_SSL_X509_EXT_SUBJECT_ALT_NAME_INCLUDED
#define ASYNC_SSL_X509_EXT_SUBJECT_ALT_NAME_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <functional>


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

/**
@brief  A class representing the X.509 Subject Alternative Name extension
@author Tobias Blomberg / SM0SVX
@date   2022-05-27

\include AsyncSslX509_demo.cpp
*/
class SslX509ExtSubjectAltName
{
  public:
    using ForeachFunction = std::function<void(int, std::string)>;

    /**
     * @brief   Default constructor
     */
    //SslX509ExtSubjectAltName(void);

    /**
     * @brief   Constructor
     * @param   names A string of comma separated names
     *
     * Names are specified on a tag:value format. For example:
     * DNS:example.org, IP:1.2.3.4, email:user@example.org
     */
    explicit SslX509ExtSubjectAltName(const std::string& names)
    {
      m_ext = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name,
                                  names.c_str());
    }

    /**
     * @brief   Constructor
     * @param   ext An existing X509_EXTENSION object
     */
    SslX509ExtSubjectAltName(const X509_EXTENSION* ext)
    {
#if OPENSSL_VERSION_MAJOR >= 3
      m_ext = X509_EXTENSION_dup(ext);
#else
      m_ext = X509_EXTENSION_dup(const_cast<X509_EXTENSION*>(ext));
#endif
    }

    /**
     * @brief   Constructor
     * @param   names A pointer to an existing GENERAL_NAMES object
     */
    explicit SslX509ExtSubjectAltName(GENERAL_NAMES* names)
    {
      int crit = 0;
      m_ext = X509V3_EXT_i2d(NID_subject_alt_name, crit, names);
    }

    /**
     * @brief   Move Constructor
     * @param   other The object to move from
     */
    SslX509ExtSubjectAltName(SslX509ExtSubjectAltName&& other)
    {
      m_ext = other.m_ext;
      other.m_ext = nullptr;
    }

    /**
     * @brief   Disallow copy construction
     */
    SslX509ExtSubjectAltName(const SslX509ExtSubjectAltName&) = delete;

    /**
     * @brief   Disallow copy assignment
     */
    SslX509ExtSubjectAltName& operator=(const SslX509ExtSubjectAltName&)
      = delete;

    /**
     * @brief   Destructor
     */
    ~SslX509ExtSubjectAltName(void)
    {
      if (m_ext != nullptr)
      {
        X509_EXTENSION_free(m_ext);
        m_ext = nullptr;
      }
    }

    /**
     * @brief   Check if the object is initialized
     * @return  Returns \em true if the object is null
     */
    bool isNull(void) const { return m_ext == nullptr; }

#if 0
    bool add(const std::string& name)
    {
      auto names = reinterpret_cast<GENERAL_NAMES*>(X509V3_EXT_d2i(m_ext));
      if (names == nullptr)
      {
        names = GENERAL_NAME_new();
      }
      auto asn1_str = ASN1_STRING_new();
      ASN1_STRING_set(asn1_str, name.c_str(), name.size());
      // How to create a GENERAL_NAME?
      // GENERAL_NAME* general_name = 
      ASN1_STRING_free(asn1_str);
      sk_GENERAL_NAME_push(names, general_name);
    }
#endif

    /**
     * @brief   Cast to X509_EXTENSION pointer
     * @returns Return a pointer to a X509_EXTENSION, or null if uninitialized
     */
    operator const X509_EXTENSION*() const { return m_ext; }

    /**
     * @brief   Loop through all names calling the given function for each one
     * @param   f     The function to call for each name
     * @param   type  The name type to call the function for (default: all)
     *
     * Type can be GEN_DNS, GEN_IPADD or GEN_EMAIL. Other types are ignored.
     */
    void forEach(ForeachFunction f, int type=-1) const
    {
      if (m_ext == nullptr)
      {
        return;
      }

      const auto names = reinterpret_cast<GENERAL_NAMES*>(X509V3_EXT_d2i(m_ext));
      const int count = sk_GENERAL_NAME_num(names);
      for (int i = 0; i < count; ++i)
      {
        const GENERAL_NAME* entry = sk_GENERAL_NAME_value(names, i);
        if ((entry == nullptr) || ((type >= 0) && (entry->type != type)))
        {
          continue;
        }

        std::string name;
        switch (entry->type)
        {
          case GEN_OTHERNAME:
            break;

          case GEN_EMAIL:
            name = asn1StringToUtf8(entry->d.rfc822Name);
            break;

          case GEN_DNS:
            name = asn1StringToUtf8(entry->d.dNSName);
            break;

          case GEN_X400:
          case GEN_DIRNAME:
          case GEN_EDIPARTY:
          case GEN_URI:
            break;

          case GEN_IPADD:
          {
            int len = ASN1_STRING_length(entry->d.iPAddress);
            if (len == 4)
            {
              const unsigned char* data =
                ASN1_STRING_get0_data(entry->d.iPAddress);
              struct in_addr in_addr = {0};
              in_addr.s_addr = *reinterpret_cast<const unsigned long*>(data);
              Async::IpAddress addr(in_addr);
              name = addr.toString();
            }
            else if (len == 16)
            {
              // FIXME: IPv6
            }
            break;
          }

          case GEN_RID:
            break;

          default:
            // Ignore unknown SAN type
            break;
        }
        if (!name.empty())
        {
          f(entry->type, name);
        }
      }
      GENERAL_NAMES_free(names);
    } /* forEach */

    /**
     * @brief   Convert all SANs to a string
     * @param   type  The name type consider (default: all)
     * @returns Return a ", " separated string with SANs
     *
     * Type can be GEN_DNS, GEN_IPADD or GEN_EMAIL.
     */
    std::string toString(int type=-1) const
    {
      std::string sep;
      std::string str;
      forEach(
          [&](int type, std::string name)
          {
            switch (type)
            {
              case GEN_OTHERNAME:
                break;

              case GEN_EMAIL:
                str += sep + "email:" + name;
                break;

              case GEN_DNS:
                str += sep + "DNS:" + name;
                break;

              case GEN_X400:
              case GEN_DIRNAME:
              case GEN_EDIPARTY:
              case GEN_URI:
                break;

              case GEN_IPADD:
                str += sep + "IP Address:" + name;
                break;

              case GEN_RID:
                break;

              default:
                // Ignore unknown SAN type
                break;
            }
            sep = ", ";
          },
          type);
      return str;
    } /* toString */

  private:
    X509_EXTENSION* m_ext = nullptr;

    std::string asn1StringToUtf8(ASN1_IA5STRING* asn1str) const
    {
      std::string str;
      if (asn1str == nullptr)
      {
        return str;
      }
      unsigned char* utf8 = nullptr;
      const int len = ASN1_STRING_to_UTF8(&utf8, asn1str);
      if ((utf8 != nullptr) && (len > 0))
      {
        str.assign(utf8, utf8+len);
      }
      OPENSSL_free(utf8);
      return str;
    }

};  /* class SslX509ExtSubjectAltName */


} /* namespace Async */

#endif /* ASYNC_SSL_X509_EXT_SUBJECT_ALT_NAME_INCLUDED */

/*
 * This file has not been truncated
 */
