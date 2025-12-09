-- Таблицы (используются IF NOT EXISTS для сохранения данных)
CREATE_CITIES=CREATE TABLE IF NOT EXISTS cities (id SERIAL PRIMARY KEY, name TEXT UNIQUE NOT NULL);
CREATE_INTEGRATORS=CREATE TABLE IF NOT EXISTS integrators (id SERIAL PRIMARY KEY, name TEXT, city_id INTEGER REFERENCES cities(id), activity TEXT);
CREATE_ADMIN=CREATE TABLE IF NOT EXISTS admin (id SERIAL PRIMARY KEY, password_hash TEXT NOT NULL);

-- Города
INSERT_CITY=INSERT INTO cities(name) VALUES($1) ON CONFLICT(name) DO UPDATE SET name=EXCLUDED.name RETURNING id;
SELECT_CITIES=SELECT id,name FROM cities ORDER BY name;
SELECT_CITY_BY_NAME=SELECT id FROM cities WHERE name=$1;

-- Интеграторы
INSERT_INTEGRATOR=INSERT INTO integrators(name,city_id,activity) VALUES($1,$2::INTEGER,$3);
SELECT_INTEGRATORS=SELECT i.id, i.name, c.name, i.activity FROM integrators i LEFT JOIN cities c ON i.city_id = c.id;

-- Админ
INSERT_ADMIN=INSERT INTO admin(password_hash) VALUES($1);
DELETE_ADMIN=DELETE FROM admin;
SELECT_ADMIN_COUNT=SELECT 1 FROM admin LIMIT 1;
SELECT_ADMIN_BY_PASSWORD=SELECT 1 FROM admin WHERE password_hash=$1;