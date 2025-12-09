#pragma once
#include <string>
#include <vector>
#include <libpq-fe.h>
#include <map>
#include <fstream>
#include <sstream>

// Загрузка SQL запросов из файла
class SqlLoader {
public:
    static std::map<std::string, std::string> queries;
    
    static void load() {
        std::ifstream file("queries.sql");
        std::string line;
        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '-') continue;
            size_t pos = line.find('=');
            if (pos != std::string::npos) {
                std::string key = line.substr(0, pos);
                std::string value = line.substr(pos + 1);
                queries[key] = value;
            }
        }
    }
    
    static std::string get(const std::string& key) {
        if (queries.empty()) load();
        return queries[key];
    }
};

namespace SQL {
    // Таблицы (используем IF NOT EXISTS чтобы сохранять данные)
    constexpr const char* CREATE_CITIES = 
        "CREATE TABLE IF NOT EXISTS cities ("
        "id SERIAL PRIMARY KEY,"
        "name TEXT UNIQUE NOT NULL);";
    
    constexpr const char* CREATE_INTEGRATORS = 
        "CREATE TABLE IF NOT EXISTS integrators ("
        "id SERIAL PRIMARY KEY,"
        "name TEXT,"
        "city_id INTEGER REFERENCES cities(id),"
        "activity TEXT);";
    
    constexpr const char* CREATE_ADMIN = 
        "CREATE TABLE IF NOT EXISTS admin ("
        "id SERIAL PRIMARY KEY,"
        "password_hash TEXT NOT NULL);";
    
    // Города
    constexpr const char* INSERT_CITY = 
        "INSERT INTO cities(name) VALUES($1) ON CONFLICT(name) DO UPDATE SET name=EXCLUDED.name RETURNING id";
    
    constexpr const char* SELECT_CITIES = "SELECT id,name FROM cities ORDER BY name";
    
    constexpr const char* SELECT_CITY_BY_NAME = "SELECT id FROM cities WHERE name=$1";
    
    // Интеграторы
    constexpr const char* INSERT_INTEGRATOR = 
        "INSERT INTO integrators(name,city_id,activity) VALUES($1,$2::INTEGER,$3)";
    
    constexpr const char* SELECT_INTEGRATORS = 
        "SELECT i.id, i.name, c.name, i.activity "
        "FROM integrators i "
        "LEFT JOIN cities c ON i.city_id = c.id";
    
    // Админ
    constexpr const char* INSERT_ADMIN = "INSERT INTO admin(password_hash) VALUES($1)";
    
    constexpr const char* DELETE_ADMIN = "DELETE FROM admin";
    
    constexpr const char* SELECT_ADMIN_COUNT = "SELECT 1 FROM admin LIMIT 1";
    
    constexpr const char* SELECT_ADMIN_BY_PASSWORD = "SELECT 1 FROM admin WHERE password_hash=$1";
}

struct City {
    int id;
    std::string name;
};

struct Integrator {
    int id;
    std::string name;
    std::string city;
    std::string activity;
};

class Database {
public:
    Database(const std::string& conninfo);
    ~Database();

    void init();
    bool has_admin();
    void set_admin_password(const std::string& password);
    bool check_admin_password(const std::string& password);

    int add_city(const std::string& name);
    std::vector<City> get_cities();
    int get_city_id(const std::string& name);

    void add_integrator(const std::string& name,
                        int city_id,
                        const std::string& activity);

    std::vector<Integrator> get_integrators();

private:
    PGconn* conn;
};
