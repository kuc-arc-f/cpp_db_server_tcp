# cpp_db_server_tcp

 Version: 0.9.3

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

* TCP client

https://github.com/kuc-arc-f/cpp_16ex/tree/main/tcp_cl_2

* DB tool , TUI node

https://github.com/kuc-arc-f/cpp_16ex/tree/main/tui_tcp_1

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
clang++ -std=c++11 -pthread tcp_server.cpp -o tcp_server -lsqlite3 -luuid -lspdlog -lfmt
```

* start
```
./start.sh

```
***

* (Option) Log file Out , app.log 
```
export LOG_FILE_WRITE=1
```

***
### version

* V_0_9_3: fix , fix, update receive
* V_0_9_2: fix , backup.db save
* V_0_9_1: new

***
### blog

https://zenn.dev/knaka0209/scraps/c669556d8dceeb

