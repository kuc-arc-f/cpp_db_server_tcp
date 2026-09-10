#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

using json = nlohmann::json;
using namespace std;

class SQLiteJSONExporter {
private:
    sqlite3* db;
    string dbPath;

public:
    SQLiteJSONExporter(const string& path){
        int rc = sqlite3_open(":memory:", &db);
    }
    ~SQLiteJSONExporter() {
        if (db) {
            sqlite3_close(db);
        }
    }
    /*
    bool connect() {
        int rc = sqlite3_open(dbPath.c_str(), &db);
        if (rc != SQLITE_OK) {
            cerr << "データベース接続エラー: " << sqlite3_errmsg(db) << endl;
            return false;
        }
        return true;
    }
    */
    // テーブルデータをJSONに変換
    json selectTableToJSON(const string& tableName) {
        json result;
        result["table"] = tableName;
        result["status"] = "success";

        string sql = "SELECT * FROM " + tableName + ";";
        std::cout << "selectTableToJSON.sql=" << sql << "\n";            

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc != SQLITE_OK) {
            result["status"] = "error";
            result["error"] = sqlite3_errmsg(db);
            return result;
        }

        // カラム情報を取得
        int columnCount = sqlite3_column_count(stmt);
        vector<string> columnNames;

        for (int i = 0; i < columnCount; i++) {
            columnNames.push_back(sqlite3_column_name(stmt, i));
        }

        // データ行を取得
        json rows = json::array();

        while (sqlite3_step(stmt) == SQLITE_ROW) {
            json row;

            for (int i = 0; i < columnCount; i++) {
                const string colName = columnNames[i];
                int colType = sqlite3_column_type(stmt, i);

                switch (colType) {
                    case SQLITE_INTEGER:
                        row[colName] = sqlite3_column_int64(stmt, i);
                        break;
                    case SQLITE_FLOAT:
                        row[colName] = sqlite3_column_double(stmt, i);
                        break;
                    case SQLITE_TEXT:
                        row[colName] = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                        break;
                    case SQLITE_BLOB:
                        // BLOBデータはBase64などに変換するか、文字列として扱う
                        row[colName] = "[BLOBデータ]";
                        break;
                    case SQLITE_NULL:
                    default:
                        row[colName] = nullptr;
                        break;
                }
            }

            rows.push_back(row);
        }

        sqlite3_finalize(stmt);

        result["columns"] = columnNames;
        result["row_count"] = rows.size();
        result["data"] = rows;

        return result;
    }

    // テーブル一覧を取得
    vector<string> getTableList() {
        vector<string> tables;
        string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%';";

        sqlite3_stmt* stmt;
        int rc = sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr);

        if (rc == SQLITE_OK) {
            while (sqlite3_step(stmt) == SQLITE_ROW) {
                tables.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
            }
        }

        sqlite3_finalize(stmt);
        return tables;
    }
};
