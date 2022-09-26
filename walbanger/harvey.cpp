/***********************************************************************
 harvey.cpp, the WAL-banger.

 Created and © 2022.09.25 by Warren Young

 Licensed under https://opensource.org/licenses/BSD-2-Clause
***********************************************************************/

#include <sqlite3.h>
#include <unistd.h>
#include <iostream>
#include <sstream>
#include <stdlib.h>

using namespace std;


//// CONSTANTS /////////////////////////////////////////////////////////

// Usage pattern scaling: how many out of the sum total of these values
// each type of statement should run, on average.  Adjust to suit.
#if 1
    // Read-heavy "application" type mix
    static const auto selects = 900;
    static const auto inserts = 10;
    static const auto updates = 10;
    static const auto deletes = 1;
#else
    // Flat mix, for testing SQL generator code coverage
    static const auto selects = 1;
    static const auto inserts = 1;
    static const auto updates = 1;
    static const auto deletes = 1;
#endif

// Derivatives of above to scale them into the [0.0 - 1.0) range.
static const auto types = selects + inserts + updates + deletes + 0.0;
static const auto select_factor = selects / types;
static const auto insert_factor = inserts / types;
static const auto update_factor = updates / types;
static const auto delete_factor = deletes / types;

// In -p mode, how many statements to skip printing a dot for.
#define PROGDOTSKIP 1000


//// GLOBALS ///////////////////////////////////////////////////////////

static sqlite3* db;


//// usage /////////////////////////////////////////////////////////////

static void
usage(const char* error = 0)
{
    if (error) cerr << "ERROR: " << error << "!\n";
    cerr << R"(
Usage: walpounder [-prv] dbname

    Creates or opens the named SQLite database, then begins updating it
    according to patterns set at compile time at the top of )" __FILE__ R"(.

Options:

    -p  print a progress dot every )" << PROGDOTSKIP << R"( records.

    -r  reset the DB first, deleting all prior data.

    -v  give verbose output
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
    if (argc < 1) usage("not enough arguments");

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

    // Execute the prepared statements in a random shuffle according to
    // the preponderance constants set above.
    size_t n = 0;
    srand48(time(0));
    while (true) {
        ++n;

        auto r = drand48();
        sqlite3_stmt* st = 0;
        if (r < delete_factor) {
            st = delete_st;
        }
        else if (r < delete_factor + update_factor) {
            st = update_st;
        }
        else if (r < delete_factor + update_factor + insert_factor) {
            st = insert_st;
        }
        else {
            st = select_st;
        }

        bool done = false;
        do {
try_again:  switch (auto rc = sqlite3_step(st)) {
                case SQLITE_OK:
                case SQLITE_DONE:
                    done = true;
                    break;

                case SQLITE_ROW:
                    if (verbose) {
                        cout << "ROW: " << sqlite3_column_text(st, 0) << "\n";
                    }
                    if (progress && ((n % PROGDOTSKIP) == 0)) {
                        cout << '.' << flush;
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
                    break;
            }
        }
        while (!done);
        sqlite3_reset(st);
        //cout << "EXECUTED " << sql << endl;

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
