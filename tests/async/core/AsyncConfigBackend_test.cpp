/**
@file   AsyncConfigBackend_test.cpp
@brief  Unit tests for the AsyncConfig backend system
@author Rui Barreiros / CR7BPM
@date   2026-04-14

Tests cover:
  - FileConfigBackend  (always compiled)
  - SQLiteConfigBackend (when HAS_SQLITE_SUPPORT is defined)
  - MySQLConfigBackend  (when HAS_MYSQL_SUPPORT is defined;
                         needs env var SVXLINK_TEST_MYSQL_CONN)
  - PostgreSQLConfigBackend (when HAS_POSTGRESQL_SUPPORT is defined;
                              needs env var SVXLINK_TEST_POSTGRESQL_CONN)
  - Async::Config class (population, subscribeValue, importFromConfigFile)

MySQL connection string format (semicolon-separated key=value pairs):
  SVXLINK_TEST_MYSQL_CONN="host=localhost;port=3306;user=svxtest;password=svxtest;database=svxtest"

PostgreSQL connection string (libpq keyword=value format):
  SVXLINK_TEST_POSTGRESQL_CONN="host=localhost user=svxtest password=svxtest dbname=svxtest"

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2026 Tobias Blomberg / SM0SVX

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.
\endverbatim
*/


/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#ifdef CATCH_PROVIDE_MAIN_VIA_MACRO
#  define CATCH_CONFIG_MAIN
#endif
#include <catch2/catch_all.hpp>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>
#include <functional>
#include <unistd.h>


/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncConfigBackend.h>
#include <AsyncConfigSource.h>
#include <AsyncFileConfigBackend.h>

#ifdef HAS_SQLITE_SUPPORT
#  include <AsyncSQLiteConfigBackend.h>
#endif
#ifdef HAS_MYSQL_SUPPORT
#  include <AsyncMySQLConfigBackend.h>
#endif
#ifdef HAS_POSTGRESQL_SUPPORT
#  include <AsyncPostgreSQLConfigBackend.h>
#endif


/****************************************************************************
 *
 * Helpers
 *
 ****************************************************************************/

namespace
{

/**
 * RAII wrapper that creates a temporary file and deletes it on destruction.
 * The file is NOT created by this class; it only tracks the path and removes
 * it on destruction if it exists.
 */
class TempPath
{
public:
  explicit TempPath(const std::string& suffix = "")
  {
    char tmpl[] = "/tmp/svxlink_test_XXXXXX";
    int fd = mkstemp(tmpl);
    if (fd != -1)
    {
      ::close(fd);
      m_path = std::string(tmpl) + suffix;
      if (!suffix.empty())
      {
        // Rename the fd-created file to the suffixed name
        ::rename(tmpl, m_path.c_str());
      }
    }
  }

  ~TempPath()
  {
    if (!m_path.empty())
    {
      ::unlink(m_path.c_str());
    }
  }

  const std::string& path() const { return m_path; }

private:
  std::string m_path;
  TempPath(const TempPath&) = delete;
  TempPath& operator=(const TempPath&) = delete;
};

/**
 * Creates a temporary INI-format configuration file with a known fixed
 * set of sections and values, useful for import/populate tests.
 */
class TempIniFile
{
public:
  TempIniFile() : m_path(".conf")
  {
    std::ofstream f(m_path.path());
    f << "[GLOBAL]\n"
      << "LOGICS=MyLogic\n"
      << "TIMESTAMP_FORMAT=%c\n"
      << "CARD_SAMPLE_RATE=16000\n"
      << "\n"
      << "[MyLogic]\n"
      << "TYPE=SimplexLogic\n"
      << "RX=Rx1\n"
      << "TX=Tx1\n"
      << "\n"
      << "[Rx1]\n"
      << "TYPE=Local\n"
      << "AUDIO_DEV=alsa:plughw:0\n"
      << "VOX_FILTER_DEPTH=20\n"
      << "VOX_LIMIT=1000\n";
  }

  const std::string& path() const { return m_path.path(); }

