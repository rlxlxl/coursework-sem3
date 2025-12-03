#pragma once
#include <string>
#include <vector>
#include <sqlite3.h>
#include <mutex>

struct Integrator {
    int id;
    std::string name;
    std::string city;
    std::string description;
};

extern std::mutex db_mutex;

sqlite3* open_db(const char *filename);
void init_db(sqlite3 *db);

bool has_admin(sqlite3 *db);
void set_admin_password(sqlite3 *db, const std::string &plain);
bool check_admin_password(sqlite3 *db, const std::string &plain);

std::vector<Integrator> list_integrators(sqlite3 *db);
void add_integrator(sqlite3 *db, const std::string &name, const std::string &city, const std::string &desc);
void delete_integrator(sqlite3 *db, int id);

void import_csv(sqlite3 *db, const std::string &path);
