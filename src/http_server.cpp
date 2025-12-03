#include "http_server.h"
#include "db.h"
#include <fstream>
#include <sstream>
#include <thread>

#ifdef CPPHTTPLIB_HEADER_INCLUDE
#include CPPHTTPLIB_HEADER_INCLUDE
using namespace httplib;

void start_http_server(sqlite3 *db){
    std::thread([db](){
        Server svr;

        // Отдать HTML
        svr.Get("/", [](const Request&, Response &res){
            std::ifstream file("web/index.html");
            if(!file){
                res.status = 404;
                res.set_content("index.html not found", "text/plain");
                return;
            }
            std::stringstream buf;
            buf << file.rdbuf();
            res.set_content(buf.str(), "text/html; charset=utf-8");
        });

        // REST API: список интеграторов
        svr.Get("/list", [db](const Request&, Response &res){
            auto list = list_integrators(db);
            std::ostringstream json;
            json << "[";
            for(size_t i=0;i<list.size();++i){
                const auto &it = list[i];
                json << "{\"id\":" << it.id
                     << ",\"name\":\"" << it.name
                     << "\",\"city\":\"" << it.city
                     << "\",\"description\":\"" << it.description
                     << "\"}";
                if(i + 1 != list.size()) json << ",";
            }
            json << "]";
            res.set_content(json.str(), "application/json; charset=utf-8");
        });

        // Добавление интегратора (POST)
        svr.Post("/admin_add", [db](const Request& req, Response &res){
            auto name = req.get_param_value("name");
            auto city = req.get_param_value("city");
            auto description = req.get_param_value("description");
            auto admin = req.get_param_value("admin");

            if(!check_admin_password(db, admin)){
                res.status = 403;
                res.set_content("Forbidden: bad admin password", "text/plain");
                return;
            }

            if(name.empty()){
                res.status = 400;
                res.set_content("Name is required", "text/plain");
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
    }).detach();
}

#else
void start_http_server(sqlite3 *db){}
#endif
