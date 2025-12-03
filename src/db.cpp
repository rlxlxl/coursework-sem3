#include "db.h"
#include "util.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cctype>

std::mutex db_mutex;
static const char *ADMIN_KEY = "admin_password_hash";

sqlite3* open_db(const char *filename){
    sqlite3 *db = nullptr;
    if(sqlite3_open(filename, &db)){
        throw std::runtime_error("Can't open DB");
    }
    return db;
}

void exec_sql(sqlite3 *db, const std::string &sql){
    char *err = nullptr;
    std::lock_guard<std::mutex> lock(db_mutex);
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);

    if(rc != SQLITE_OK){
        std::string msg = err ? err : "unknown";
        sqlite3_free(err);
        throw std::runtime_error("SQL error: " + msg);
    }
}

void init_db(sqlite3 *db){
    const char *sql =
        "BEGIN;"
        "CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT);"
        "CREATE TABLE IF NOT EXISTS integrators ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "name TEXT,"
        "city TEXT,"
        "description TEXT"
        ");"
        "COMMIT;";
    exec_sql(db, sql);
}

bool has_admin(sqlite3 *db){
    std::lock_guard<std::mutex> lock(db_mutex);

    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT value FROM kv WHERE key = ?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, ADMIN_KEY, -1, SQLITE_STATIC);

    bool found = false;
    if(sqlite3_step(stmt) == SQLITE_ROW){
        found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

void set_admin_password(sqlite3 *db, const std::string &plain){
    std::string h = simple_hash(plain);

    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db,
        "INSERT OR REPLACE INTO kv (key,value) VALUES(?,?);",
        -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, ADMIN_KEY, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, h.c_str(), -1, SQLITE_STATIC);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

bool check_admin_password(sqlite3 *db, const std::string &plain){
    std::string h = simple_hash(plain);

    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, "SELECT value FROM kv WHERE key=?;", -1, &stmt, nullptr);
    sqlite3_bind_text(stmt, 1, ADMIN_KEY, -1, SQLITE_STATIC);

    bool ok = false;
    if(sqlite3_step(stmt) == SQLITE_ROW){
        const unsigned char *val = sqlite3_column_text(stmt, 0);
        if(val && h == reinterpret_cast<const char*>(val))
            ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<Integrator> list_integrators(sqlite3 *db){
    std::vector<Integrator> v;
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db,
        "SELECT id,name,city,description FROM integrators ORDER BY id;",
        -1, &stmt, nullptr);

    while(sqlite3_step(stmt) == SQLITE_ROW){
        Integrator it;
        it.id = sqlite3_column_int(stmt, 0);
        it.name = (const char*)sqlite3_column_text(stmt, 1);
        it.city = (const char*)sqlite3_column_text(stmt, 2);
        it.description = (const char*)sqlite3_column_text(stmt, 3);
        v.push_back(it);
    }
    sqlite3_finalize(stmt);
    return v;
}

void add_integrator(sqlite3 *db, const std::string &name,
                    const std::string &city, const std::string &desc){
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db,
        "INSERT INTO integrators(name,city,description) VALUES(?,?,?);",
        -1, &stmt, nullptr);

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, city.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, desc.c_str(), -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

void delete_integrator(sqlite3 *db, int id){
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;

    sqlite3_prepare_v2(db,
        "DELETE FROM integrators WHERE id=?;",
        -1, &stmt, nullptr);

    sqlite3_bind_int(stmt, 1, id);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

// CSV IMPORT
static std::vector<std::vector<std::string>> parse_csv(const std::string &path){
    std::ifstream in(path);
    if(!in) throw std::runtime_error("Cannot open CSV: " + path);

    std::vector<std::vector<std::string>> rows;
    std::string line;

    while(std::getline(in, line)){
        std::vector<std::string> cols;
        std::string cur;
        bool inq = false;

        for(char c : line){
            if(c == '"') inq = !inq;
            else if(c == ',' && !inq){
                cols.push_back(cur);
                cur.clear();
            } else cur.push_back(c);
        }
        cols.push_back(cur);

        for(auto &c : cols){
            while(!c.empty() && std::isspace((unsigned char)c.front())) c.erase(c.begin());
            while(!c.empty() && std::isspace((unsigned char)c.back())) c.pop_back();
        }
        rows.push_back(cols);
    }
    return rows;
}

void import_csv(sqlite3 *db, const std::string &path){
    auto rows = parse_csv(path);
    for(auto &r : rows){
        if(r.empty()) continue;
        std::string name = r.size() > 0 ? r[0] : "";
        std::string city = r.size() > 1 ? r[1] : "";
        std::string desc = r.size() > 2 ? r[2] : "";
        if(!name.empty()) add_integrator(db, name, city, desc);
    }
}
