/**
@file   AsyncSslX509.h
@brief  A_brief_description_for_this_file
@author Tobias Blomberg / SM0SVX
@date   2020-08-03

\verbatim
<A brief description of the program or library this file belongs to>
Copyright (C) 2003-2020 Tobias Blomberg / SM0SVX

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

/** @example MyNamespaceTemplate_demo.cpp
An example of how to use the SslX509 class
*/

#ifndef ASYNC_SSL_X509_INCLUDED
#define ASYNC_SSL_X509_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <openssl/x509.h>
#include <openssl/bn.h>

#include <iomanip>
#include <numeric>
#include <ctime>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncSslKeypair.h>
#include <AsyncSslX509Extensions.h>


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
@date   2020-08-03

A_detailed_class_description

\include MyNamespaceTemplate_demo.cpp
*/
class SslX509
{
  public:
    enum : long
    {
      VERSION_1 = 0,
      VERSION_2 = 1,
      VERSION_3 = 2
    };

    /**
     * @brief   Default constructor
     */
    SslX509(void) : m_cert(X509_new())
    {
      //std::cout << "### SslX509()" << std::endl;
    }

    /**
     * @brief   Constructor
     */
    SslX509(X509* cert, bool managed=true)
      : m_cert(cert), m_managed(managed)
    {
      //std::cout << "### SslX509(X509*)" << std::endl;
    }

    /**
     * @brief   Constructor
     */
    explicit SslX509(X509_STORE_CTX& ctx)
      : m_cert(X509_STORE_CTX_get_current_cert(&ctx)), m_managed(false)
    {
      //std::cout << "### SslX509(X509_STORE_CTX&)" << std::endl;
      //int ret = X509_up_ref(m_cert);
      //assert(ret == 1);
    }

    /**
     * @brief   Move constructor
     * @param   other The object to move from
     */
    SslX509(SslX509&& other)
    {
      //std::cout << "### SslX509(SslX509&&)" << std::endl;
      set(other.m_cert, other.m_managed);
      //if (m_managed && (m_cert != nullptr))
      //{
      //  X509_free(m_cert);
      //}
      //m_cert = other.m_cert;
      //m_managed = other.m_managed;
      other.m_cert = nullptr;
      other.m_managed = true;
    }

    SslX509& operator=(SslX509&& other)
    {
      //std::cout << "### SslX509::operator=(SslX509&&)" << std::endl;
      set(other.m_cert, other.m_managed);
      //if (m_cert != nullptr)
      //{
      //  X509_free(m_cert);
      //}
      //m_cert = other.m_cert;
      //m_managed = other.m_managed;
      other.m_cert = nullptr;
      other.m_managed = true;
      return *this;
    }

    SslX509(const SslX509&) = delete;

    /**
     * @brief   Constructor taking PEM data
     * @param   pem The PEM data to parse into a CSR object
     */
    //explicit SslX509(const std::string& pem)
    //{
    //  std::cout << "### SslX509(const std::string&)" << std::endl;
    //  readPem(pem);
    //}

    /**
     * @brief   Destructor
     */
    ~SslX509(void)
    {
      //std::cout << "### ~SslX509()" << std::endl;
      set(nullptr);
    }

    void set(X509* cert, bool managed=true)
    {
      if (m_managed && (m_cert != nullptr))
      {
        X509_free(m_cert);
      }
      m_cert = cert;
      m_managed = managed;
    }

    void clear(void)
    {
      if (m_managed && (m_cert != nullptr))
      {
        X509_free(m_cert);
      }
      m_cert = X509_new();
    }

    bool isNull(void) const { return m_cert == nullptr; }

    /**
     * @brief   A_brief_member_function_description
     * @param   param1 Description_of_param1
     * @return  Return_value_of_this_member_function
     */
    SslX509& operator=(const SslX509&) = delete;

    bool setIssuerName(const X509_NAME* name)
    {
      assert(m_cert != nullptr);
#if OPENSSL_VERSION_MAJOR >= 3
      return (X509_set_issuer_name(m_cert, name) == 1);
#else
      auto n = X509_NAME_dup(const_cast<X509_NAME*>(name));
      int ret = X509_set_issuer_name(m_cert, n);
      X509_NAME_free(n);
     return (ret == 1);
#endif
    }

    const X509_NAME* issuerName(void) const
    {
      assert(m_cert != nullptr);
      return X509_get_issuer_name(m_cert);
    }

    bool setSubjectName(const X509_NAME* name)
    {
      assert(m_cert != nullptr);
#if OPENSSL_VERSION_MAJOR >= 3
      return (X509_set_subject_name(m_cert, name) == 1);
#else
      auto n = X509_NAME_dup(const_cast<X509_NAME*>(name));
      int ret = X509_set_subject_name(m_cert, n);
      X509_NAME_free(n);
     return (ret == 1);
#endif
    }

