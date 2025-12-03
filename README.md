```cpp
g++ src/main.cpp src/db.cpp src/console.cpp src/util.cpp src/http_server.cpp -Iinclude \
    -lsqlite3 -pthread -std=c++17 \
    -DCPPHTTPLIB_HEADER_INCLUDE="\"httplib.h\"" \
    -o build/app && ./build/app