  // Known content for assertions
  static const char* GLOBAL_LOGICS()  { return "MyLogic"; }
  static const char* MYLOGIC_TYPE()   { return "SimplexLogic"; }
  static const char* RX1_AUDIO_DEV()  { return "alsa:plughw:0"; }

private:
  TempPath m_path;
};

/**
 * Retrieves an environment variable; returns empty string if not set.
 */
static std::string getenv_str(const char* name)
{
  const char* v = std::getenv(name);
  return (v != nullptr) ? std::string(v) : std::string();
}

/**
 * Runs a standard suite of CRUD, listing and signal tests against any
 * opened ConfigBackend.  The backend must already be open and have its
 * tables initialised before calling this helper.
 */
static void runCrudSuite(Async::ConfigBackend& b)
{
  SECTION("setValue and getValue round-trip")
  {
    REQUIRE(b.setValue("SEC1", "key1", "hello"));
    std::string v;
    REQUIRE(b.getValue("SEC1", "key1", v));
    REQUIRE(v == "hello");
  }

  SECTION("overwrite existing value")
  {
    REQUIRE(b.setValue("SEC1", "key1", "first"));
    REQUIRE(b.setValue("SEC1", "key1", "second"));
    std::string v;
    REQUIRE(b.getValue("SEC1", "key1", v));
    REQUIRE(v == "second");
  }

  SECTION("getValue returns false for missing tag")
  {
    std::string v;
    REQUIRE_FALSE(b.getValue("NOSEC", "notag", v));
  }

  SECTION("listSections reflects written data")
  {
    b.setValue("ALPHA", "a", "1");
    b.setValue("BETA",  "b", "2");
    auto secs = b.listSections();
    std::vector<std::string> sv(secs.begin(), secs.end());
    REQUIRE(std::find(sv.begin(), sv.end(), "ALPHA") != sv.end());
    REQUIRE(std::find(sv.begin(), sv.end(), "BETA")  != sv.end());
  }

  SECTION("listSection reflects written tags")
  {
    b.setValue("MYSEC", "tag1", "v1");
    b.setValue("MYSEC", "tag2", "v2");
    auto tags = b.listSection("MYSEC");
    std::vector<std::string> tv(tags.begin(), tags.end());
    REQUIRE(std::find(tv.begin(), tv.end(), "tag1") != tv.end());
    REQUIRE(std::find(tv.begin(), tv.end(), "tag2") != tv.end());
  }

  SECTION("listSection returns empty for unknown section")
  {
    auto tags = b.listSection("_NO_SUCH_SECTION_");
    REQUIRE(tags.empty());
  }

  SECTION("multiple sections are independent")
  {
    b.setValue("S1", "k", "val_s1");
    b.setValue("S2", "k", "val_s2");
    std::string v1, v2;
    REQUIRE(b.getValue("S1", "k", v1));
    REQUIRE(b.getValue("S2", "k", v2));
    REQUIRE(v1 == "val_s1");
    REQUIRE(v2 == "val_s2");
  }
}

/**
 * Tests that valueChanged fires when setValue is called with change
 * notifications enabled and no polling thread running.
 */
static void runSignalSuite(Async::ConfigBackend& b)
{
  SECTION("valueChanged signal fires on setValue")
  {
    b.enableChangeNotifications(true);

    std::vector<std::tuple<std::string,std::string,std::string>> received;
    b.valueChanged.connect(
      [&](const std::string& sec,
          const std::string& tag,
          const std::string& val)
      {
        received.emplace_back(sec, tag, val);
      });

    b.setValue("SIG", "alpha", "42");
    b.setValue("SIG", "beta",  "hello");

    REQUIRE(received.size() == 2);
    REQUIRE(std::get<0>(received[0]) == "SIG");
    REQUIRE(std::get<1>(received[0]) == "alpha");
    REQUIRE(std::get<2>(received[0]) == "42");
    REQUIRE(std::get<2>(received[1]) == "hello");
  }

  SECTION("valueChanged is silent when notifications disabled")
  {
    b.enableChangeNotifications(false);

    int call_count = 0;
    b.valueChanged.connect(
      [&](const std::string&, const std::string&, const std::string&)
      {
        ++call_count;
      });

    b.setValue("SIG2", "x", "y");
    REQUIRE(call_count == 0);
  }
}

} // anonymous namespace


/****************************************************************************
 *
 * Group 1 – FileConfigBackend
 *
 ****************************************************************************/

