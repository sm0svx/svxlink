/**
@file   AsyncSslX509.h
@brief  Implements a representation of a X.509 certificate
@author Tobias Blomberg / SM0SVX
@date   2020-08-03

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
#include <limits>


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
@brief  A class representing an X.509 certificate
@author Tobias Blomberg / SM0SVX
@date   2020-08-03

\include AsyncSslX509_demo.cpp
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
     * @param   cert    A pointer to an existing OpenSSL X509 object
     * @param   managed If \em true, the pointer will be freed on destruction
     */
    SslX509(X509* cert, bool managed=true)
      : m_cert(cert), m_managed(managed)
    {
      //std::cout << "### SslX509(X509*)" << std::endl;
    }

    /**
     * @brief   Constructor
     * @param   ctx An OpenSSL X509_STORE_CTX
     *
     * Get the current certificate from the given store context. The returned
     * pointer will be used as the data container in this object but will not
     * be freed on dustruction since the store context is assumed to own the
     * certificate.
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

    /**
     * @brief   Move assignment operator
     * @param   other The object to move from
     */
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

    /**
     * @brief   Don't allow copy construction
     */
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

    /**
     * @brief   Set the internal X509 object to use
     * @param   cert    A pointer to an existing OpenSSL X509 object
     * @param   managed Set to \em true to free the X509 object on destruction
     */
    void set(X509* cert, bool managed=true)
    {
      if (m_managed && (m_cert != nullptr))
      {
        X509_free(m_cert);
      }
      m_cert = cert;
      m_managed = managed;
    }

    /**
     * @brief   Set this object to empty
     *
     * The internal OpenSSL X509 object will be freed if it's managed by this
     * object.
     */
    void clear(void)
    {
      if (m_managed && (m_cert != nullptr))
      {
        X509_free(m_cert);
      }
      m_cert = X509_new();
    }

    /**
     * @brief   Check if this object is empty
     * @return  Returns \em true if this object is empty
     */
    bool isNull(void) const { return m_cert == nullptr; }

    /**
     * @brief   Disallow use of the copy assignment operator
     */
    SslX509& operator=(const SslX509&) = delete;

    /**
     * @brief   Set the issuer distinguished name
     * @param   name A pointer to an already existing X509_NAME OpenSSL object
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Get the issuer distinguished name
     * @return  Returns a pointer to a X509_NAME OpenSSL object
     */
    const X509_NAME* issuerName(void) const
    {
      assert(m_cert != nullptr);
      return X509_get_issuer_name(m_cert);
    }

    /**
     * @brief   Set the subject distinguished name
     * @param   name A pointer to an already existing X509_NAME OpenSSL object
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Get the subject distinguished name
     * @return  Returns a pointer to a X509_NAME OpenSSL object
     */
    const X509_NAME* subjectName(void) const
    {
      assert(m_cert != nullptr);
      return X509_get_subject_name(m_cert);
    }

    /**
     * @brief   Cast to an OpenSSL X509 pointer
     * @return  Returns a pointer to an OpenSSL X509 object
     */
    operator const X509*(void) const { return m_cert; }

    /**
     * @brief   Get the common name of the subject
     * @return  Returns the common name as a string
     */
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

    /**
     * @brief   Verify that this certificate is signed by the given key
     * @param   keypair The key to check against
     * @return  Returns \em true if the verification succeeds
     */
    bool verify(SslKeypair& keypair)
    {
      assert(m_cert != nullptr);
      return (X509_verify(m_cert, keypair) == 1);
    }

    /**
     * @brief   Initialize this certificate from a string containing PEM data
     * @param   pem The PEM data
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Get this certificate as PEM data
     * @return  Returns the PEM data as a string
     */
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

    /**
     * @brief   Initialize this object with PEM data read from given file
     * @param   filename The path to the file to read PEM data from
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Write this certificate to file in PEM format
     * @param   f An open file to write data to
     * @return  Returns \em true on success
     */
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

    /**
     * @brief   Write this certificate to file in PEM format
     * @param   filename The path to the file to write PEM data to
     * @return  Returns \em true on success
     */
    bool writePemFile(const std::string& filename)
    {
      return writePemFile(fopen(filename.c_str(), "w"));
    }

    /**
     * @brief   Append this certificate to file in PEM format
     * @param   filename The path to the file to append PEM data to
     * @return  Returns \em true on success
     */
    bool appendPemFile(const std::string& filename)
    {
      return writePemFile(fopen(filename.c_str(), "a"));
    }

    /**
     * @brief   Set the version of this certificate
     * @param   version The version that this certificate adheres to
     * @return  Returns \em true on success
     *
     * Ex: setVersion(Async::SslX509::VERSION_3)
     */
    bool setVersion(long version)
    {
      return (X509_set_version(m_cert, version) == 1);
    }

    /**
     * @brief   Get the version of this certificate
     * @return  Returns the version of this certificate
     */
    long version(void) const { return X509_get_version(m_cert); }

    /**
     * @brief   Set the date and time from which this certificate is valid
     * @param   in_time The time as seconds since the Unix epoch
     */
    void setNotBefore(std::time_t in_time)
    {
      X509_time_adj_ex(X509_get_notBefore(m_cert), 0L, 0L, &in_time);
    }

    /**
     * @brief   Get the date and time from which this certificate is valid
     * @return  Returns the time as seconds since the Unix epoch
     */
    std::time_t notBefore(void) const
    {
      ASN1_TIME* epoch = ASN1_TIME_set(nullptr, 0);
      const ASN1_TIME* not_before = X509_get0_notBefore(m_cert);
      int pday=0, psec=0;
      ASN1_TIME_diff(&pday, &psec, epoch, not_before);
      ASN1_STRING_free(epoch);
      return static_cast<std::time_t>(pday)*24*3600 + psec;
    }

    /**
     * @brief   Get the date and time from which this certificate is valid
     * @return  Returns the time in UTC as a readable string
     */
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

    /**
     * @brief   Get the date and time from which this certificate is valid
     * @return  Returns the time in the local timezone as a readable string
     */
    std::string notBeforeLocaltimeString(void) const
    {
      std::time_t t = notBefore();
      std::ostringstream ss;
      ss << std::put_time(std::localtime(&t), "%c");
      return ss.str();
    }

    /**
     * @brief   Set the date and time up to which this certificate is valid
     * @param   in_time The time as seconds since the Unix epoch
     */
    void setNotAfter(std::time_t in_time)
    {
      X509_time_adj_ex(X509_get_notAfter(m_cert), 0L, 0L, &in_time);
    }

    /**
     * @brief   Get the date and time up to which this certificate is valid
     * @return  Returns the time as seconds since the Unix epoch
     */
    std::time_t notAfter(void) const
    {
      ASN1_TIME* epoch = ASN1_TIME_set(nullptr, 0);
      const ASN1_TIME* not_after = X509_get0_notAfter(m_cert);
      int pday=0, psec=0;
      ASN1_TIME_diff(&pday, &psec, epoch, not_after);
      ASN1_STRING_free(epoch);
      return static_cast<std::time_t>(pday)*24*3600 + psec;
    }

    /**
     * @brief   Get the date and time up to which this certificate is valid
     * @return  Returns the time in UTC as a readable string
     */
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

    /**
     * @brief   Get the date and time up to which this certificate is valid
     * @return  Returns the time in the local timezone as a readable string
     */
    std::string notAfterLocaltimeString(void) const
    {
      std::time_t t = notAfter();
      std::ostringstream ss;
      ss << std::put_time(std::localtime(&t), "%c");
      return ss.str();
    }

    /**
     * @brief   Set the validity time relative to current time
     * @param   days        The number of days this certificate should be valid
     * @param   offset_days The number of days to offset from current time
     */
    void setValidityTime(unsigned days, int offset_days=0)
    {
      std::time_t validity_secs = days * 24 * 60 * 60;
      std::time_t tnow = time(NULL);
      tnow += offset_days * 24 * 60 * 60;
      if (std::numeric_limits<time_t>::max() - validity_secs < tnow)
      {
        validity_secs = std::numeric_limits<time_t>::max() - tnow;
      }
      setNotBefore(tnow);
      setNotAfter(tnow + validity_secs);
    }

    /**
     * @brief   The duration that this certificate is valid
     * @param   days    Return the number of days of validity
     * @param   seconds Return the number of additional seconds of validity
     */
    void validityTime(int& days, int& seconds) const
    {
      const ASN1_TIME* not_before = X509_get_notBefore(m_cert);
      const ASN1_TIME* not_after = X509_get_notAfter(m_cert);
      ASN1_TIME_diff(&days, &seconds, not_before, not_after);
    }

    /**
     * @brief   Check if the certificate is valid within the given range
     * @param   tbegin  The earliest time the certificate must be valid
     * @param   tend    The latest time the certificate must be valid
     * @return  Returns \em true if the certificate is valid
     */
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

    /**
     * @brief   Get the signature type
     * @return  Returns the signature type
     *
     * See the documentation for the OpenSSL X509_get_signature_type function
     * for more information.
     */
    int signatureType(void) const
    {
      return X509_get_signature_type(m_cert);
    }

    /**
     * @brief   Set the serial number of the certificate
     * @param   serial_number The serial number to set
     *
     * If no serial number is given, it will be randomized.
     */
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

    /**
     * @brief   Get the serial number as a string
     * @return  Returns the serial number as a hex string
     */
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

    /**
     * @brief   Add a name to the issuer distinguished name
     * @param   field The name of the DN field to set
     * @param   value The value to set the DN field to
     */
    void addIssuerName(const std::string& field, const std::string& value)
    {
      // FIXME: Error handling
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

    /**
     * @brief   Add a name to the subject distinguished name
     * @param   field The name of the DN field to set
     * @param   value The value to set the DN field to
     */
    void addSubjectName(const std::string& field, const std::string& value)
    {
      // FIXME: Error handling
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

    /**
     * @brief   Get the issuer distinguished name as a string
     * @return  Returns the issuer DN as a string
     */
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

    /**
     * @brief   Get the subject distinguished name as a string
     * @return  Returns the subject DN as a string
     */
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

    /**
     * @brief   Add v3 extensions to this certificate
     * @param   exts Add the given extensions to this certificate
     */
    void addExtensions(const SslX509Extensions& exts)
    {
      for (int i=0; i<X509v3_get_ext_count(exts); ++i)
      {
        auto ext = X509v3_get_ext(exts, i);
        X509_add_ext(m_cert, ext, -1);
      }
    }

    /**
     * @brief   Get the public key
     * @retrun  Returns the public key
     */
    SslKeypair publicKey(void) const
    {
      assert(m_cert != nullptr);
      return SslKeypair(X509_get_pubkey(m_cert));
    }

    /**
     * @brief   Set the public key for this certificate
     * @param   pkey The public key to set
     * @return  Returns \em true on success
     */
    bool setPublicKey(SslKeypair& pkey)
    {
      return (X509_set_pubkey(m_cert, pkey) == 1);
    }

    /**
     * @brief   Sign this certificate using the given key
     * @param   pkey The key to sign with
     * @return  Returns \em true on success
     */
    bool sign(SslKeypair& pkey)
    {
      auto md = EVP_sha256();
      auto md_size = EVP_MD_size(md);
      return (X509_sign(m_cert, pkey, md) != md_size);
    }

    /**
     * @brief   Get the digest of this certificate
     * @return  Returns the sha256 digest of this certificate
     */
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

    /**
     * @brief   Check if the given hostname match this certificate
     * @param   name The hostname to match against
     * @return  Returns \em true on success
     */
    bool matchHost(const std::string& name) const
    {
      int chk = X509_check_host(m_cert, name.c_str(), name.size(), 0, nullptr);
      return chk > 0;
    }

    /**
     * @brief   Check if the given IP address match this certificate
     * @param   ip The IP address to match against
     * @return  Returns \em true on success
     */
    bool matchIp(const IpAddress& ip) const
    {
      int chk = X509_check_ip_asc(m_cert, ip.toString().c_str(), 0);
      return chk > 0;
    }

    /**
     * @brief   Print this certificate to std::cout
     * @param   prefix A prefix to add to each printed row
     */
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
