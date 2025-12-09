#include "db.h"
#include <stdexcept>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>
#include <iostream>

std::map<std::string, std::string> SqlLoader::queries;

int Database::add_city(const std::string& name) {
    const char* values[] = {name.c_str()};
    PGresult* r = PQexecParams(conn, SqlLoader::get("INSERT_CITY").c_str(),
        1, NULL, values, NULL, NULL, 0);
    
    int city_id = -1;
    if (PQresultStatus(r) == PGRES_TUPLES_OK && PQntuples(r) > 0) {
        city_id = std::stoi(PQgetvalue(r, 0, 0));
    } else {
        std::cerr << "Insert city error: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(r);
    return city_id;
}

std::vector<City> Database::get_cities() {
    PGresult* r = PQexec(conn, SqlLoader::get("SELECT_CITIES").c_str());
    std::vector<City> v;
    
    for (int i = 0; i < PQntuples(r); i++) {
        v.push_back({
            std::stoi(PQgetvalue(r,i,0)),
            PQgetvalue(r,i,1)
        });
    }
    PQclear(r);
    return v;
}

int Database::get_city_id(const std::string& name) {
    const char* values[] = {name.c_str()};
    PGresult* r = PQexecParams(conn,
        SqlLoader::get("SELECT_CITY_BY_NAME").c_str(),
        1, NULL, values, NULL, NULL, 0);
    
    int city_id = -1;
    if (PQntuples(r) > 0) {
        city_id = std::stoi(PQgetvalue(r, 0, 0));
    }
    PQclear(r);
    return city_id;
}

static std::string sha256(const std::string& str) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((unsigned char*)str.c_str(), str.size(), hash);
    std::stringstream ss;
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++)
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    return ss.str();
}

Database::Database(const std::string& conninfo) {
    conn = PQconnectdb(conninfo.c_str());
    if (PQstatus(conn) != CONNECTION_OK)
        throw std::runtime_error(PQerrorMessage(conn));
}

Database::~Database() {
    PQfinish(conn);
}

void Database::init() {
    PGresult* r;
    
    r = PQexec(conn, SQL::CREATE_CITIES);
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
        std::cerr << "Create cities error: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(r);
    
    r = PQexec(conn, SQL::CREATE_INTEGRATORS);
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
        std::cerr << "Create integrators error: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(r);
    
    r = PQexec(conn, SQL::CREATE_ADMIN);
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
        std::cerr << "Create admin error: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(r);
}

bool Database::has_admin() {
    PGresult* r = PQexec(conn, SqlLoader::get("SELECT_ADMIN_COUNT").c_str());
    bool exists = PQntuples(r) > 0;
    PQclear(r);
    return exists;
}

void Database::set_admin_password(const std::string& password) {
    std::string h = sha256(password);
    PGresult* r = PQexec(conn, SqlLoader::get("DELETE_ADMIN").c_str());
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
        std::cerr << "Delete error: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(r);
    
    const char* values[] = {h.c_str()};
    r = PQexecParams(conn,
        SqlLoader::get("INSERT_ADMIN").c_str(),
        1, NULL, values, NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
        std::cerr << "Insert error: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(r);
}

bool Database::check_admin_password(const std::string& password) {
    std::string h = sha256(password);
    const char* values[] = {h.c_str()};
    PGresult* r = PQexecParams(conn,
        SqlLoader::get("SELECT_ADMIN_BY_PASSWORD").c_str(),
        1, NULL, values, NULL, NULL, 0);
    bool ok = PQntuples(r) > 0;
    PQclear(r);
    return ok;
}

void Database::add_integrator(const std::string& n, int city_id, const std::string& a) {
    std::string city_id_str = std::to_string(city_id);
    const char* values[] = {n.c_str(), city_id_str.c_str(), a.c_str()};
    PGresult* r = PQexecParams(conn,
        SqlLoader::get("INSERT_INTEGRATOR").c_str(),
        3, NULL, values, NULL, NULL, 0);
    if (PQresultStatus(r) != PGRES_COMMAND_OK) {
        std::cerr << "Insert error: " << PQerrorMessage(conn) << std::endl;
    }
    PQclear(r);
}

std::vector<Integrator> Database::get_integrators() {
    PGresult* r = PQexec(conn, SqlLoader::get("SELECT_INTEGRATORS").c_str());
    std::vector<Integrator> v;

    for (int i = 0; i < PQntuples(r); i++) {
        v.push_back({
            std::stoi(PQgetvalue(r,i,0)),
            PQgetvalue(r,i,1),
            PQgetvalue(r,i,2),
            PQgetvalue(r,i,3)
        });
    }
    PQclear(r);
    return v;
}