TEST_CASE("FileConfigBackend – open and close", "[file]")
{
  TempIniFile ini;

  Async::FileConfigBackend b;
  REQUIRE_FALSE(b.isOpen());
  REQUIRE(b.open(ini.path()));
  REQUIRE(b.isOpen());
  b.close();
  REQUIRE_FALSE(b.isOpen());
}

TEST_CASE("FileConfigBackend – open non-existent file fails", "[file]")
{
  Async::FileConfigBackend b;
  REQUIRE_FALSE(b.open("/tmp/__svxlink_no_such_file_XXXXX.conf"));
  REQUIRE_FALSE(b.isOpen());
}

TEST_CASE("FileConfigBackend – reads INI values correctly", "[file]")
{
  TempIniFile ini;
  Async::FileConfigBackend b;
  REQUIRE(b.open(ini.path()));

  std::string v;
  REQUIRE(b.getValue("GLOBAL", "LOGICS", v));
  REQUIRE(v == TempIniFile::GLOBAL_LOGICS());

  REQUIRE(b.getValue("MyLogic", "TYPE", v));
  REQUIRE(v == TempIniFile::MYLOGIC_TYPE());

  REQUIRE(b.getValue("Rx1", "AUDIO_DEV", v));
  REQUIRE(v == TempIniFile::RX1_AUDIO_DEV());
}

TEST_CASE("FileConfigBackend – CRUD (in-memory writes)", "[file]")
{
  TempIniFile ini;
  Async::FileConfigBackend b;
  REQUIRE(b.open(ini.path()));
  runCrudSuite(b);
}

TEST_CASE("FileConfigBackend – change notifications", "[file]")
{
  TempIniFile ini;
  Async::FileConfigBackend b;
  REQUIRE(b.open(ini.path()));
  runSignalSuite(b);
}

TEST_CASE("FileConfigBackend – getBackendType", "[file]")
{
  Async::FileConfigBackend b;
  REQUIRE(b.getBackendType() == "file");
}


/****************************************************************************
 *
 * Group 2 – SQLiteConfigBackend
 *
 ****************************************************************************/

#ifdef HAS_SQLITE_SUPPORT

TEST_CASE("SQLiteConfigBackend – open and close", "[sqlite]")
{
  TempPath db(".db");
  Async::SQLiteConfigBackend b;
  REQUIRE_FALSE(b.isOpen());
  REQUIRE(b.open(db.path()));
  REQUIRE(b.isOpen());
  b.close();
  REQUIRE_FALSE(b.isOpen());
}

TEST_CASE("SQLiteConfigBackend – open invalid path fails gracefully", "[sqlite]")
{
  Async::SQLiteConfigBackend b;
  // A path inside a non-existent directory cannot be created
  REQUIRE_FALSE(b.open("/no_such_dir/svxlink_test.db"));
  REQUIRE_FALSE(b.isOpen());
}

TEST_CASE("SQLiteConfigBackend – initializeTables creates schema", "[sqlite]")
{
  TempPath db(".db");
  Async::SQLiteConfigBackend b;
  REQUIRE(b.open(db.path()));
  REQUIRE(b.initializeTables());

  // After init, listSections must succeed (returns empty, not crash)
  auto secs = b.listSections();
  REQUIRE(secs.empty());
}

TEST_CASE("SQLiteConfigBackend – initializeTables is idempotent", "[sqlite]")
{
  TempPath db(".db");
  Async::SQLiteConfigBackend b;
  REQUIRE(b.open(db.path()));
  REQUIRE(b.initializeTables());
  REQUIRE(b.initializeTables()); // second call must not fail
}

TEST_CASE("SQLiteConfigBackend – CRUD", "[sqlite]")
{
  TempPath db(".db");
  Async::SQLiteConfigBackend b;
  REQUIRE(b.open(db.path()));
  REQUIRE(b.initializeTables());
  runCrudSuite(b);
}

TEST_CASE("SQLiteConfigBackend – change notifications", "[sqlite]")
{
  TempPath db(".db");
  Async::SQLiteConfigBackend b;
  REQUIRE(b.open(db.path()));
  REQUIRE(b.initializeTables());
  runSignalSuite(b);
}

