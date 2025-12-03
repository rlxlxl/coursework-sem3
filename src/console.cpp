#include "console.h"
#include "db.h"
#include <iostream>

void print_table(const std::vector<Integrator> &list){
    std::cout << "ID | Name | City | Description\n";
    std::cout << "----------------------------------------------\n";
    for(const auto &it : list){
        std::cout << it.id << " | " << it.name << " | " << it.city
                  << " | " << it.description << "\n";
    }
}

void console_loop(sqlite3 *db){
    while(true){
        std::cout << "\n1) Просмотр\n2) Admin\n3) Импорт CSV\n4) Выход\n> ";
        int cmd;
        if(!(std::cin >> cmd)) { std::cin.clear(); std::string s; std::getline(std::cin, s); continue; }

        if(cmd == 1){
            print_table(list_integrators(db));

        } else if(cmd == 2){
            std::string pwd;
            std::cout << "Пароль: ";
            std::cin >> pwd;

            if(!check_admin_password(db, pwd)){
                std::cout << "Неверно\n";
                continue;
            }
            std::cout << "Admin mode\n";

            while(true){
                std::cout << "1) Добавить\n2) Удалить\n3) Показать\n4) Выход\n> ";
                int a; std::cin >> a;

                if(a == 1){
                    std::string n, c, d;
                    std::getline(std::cin, n);
                    std::cout << "Имя: "; std::getline(std::cin, n);
                    std::cout << "Город: "; std::getline(std::cin, c);
                    std::cout << "Описание: "; std::getline(std::cin, d);
                    add_integrator(db, n, c, d);

                } else if(a == 2){
                    int id; std::cout << "ID: "; std::cin >> id;
                    delete_integrator(db, id);

                } else if(a == 3){
                    print_table(list_integrators(db));

                } else break;
            }

        } else if(cmd == 3){
            std::string pwd;
            std::cout << "Админ пароль: ";
            std::cin >> pwd;

            if(!check_admin_password(db, pwd)){
                std::cout << "Неверно\n";
                continue;
            }

            std::string path;
            std::cout << "CSV путь: ";
            std::cin >> path;

            try{
                import_csv(db, path);
                std::cout << "Импорт ok.\n";
            } catch(std::exception &e){
                std::cout << "Ошибка: " << e.what() << "\n";
            }

        } else if(cmd == 4){
            break;
        }
    }
}