    const X509_NAME* subjectName(void) const
    {
      assert(m_cert != nullptr);
      return X509_get_subject_name(m_cert);
    }

    operator const X509*(void) const { return m_cert; }

    std::string commonName(void) const
    {
      std::string cn;

      const X509_NAME* subj = subjectName();
      if (subj == nullptr)
      {
        return cn;
      }

        // Assume there is only one CN
#if OPENSSL_VERSION_MAJOR >= 3
      int lastpos = X509_NAME_get_index_by_NID(subj, NID_commonName, -1);
#else
      auto s = X509_NAME_dup(const_cast<X509_NAME*>(subj));
      int lastpos = X509_NAME_get_index_by_NID(s, NID_commonName, -1);
      X509_NAME_free(s);
#endif
      if (lastpos >= 0)
      {
        X509_NAME_ENTRY *e = X509_NAME_get_entry(subj, lastpos);
        ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
        cn = reinterpret_cast<const char*>(ASN1_STRING_get0_data(d));
      }
      return cn;
    }

    bool verify(SslKeypair& keypair)
    {
      assert(m_cert != nullptr);
      return (X509_verify(m_cert, keypair) == 1);
    }

    bool readPem(const std::string& pem)
    {
      BIO *mem = BIO_new(BIO_s_mem());
      BIO_puts(mem, pem.c_str());
      if (m_managed && (m_cert != nullptr))
      {
        X509_free(m_cert);
      }
      m_cert = PEM_read_bio_X509(mem, nullptr, nullptr, nullptr);
      BIO_free(mem);
      return (m_cert != nullptr);
    }

    std::string pem(void) const
    {
      assert(m_cert != nullptr);
      BIO *mem = BIO_new(BIO_s_mem());
      int ret = PEM_write_bio_X509(mem, m_cert);
      assert(ret == 1);
      char buf[16384];
      int len = BIO_read(mem, buf, sizeof(buf));
      assert(len > 0);
      BIO_free(mem);
      return std::string(buf, len);
    }

    bool readPemFile(const std::string& filename)
    {
      FILE *p_file = fopen(filename.c_str(), "r");
      if (p_file == nullptr)
      {
        //std::cerr << "### Failed to open file '" << filename
        //          << "' for reading certificate" << std::endl;
        return false;
      }
      if (m_managed && (m_cert != nullptr))
      {
        X509_free(m_cert);
      }
      m_cert = PEM_read_X509(p_file, nullptr, nullptr, nullptr);
      fclose(p_file);
      return (m_cert != nullptr);
    }

    bool writePemFile(FILE* f)
    {
      assert(m_cert != nullptr);
      if (f == nullptr)
      {
        //std::cerr << "### Failed to open file '" << filename
        //          << "' for writing certificate" << std::endl;
        return false;
      }
      int ret = PEM_write_X509(f, m_cert);
      fclose(f);
      return (ret == 1);
    }

    bool writePemFile(const std::string& filename)
    {
      return writePemFile(fopen(filename.c_str(), "w"));
    }

    bool appendPemFile(const std::string& filename)
    {
      return writePemFile(fopen(filename.c_str(), "a"));
    }

    bool setVersion(long version)
    {
      return (X509_set_version(m_cert, version) == 1);
    }

    long version(void) const { return X509_get_version(m_cert); }

    void setNotBefore(std::time_t in_time)
    {
      X509_time_adj_ex(X509_get_notBefore(m_cert), 0L, 0L, &in_time);
    }

    std::time_t notBefore(void) const
    {
      ASN1_TIME* epoch = ASN1_TIME_set(nullptr, 0);
      const ASN1_TIME* not_before = X509_get0_notBefore(m_cert);
      int pday=0, psec=0;
      ASN1_TIME_diff(&pday, &psec, epoch, not_before);
      ASN1_STRING_free(epoch);
      return static_cast<std::time_t>(pday)*24*3600 + psec;
    }

    std::string notBeforeString(void) const
    {
      const ASN1_TIME* t = X509_get_notBefore(m_cert);
      int len = ASN1_STRING_length(t);
      if (t == nullptr)
      {
        return std::string();
      }
      const unsigned char* data = ASN1_STRING_get0_data(t);
      return std::string(data, data+len);
    }

    std::string notBeforeLocaltimeString(void) const
    {
      std::time_t t = notBefore();
      std::ostringstream ss;
      ss << std::put_time(std::localtime(&t), "%c");
      return ss.str();
    }

    void setNotAfter(std::time_t in_time)
    {
      X509_time_adj_ex(X509_get_notAfter(m_cert), 0L, 0L, &in_time);
    }