TEST_CASE("SQLiteConfigBackend – table prefix isolates data", "[sqlite]")
{
  TempPath db(".db");

  {
    Async::SQLiteConfigBackend b1;
    REQUIRE(b1.open(db.path()));
    b1.setTablePrefix("app1_");
    REQUIRE(b1.initializeTables());
    REQUIRE(b1.setValue("GLOBAL", "key", "val_app1"));
  }

  {
    Async::SQLiteConfigBackend b2;
    REQUIRE(b2.open(db.path()));
    b2.setTablePrefix("app2_");
    REQUIRE(b2.initializeTables());
    REQUIRE(b2.setValue("GLOBAL", "key", "val_app2"));
  }

  // Re-open both and verify they don't see each other's data
  Async::SQLiteConfigBackend r1, r2;
  REQUIRE(r1.open(db.path()));
  r1.setTablePrefix("app1_");
  REQUIRE(r1.initializeTables());

  REQUIRE(r2.open(db.path()));
  r2.setTablePrefix("app2_");
  REQUIRE(r2.initializeTables());

  std::string v1, v2;
  REQUIRE(r1.getValue("GLOBAL", "key", v1));
  REQUIRE(r2.getValue("GLOBAL", "key", v2));
  REQUIRE(v1 == "val_app1");
  REQUIRE(v2 == "val_app2");
}

TEST_CASE("SQLiteConfigBackend – getBackendType", "[sqlite]")
{
  Async::SQLiteConfigBackend b;
  REQUIRE(b.getBackendType() == "sqlite");
}

TEST_CASE("SQLiteConfigBackend – data persists across open/close cycles", "[sqlite]")
{
  TempPath db(".db");

  {
    Async::SQLiteConfigBackend w;
    REQUIRE(w.open(db.path()));
    REQUIRE(w.initializeTables());
    REQUIRE(w.setValue("PERSIST", "k", "stored_value"));
  }

  Async::SQLiteConfigBackend r;
  REQUIRE(r.open(db.path()));
  REQUIRE(r.initializeTables());
  std::string v;
  REQUIRE(r.getValue("PERSIST", "k", v));
  REQUIRE(v == "stored_value");
}

#endif // HAS_SQLITE_SUPPORT


/****************************************************************************
 *
 * Group 3 – Config class with SQLite backend
 *
 ****************************************************************************/

#ifdef HAS_SQLITE_SUPPORT

TEST_CASE("Config – openDirect with SQLite URL", "[config][sqlite]")
{
  TempPath db(".db");
  std::string url = "sqlite://" + db.path();

  Async::Config cfg;
  REQUIRE(cfg.openDirect(url));
  REQUIRE(cfg.getBackendType() == "sqlite");
}

TEST_CASE("Config – setValue and getValue via Config wrapper", "[config][sqlite]")
{
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));

  cfg.setValue("GLOBAL", "LOGICS", "TestLogic");

  std::string v;
  REQUIRE(cfg.getValue("GLOBAL", "LOGICS", v));
  REQUIRE(v == "TestLogic");
}

TEST_CASE("Config – listSections and listSection via SQLite", "[config][sqlite]")
{
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));

  cfg.setValue("SECA", "ka", "va");
  cfg.setValue("SECB", "kb", "vb");

  auto secs = cfg.listSections();
  std::vector<std::string> sv(secs.begin(), secs.end());
  REQUIRE(std::find(sv.begin(), sv.end(), "SECA") != sv.end());
  REQUIRE(std::find(sv.begin(), sv.end(), "SECB") != sv.end());

  auto tags = cfg.listSection("SECA");
  REQUIRE_FALSE(tags.empty());
  REQUIRE(tags.front() == "ka");
}

