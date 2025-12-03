// main.cpp
// Простой backend на C++ + sqlite3 и опциональный HTTP frontend (cpp-httplib).
// Компиляция: g++ main.cpp -lsqlite3 -pthread -o integrator_app
// Для HTTP фронтенда скачайте single-header cpp-httplib и определите CPPHTTPLIB_HEADER_INCLUDE
// (см. инструкцию ниже).

#include <sqlite3.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <functional>
#include <thread>
#include <mutex>
#include <algorithm>
#include <cctype>
#include <sys/stat.h>

#ifdef CPPHTTPLIB_HEADER_INCLUDE
// Если вы скачали cpp-httplib single header и прописали -DCPPHTTPLIB_HEADER_INCLUDE
#include CPPHTTPLIB_HEADER_INCLUDE
using namespace httplib;
#endif

std::mutex db_mutex;

static const char *DB_FILENAME = "integrators.db";
static const char *ADMIN_KEY = "admin_password_hash";

std::string to_hex(size_t x){
    std::ostringstream ss;
    ss << std::hex << x;
    return ss.str();
}

std::string simple_hash(const std::string &s){
    // Простая демонстрационная хеш-функция (не криптографическая).
    // Для продакшна используйте bcrypt/argon2/OpenSSL SHA2.
    std::hash<std::string> h;
    size_t v = h(s + "SALT_DEMO_v1");
    return to_hex(v);
}

bool file_exists(const std::string &path) {
    struct stat buffer;
    return (stat(path.c_str(), &buffer) == 0);
}

void exec_sql(sqlite3 *db, const std::string &sql){
    char *err = nullptr;
    std::lock_guard<std::mutex> lock(db_mutex);
    int rc = sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err);
    if(rc != SQLITE_OK){
        std::string e = err ? err : "unknown";
        sqlite3_free(err);
        throw std::runtime_error("SQL error: " + e + "\nWhen running: " + sql);
    }
}

sqlite3* open_db(const char *filename){
    sqlite3 *db = nullptr;
    int rc = sqlite3_open(filename, &db);
    if(rc){
        throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
    }
    return db;
}

void init_db(sqlite3 *db){
    const char *sql_create =
        "BEGIN TRANSACTION;"
        "CREATE TABLE IF NOT EXISTS kv (key TEXT PRIMARY KEY, value TEXT);"
        "CREATE TABLE IF NOT EXISTS integrators ("
        " id INTEGER PRIMARY KEY AUTOINCREMENT,"
        " name TEXT NOT NULL,"
        " city TEXT,"
        " description TEXT"
        ");"
        "COMMIT;";
    exec_sql(db, sql_create);
}

bool has_admin(sqlite3 *db){
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT value FROM kv WHERE key = ?;";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, ADMIN_KEY, -1, SQLITE_STATIC);
    bool found = false;
    if(sqlite3_step(stmt) == SQLITE_ROW){
        const unsigned char *val = sqlite3_column_text(stmt, 0);
        if(val) found = true;
    }
    sqlite3_finalize(stmt);
    return found;
}

void set_admin_password(sqlite3 *db, const std::string &plain){
    std::string h = simple_hash(plain);
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    const char *sql = "INSERT OR REPLACE INTO kv (key, value) VALUES (?, ?);";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("prepare failed");
    sqlite3_bind_text(stmt, 1, ADMIN_KEY, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, h.c_str(), -1, SQLITE_STATIC);
    if(sqlite3_step(stmt) != SQLITE_DONE){
        sqlite3_finalize(stmt);
        throw std::runtime_error("failed to set admin password");
    }
    sqlite3_finalize(stmt);
}

bool check_admin_password(sqlite3 *db, const std::string &plain){
    std::string h = simple_hash(plain);
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT value FROM kv WHERE key = ?;";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;
    sqlite3_bind_text(stmt, 1, ADMIN_KEY, -1, SQLITE_STATIC);
    bool ok = false;
    if(sqlite3_step(stmt) == SQLITE_ROW){
        const unsigned char *val = sqlite3_column_text(stmt, 0);
        if(val && h == std::string(reinterpret_cast<const char*>(val))) ok = true;
    }
    sqlite3_finalize(stmt);
    return ok;
}

struct Integrator {
    int id;
    std::string name;
    std::string city;
    std::string description;
};

