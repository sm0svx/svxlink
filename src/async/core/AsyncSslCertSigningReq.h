/**
@file   AsyncCertSigningReq.h
@brief  A class representing a certificate signing request
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
An example of how to use the SslCertSigningReq class
*/

#ifndef ASYNC_CERT_SIGNING_REQ_INCLUDED
#define ASYNC_CERT_SIGNING_REQ_INCLUDED


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <openssl/x509.h>
#include <cassert>
#include <string>
#include <iostream>
#include <vector>
#include <algorithm>
#include <iomanip>


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
@brief  A class representing a certificate signing request
@author Tobias Blomberg / SM0SVX
@date   2020-08-03

\include AsyncSslX509_demo.cpp
*/
class SslCertSigningReq
{
  public:
    enum : long
    {
      VERSION_1 = 0
    };

    /**
     * @brief   Default constructor
     */
    SslCertSigningReq(void)
    {
      m_req = X509_REQ_new();
      assert(m_req != nullptr);
    }

    /**
     * @brief   Constructor using existing X509_REQ
     * @param   req An existing X509_REQ
     *
     * This object will take ownership of the X509_REQ and so it will be freed
     * at the destruction of this object.
     */
    SslCertSigningReq(X509_REQ* req) : m_req(req) {}

    /**
     * @brief   Move constructor
     * @param   other The other object to move data from
     */
    SslCertSigningReq(SslCertSigningReq&& other)
      : m_req(other.m_req), m_file_path(other.m_file_path)
    {
      other.m_req = nullptr;
      other.m_file_path.clear();
    }

    /**
     * @brief   Copy constructor
     * @param   other The other object to copy data from
     */
    SslCertSigningReq(SslCertSigningReq& other)
    {
#if OPENSSL_VERSION_MAJOR >= 3
      m_req = X509_REQ_dup(other);
#else
      m_req = X509_REQ_dup(const_cast<X509_REQ*>(other.m_req));
#endif
      m_file_path = other.m_file_path;
    }

    /**
     * @brief   Constructor taking PEM data
     * @param   pem The PEM data to parse into a CSR object
     */
    //SslCertSigningReq(const std::string& pem)
    //{
    //  readPem(pem);
    //}

    /**
     * @brief   Destructor
     */
    ~SslCertSigningReq(void)
    {
      if (m_req != nullptr)
      {
        X509_REQ_free(m_req);
        m_req = nullptr;
      }
      m_file_path.clear();
    }

    /**
     * @brief   Copy assignment operator
     * @param   other The object to copy
     * @return  Returns a reference to this object
     */
    SslCertSigningReq& operator=(SslCertSigningReq& other)
    {
#if OPENSSL_VERSION_MAJOR >= 3
      m_req = X509_REQ_dup(other);
#else
      m_req = X509_REQ_dup(const_cast<X509_REQ*>(other.m_req));
#endif
      m_file_path = other.m_file_path;
      return *this;
    }

    /**
     * @brief   Move assigned operator
     * @param   other The object to move from
     * @return  Returns a reference to this object
     */
    SslCertSigningReq& operator=(SslCertSigningReq&& other)
    {
      m_req = other.m_req;
      m_file_path = other.m_file_path;
      other.m_req = nullptr;
      other.m_file_path.clear();
      return *this;
    }

    /**
     * @brief   Cast to a pointer to a X509_REQ object
     * @return  Returns a pointer to a X509_REQ object
       */
    operator const X509_REQ*() const { return m_req; }

    /**
     * @brief   Initialize this object from an existing X509_REQ object
     * @param   req Pointer to an existing X509_REQ object
     */
    void set(X509_REQ* req)
    {
      if (m_req != nullptr)
      {
        X509_REQ_free(m_req);
      }
      m_req = req;
    }

    /**
     * @brief   Remove all information in this object
     *
     * After calling this function the isNull method will return true.
     */
    void clear(void)
    {
      if (m_req != nullptr)
      {
        X509_REQ_free(m_req);
      }
      m_req = X509_REQ_new();
    }

    /**
     * @brief   Check if this object is empty
     * @return  Returns \em true if this object is empty
     */
    bool isNull(void) const { return (m_req == nullptr); }

    /**
     * @brief   Set the version of the request
     * @param   version The version to set
     *
     * The version indicate what information the request can contain.
     *
     * Ex: setVersion(Async::SslCertSigningReq::VERSION_1);
     */
    bool setVersion(long version)
    {
      assert(m_req != nullptr);
      return (X509_REQ_set_version(m_req, version) == 1);
    }

    /**
     * @brief   Get the version of this CSR
     * @return  Returns the version of this CSR
     */
    long version(void) const { return X509_REQ_get_version(m_req); }