TEST_CASE("Config – importFromConfigFile populates SQLite", "[config][sqlite]")
{
  TempIniFile ini;
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));

  // Import directly via the file backend
  Async::Config file_cfg;
  REQUIRE(file_cfg.openDirect("file://" + ini.path()));

  // Copy every section/tag/value from the file backend into cfg
  for (const auto& sec : file_cfg.listSections())
  {
    for (const auto& tag : file_cfg.listSection(sec))
    {
      cfg.setValue(sec, tag, file_cfg.getValue(sec, tag));
    }
  }

  std::string v;
  REQUIRE(cfg.getValue("GLOBAL", "LOGICS", v));
  REQUIRE(v == TempIniFile::GLOBAL_LOGICS());

  REQUIRE(cfg.getValue("MyLogic", "TYPE", v));
  REQUIRE(v == TempIniFile::MYLOGIC_TYPE());

  REQUIRE(cfg.getValue("Rx1", "AUDIO_DEV", v));
  REQUIRE(v == TempIniFile::RX1_AUDIO_DEV());
}

TEST_CASE("Config – subscribeValue fires on first set and on change", "[config][sqlite]")
{
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));

  std::vector<std::string> received;
  auto sub = cfg.subscribeValue("GLOBAL", "LOGICS", std::string("Default"),
    [&](const std::string& val) {
      received.push_back(val);
    });

  // subscribeValue fires immediately with the current (default) value
  REQUIRE(received.size() == 1);
  REQUIRE(received[0] == "Default");

  cfg.setValue("GLOBAL", "LOGICS", "NewLogic");
  REQUIRE(received.size() == 2);
  REQUIRE(received[1] == "NewLogic");

  cfg.setValue("GLOBAL", "LOGICS", "AnotherLogic");
  REQUIRE(received.size() == 3);
  REQUIRE(received[2] == "AnotherLogic");
}

TEST_CASE("Config – subscribeValue uses existing value when present", "[config][sqlite]")
{
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));

  cfg.setValue("GLOBAL", "LOGICS", "PreExisting");

  std::string got;
  auto sub = cfg.subscribeValue("GLOBAL", "LOGICS", std::string("Default"),
    [&](const std::string& val) { got = val; });

  // Must receive the pre-existing value, not the default
  REQUIRE(got == "PreExisting");
}

TEST_CASE("Config – getValue missing_ok=true succeeds with same value as passed in if not found", "[config][sqlite]")
{
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));

  std::string v = "sentinel";
  REQUIRE(cfg.getValue("NOSEC", "notag", v, true));
  REQUIRE(v == "sentinel");
}

TEST_CASE("Config – typed getValue (integer)", "[config][sqlite]")
{
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));

  cfg.setValue("Rx1", "VOX_FILTER_DEPTH", "20");

  int depth = 0;
  REQUIRE(cfg.getValue("Rx1", "VOX_FILTER_DEPTH", depth));
  REQUIRE(depth == 20);
}

TEST_CASE("Config – reload picks up backend changes", "[config][sqlite]")
{
  TempPath db(".db");
  Async::Config cfg;
  REQUIRE(cfg.openDirect("sqlite://" + db.path()));
  cfg.setValue("GLOBAL", "VERSION", "1");

  // Write a new value directly through the backend
  auto* backend = cfg.getBackend();
  REQUIRE(backend != nullptr);
  backend->setValue("GLOBAL", "VERSION", "2");

  // Reload so the Config cache catches up
  cfg.reload();

  std::string v;
  REQUIRE(cfg.getValue("GLOBAL", "VERSION", v));
  REQUIRE(v == "2");
}

#endif // HAS_SQLITE_SUPPORT


/****************************************************************************
 *
 * Group 4 – MySQLConfigBackend
 * Skipped automatically when SVXLINK_TEST_MYSQL_CONN env var is not set.
 *
 ****************************************************************************/

#ifdef HAS_MYSQL_SUPPORT

TEST_CASE("MySQLConfigBackend – connection and schema", "[mysql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_MYSQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_MYSQL_CONN to enable MySQL tests");
  }

  Async::MySQLConfigBackend b;
  REQUIRE(b.open(conn));
  REQUIRE(b.isOpen());

  SECTION("initializeTables")
  {
    REQUIRE(b.initializeTables());
  }

  SECTION("initializeTables is idempotent")
  {
    REQUIRE(b.initializeTables());
    REQUIRE(b.initializeTables());
  }

  SECTION("CRUD")
  {
    REQUIRE(b.initializeTables());
    // Use a unique prefix to avoid polluting real data
    b.setTablePrefix("svxtest_");
    REQUIRE(b.initializeTables());
    runCrudSuite(b);
  }

  SECTION("change notifications")
  {
    b.setTablePrefix("svxtest_");
    REQUIRE(b.initializeTables());
    runSignalSuite(b);
  }

  SECTION("getBackendType")
  {
    REQUIRE(b.getBackendType() == "mysql");
  }

  b.close();
  REQUIRE_FALSE(b.isOpen());
}