std::vector<Integrator> list_integrators(sqlite3 *db){
    std::vector<Integrator> out;
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    const char *sql = "SELECT id, name, city, description FROM integrators ORDER BY id;";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("prepare failed");
    while(sqlite3_step(stmt) == SQLITE_ROW){
        Integrator it;
        it.id = sqlite3_column_int(stmt, 0);
        const unsigned char *n = sqlite3_column_text(stmt, 1);
        const unsigned char *c = sqlite3_column_text(stmt, 2);
        const unsigned char *d = sqlite3_column_text(stmt, 3);
        it.name = n ? reinterpret_cast<const char*>(n) : "";
        it.city = c ? reinterpret_cast<const char*>(c) : "";
        it.description = d ? reinterpret_cast<const char*>(d) : "";
        out.push_back(it);
    }
    sqlite3_finalize(stmt);
    return out;
}

void add_integrator(sqlite3 *db, const std::string &name, const std::string &city, const std::string &description){
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    const char *sql = "INSERT INTO integrators (name, city, description) VALUES (?, ?, ?);";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("prepare failed");
    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, city.c_str(), -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_STATIC);
    if(sqlite3_step(stmt) != SQLITE_DONE){
        sqlite3_finalize(stmt);
        throw std::runtime_error("failed to insert integrator");
    }
    sqlite3_finalize(stmt);
}

void delete_integrator(sqlite3 *db, int id){
    std::lock_guard<std::mutex> lock(db_mutex);
    sqlite3_stmt *stmt;
    const char *sql = "DELETE FROM integrators WHERE id = ?;";
    if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK)
        throw std::runtime_error("prepare failed");
    sqlite3_bind_int(stmt, 1, id);
    if(sqlite3_step(stmt) != SQLITE_DONE){
        sqlite3_finalize(stmt);
        throw std::runtime_error("failed to delete integrator");
    }
    sqlite3_finalize(stmt);
}

std::vector<std::vector<std::string>> parse_csv(const std::string &path){
    std::ifstream ifs(path);
    if(!ifs) throw std::runtime_error("Can't open CSV: " + path);
    std::vector<std::vector<std::string>> rows;
    std::string line;
    while(std::getline(ifs, line)){
        std::vector<std::string> cols;
        std::string cur;
        bool inq = false;
        for(size_t i=0;i<line.size();++i){
            char ch = line[i];
            if(ch == '"' ){
                inq = !inq;
            } else if(ch == ',' && !inq){
                cols.push_back(cur);
                cur.clear();
            } else {
                cur.push_back(ch);
            }
        }
        cols.push_back(cur);
        // trim spaces
        for(auto &c: cols){
            while(!c.empty() && std::isspace((unsigned char)c.front())) c.erase(c.begin());
            while(!c.empty() && std::isspace((unsigned char)c.back())) c.pop_back();
        }
        if(!cols.empty()) rows.push_back(cols);
    }
    return rows;
}

void import_csv(sqlite3 *db, const std::string &path){
    auto rows = parse_csv(path);
    // Expect header optionally: name,city,description
    for(auto &r: rows){
        if(r.size() < 1) continue;
        std::string name = r.size() > 0 ? r[0] : "";
        std::string city = r.size() > 1 ? r[1] : "";
        std::string desc = r.size() > 2 ? r[2] : "";
        if(name.empty()) continue;
        add_integrator(db, name, city, desc);
    }
}

void print_table(const std::vector<Integrator> &list){
    std::cout << "ID | Name | City | Description\n";
    std::cout << "------------------------------------------------------------\n";
    for(auto &it: list){
        std::cout << it.id << " | " << it.name << " | " << it.city << " | " << it.description << "\n";
    }
    std::cout << "------------------------------------------------------------\n";
}