    /**
     * @brief   Add a subject name component
     * @param   field The name of the field to add
     * @param   value The value of the field to add
     *
     * Ex: addSubjectName("CN", "host.example.org");
     */
    bool addSubjectName(const std::string& field, const std::string& value)
    {
      assert(m_req != nullptr);
      X509_NAME* name = X509_REQ_get_subject_name(m_req);
      if (name == nullptr)
      {
        name = X509_NAME_new();
      }
      assert(name != nullptr);
      bool success = (X509_NAME_add_entry_by_txt(name, field.c_str(),
            MBSTRING_UTF8,
            reinterpret_cast<const unsigned char*>(value.c_str()),
            value.size(), -1, 0) == 1);
      success = success && (X509_REQ_set_subject_name(m_req, name) == 1);
      return success;
    }

    /**
     * @brief   Set the subject name from a X509_NAME pointer
     * @param   name The X509_NAME pointer
     * @return  Returns \em true on success
     */
    bool setSubjectName(X509_NAME* name)
    {
      assert(m_req != nullptr);
      return X509_REQ_set_subject_name(m_req, name);
    }

    /**
     * @brief   Return the subject name as a X509_NAME pointer
     * @return  Returns the internal X509_NAME pointer
     */
    const X509_NAME* subjectName(void) const
    {
      if (m_req == nullptr) return nullptr;
      return X509_REQ_get_subject_name(m_req);
    }

    /**
     * @brief   Get the subject digest
     * @return  A sha256 digest of the subject
     */
    std::vector<unsigned char> subjectDigest(void) const
    {
      std::vector<unsigned char> md;
      auto subj = subjectName();
      if (subj != nullptr)
      {
        auto mdtype = EVP_sha256();
        //unsigned int mdlen = EVP_MD_meth_get_result_size(mdtype);
        //unsigned int mdlen = EVP_MD_get_size(mdtype);
        unsigned int mdlen = EVP_MD_size(mdtype);
        md.resize(mdlen);
        if (X509_NAME_digest(subj, mdtype, md.data(), &mdlen) != 1)
        {
          md.resize(0);
        }
      }
      return md;
    }

    /**
     * @brief   Get the subject DN as a string
     * @return  Returns a string representation of the subject DN
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
     * @brief   Get the subject common name
     * @return  Returns the common name (CN)
     */
    std::string commonName(void) const
    {
      std::string cn;

      const auto subj = subjectName();
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
      //int lastpos = X509_NAME_get_index_by_NID(subj, NID_commonName, -1);
      if (lastpos >= 0)
      {
        X509_NAME_ENTRY *e = X509_NAME_get_entry(subj, lastpos);
        ASN1_STRING *d = X509_NAME_ENTRY_get_data(e);
        cn = reinterpret_cast<const char*>(ASN1_STRING_get0_data(d));
      }
      return cn;
    }

    /**
     * @brief   Add extensions to this CSR
     * @param   exts The extensions to add
     */
    void addExtensions(SslX509Extensions& exts)
    {
      assert(m_req != nullptr);
#if OPENSSL_VERSION_MAJOR >= 3
      X509_REQ_add_extensions(m_req, exts);
#else
      auto e = sk_X509_EXTENSION_dup(exts);
      X509_REQ_add_extensions(m_req, e);
      sk_X509_EXTENSION_free(e);
#endif
    }

    /**
     * @brief   Get the extensions in this CSR
     * @return  Returns an object representing the extensions in this object
     */
    SslX509Extensions extensions(void) const
    {
      assert(m_req != nullptr);
      return SslX509Extensions(X509_REQ_get_extensions(m_req));
    }

    /**
     * @brief   Get the public key
     * @return  Returns the public key as a SslKeypair object
     */
    SslKeypair publicKey(void) const
    {
      assert(m_req != nullptr);
      return SslKeypair(X509_REQ_get_pubkey(m_req));
    }

    /**
     * @brief   Set the public key
     * @param   pubkey The public key to set given as a SslKeypair object
     * @return  Returns \em true on success
     */
    bool setPublicKey(SslKeypair& pubkey)
    {
      assert(m_req != nullptr);
      return (X509_REQ_set_pubkey(m_req, pubkey) == 1);
    }

    /**
     * @brief   Sign the CSR using the given private key
     * @param   The private key to sign the CSR with
     * @return  Returns \em true on sucess
     */
    bool sign(SslKeypair& privkey)
    {
      assert(m_req != nullptr);
      auto md = EVP_sha256();
      //auto md_size = EVP_MD_get_size(md);
      auto md_size = EVP_MD_size(md);
      return (X509_REQ_sign(m_req, privkey, md) == md_size);
    }

    /**
     * @brief   Verify the signature of this CSR
     * @param   pubkey The public key to use in the verification
     *
     * Verify that this CSR was signed using the private key matching the given
     * public key.
     */
    bool verify(SslKeypair& pubkey) const
    {
      assert(m_req != nullptr);
      return (X509_REQ_verify(m_req, pubkey) == 1);
    }

    /**
     * @brief   Get the sha256 digest of this CSR
     * @return  Returns the sha256 digest of this CSR
     */
    std::vector<unsigned char> digest(void) const
    {
      assert(m_req != nullptr);
      std::vector<unsigned char> md;
      auto mdtype = EVP_sha256();
      //unsigned int mdlen = EVP_MD_get_size(mdtype);
      unsigned int mdlen = EVP_MD_size(mdtype);
      md.resize(mdlen);
      unsigned int len = md.size();
      if (X509_REQ_digest(m_req, mdtype, md.data(), &len) != 1)
      {
        md.resize(0);
      }
      return md;
    }

