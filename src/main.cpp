#include "db.h"
#include "console.h"
#include "util.h"
#include "http_server.h"
#include <iostream>

int main(){
    sqlite3 *db = open_db("integrators.db");
    init_db(db);

    if(!has_admin(db)){
        std::string p;
        std::cout << "Установите админ пароль: ";
        std::cin >> p;
        set_admin_password(db, p);
    }

    std::cout << "Запуск HTTP сервера на localhost:8080\n";
    start_http_server(db);  // сервер в фоне

    console_loop(db);

    sqlite3_close(db);
    return 0;
}