void console_loop(sqlite3 *db){
    while(true){
        std::cout << "\nВыберите режим:\n1) Просмотр (user)\n2) Войти как admin\n3) Импорт CSV (через консоль) [admin only]\n4) Запустить HTTP frontend (опционально)\n5) Выход\n> ";
        int cmd;
        if(!(std::cin >> cmd)){
            std::cin.clear();
            std::string skip;
            std::getline(std::cin, skip);
            continue;
        }
        if(cmd == 1){
            auto list = list_integrators(db);
            print_table(list);
        } else if(cmd == 2){
            std::string pwd;
            std::cout << "Введите админ пароль: ";
            std::cin >> pwd;
            if(check_admin_password(db, pwd)){
                std::cout << "Успешно авторизированы как admin.\n";
                while(true){
                    std::cout << "\nAdmin меню:\n1) Добавить интегратора\n2) Удалить по ID\n3) Показать все\n4) Выход из admin\n> ";
                    int a;
                    if(!(std::cin >> a)){
                        std::cin.clear();
                        std::string sk; std::getline(std::cin, sk);
                        continue;
                    }
                    if(a == 1){
                        std::string name, city, desc;
                        std::getline(std::cin, name); // consume newline
                        std::cout << "Имя: "; std::getline(std::cin, name);
                        std::cout << "Город: "; std::getline(std::cin, city);
                        std::cout << "Чем занимаются (описание): "; std::getline(std::cin, desc);
                        add_integrator(db, name, city, desc);
                        std::cout << "Добавлено.\n";
                    } else if(a == 2){
                        int id; std::cout << "ID для удаления: "; std::cin >> id;
                        try{
                            delete_integrator(db, id);
                            std::cout << "Удалено (если такой ID был).\n";
                        } catch(std::exception &e){
                            std::cout << "Ошибка: " << e.what() << "\n";
                        }
                    } else if(a == 3){
                        auto list = list_integrators(db);
                        print_table(list);
                    } else if(a == 4) break;
                }
            } else {
                std::cout << "Неверный пароль.\n";
            }
        } else if(cmd == 3){
            std::string pwd;
            std::cout << "Введите админ пароль: ";
            std::cin >> pwd;
            if(!check_admin_password(db, pwd)){
                std::cout << "Неверный пароль.\n";
                continue;
            }
            std::string path;
            std::cout << "Путь к CSV файлу: ";
            std::cin >> path;
            try{
                import_csv(db, path);
                std::cout << "Импорт завершён.\n";
            } catch(std::exception &e){
                std::cout << "Ошибка при импорте: " << e.what() << "\n";
            }
        } else if(cmd == 4){
#ifdef CPPHTTPLIB_HEADER_INCLUDE
            std::cout << "Запуск HTTP frontend на localhost:8080\n";
            std::thread server_thread([db](){
                Server svr;
                svr.Get("/", [db](const Request&, Response& res){
                    std::ostringstream html;
                    html << "<html><head><meta charset='utf-8'><title>Integrators</title></head><body>";
                    html << "<h2>Integrators list</h2>";
                    html << "<table border='1'><tr><th>ID</th><th>Name</th><th>City</th><th>Description</th></tr>";
                    auto list = list_integrators(db);
                    for(auto &it: list){
                        html << "<tr><td>" << it.id << "</td><td>" << it.name << "</td><td>" << it.city << "</td><td>" << it.description << "</td></tr>";
                    }
                    html << "</table>";
                    html << "<p>To admin add via POST /admin_add with fields name,city,description and admin=<password></p>";
                    html << "</body></html>";
                    res.set_content(html.str(), "text/html; charset=utf-8");
                });
                svr.Post("/admin_add", [db](const Request& req, Response& res){
                    auto name = req.get_param_value("name");
                    auto city = req.get_param_value("city");
                    auto description = req.get_param_value("description");
                    auto admin = req.get_param_value("admin");
                    if(!check_admin_password(db, admin)){
                        res.status = 403;
                        res.set_content("Forbidden: bad admin password", "text/plain");
                        return;
                    }
                    try{
                        add_integrator(db, name, city, description);
                        res.set_content("Added", "text/plain");
                    } catch(std::exception &e){
                        res.status = 500;
                        res.set_content(std::string("Error: ") + e.what(), "text/plain");
                    }
                });
                svr.listen("0.0.0.0", 8080);
            });
            server_thread.detach();
            std::cout << "HTTP server запущен в фоне (detach). Откройте http://localhost:8080\n";
#else
            std::cout << "HTTP frontend не включён. Чтобы включить, скачайте cpp-httplib single-header\n"
                      << "и компилируйте с -DCPPHTTPLIB_HEADER_INCLUDE=\\\"httplib.h\\\" (см. README).\n";
#endif
        } else if(cmd == 5){
            std::cout << "Выход.\n"; break;
        } else {
            std::cout << "Неизвестная команда.\n";
        }
    }
}

int main(int argc, char **argv){
    try{
        sqlite3 *db = open_db(DB_FILENAME);
        init_db(db);

        if(!has_admin(db)){
            std::cout << "Админ пароль не установлен. Установите новый админ пароль (будет сохранён хеш): ";
            std::string pwd;
            std::cin >> pwd;
            set_admin_password(db, pwd);
            std::cout << "Пароль сохранён.\n";
        }

        // Простая поддержка командной строки:
        // ./integrator_app import path/to/file.csv -- импорт csv однократно
        if(argc >= 3){
            std::string cmd = argv[1];
            if(cmd == "import"){
                std::string path = argv[2];
                import_csv(db, path);
                std::cout << "Импорт завершён из: " << path << "\n";
                sqlite3_close(db);
                return 0;
            }
        }

        // Консольный UI
        console_loop(db);

        sqlite3_close(db);
    } catch(std::exception &e){
        std::cerr << "Fatal: " << e.what() << "\n";
        return 1;
    }
    return 0;
}