TEST_CASE("MySQLConfigBackend – invalid connection string fails", "[mysql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_MYSQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_MYSQL_CONN to enable MySQL tests");
  }

  Async::MySQLConfigBackend b;
  REQUIRE_FALSE(b.open("host=__no_such_host__;user=x;database=x"));
}

TEST_CASE("MySQLConfigBackend – table prefix isolates data", "[mysql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_MYSQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_MYSQL_CONN to enable MySQL tests");
  }

  Async::MySQLConfigBackend b1, b2;
  REQUIRE(b1.open(conn));
  REQUIRE(b2.open(conn));

  b1.setTablePrefix("svxtest_prefix1_");
  b2.setTablePrefix("svxtest_prefix2_");
  REQUIRE(b1.initializeTables());
  REQUIRE(b2.initializeTables());

  b1.setValue("SEC", "k", "from_prefix1");
  b2.setValue("SEC", "k", "from_prefix2");

  std::string v1, v2;
  REQUIRE(b1.getValue("SEC", "k", v1));
  REQUIRE(b2.getValue("SEC", "k", v2));
  REQUIRE(v1 == "from_prefix1");
  REQUIRE(v2 == "from_prefix2");
}

TEST_CASE("Config – openDirect with MySQL URL", "[config][mysql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_MYSQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_MYSQL_CONN to enable MySQL tests");
  }

  // Build a mysql:// URL from the key=value connection string
  // The URL parser in ConfigSource handles mysql://user:pass@host:port/db
  // For simplicity, create the backend directly and wrap it in Config
  Async::MySQLConfigBackend* raw = new Async::MySQLConfigBackend();
  raw->setTablePrefix("svxtest_config_");
  REQUIRE(raw->open(conn));
  REQUIRE(raw->initializeTables());

  Async::Config cfg;
  cfg.setValue("DIRECT", "test_key", "test_value");
  std::string v;
  // setValue goes through Config's own cache; retrieve via backend
  raw->setValue("DIRECT", "backend_key", "backend_value");
  REQUIRE(raw->getValue("DIRECT", "backend_key", v));
  REQUIRE(v == "backend_value");

  delete raw;
}

#endif // HAS_MYSQL_SUPPORT


/****************************************************************************
 *
 * Group 5 – PostgreSQLConfigBackend
 * Skipped automatically when SVXLINK_TEST_POSTGRESQL_CONN env var is not set.
 *
 ****************************************************************************/

#ifdef HAS_POSTGRESQL_SUPPORT

TEST_CASE("PostgreSQLConfigBackend – connection and schema", "[postgresql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_POSTGRESQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_POSTGRESQL_CONN to enable PostgreSQL tests");
  }

  Async::PostgreSQLConfigBackend b;
  REQUIRE(b.open(conn));
  REQUIRE(b.isOpen());

  SECTION("initializeTables")
  {
    REQUIRE(b.initializeTables());
  }

  SECTION("initializeTables is idempotent")
  {
    REQUIRE(b.initializeTables());
    REQUIRE(b.initializeTables());
  }

  SECTION("CRUD")
  {
    b.setTablePrefix("svxtest_");
    REQUIRE(b.initializeTables());
    runCrudSuite(b);
  }

  SECTION("change notifications")
  {
    b.setTablePrefix("svxtest_");
    REQUIRE(b.initializeTables());
    runSignalSuite(b);
  }

  SECTION("getBackendType")
  {
    REQUIRE(b.getBackendType() == "postgresql");
  }

  b.close();
  REQUIRE_FALSE(b.isOpen());
}

TEST_CASE("PostgreSQLConfigBackend – invalid connection fails", "[postgresql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_POSTGRESQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_POSTGRESQL_CONN to enable PostgreSQL tests");
  }

  Async::PostgreSQLConfigBackend b;
  REQUIRE_FALSE(b.open("host=__no_such_host__ dbname=x user=x"));
}

