# 1. Базовый образ
FROM ubuntu:24.04

# 2. Устанавливаем зависимости
RUN apt-get update && apt-get install -y \
    g++ \
    sqlite3 \
    libsqlite3-dev \
    && rm -rf /var/lib/apt/lists/*

# 3. Создаём рабочую директорию
WORKDIR /app

# 4. Копируем исходники и фронт
COPY src/ src/
COPY include/ include/
COPY web/ web/

# 5. Создаём папку build и компилируем
RUN mkdir -p build && \
    g++ src/main.cpp src/db.cpp src/console.cpp src/util.cpp src/http_server.cpp \
    -Iinclude -lsqlite3 -pthread -std=c++17 \
    -DCPPHTTPLIB_HEADER_INCLUDE="\"httplib.h\"" \
    -o build/app

# 6. Открываем порт
EXPOSE 8080

# 7. Запуск приложения
CMD ["./build/app"]