    std::time_t notAfter(void) const
    {
      ASN1_TIME* epoch = ASN1_TIME_set(nullptr, 0);
      const ASN1_TIME* not_after = X509_get0_notAfter(m_cert);
      int pday=0, psec=0;
      ASN1_TIME_diff(&pday, &psec, epoch, not_after);
      ASN1_STRING_free(epoch);
      return static_cast<std::time_t>(pday)*24*3600 + psec;
    }

    std::string notAfterString(void) const
    {
      const ASN1_TIME* t = X509_get_notAfter(m_cert);
      const unsigned char* data = ASN1_STRING_get0_data(t);
      int len = ASN1_STRING_length(t);
      if (t == nullptr)
      {
        return std::string();
      }
      return std::string(data, data+len);
    }

    std::string notAfterLocaltimeString(void) const
    {
      std::time_t t = notAfter();
      std::ostringstream ss;
      ss << std::put_time(std::localtime(&t), "%c");
      return ss.str();
    }

    void timeSpan(int& days, int& seconds) const
    {
      const ASN1_TIME* not_before = X509_get_notBefore(m_cert);
      const ASN1_TIME* not_after = X509_get_notAfter(m_cert);
      ASN1_TIME_diff(&days, &seconds, not_before, not_after);
    }

    bool timeIsWithinRange(std::time_t tbegin=time(NULL),
                           std::time_t tend=time(NULL)) const
    {
      const ASN1_TIME* not_before = X509_get_notBefore(m_cert);
      const ASN1_TIME* not_after = X509_get_notAfter(m_cert);
      return ((not_before == nullptr) ||
              (X509_cmp_time(not_before, &tbegin) == -1)) &&
             ((not_after == nullptr) ||
              (X509_cmp_time(not_after, &tend) == 1));
    }

    int signatureType(void) const
    {
      return X509_get_signature_type(m_cert);
    }

    void setSerialNumber(long serial_number=-1)
    {
      // FIXME: Error handling
      ASN1_INTEGER *p_serial_number = ASN1_INTEGER_new();
      if (serial_number < 0)
      {
        randSerial(p_serial_number);
      }
      else
      {
        ASN1_INTEGER_set(p_serial_number, serial_number);
      }
      X509_set_serialNumber(m_cert, p_serial_number);
      ASN1_INTEGER_free(p_serial_number);
    }

    std::string serialNumberString(void) const
    {
      const ASN1_INTEGER* i = X509_get0_serialNumber(m_cert);
      if (i == nullptr)
      {
        return std::string();
      }
      BIGNUM *bn = ASN1_INTEGER_to_BN(i, nullptr);
      if (bn == nullptr)
      {
        return std::string();
      }
      char *hex = BN_bn2hex(bn);
      BN_free(bn);
      if (hex == nullptr)
      {
        return std::string();
      }
      std::string ret(hex, hex+strlen(hex));
      ret = std::string("0x") + ret;
      OPENSSL_free(hex);
      return ret;
    }

    //void setIssuerName()
    //{
    //  X509_set_issuer_name(p_generated_cert, X509_REQ_get_subject_name(pCertReq));
    //}

    void addIssuerName(const std::string& field, const std::string& value)
    {
      assert(m_cert != nullptr);
      X509_NAME* name = X509_get_issuer_name(m_cert);
      if (name == nullptr)
      {
        name = X509_NAME_new();
      }
      assert(name != nullptr);
      int ret = X509_NAME_add_entry_by_txt(name, field.c_str(), MBSTRING_UTF8,
          reinterpret_cast<const unsigned char*>(value.c_str()),
          value.size(), -1, 0);
      assert(ret == 1);
      ret = X509_set_issuer_name(m_cert, name);
      assert(ret == 1);
    }

    void addSubjectName(const std::string& field, const std::string& value)
    {
      assert(m_cert != nullptr);
      X509_NAME* name = X509_get_subject_name(m_cert);
      if (name == nullptr)
      {
        name = X509_NAME_new();
      }
      assert(name != nullptr);
      int ret = X509_NAME_add_entry_by_txt(name, field.c_str(), MBSTRING_UTF8,
          reinterpret_cast<const unsigned char*>(value.c_str()),
          value.size(), -1, 0);
      assert(ret == 1);
      ret = X509_set_subject_name(m_cert, name);
      assert(ret == 1);
    }

    std::string issuerNameString(void) const
    {
      std::string str;
      const X509_NAME* nm = issuerName();
      if (nm != nullptr)
      {
        BIO *mem = BIO_new(BIO_s_mem());
        assert(mem != nullptr);
          // Print all subject names on one line. Don't escape multibyte chars.
        int len = X509_NAME_print_ex(mem, nm, 0,
            XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);
        if (len > 0)
        {
          char buf[len+1];
          len = BIO_read(mem, buf, sizeof(buf));
          if (len > 0)
          {
            str = std::string(buf, len);
          }
        }
        BIO_free(mem);
      }
      return str;
    }

