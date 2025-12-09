#include "console.h"
#include <iostream>

void print_table(const std::vector<Integrator>& v) {
    for (auto& i : v) {
        std::cout << i.id << " | "
                  << i.name << " | "
                  << i.city << " | "
                  << i.activity << "\n";
    }
}

void console_loop(Database& db) {
    while (true) {
        std::cout << "\n1. Показать интеграторов\n"
                  << "2. Добавить интегратора (admin)\n"
                  << "0. Выход\n> ";

        int c;
        std::cin >> c;

        if (c == 0) break;

        if (c == 1) {
            print_table(db.get_integrators());
        }

        if (c == 2) {
            std::string pwd;
            std::cout << "Пароль: ";
            std::cin >> pwd;

            if (!db.check_admin_password(pwd)) {
                std::cout << "Неверно\n";
                continue;
            }

            std::string n, cit, a;
            std::cout << "Название: "; std::cin >> n;
            std::cout << "Город: "; std::cin >> cit;
            std::cout << "Деятельность: "; std::cin >> a;

            int city_id = db.add_city(cit);
            if (city_id < 0) {
                std::cout << "Ошибка добавления города\n";
                continue;
            }

            db.add_integrator(n, city_id, a);
            std::cout << "Интегратор добавлен\n";
        }
    }
}
