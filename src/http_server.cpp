#include "http_server.h"
#include <fstream>
#include <sstream>
#include <thread>
#include <cstdio>

#include "httplib.h"
using namespace httplib;

static std::string escape_json(const std::string& str) {
    std::string result;
    for (unsigned char c : str) {
        switch (c) {
            case '"': result += "\\\""; break;
            case '\\': result += "\\\\"; break;
            case '\b': result += "\\b"; break;
            case '\f': result += "\\f"; break;
            case '\n': result += "\\n"; break;
            case '\r': result += "\\r"; break;
            case '\t': result += "\\t"; break;
            default:
                if (c < 32) {
                    char buf[7];
                    snprintf(buf, sizeof(buf), "\\u%04x", c);
                    result += buf;
                } else {
                    result += c;
                }
        }
    }
    return result;
}

void start_http_server(Database& db) {
    std::thread([&db]() {
        Server svr;

        // HTML
        svr.Get("/", [](const Request&, Response& res) {
            std::ifstream file("web/index.html");
            if (!file) {
                res.status = 404;
                res.set_content("index.html not found", "text/plain");
                return;
            }
            std::stringstream buf;
            buf << file.rdbuf();
            res.set_content(buf.str(), "text/html; charset=utf-8");
        });

        // Получить список интеграторов
        svr.Get("/list", [&db](const Request&, Response& res) {
            auto list = db.get_integrators();
            std::ostringstream json;
            json << "[";

            for (size_t i = 0; i < list.size(); ++i) {
                const auto& it = list[i];
                json << "{"
                     << "\"id\":" << it.id << ","
                     << "\"name\":\"" << escape_json(it.name) << "\","
                     << "\"city\":\"" << escape_json(it.city) << "\","
                     << "\"activity\":\"" << escape_json(it.activity) << "\"}";
                if (i + 1 < list.size()) json << ",";
            }
            json << "]";
            res.set_content(json.str(), "application/json; charset=utf-8");
        });

        // Логин админа
        svr.Post("/admin_login", [&db](const Request& req, Response& res) {
            auto pass = req.get_param_value("admin");
            if (db.check_admin_password(pass)) {
                res.set_content("ok", "text/plain");
            } else {
                res.status = 403;
                res.set_content("bad password", "text/plain");
            }
        });

        // Добавление интегратора
        svr.Post("/admin_add", [&db](const Request& req, Response& res) {
            if (!db.check_admin_password(req.get_param_value("admin"))) {
                res.status = 403;
                res.set_content("forbidden", "text/plain");
                return;
            }

            int city_id = db.add_city(req.get_param_value("city"));
            if (city_id < 0) {
                res.status = 400;
                res.set_content("city error", "text/plain");
                return;
            }

            db.add_integrator(
                req.get_param_value("name"),
                city_id,
                req.get_param_value("activity")
            );

            res.set_content("added", "text/plain");
        });

        svr.listen("0.0.0.0", 8080);
    }).detach();
}
