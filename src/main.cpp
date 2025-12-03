#include "db.h"
#include "console.h"
#include "util.h"
#include "http_server.h"
#include <iostream>
#include <thread>

int main() {
    // Открываем или создаём базу
    sqlite3* db = open_db("integrators.db");
    init_db(db);

    // Если админ ещё не установлен, запрашиваем пароль
    if(!has_admin(db)){
        std::string p;
        std::cout << "Установите админ пароль: ";
        std::cin >> p;
        set_admin_password(db, p);
        std::cout << "Админ пароль установлен.\n";
    }

    // Запускаем веб-сервер в отдельном потоке
    std::thread server_thread([db]() {
        std::cout << "Запуск HTTP сервера на localhost:8080\n";
        start_http_server(db);
    });

    // Главный поток остаётся для консоли
    console_loop(db);

    // Ожидаем завершения сервера после выхода из консоли
    server_thread.join();

    sqlite3_close(db);
    return 0;
}