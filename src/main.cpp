#include "db.h"
#include "console.h"
#include "http_server.h"
#include <thread>
#include <iostream>

int main() {
    Database db(
      "host=localhost dbname=integrator_db user=postgres password=postgres"
    );

    db.init();

    if (!db.has_admin()) {
        std::string p;
        std::cout << "Создайте админ пароль: ";
        std::cin >> p;
        db.set_admin_password(p);
    }

    std::thread web([&](){
        start_http_server(db);
    });

    console_loop(db);
    web.join();
}
