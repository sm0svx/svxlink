# SVXLink Database Configuration Backend

This document describes the pluggable configuration backend system introduced in
SVXLink.  It allows configuration to be stored in a relational database (SQLite,
MySQL/MariaDB, or PostgreSQL) instead of — or alongside — the traditional `.conf`
files.

---

## Table of Contents

1. [Overview](#1-overview)
2. [Configuration Loading Priority](#2-configuration-loading-priority)
3. [The `db.conf` File](#3-the-dbconf-file)
4. [Database Table Schema](#4-database-table-schema)
5. [Backend Setup](#5-backend-setup)
  - [SQLite](#51-sqlite)
  - [MySQL / MariaDB](#52-mysql--mariadb)
  - [PostgreSQL](#53-postgresql)
6. [Initialising from an Existing Config File (`--init-db`)](#6-initialising-from-an-existing-config-file----init-db)
7. [Command-Line Options](#7-command-line-options)
8. [Calibration Utilities](#8-calibration-utilities)
  - [devcal](#81-devcal)
  - [siglevdetcal](#82-siglevdetcal)
9. [Live Change Notifications](#9-live-change-notifications)
10. [Table Naming and Prefix Rules](#10-table-naming-and-prefix-rules)
11. [Sharing One Database Between Binaries](#11-sharing-one-database-between-binaries)
12. [Quick-Start Examples](#12-quick-start-examples)

---

## 1. Overview

SVXLink applications (`svxlink`, `remotetrx`, `svxreflector`, `svxserver`) all
use a single `AsyncConfig` class to read their configuration.  That class
delegates every I/O operation to a **backend plugin**.  Four backends ship with
SVXLink:


| Backend type      | `TYPE=` value | Notes                                                   |
| ----------------- | ------------- | ------------------------------------------------------- |
| Plain config file | `file`        | Original behaviour; reads `.conf` files                 |
| SQLite            | `sqlite`      | Single-file embedded database                           |
| MySQL / MariaDB   | `mysql`       | Requires the `libmysqlclient` dev package at build time |
| PostgreSQL        | `postgresql`  | Requires the `libpq-dev` package at build time          |


Whether a database backend was compiled in can be checked at runtime:

```
svxlink --help
```

The available backends are listed in the error message if an unsupported type is
requested.

---

## 2. Configuration Loading Priority

Each binary follows a strict lookup chain at startup.  The first match wins:

```
1. --config <file>       → use the file backend with the given path
       ↓ (not given)
2. --dbconfig <file>     → parse <file> as db.conf, use the configured backend
       ↓ (not given)
3. Search standard paths for db.conf
       ~/.svxlink/db.conf
       /etc/svxlink/db.conf
       <install-prefix>/etc/svxlink/db.conf
   if found → use the configured backend
       ↓ (not found)
4. Search standard paths for the application's default config file
       (~/.svxlink/svxlink.conf, /etc/svxlink/svxlink.conf, …)
   if found → use the file backend
       ↓ (not found)
5. Fatal error — no configuration source available
```

`**CFG_DIR` processing** is only performed for the `file` backend.  Database
backends load all their data directly from the database and ignore `CFG_DIR`.

---

## 3. The `db.conf` File

`db.conf` is a lightweight INI-style file that tells SVXLink *which* backend to
use and how to connect to it.  An example is installed at
`/etc/svxlink/db.conf` (or `~/.svxlink/db.conf` for per-user configuration).

### Format

```ini
[DATABASE]
# Required: backend type
TYPE=sqlite

# Required: connection source (format depends on TYPE — see below)
SOURCE=/var/lib/svxlink/svxlink.db

# Optional: prefix prepended to the binary name when naming tables
# Default: no extra prefix (binary name is used as-is, e.g. svxlink_config)
#TABLE_PREFIX=prod_

# Optional: enable live change notifications
# When 1/true/yes, the application reacts to rows changed in the database
# by another process without needing a restart or SIGHUP.
ENABLE_CHANGE_NOTIFICATIONS=1

# Optional: how often to poll the database for external changes (seconds)
# Only meaningful when ENABLE_CHANGE_NOTIFICATIONS is enabled.
# Default: 0 (polling disabled)
POLL_INTERVAL=30
```

### `SOURCE` format by backend type


| `TYPE`       | `SOURCE` example                                                                |
| ------------ | ------------------------------------------------------------------------------- |
| `file`       | `SOURCE=/etc/svxlink/svxlink.conf`                                              |
| `sqlite`     | `SOURCE=/var/lib/svxlink/svxlink.db`                                            |
| `mysql`      | `SOURCE=host=localhost;port=3306;user=svxlink;password=secret;database=svxlink` |
| `postgresql` | `SOURCE=host=localhost port=5432 dbname=svxlink user=svxlink password=secret`   |


> **MySQL note:** the `SOURCE` value uses semicolon-separated `key=value` pairs.
>
> **PostgreSQL note:** the `SOURCE` value is passed directly to `PQconnectdb()` as
> a libpq connection string (space-separated `key=value` pairs).

---

## 4. Database Table Schema

Every backend stores configuration in one table per application binary.  The
table name is `<prefix>config` where `<prefix>` is derived from the binary name
(see [Section 9](#9-table-naming-and-prefix-rules)).

### Schema (common to all database backends)


| Column       | Type                   | Description                                    |
| ------------ | ---------------------- | ---------------------------------------------- |
| `id`         | auto-increment integer | Internal row identifier                        |
| `section`    | VARCHAR(255) / TEXT    | INI section name, e.g. `GLOBAL`                |
| `tag`        | VARCHAR(255) / TEXT    | Key name within the section, e.g. `CALLSIGN`   |
| `value`      | TEXT                   | Value string                                   |
| `created_at` | TIMESTAMP              | Row creation time                              |
| `updated_at` | TIMESTAMP              | Last modification time (auto-updated on MySQL) |


A `UNIQUE(section, tag)` constraint is enforced on all backends so that upsert (update/insert)
operations are safe.

### SQLite DDL

```sql
CREATE TABLE IF NOT EXISTS svxlink_config (
    id          INTEGER PRIMARY KEY AUTOINCREMENT,
    section     TEXT NOT NULL,
    tag         TEXT NOT NULL,
    value       TEXT NOT NULL,
    created_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
    updated_at  DATETIME DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(section, tag)
);
```

### MySQL / MariaDB DDL

```sql
CREATE TABLE IF NOT EXISTS svxlink_config (
    id         INT AUTO_INCREMENT PRIMARY KEY,
    section    VARCHAR(255) NOT NULL,
    tag        VARCHAR(255) NOT NULL,
    value      TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    UNIQUE KEY unique_svxlink_config (section, tag),
    INDEX idx_svxlink_config_section (section)
) ENGINE=InnoDB DEFAULT CHARSET=utf8;
```

### PostgreSQL DDL

```sql
CREATE TABLE IF NOT EXISTS svxlink_config (
    id         SERIAL PRIMARY KEY,
    section    VARCHAR(255) NOT NULL,
    tag        VARCHAR(255) NOT NULL,
    value      TEXT NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    UNIQUE(section, tag)
);

CREATE INDEX IF NOT EXISTS idx_svxlink_config_section
    ON svxlink_config (section);
```

> **Note:** Tables are created automatically on first startup if the database
> user has `CREATE TABLE` privileges.  You only need to create them manually if
> you want a different storage configuration.

---

## 5. Backend Setup

### 5.1 SQLite

No server required — just ensure the directory for the database file is
writable by the SVXLink process.

`**/etc/svxlink/db.conf`**

```ini
[DATABASE]
TYPE=sqlite
SOURCE=/var/lib/svxlink/svxlink.db
ENABLE_CHANGE_NOTIFICATIONS=1
POLL_INTERVAL=60
```

Create the directory:

```bash
sudo mkdir -p /var/lib/svxlink
sudo chown svxlink:svxlink /var/lib/svxlink
```

The database file and table are created automatically on first run.

Inspect the database at any time:

```bash
sqlite3 /var/lib/svxlink/svxlink.db \
    "SELECT section, tag, value FROM svxlink_config ORDER BY section, tag;"
```

---

### 5.2 MySQL / MariaDB

**Create the database and user:**

```sql
CREATE DATABASE svxlink CHARACTER SET utf8 COLLATE utf8_general_ci;
CREATE USER 'svxlink'@'localhost' IDENTIFIED BY 'changeme';
GRANT ALL PRIVILEGES ON svxlink.* TO 'svxlink'@'localhost';
FLUSH PRIVILEGES;
```

`**/etc/svxlink/db.conf**`

```ini
[DATABASE]
TYPE=mysql
SOURCE=host=localhost;port=3306;user=svxlink;password=changeme;database=svxlink
ENABLE_CHANGE_NOTIFICATIONS=1
POLL_INTERVAL=30
```

The `svxlink_config` table (and the table for every other binary that shares the
same database) is created automatically on first run.

Inspect the configuration:

```bash
mysql -u svxlink -p svxlink \
    -e "SELECT section, tag, value FROM svxlink_config ORDER BY section, tag;"
```

---

### 5.3 PostgreSQL

**Create the database and user:**

```sql
CREATE USER svxlink WITH PASSWORD 'changeme';
CREATE DATABASE svxlink OWNER svxlink;
\c svxlink
GRANT ALL ON SCHEMA public TO svxlink;
```

`**/etc/svxlink/db.conf**`

```ini
[DATABASE]
TYPE=postgresql
SOURCE=host=localhost port=5432 dbname=svxlink user=svxlink password=changeme
ENABLE_CHANGE_NOTIFICATIONS=1
POLL_INTERVAL=30
```

Inspect the configuration:

```bash
psql -U svxlink -d svxlink \
    -c "SELECT section, tag, value FROM svxlink_config ORDER BY section, tag;"
```

---

## 6. Initialising from an Existing Config File (`--init-db`)

All four binaries support a `--init-db` flag.  When given, the application:

1. Opens the backend configured in `db.conf` (or specified via `--dbconfig`).
2. Checks whether the configuration table is **empty**.
3. If empty, searches the standard config paths for the application's installed
  example `.conf` file and copies all sections/keys/values into the database.
4. Exits immediately after the import — the application does **not** start
  normally.


| Binary         | Source config file  |
| -------------- | ------------------- |
| `svxlink`      | `svxlink.conf`      |
| `remotetrx`    | `remotetrx.conf`    |
| `svxreflector` | `svxreflector.conf` |
| `svxserver`    | `svxserver.conf`    |


**The database must already be reachable and empty.**  If it already contains
rows the import is skipped and a warning is printed.

### Example workflow

```bash
# 1. Install db.conf pointing at an SQLite database
sudo cp /usr/share/doc/svxlink/examples/db.conf /etc/svxlink/db.conf
# Edit TYPE and SOURCE as needed …

# 2. Import the installed example config into the (empty) database
sudo svxlink --init-db

# 3. Edit values directly in the database
sqlite3 /var/lib/svxlink/svxlink.db \
    "UPDATE svxlink_config SET value='SM0XYZ' WHERE section='GLOBAL' AND tag='CALLSIGN';"

# 4. Start SVXLink normally — it reads from the database
sudo svxlink
```

The same pattern works for `remotetrx` and `svxreflector`:

```bash
sudo remotetrx   --init-db
sudo svxreflector --init-db
```

> **Tip:** If you want to re-import, drop and recreate the table (or delete all
> rows) before running `--init-db` again.

---

## 7. Command-Line Options

These options are available on all four binaries.


| Option              | Description                                                                                     |
| ------------------- | ----------------------------------------------------------------------------------------------- |
| `--config <file>`   | Use this file directly as the configuration (file backend only). Overrides all other discovery. |
| `--dbconfig <file>` | Use this file as `db.conf` (specifies the backend). Overrides the automatic `db.conf` search.   |
| `--init-db`         | Import the installed example `.conf` file into an empty database backend, then exit.            |


Examples:

```bash
# Run with an explicit config file (file backend)
svxlink --config /home/alice/test.conf

# Run with a custom db.conf
svxlink --dbconfig /opt/svxlink/etc/db.conf

# Initialise the database that a specific db.conf points to
svxlink --dbconfig /opt/svxlink/etc/db.conf --init-db
```

---

## 8. Calibration Utilities

`devcal` and `siglevdetcal` are interactive calibration tools that open the
same SVXLink hardware sections as the main `svxlink` binary.  Both support
the full backend system — they can read configuration from a file, a database,
or auto-discover it from the standard search paths.

Because they operate on a specific hardware section (a receiver or transmitter
named in the config), **a section name is always required** as a command-line
argument.

### 8.1 `devcal`

FM deviation calibration utility.  The hardware section name is the last
positional argument; the config source is given with `--config`, `--dbconfig`,
or as a legacy positional path.

```
devcal [devcal-options] --config   <file>    <section>
devcal [devcal-options] --dbconfig <db.conf> <section>
devcal [devcal-options] <config-file-or-db.conf> <section>   (legacy)
```

Examples:

```bash
# File backend — explicit path
devcal -r --config /etc/svxlink/svxlink.conf Rx1

# SQLite database via db.conf
devcal -r --dbconfig /etc/svxlink/db.conf Rx1

# Legacy positional form (still works)
devcal -r /etc/svxlink/svxlink.conf Rx1

# TX calibration against a database
devcal -t --dbconfig /etc/svxlink/db.conf Tx1

# Measure deviation (wideband RTL-SDR receiver)
devcal -M -w --config /etc/svxlink/svxlink.conf RxRtl
```

---

### 8.2 `siglevdetcal`

Signal level detector calibration utility.  Guides the operator through a
series of PTT measurements to derive `SIGLEV_SLOPE` and `SIGLEV_OFFSET`
values for a given receiver section.

```
siglevdetcal [--config   <file>]    <section>
siglevdetcal [--dbconfig <db.conf>] <section>
siglevdetcal <config-file-or-db.conf> <section>   (legacy)
siglevdetcal <section>              (auto-discover config)
```

When no config argument is given at all, the standard search paths are tried
in order — `db.conf` first, then `svxlink.conf` — exactly as `svxlink` itself
does.

Examples:

```bash
# File backend — explicit path
siglevdetcal --config /etc/svxlink/svxlink.conf Rx1

# Database via db.conf
siglevdetcal --dbconfig /etc/svxlink/db.conf Rx1

# Auto-discover: reads /etc/svxlink/db.conf or svxlink.conf automatically
siglevdetcal Rx1

# Legacy positional form (still works)
siglevdetcal /etc/svxlink/svxlink.conf Rx1
```

After the run, paste the printed `SIGLEV_SLOPE` and `SIGLEV_OFFSET` values
into the appropriate receiver section of your config — either in the `.conf`
file or directly in the database:

```bash
# Update values in an SQLite database
sqlite3 /var/lib/svxlink/svxlink.db <<'SQL'
UPDATE svxlink_config SET value='12.50', updated_at=datetime('now')
  WHERE section='Rx1' AND tag='SIGLEV_SLOPE';
UPDATE svxlink_config SET value='-5.30', updated_at=datetime('now')
  WHERE section='Rx1' AND tag='SIGLEV_OFFSET';
SQL
```

---

## 9. Live Change Notifications

When `ENABLE_CHANGE_NOTIFICATIONS=1` and `POLL_INTERVAL=<seconds>` are set in
`db.conf`, SVXLink spawns a background thread that wakes up every
`POLL_INTERVAL` seconds and queries the database for rows that changed since the
last check (using the `updated_at` timestamp).

Changed values are delivered to the application **on the main event-loop thread**
via a self-pipe mechanism — the poll thread never calls application code
directly.

Parameters that can be hot-updated at runtime (no restart needed):

- `GLOBAL/CALLSIGN` and most text identifiers
- `GLOBAL/LOCATION_INFO` (reinitialises LocationInfo)
- `GLOBAL/RANDOM_QSY_RANGE` (svxreflector — randomises QSY TG range)
- Most RF-related thresholds (CTCSS tones, squelch levels, etc.)

Parameters that require a restart (hardware, network sockets, module loading):

- Audio device settings (`AUDIO_DEV`, `AUDIO_SAMPLE_RATE`, …)
- Network bind addresses and ports (`LISTEN_PORT`, `HOST`, …)
- Module loading lists (`MODULES=`)
- Logic type (`TYPE=`)

For a SIGHUP-triggered reload of the file backend (legacy behaviour), send
`SIGHUP` to the process:

```bash
sudo kill -HUP $(pidof svxlink)
```

---

## 10. Table Naming and Prefix Rules

Each binary automatically uses a table prefix derived from its own name to keep
configuration isolated even when multiple binaries share one database:


| Binary         | Automatic prefix | Table name            |
| -------------- | ---------------- | --------------------- |
| `svxlink`      | `svxlink_`       | `svxlink_config`      |
| `remotetrx`    | `remotetrx_`     | `remotetrx_config`    |
| `svxreflector` | `svxreflector_`  | `svxreflector_config` |
| `svxserver`    | `svxserver_`     | `svxserver_config`    |


Setting `TABLE_PREFIX` in `db.conf` **prepends** the given string to the
automatic binary prefix:

```ini
TABLE_PREFIX=prod_
```


| Binary      | Resulting prefix  | Table name              |
| ----------- | ----------------- | ----------------------- |
| `svxlink`   | `prod_svxlink_`   | `prod_svxlink_config`   |
| `remotetrx` | `prod_remotetrx_` | `prod_remotetrx_config` |


This is useful for environment separation (`prod_`, `test_`, `dev_`) or for
running multiple site configurations in one database.

> `**file` backend note:** `TABLE_PREFIX` has no effect when `TYPE=file`.

---

## 11. Sharing One Database Between Binaries

A single `db.conf` — and therefore a single database connection — can serve all
SVXLink binaries on the same host.  Because each binary automatically uses its
own table prefix, there is no conflict:

```
/etc/svxlink/db.conf
    TYPE=mysql
    SOURCE=host=localhost;port=3306;user=svxlink;password=…;database=svxlink

Tables created:
    svxlink_config       ← used by svxlink
    remotetrx_config     ← used by remotetrx
    svxreflector_config  ← used by svxreflector
```

Initialise each binary separately:

```bash
sudo svxlink      --init-db
sudo remotetrx    --init-db
sudo svxreflector --init-db
```

---

## 12. Quick-Start Examples

### SQLite — simplest path

```bash
# 1. Create /etc/svxlink/db.conf
sudo tee /etc/svxlink/db.conf <<'EOF'
[DATABASE]
TYPE=sqlite
SOURCE=/var/lib/svxlink/svxlink.db
ENABLE_CHANGE_NOTIFICATIONS=1
POLL_INTERVAL=60
EOF

# 2. Create the data directory
sudo mkdir -p /var/lib/svxlink
sudo chown svxlink:svxlink /var/lib/svxlink

# 3. Import the installed config into the database
sudo svxlink --init-db

# 4. Verify the import
sqlite3 /var/lib/svxlink/svxlink.db \
    "SELECT section, tag, value FROM svxlink_config LIMIT 20;"

# 5. Run svxlink
sudo svxlink
```

---

### MySQL — production example

```bash
# 1. Create the database and user (run as MySQL root)
mysql -u root -p <<'SQL'
CREATE DATABASE svxlink CHARACTER SET utf8 COLLATE utf8_general_ci;
CREATE USER 'svxlink'@'localhost' IDENTIFIED BY 'changeme';
GRANT ALL PRIVILEGES ON svxlink.* TO 'svxlink'@'localhost';
FLUSH PRIVILEGES;
SQL

# 2. Create /etc/svxlink/db.conf
sudo tee /etc/svxlink/db.conf <<'EOF'
[DATABASE]
TYPE=mysql
SOURCE=host=localhost;port=3306;user=svxlink;password=changeme;database=svxlink
ENABLE_CHANGE_NOTIFICATIONS=1
POLL_INTERVAL=30
EOF

# 3. Import config for all binaries
sudo svxlink      --init-db
sudo remotetrx    --init-db
sudo svxreflector --init-db

# 4. Verify
mysql -u svxlink -pchangeme svxlink \
    -e "SELECT section, tag, value FROM svxlink_config ORDER BY section, tag LIMIT 20;"

# 5. Edit a value on the fly — SVXLink picks it up within POLL_INTERVAL seconds
mysql -u svxlink -pchangeme svxlink \
    -e "UPDATE svxlink_config SET value='SM0XYZ', updated_at=NOW()
        WHERE section='GLOBAL' AND tag='CALLSIGN';"
```

---

### PostgreSQL — production example

```bash
# 1. Create the database and user (run as postgres superuser)
sudo -u postgres psql <<'SQL'
CREATE USER svxlink WITH PASSWORD 'changeme';
CREATE DATABASE svxlink OWNER svxlink;
\c svxlink
GRANT ALL ON SCHEMA public TO svxlink;
SQL

# 2. Create /etc/svxlink/db.conf
sudo tee /etc/svxlink/db.conf <<'EOF'
[DATABASE]
TYPE=postgresql
SOURCE=host=localhost port=5432 dbname=svxlink user=svxlink password=changeme
ENABLE_CHANGE_NOTIFICATIONS=1
POLL_INTERVAL=30
EOF

# 3. Import config
sudo svxlink --init-db

# 4. Edit a value on the fly
psql -U svxlink -d svxlink \
    -c "UPDATE svxlink_config SET value='SM0XYZ', updated_at=NOW()
        WHERE section='GLOBAL' AND tag='CALLSIGN';"
```

