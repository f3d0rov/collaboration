![COLLABORATION.](https://github.com/f3d0rov/collaboration/blob/f3d0rov-patch-1/logo.png?raw=true)
# COLLABORATION.
Веб-сервис базы данных участия разных музыкантов в написании альбомов, участии в концертах и прочем. Разрабатывался для запуска на платформах под управлением Linux (Скорее всего, возможен запуск и на Windows - исходный код сервера не использует ничего специфичного для Linux). В настоящее время доступен по адресу [collaboration.pw](https://www.collaboration.pw) (Браузер будет жаловаться на угрозу безопасности из-за самоподписанного сертификата - можно спокойно принять риск и продолжить).

## Зависимости
- [PostgreSQL](https://www.postgresql.org/) - база данных для хранения всей пользовательской информации;
- [CivetWeb](https://github.com/civetweb/civetweb) - http/https сервер для C/C++;
- [libpqxx](https://github.com/jtv/libpqxx) - коннектор к БД PostgreSQL для C++;
- [nlohmann/json](https://github.com/nlohmann/json) - библиотека для работы с JSON-объектами для C++;
- [Jed# C++ SMTP Client Library](https://github.com/jeremydumais/CPP-SMTPClient-library) - библиотека C++ для работы с почтой.
- [OpenSSL](https://www.openssl.org/) - криптографическая библиотека для поддержки HTTPS и хэширования паролей пользователей.

Кроме того, для компиляции сервера необходима утилита `make` и `c++20`-совместимый компилятор (проект настроен для компиляции с помощью `clang++`).

## Компиляция

1. Установите необходимые зависимости. Заголовочные файлы библиотек `CivetWeb` и `nlohmann/json` уже содержатся в проекте. (Для `CivetWeb` тем не менее требуется компиляция самой библиотеки). При желании использовать HTTPS необходимо компилировать `CivetWeb` с ключом `WITH_OPENSSL_API_3_0=1`.
2. Загрузите файлы данного репозитория в необходимую директорию.
```
    git clone https://github.com/f3d0rov/collaboration.git
```
3. Перейдите в директорию сервера.
```
  cd collaboration/backend
```
4. `makefile`, находщийся в данной директории, настроен для компиляции сервера с помощью компилятора `clang++`. Возможно использование любого другого компилятора с поддержкой стандарта `c++20`. Для этого потребуется изменить переменную `CXX` в файле `makefile`. При необходимости можно добавить флаги `-I` путей к исходным файлам библиотек в переменной `INCLUDE` и пути к lib-файлам библиотек `-L` в переменной `LIB_INCLUDE`. После конфигурации исполняем:
```
  make
```
5. При успешной компиляции (без вывода каких-либо ошибок) будет необходимо сконфигурировать файлы с данными для подключения к БД и к почтовому серверу (если он будет использоваться). Примеры файлов конфигурации - `build/db-config-example.json` (для файла конфигурации БД `build/db-config.json`) и `build/smtp-config-example.json` (для файла конфигурации почты `build/smtp-config.json`). Для использования HTTPS необходим файл `build/server.pem` ([Как создать](https://github.com/civetweb/civetweb/blob/master/docs/OpenSSL.md#creating-a-self-signed-certificate)).

## Запуск
1. При первом запуске необходимо сконфигурировать базу данных. Для этого используется ключ `--remake-db`. Он выполнит файлы `sql/reset-db.sql` и `sql/make-db.sql` перед запуском сервера.
2. В отсутствии ключей сервер будет запущен на порту 8080 без использования HTTPS, но с подключением к почтовому серверу. Для запуска без использования почты используется ключ `--no-smtp`. Для изменения порта используется ключ `--port <n>` (Для запуска на стандартном для HTTP порту 80 скорее всего потребуются права администратора). Для запуска с HTTPS используется ключ `--https`. Если сервер будет доступен по определенному адресу (например, [collaboration.pw](collaboration.pw)), можно указать его как `--domain collaboration.pw`, чтобы в письмах на регистрацию указывался корректный адрес подтверждения почты.
```
  cd build
  sudo ./collab-server --remake-db --no-smtp
```
3. По умолчанию сервер инициализирует 4 подключения к БД для совместного использования всеми потоками. Можно регулировать это количество с помощью ключа `--db-connections 8`. Не рекомендуется использование менее 4 подключений.
4. Для вывода в лог исполняемых запросов к БД используется ключ `--log-sql`. Для вывода в лог запросов и ответов API используются ключи `--log-api-in` и `--log-api-out` (или общий `--log-api`). Учтите, что использование этих ключей будет выводить пароли и ID пользовательских сессий в лог.
5. После запуска по адресу `http://localhost:8080/` (для порта 8080) будет доступен веб-интерфейс базы данных.
6. Полный список возможных ключей доступен с помощью `./collab-server --help`.


