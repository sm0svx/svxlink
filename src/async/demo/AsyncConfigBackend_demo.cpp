/**
@file	 AsyncConfigBackend_demo.cpp
@brief   Demo program showing the new configuration backend system
@author  Rui Barreiros / CR7BPM
@date	 2025-09-19

This program demonstrates how to use the new configuration backend abstraction
layer to load configuration from different sources.

\verbatim
Async - A library for programming event driven applications
Copyright (C) 2004-2026 Tobias Blomberg / SM0SVX

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

/****************************************************************************
 *
 * System Includes
 *
 ****************************************************************************/

#include <iostream>
#include <string>
#include <list>
#include <fstream>

/****************************************************************************
 *
 * Project Includes
 *
 ****************************************************************************/

#include <AsyncConfig.h>
#include <AsyncConfigBackend.h>
#include <AsyncConfigSource.h>

/****************************************************************************
 *
 * Local Includes
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Namespaces to use
 *
 ****************************************************************************/

using namespace std;
using namespace Async;

/****************************************************************************
 *
 * Defines & typedefs
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Local class definitions
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Prototypes
 *
 ****************************************************************************/

void showAvailableBackends();
void demonstrateBackend(Config& cfg);

/****************************************************************************
 *
 * Exported Global Variables
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Local Global Variables
 *
 ****************************************************************************/

/****************************************************************************
 *
 * Main program
 *
 ****************************************************************************/

int main(int argc, char **argv)
{
  cout << "SVXLink Configuration Backend Demo" << endl;
  cout << "==================================" << endl << endl;

  showAvailableBackends();

  Config cfg;

  if (argc >= 2)
  {
    // User supplied a db.conf path.  We only read it — the directory must
    // already exist and be accessible.  Exit immediately if the file is
    // missing rather than trying to create it.
    const string db_conf_path = argv[1];
    {
      ifstream check(db_conf_path);
      if (!check.good())
      {
        cerr << "*** ERROR: db.conf not found or not readable: "
             << db_conf_path << endl;
        return 1;
      }
    }

    cout << "Using db.conf: " << db_conf_path << endl;
    if (!cfg.open(db_conf_path))
    {
      cerr << "*** ERROR: Failed to open configuration from: "
           << db_conf_path << endl;
      return 1;
    }
  }
  else
  {
    // No db.conf provided — fall back to a temporary SQLite database so the
    // demo can run without any prior setup.
    const string fallback_url = "sqlite:///tmp/AsyncConfigBackend_demo.db";
    cout << "No db.conf specified.  Using fallback SQLite database:" << endl;
    cout << "  " << fallback_url << endl << endl;
    if (!cfg.openDirect(fallback_url))
    {
      cerr << "*** ERROR: Failed to open fallback SQLite database" << endl;
      return 1;
    }
  }

  demonstrateBackend(cfg);
  return 0;
} /* main */

/****************************************************************************
 *
 * Functions
 *
 ****************************************************************************/

void showAvailableBackends()
{
  cout << "Available configuration backends: "
       << ConfigBackendFactory::validFactories() << endl << endl;

  cout << "Backend availability:" << endl;
  cout << "  File:       "
       << (ConfigSource::isBackendAvailable("file")       ? "Yes" : "No")
       << endl;
  cout << "  SQLite:     "
       << (ConfigSource::isBackendAvailable("sqlite")     ? "Yes" : "No")
       << endl;
  cout << "  MySQL:      "
       << (ConfigSource::isBackendAvailable("mysql")      ? "Yes" : "No")
       << endl;
  cout << "  PostgreSQL: "
       << (ConfigSource::isBackendAvailable("postgresql") ? "Yes" : "No")
       << endl << endl;
} /* showAvailableBackends */


void demonstrateBackend(Config& cfg)
{
  cout << "Backend type: " << cfg.getBackendType() << endl << endl;

  // Populate a few values so the demo is interesting even with a blank DB
  cfg.setValue("GLOBAL",   "LOGICS",           "MyLogic");
  cfg.setValue("MyLogic",  "TYPE",              "SimplexLogic");
  cfg.setValue("MyLogic",  "RX",               "Rx1");
  cfg.setValue("MyLogic",  "TX",               "Tx1");
  cfg.setValue("Rx1",      "TYPE",             "Local");
  cfg.setValue("Rx1",      "AUDIO_DEV",        "alsa:plughw:0");
  cfg.setValue("Rx1",      "VOX_FILTER_DEPTH", "20");
  cfg.setValue("Rx1",      "VOX_LIMIT",        "1000");

  // List all sections and their tags
  cout << "Configuration sections:" << endl;
  list<string> sections = cfg.listSections();
  for (const string& section : sections)
  {
    cout << "  [" << section << "]" << endl;
    list<string> tags = cfg.listSection(section);
    for (const string& tag : tags)
    {
      cout << "    " << tag << " = " << cfg.getValue(section, tag) << endl;
    }
    cout << endl;
  }

  // Typed getValue
  cout << "Typed getValue demonstrations:" << endl;

  string logics;
  if (cfg.getValue("GLOBAL", "LOGICS", logics))
  {
    cout << "  GLOBAL/LOGICS = \"" << logics << "\"" << endl;
  }

  int vox_depth = 0;
  if (cfg.getValue("Rx1", "VOX_FILTER_DEPTH", vox_depth))
  {
    cout << "  Rx1/VOX_FILTER_DEPTH = " << vox_depth << endl;
  }

  int vox_limit = 0;
  if (cfg.getValue("Rx1", "VOX_LIMIT", -30, 0, vox_limit))
  {
    cout << "  Rx1/VOX_LIMIT = " << vox_limit << " (range checked)" << endl;
  }

  string missing_value;
  if (cfg.getValue("GLOBAL", "MISSING_VALUE", missing_value, true))
  {
    cout << "  GLOBAL/MISSING_VALUE = \""
         << (missing_value.empty() ? "<not set>" : missing_value)
         << "\" (missing_ok=true)" << endl;
  }

  cout << endl;

  // Value subscription
  cout << "Demonstrating value subscription..." << endl;

  auto sub = cfg.subscribeValue("GLOBAL", "LOGICS", string("DefaultLogic"),
                                [](const string& val) {
                                  cout << "  Subscription callback: GLOBAL/LOGICS = \""
                                       << val << "\"" << endl;
                                });

  cfg.setValue("GLOBAL", "LOGICS", "ChangedLogic");

  cout << endl << "Demo completed successfully!" << endl;
} /* demonstrateBackend */

/*
 * This file has not been truncated
 */
