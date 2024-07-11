/**
@file   AsyncSslExtX509SubjectAltName.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2022-05-27

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2022 Tobias Blomberg / SM0SVX

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

/** @example AsyncSslExtX509SubjectAltName_demo.cpp
An example of how to use the SslX509ExtSubjectAltName class
*/

#ifndef ASYNC_SSL_X509_EXT_SUBJECT_ALT_NAME_INCLUDED
#define ASYNC_SSL_X509_EXT_SUBJECT_ALT_NAME_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/



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
@brief  A_brief_class_description
@author Tobias Blomberg / SM0SVX
@date   2022-05-27

A_detailed_class_description

\include AsyncSslExtX509SubjectAltName_demo.cpp
*/
class SslX509ExtSubjectAltName
{
  public:
    /**
     * @brief   Default constructor
     */
    //SslX509ExtSubjectAltName(void);

    explicit SslX509ExtSubjectAltName(const std::string& names)
    {
      m_ext = X509V3_EXT_conf_nid(NULL, NULL, NID_subject_alt_name,
                                  names.c_str());
    }

    /**
     * @brief   Constructor
     * @param   names An existing X509_EXTENSION object
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
     * @param   names An existing GENERAL_NAMES object
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
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
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

    operator const X509_EXTENSION*() const { return m_ext; }

    std::string toString(void) const
    {
      std::string str;
      auto names = reinterpret_cast<GENERAL_NAMES*>(X509V3_EXT_d2i(m_ext));
      do
      {
        if (m_ext == nullptr)
        {
          break;
        }

        int count = sk_GENERAL_NAME_num(names);
        if (count == 0)
        {
          break;
        }

        std::string sep;
        for (int i = 0; i < count; ++i)
        {
          GENERAL_NAME* entry = sk_GENERAL_NAME_value(names, i);
          if (entry == nullptr)
          {
            continue;
          }

          if (entry->type == GEN_DNS)
          {
            unsigned char* utf8 = nullptr;
            int len = ASN1_STRING_to_UTF8(&utf8, entry->d.dNSName);
            if ((utf8 == nullptr) || (len < 1))
            {
              break;
            }
            std::string name(utf8, utf8+len);
            OPENSSL_free(utf8);
            if (name.empty())
            {
              break;
            }
            str += sep + "DNS:" + name;
          }
          else if (entry->type == GEN_EMAIL)
          {
            unsigned char* utf8 = nullptr;
            int len = ASN1_STRING_to_UTF8(&utf8, entry->d.rfc822Name);
            if ((utf8 == nullptr) || (len < 1))
            {
              break;
            }
            std::string name(utf8, utf8+len);
            OPENSSL_free(utf8);
            if (name.empty())
            {
              break;
            }
            str += sep + "email:" + name;
          }
          else if (entry->type == GEN_IPADD)
          {
            int len = ASN1_STRING_length(entry->d.iPAddress);
            if (len == 4)
            {
              const unsigned char* data =
                ASN1_STRING_get0_data(entry->d.iPAddress);
              struct in_addr in_addr;
              in_addr.s_addr = *reinterpret_cast<const unsigned long*>(data);
              Async::IpAddress addr(in_addr);
              str += sep + "IP Address:" + addr.toString();
            }
            else if (len == 16)
            {
                // FIXME: IPv6
              continue;
            }
            else
            {
              break;
            }
          }
          else
          {
            continue;
          }

          sep = ", ";
        }
      } while (false);

      GENERAL_NAMES_free(names);

      return str;
    }

  private:
    X509_EXTENSION* m_ext = nullptr;

};  /* class SslX509ExtSubjectAltName */


} /* namespace Async */

#endif /* ASYNC_SSL_X509_EXT_SUBJECT_ALT_NAME_INCLUDED */

/*
 * This file has not been truncated
 */
