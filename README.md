#Комаиляция вручную
```bash
g++ src/main.cpp src/db.cpp src/console.cpp src/http_server.cpp src/util.cpp \
-Iinclude \
-I/opt/homebrew/opt/libpq/include \
-I/opt/homebrew/opt/oenssl/include \
-L/opt/homebrew/opt/libpq/lib \
-L/opt/homebrew/opt/openssl/lib \
-lpq -lssl -lcrypto -pthread -std=c++17 \
-o app
```
#Через докер
```bash
docker build -t integrator-app .
```