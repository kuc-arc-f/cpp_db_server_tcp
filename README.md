# cpp_db_server_tcp

 Version: 0.9.2

 date    : 2026/09/09
 
 update :

***

C++ DB Server TCP , IN memory database SQLite

* LLVM CLang
* sqlite3 use

***
* Speed INSERT 1,000 record , 181 msec

![img1](/images/cpp_db_server_tcp.png)

* related blog
* https://zenn.dev/link/comments/d44f1840ee714f

***
### related Client TCP

https://github.com/kuc-arc-f/cpp_16ex/tree/main/tcp_cl_2

***
* LIB add
```
sudo apt update
sudo apt-get install libsqlite3-dev
sudo apt-get install nlohmann-json3-dev
sudo apt install libspdlog-dev libfmt-dev
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
### version

* V_0_9_1: new

***
### blog

https://zenn.dev/knaka0209/scraps/c669556d8dceeb