    std::string subjectNameString(void) const
    {
      std::string str;
      const X509_NAME* nm = subjectName();
      if (nm != nullptr)
      {
        BIO *mem = BIO_new(BIO_s_mem());
        assert(mem != nullptr);
          // Print all subject names on one line. Don't escape multibyte chars.
        int len = X509_NAME_print_ex(mem, nm, 0,
            XN_FLAG_ONELINE & ~ASN1_STRFLGS_ESC_MSB);
        if (len > 0)
        {
          char buf[len+1];
          len = BIO_read(mem, buf, sizeof(buf));
          if (len > 0)
          {
            str = std::string(buf, len);
          }
        }
        BIO_free(mem);
      }
      return str;
    }

    void addExtensions(const SslX509Extensions& exts)
    {
      for (int i=0; i<X509v3_get_ext_count(exts); ++i)
      {
        auto ext = X509v3_get_ext(exts, i);
        X509_add_ext(m_cert, ext, -1);
      }
    }

    SslKeypair publicKey(void) const
    {
      assert(m_cert != nullptr);
      return SslKeypair(X509_get_pubkey(m_cert));
    }

    bool setPublicKey(SslKeypair& pkey)
    {
      return (X509_set_pubkey(m_cert, pkey) == 1);
    }

    bool sign(SslKeypair& pkey)
    {
      auto md = EVP_sha256();
      auto md_size = EVP_MD_size(md);
      return (X509_sign(m_cert, pkey, md) != md_size);
    }

    std::vector<unsigned char> digest(void) const
    {
      assert(m_cert != nullptr);
      std::vector<unsigned char> md(EVP_MAX_MD_SIZE);
      unsigned int len = md.size();
      if (X509_digest(m_cert, EVP_sha256(), md.data(), &len) != 1)
      {
        len = 0;
      }
      md.resize(len);
      return md;
    }

#if 0
    int certificateType(void) const
    {
      auto pkey = X509_get0_pubkey(m_cert);
      if (pkey == nullptr)
      {
        return -1;
      }
      return X509_certificate_type(m_cert, pkey);
    }
#endif

    bool matchHost(const std::string& name) const
    {
      int chk = X509_check_host(m_cert, name.c_str(), name.size(), 0, nullptr);
      return chk > 0;
    }

    bool matchIp(const IpAddress& ip) const
    {
      int chk = X509_check_ip_asc(m_cert, ip.toString().c_str(), 0);
      return chk > 0;
    }

    void print(const std::string& prefix="") const
    {
      if (isNull())
      {
        std::cout << "NULL" << std::endl;
        return;
      }

      int ext_idx = X509_get_ext_by_NID(m_cert, NID_subject_alt_name, -1);
      auto ext = X509_get_ext(m_cert, ext_idx);
      auto sanstr = SslX509ExtSubjectAltName(ext).toString();
      const auto md = digest();
      std::cout
        //<< prefix << "Version          : " << (version()+1) << "\n"
        << prefix << "Serial No.       : " << serialNumberString() << "\n"
        << prefix << "Issuer           : " << issuerNameString() << "\n"
        << prefix << "Subject          : " << subjectNameString() << "\n"
        << prefix << "Not Before       : "
                  << notBeforeLocaltimeString() << "\n"
        << prefix << "Not After        : "
                  << notAfterLocaltimeString() << "\n"
        //<< prefix << "Signature Type   : " << signatureType() << "\n"
        ;
      if (!sanstr.empty())
      {
        std::cout << prefix << "Subject Alt Name : " << sanstr << "\n";
      }
      //std::cout << prefix << "SHA256 Digest    : " << std::accumulate(
      //    md.begin(),
      //    md.end(),
      //    std::ostringstream() << std::hex << std::setfill('0'),
      //    [](std::ostringstream& ss, unsigned char x)
      //    {
      //      if (!ss.str().empty()) { ss << ":"; }
      //      return std::move(ss) << std::setw(2) << unsigned(x);
      //    }).str() + "\n"
      std::cout << std::flush;
    }

  protected:

  private:
    X509* m_cert = nullptr;
    bool  m_managed = true;

    int randSerial(ASN1_INTEGER *ai)
    {
      BIGNUM *p_bignum = NULL;
      int ret = -1;

      if (NULL == (p_bignum = BN_new())) {
        goto CLEANUP;
      }

      if (!BN_rand(p_bignum, 159, 0, 0)) {
        goto CLEANUP;
      }

      if (ai && !BN_to_ASN1_INTEGER(p_bignum, ai)) {
        goto CLEANUP;
      }

      ret = 1;

      CLEANUP:
        BN_free(p_bignum);
        return ret;
    }
};  /* class SslX509 */


} /* namespace */

#endif /* ASYNC_SSL_X509_INCLUDED */

/*
 * This file has not been truncated
 */
