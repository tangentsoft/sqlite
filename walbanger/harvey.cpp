/***********************************************************************
 harvey.cpp, the WAL-banger.  See usage() below for details.

 Created and © 2022.09.25 by Warren Young

 Licensed under the 2-clause BSD license in ../LICENSE.md.  Generic
 version at https://opensource.org/licenses/BSD-2-Clause
***********************************************************************/

#include <sqlite3.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <csignal>
#include <cstdlib>
#include <cstring>

using namespace std;


//// CONSTANTS /////////////////////////////////////////////////////////

// Usage pattern scaling: how many out of the sum total of these values
// each type of statement should run, on average.  Adjust to suit.
#if 1
    // Read-heavy "application" type mix
    static const auto select_proportion = 900;
    static const auto insert_proportion = 10;
    static const auto update_proportion = 10;
    static const auto delete_proportion = 1;
#else
    // Flat mix, for testing SQL generator code coverage
    static const auto select_proportion = 1;
    static const auto insert_proportion = 1;
    static const auto update_proportion = 1;
    static const auto delete_proportion = 1;
#endif

// Derivatives of above to scale them into the [0.0 - 1.0) range.
static const double types =
        select_proportion +
        insert_proportion +
        update_proportion +
        delete_proportion;
static const auto select_factor = select_proportion / types;
static const auto insert_factor = insert_proportion / types;
static const auto update_factor = update_proportion / types;
static const auto delete_factor = delete_proportion / types;

// In -p mode, how many statements to skip printing a dot for.
#define PROGDOTSKIP 1000


//// GLOBALS ///////////////////////////////////////////////////////////

static sqlite3* db;
static size_t records, deletes, inserts, updates, selects;


//// usage /////////////////////////////////////////////////////////////

static void
usage(const char* error = 0)
{
    if (error) cerr << "ERROR: " << error << "!\n";
    cerr << R"(
Usage: walpounder [-prv] dbname

    Creates or opens the named SQLite database in WAL mode, then begins
    updating it according to patterns set at compile time at the top of
    )" __FILE__ R"(.

Options:

    -p  print a progress dot every )" << PROGDOTSKIP << R"( records
    -r  reset the DB first, deleting all prior data
    -v  give verbose output on each SELECT
)" << endl;

    exit(1);
}

template <typename T>
static void
usage(const char* error, T x)
{
    ostringstream os;
    os << error << " (" << x << ")";
    usage(os.str().c_str());
}

static void
usage(const char* error, int rc)
{
    usage(error, sqlite3_errmsg(db));
}


//// cleanup ///////////////////////////////////////////////////////////
// Program termination handlers.

static void
cleanup(void)
{
    if (db) sqlite3_close(db);
    cout << "\nHit the DB " << records << " times:"
            "\n  SELECT: " << selects <<
            "\n  INSERT: " << inserts <<
            "\n  UPDATE: " << updates <<
            "\n  DELETE: " << deletes <<
            "\n";
}

static void
die(int sig)
{
    exit(100 + sig);
}


//// main //////////////////////////////////////////////////////////////