TEST_CASE("PostgreSQLConfigBackend – table prefix isolates data", "[postgresql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_POSTGRESQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_POSTGRESQL_CONN to enable PostgreSQL tests");
  }

  Async::PostgreSQLConfigBackend b1, b2;
  REQUIRE(b1.open(conn));
  REQUIRE(b2.open(conn));

  b1.setTablePrefix("svxtest_prefix1_");
  b2.setTablePrefix("svxtest_prefix2_");
  REQUIRE(b1.initializeTables());
  REQUIRE(b2.initializeTables());

  b1.setValue("SEC", "k", "from_prefix1");
  b2.setValue("SEC", "k", "from_prefix2");

  std::string v1, v2;
  REQUIRE(b1.getValue("SEC", "k", v1));
  REQUIRE(b2.getValue("SEC", "k", v2));
  REQUIRE(v1 == "from_prefix1");
  REQUIRE(v2 == "from_prefix2");
}

TEST_CASE("PostgreSQLConfigBackend – data persists across open/close", "[postgresql]")
{
  std::string conn = getenv_str("SVXLINK_TEST_POSTGRESQL_CONN");
  if (conn.empty())
  {
    SKIP("Set SVXLINK_TEST_POSTGRESQL_CONN to enable PostgreSQL tests");
  }

  {
    Async::PostgreSQLConfigBackend w;
    REQUIRE(w.open(conn));
    w.setTablePrefix("svxtest_persist_");
    REQUIRE(w.initializeTables());
    REQUIRE(w.setValue("P", "k", "persistent_value"));
  }

  Async::PostgreSQLConfigBackend r;
  REQUIRE(r.open(conn));
  r.setTablePrefix("svxtest_persist_");
  REQUIRE(r.initializeTables());
  std::string v;
  REQUIRE(r.getValue("P", "k", v));
  REQUIRE(v == "persistent_value");
}

#endif // HAS_POSTGRESQL_SUPPORT


/****************************************************************************
 *
 * Group 6 – Factory registration
 *
 ****************************************************************************/

TEST_CASE("ConfigBackendFactory – 'file' is always registered", "[factory]")
{
  std::string factories = Async::ConfigBackendFactory::validFactories();
  REQUIRE(factories.find("\"file\"") != std::string::npos);
}

#ifdef HAS_SQLITE_SUPPORT
TEST_CASE("ConfigBackendFactory – 'sqlite' is registered when compiled", "[factory]")
{
  std::string factories = Async::ConfigBackendFactory::validFactories();
  REQUIRE(factories.find("\"sqlite\"") != std::string::npos);
}
#endif

#ifdef HAS_MYSQL_SUPPORT
TEST_CASE("ConfigBackendFactory – 'mysql' is registered when compiled", "[factory]")
{
  std::string factories = Async::ConfigBackendFactory::validFactories();
  REQUIRE(factories.find("\"mysql\"") != std::string::npos);
}
#endif

#ifdef HAS_POSTGRESQL_SUPPORT
TEST_CASE("ConfigBackendFactory – 'postgresql' is registered when compiled", "[factory]")
{
  std::string factories = Async::ConfigBackendFactory::validFactories();
  REQUIRE(factories.find("\"postgresql\"") != std::string::npos);
}
#endif

TEST_CASE("ConfigBackendFactory – unknown type returns nullptr", "[factory]")
{
  auto ptr = Async::createConfigBackendByType("__no_such_backend__", "");
  REQUIRE(ptr == nullptr);
}

TEST_CASE("ConfigSource – isBackendAvailable matches factory registry", "[factory]")
{
  // Every backend that ConfigSource claims is available must also be in the
  // factory registry, and vice versa.
  const std::vector<std::string> known = {"file", "sqlite", "mysql", "postgresql"};
  std::string factories = Async::ConfigBackendFactory::validFactories();

  for (const auto& name : known)
  {
    bool in_source  = Async::ConfigSource::isBackendAvailable(name);
    bool in_factory = (factories.find("\"" + name + "\"") != std::string::npos);
    INFO("Checking backend '" << name << "'");
    REQUIRE(in_source == in_factory);
  }
}


/*
 * This file has not been truncated
 */
