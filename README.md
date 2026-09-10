# cpp_db_server_tcp

 Version: 0.9.1

 date    : 2026/09/09
 
 update :

***

C++ DB Server TCP , IN memory database SQLite

* LLVM CLang
* sqlite3 use

***
### related Client TCP

https://github.com/kuc-arc-f/cpp_15ex/tree/main/tcp_cl_2

***
* LIB add
```
sudo apt-get install libsqlite3-dev
sudo apt-get install nlohmann-json3-dev
```
***
* table add
```
sqlite3 ./data/backup.db < table.sql
```
***
* build
```
clang++ -std=c++11 -pthread tcp_server.cpp -o tcp_server -lsqlite3 -luuid
```

* start
```
./start.sh

```

***
### blog

https://zenn.dev/knaka0209/scraps/c669556d8dceeb