int
main(int argc, char* const argv[])
{
    // Parse command line
    char* errmsg = 0;
    bool progress = false, reset = false, verbose = false;
    int ch;
    while ((ch = getopt(argc, argv, "prv")) != EOF) {
        switch (ch) {
            case 'p': progress = true; break;
            case 'r': reset = true;    break;
            case 'v': verbose = true;  break;
            case 'h':
            case '?':
            default: usage("bad flag", char(optopt));
        }
    }
    argc -= optind;
    argv += optind;
    if (argc < 1) usage("dbname required");

    // Open/create and initialize the database
    if (auto rc = sqlite3_open(argv[0], &db)) {
        usage("could not open database", rc);
    }
    cout << "Opened DB " << argv[0] << "...\n";
    if (reset) {
        auto sql = R"(
            PRAGMA journal_mode=wal;
            DROP TABLE IF EXISTS t1;
        )";
        if (auto rc = sqlite3_exec(db, sql, 0, 0, &errmsg)) {
            usage("reset failed", errmsg);
        }
    }
    {
        auto sql = "CREATE TABLE IF NOT EXISTS t1 (hash TEXT)";
        if (auto rc = sqlite3_exec(db, sql, 0, 0, &errmsg)) {
            usage("DB creation failed", errmsg);
        }
    }
    if (reset) {
        auto sql = R"(
            INSERT INTO t1 VALUES(randomblob(16));
        )";
        if (auto rc = sqlite3_exec(db, sql, 0, 0, &errmsg)) {
            usage("reset failed", errmsg);
        }
    }

    // Prepare the statements we use against the opened database
    sqlite3_stmt* delete_st = 0;
    if (auto rc = sqlite3_prepare_v2(db, R"(
                DELETE FROM t1
                    WHERE rowid = random() % (
                        SELECT max(rowid) FROM t1
                    ) + 1
            )", -1, &delete_st, 0)) {
        usage("delete statement preparation failed", rc);
    }
    sqlite3_stmt* update_st = 0;
    if (auto rc = sqlite3_prepare_v2(db, R"(
                UPDATE t1
                    SET hash = randomblob(16)
                    WHERE rowid = random() % (
                        SELECT max(rowid) FROM t1
                    ) + 1
            )", -1, &update_st, 0)) {
        usage("update statement preparation failed", rc);
    }
    sqlite3_stmt* insert_st = 0;
    if (auto rc = sqlite3_prepare_v2(db,
            "INSERT INTO t1 VALUES(randomblob(16))",
            -1, &insert_st, 0)) {
        usage("insert statement preparation failed", rc);
    }
    sqlite3_stmt* select_st = 0;
    if (auto rc = sqlite3_prepare_v2(db, R"(
                SELECT hex(hash) FROM t1
                    WHERE rowid = random() % (
                        SELECT max(rowid) FROM t1
                    ) + 1
            )", -1, &select_st, 0)) {
        usage("select statement preparation failed", rc);
    }
    sqlite3_stmt* check_st = 0;
    if (auto rc = sqlite3_prepare_v2(db, "PRAGMA integrity_check",
            -1, &check_st, 0)) {
        usage("check statement preparation failed", rc);
    }

    // Cleanup and print a nice message when dying
    signal(SIGINT,  die);
    signal(SIGTERM, die);
    atexit(cleanup);

    // Main work loop
    size_t n = 0;
    srand48(time(0));
    while (true) {
        // Select one of the prepared statements at random according to
        // the preponderance constants set above.
        size_t* pcounter;
        auto r = drand48();
        sqlite3_stmt* st = 0;
        if (r < delete_factor) {
            st = delete_st;
            pcounter = &deletes;
        }
        else if (r < delete_factor + update_factor) {
            st = update_st;
            pcounter = &updates;
        }
        else if (r < delete_factor + update_factor + insert_factor) {
            st = insert_st;
            pcounter = &inserts;
        }
        else {
            st = select_st;
            pcounter = &selects;
        }

        // Execute the selected statement
        bool done = false;
        do {
try_again:  switch (auto rc = sqlite3_step(st)) {
                case SQLITE_OK:
                case SQLITE_DONE:
                    done = true;
                    break;

                case SQLITE_ROW:
                    if (verbose) {
                        cout << "\nHASH[" << sqlite3_column_text(st, 0) <<
                                ']' << flush;
                    }
                    break;

                case SQLITE_ERROR:
                    cerr << "Failed to execute " <<
                            sqlite3_expanded_sql(st) <<
                            "\nERROR: " << sqlite3_errmsg(db) << "\n";
                    break;

                case SQLITE_BUSY: 
                    usleep(r * 1000);
                    goto try_again;

                default:
                    cerr << "State machine out of whack executing " <<
                            sqlite3_expanded_sql(st) <<
                            "\nERROR: " << sqlite3_errmsg(db) <<
                            ", RC=" << rc << ".\n";
                    exit(1);
            }
        }
        while (!done);
        sqlite3_reset(st);

        // Keep stats for exit report
        if (((++records % PROGDOTSKIP) == 0) && progress) {
            cout << "ID[" << sqlite3_last_insert_rowid(db) << "]\n";
        }
        ++*pcounter;

        // Is the database still well-formed?
        bool first = true;
        while (sqlite3_step(check_st) == SQLITE_ROW) {
            auto res = sqlite3_column_text(check_st, 0);
            if (first) {
                if (strcmp((char*)res, "ok") == 0) {
                    break;
                }
                else {
                    cerr << "Integrity check failed:\n";
                    first = false;
                }
            }
            cerr << "    " << res << "\n";
        }
        sqlite3_reset(check_st);
    }

    return 0;
}