    /**
     * @brief   Read PEM formatted CSR data into this object
     * @param   pem The PEM data
     * @return  Returns \em true on success
     */
    bool readPem(const std::string& pem)
    {
      BIO *mem = BIO_new(BIO_s_mem());
      BIO_puts(mem, pem.c_str());
      if (m_req != nullptr)
      {
        X509_REQ_free(m_req);
      }
      m_req = PEM_read_bio_X509_REQ(mem, nullptr, nullptr, nullptr);
      BIO_free(mem);
      return (m_req != nullptr);
    }

    /**
     * @brief   Read PEM formatted CSR data from file into this object
     * @param   filename The name of a file containing a CSR in PEM format
     * @return  Returns \em true on success
     */
    bool readPemFile(const std::string& filename)
    {
      m_file_path = filename;
      if (m_req != nullptr)
      {
        X509_REQ_free(m_req);
        m_req = nullptr;
      }
      FILE *p_file = fopen(filename.c_str(), "r");
      if (p_file == nullptr)
      {
        return false;
      }
      m_req = PEM_read_X509_REQ(p_file, nullptr, nullptr, nullptr);
      fclose(p_file);
      return (m_req != nullptr);
    }

    /**
     * @brief   Get the file path associated with this CSR
     * @return  Returns the path
     *
     * This CSR is considered associated to a file if the readPemFile method
     * was used to populate it with data.
     */
    const std::string& filePath(void) const { return m_file_path; }

    /**
     * @brief   Write the CSR data to a PEM file
     * @param   f An opened file object to write the PEM data to
     * @return  Returns \em true on success
     */
    bool writePemFile(FILE* f)
    {
      assert(m_req != nullptr);
      if (f == nullptr)
      {
        return false;
      }
      int ret = PEM_write_X509_REQ(f, m_req);
      fclose(f);
      return (ret == 1);
    }

    /**
     * @brief   Write the CSR data to a PEM file
     * @param   filename The path to the file to write PEM data to
     * @return  Returns \em true on success
     */
    bool writePemFile(const std::string& filename)
    {
      return writePemFile(fopen(filename.c_str(), "w"));
    }

    /**
     * @brief   Append the CSR data to a PEM file
     * @param   filename The path to the file to append PEM data to
     * @return  Returns \em true on success
     */
    bool appendPemFile(const std::string& filename)
    {
      return writePemFile(fopen(filename.c_str(), "a"));
    }

    /**
     * @brief   Get the data in this CSR as a PEM string
     * @return  Returns the PEM data as a string
     */
    std::string pem(void) const
    {
      assert(m_req != nullptr);
      BIO *mem = BIO_new(BIO_s_mem());
      int ret = PEM_write_bio_X509_REQ(mem, m_req);
      assert(ret == 1);
      char buf[16384];
      int len = BIO_read(mem, buf, sizeof(buf));
      assert(len > 0);
      BIO_free(mem);
      return std::string(buf, len);
    }

    /**
     * @brief   Print the info in this CSR to std::cout
     * @param   prefix A string to prefix each printed row with
     */
    void print(const std::string& prefix="") const
    {
      //int ext_idx = X509_REQ_get_ext_by_NID(m_req, NID_subject_alt_name, -1);
      auto sanstr = extensions().subjectAltName().toString();
      //const auto csr_digest = byteVecToString(digest());
      //const auto subj_digest = byteVecToString(subjectDigest());
      std::cout
          //<< prefix << "Version          : " << (version()+1) << "\n"
          << prefix << "Subject          : " << subjectNameString() << "\n"
          //<< prefix << "Subject Digest   : " << subj_digest << "\n"
          ;
      if (!sanstr.empty())
      {
        std::cout << prefix << "Subject Alt Name : " << sanstr << "\n";
      }
      std::cout
          //<< prefix << "SHA256 Digest    : " << csr_digest << "\n"
          << std::flush;
    }

  protected:

  private:
    X509_REQ*     m_req       = nullptr;
    std::string   m_file_path;

    //std::string byteVecToString(const std::vector<unsigned char> vec) const
    //{
    //  if (vec.empty())
    //  {
    //    return std::string();
    //  }
    //  std::ostringstream oss;
    //  oss << std::hex << std::setfill('0');
    //  oss << std::setw(2) << static_cast<unsigned>(vec.front());
    //  std::for_each(
    //      std::next(vec.begin()),
    //      vec.end(),
    //      [&](unsigned char x)
    //      {
    //        oss << ":" << std::setw(2) << static_cast<unsigned>(x);
    //      });
    //  return oss.str();
    //}
};  /* class SslCertSigningReq */


} /* namespace */

#endif /* ASYNC_CERT_SIGNING_REQ_INCLUDED */

/*
 * This file has not been truncated
 */
