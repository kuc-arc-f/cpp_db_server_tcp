#!/bin/bash

sqlite3 ./data/backup.db .dump > ./data/backup.sql

./tcp_server 8888
