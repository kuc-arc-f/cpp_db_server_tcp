#include <cstring>
#include <future>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sqlite3.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <mutex>
#include <netinet/in.h>
#include <thread>
#include <vector>
#include <algorithm>
#include <uuid/uuid.h>
#include <nlohmann/json.hpp>

#include "include/models.hpp"
#include "include/BackupDb.hpp"

using namespace std;
// JSON用エイリアス
using json = nlohmann::json;

std::string BACKUP_DB_PATH = "./data/backup.db";
std::string BACKUP_SQL_PATH = "./data/backup.sql";
std::vector<QueItem> VecQue;

std::string readFileToString(const std::string& filePath) {
    std::ifstream file(filePath);
    
    // ファイルが開けたか確認
    if (!file.is_open()) {
        std::cerr << "エラー: ファイルを開けませんでした -> " << filePath << std::endl;
        return "";
    }

    // ファイルバッファを stringstream に読み込む
    std::ostringstream ss;
    ss << file.rdbuf();
    
    return ss.str();
}

class TodoDatabase {
private:
    sqlite3* db;
    
public:
    TodoDatabase(const std::string& dbPath = "todos.db") {
        int rc = sqlite3_open(":memory:", &db);
    }
    ~TodoDatabase() {
        if (db) {
            sqlite3_close(db);
        }
    }

    bool init_import(const char* sql) {
        try{    
            char* errMsg = nullptr;

            if (sqlite3_exec(db, sql, nullptr, nullptr, &errMsg)
                != SQLITE_OK) {

                std::cerr << "error: "
                        << errMsg << std::endl;

                sqlite3_free(errMsg);
            }
            std::cout << "init_import , completed." << std::endl;            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "error:" << e.what() << std::endl;
            return false;
        }
    }       
    bool executeSql(const std::string& sql) {
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }        
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }

    json selectTableSql(const string& tableName, const std::string& sql ) {
        json result;
        result["table"] = tableName;
        result["status"] = "success";

        std::cout << "selectTableSql.sql=" << sql << "\n";            

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

    // 登録（INSERT）
    bool insertTodo(const std::string& title, const std::string& description = "") {
        std::string sql = "INSERT INTO todos (title, description) VALUES (?, ?);";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_text(stmt, 1, title.c_str(), -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, description.c_str(), -1, SQLITE_STATIC);
        
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }
    
    // 一覧取得（SELECT）
    std::vector<std::string> getAllTodos() {
        std::vector<std::string> todos;
        const char* sql = "SELECT id, title, description, created_at, completed FROM todos ORDER BY created_at DESC;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return todos;
        }
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* title = sqlite3_column_text(stmt, 1);
            const unsigned char* description = sqlite3_column_text(stmt, 2);
            const unsigned char* created_at = sqlite3_column_text(stmt, 3);
            int completed = sqlite3_column_int(stmt, 4);
            
            std::stringstream ss;
            ss << "ID: " << id 
               << ", Title: " << (title ? reinterpret_cast<const char*>(title) : "")
               << ", Description: " << (description ? reinterpret_cast<const char*>(description) : "")
               << ", Created: " << (created_at ? reinterpret_cast<const char*>(created_at) : "")
               << ", Completed: " << (completed ? "Yes" : "No");
            todos.push_back(ss.str());
        }
        
        sqlite3_finalize(stmt);
        return todos;
    }
    
    // 削除（DELETE）
    bool deleteTodo(int id) {
        std::string sql = "DELETE FROM todos WHERE id = ?;";
        sqlite3_stmt* stmt;
        
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            return false;
        }
        
        sqlite3_bind_int(stmt, 1, id);
        bool success = (sqlite3_step(stmt) == SQLITE_DONE);
        sqlite3_finalize(stmt);
        return success;
    }
    
    // JSON形式で一覧取得（API用）
    std::string getTodosAsJSON() {
        std::stringstream json;
        json << "[";
        
        const char* sql = "SELECT id, title, description, created_at, completed FROM todos ORDER BY created_at DESC;";
        sqlite3_stmt* stmt;
        if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
            return "[]";
        }
        
        bool first = true;
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            if (!first) json << ",";
            first = false;
            
            int id = sqlite3_column_int(stmt, 0);
            const unsigned char* title = sqlite3_column_text(stmt, 1);
            const unsigned char* description = sqlite3_column_text(stmt, 2);
            const unsigned char* created_at = sqlite3_column_text(stmt, 3);
            int completed = sqlite3_column_int(stmt, 4);
            
            json << "{"
                 << "\"id\":" << id << ","
                 << "\"title\":\"" << (title ? reinterpret_cast<const char*>(title) : "") << "\","
                 << "\"description\":\"" << (description ? reinterpret_cast<const char*>(description) : "") << "\","
                 << "\"created_at\":\"" << (created_at ? reinterpret_cast<const char*>(created_at) : "") << "\","
                 << "\"completed\":" << (completed ? "true" : "false")
                 << "}";
        }
        
        sqlite3_finalize(stmt);
        json << "]";
        return json.str();
    }
};
TodoDatabase todoDb;

void backup_save() {
    try{    
        if (VecQue.size() > 0) {
            BackupDb bLib(BACKUP_DB_PATH);
            QueItem item = VecQue[0];
            //std::cout << "uuid=" << item.uuid << std::endl;
            VecQue.erase(VecQue.begin());
            bLib.executeSql(item.sql);
        }
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return;
    }
}

class TCPServer {
private:
    int server_fd;
    int port;
    bool running;
    std::vector<int> client_sockets;
    
public:
    TCPServer(int port) : port(port), running(false) {
        server_fd = -1;
    }
    
    ~TCPServer() {
        stop();
    }
    
    bool start() {
        // ソケット作成
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "ソケット作成失敗" << std::endl;
            return false;
        }
        
        // ソケットオプション設定（アドレス再利用）
        int opt = 1;
        if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
            std::cerr << "setsockopt失敗" << std::endl;
            close(server_fd);
            return false;
        }
        
        // アドレス設定
        struct sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);
        
        // バインド
        if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "バインド失敗" << std::endl;
            close(server_fd);
            return false;
        }
        
        // リスン
        if (listen(server_fd, 10) < 0) {
            std::cerr << "リスン失敗" << std::endl;
            close(server_fd);
            return false;
        }
        
        running = true;
        std::cout << "サーバー起動: ポート " << port << " で待受中..." << std::endl;
        
        // クライアント接続受付スレッド開始
        std::thread accept_thread(&TCPServer::acceptClients, this);
        accept_thread.detach();
        
        return true;
    }
    
    void stop() {
        running = false;
        
        // 全てのクライアント接続を閉じる
        for (int client_fd : client_sockets) {
            close(client_fd);
        }
        client_sockets.clear();
        
        if (server_fd >= 0) {
            close(server_fd);
            server_fd = -1;
        }
        
        std::cout << "サーバー停止" << std::endl;
    }
    
private:
    void acceptClients() {
        while (running) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            
            // クライアント接続待機
            int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);
            if (client_fd < 0) {
                if (running) {
                    std::cerr << "接続受付エラー" << std::endl;
                }
                continue;
            }
            
            // クライアント情報表示
            char client_ip[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
            std::cout << "クライアント接続: " << client_ip << ":" << ntohs(client_addr.sin_port) << std::endl;
            
            // クライアントソケットをリストに追加
            client_sockets.push_back(client_fd);
            
            // クライアント処理スレッド開始
            std::thread client_thread(&TCPServer::handleClient, this, client_fd);
            client_thread.detach();
        }
    }
    
    void handleClient(int client_fd) {
        char buffer[1024];
        
        while (running) {
            memset(buffer, 0, sizeof(buffer));
            
            // データ受信
            int bytes_read = read(client_fd, buffer, sizeof(buffer) - 1);
            
            if (bytes_read <= 0) {
                if (bytes_read == 0) {
                    std::cout << "クライアント切断" << std::endl;
                } else {
                    std::cerr << "受信エラー" << std::endl;
                }
                break;
            }
            
            // 受信データ表示
            std::cout << "受信: " << buffer;
            std::string body = buffer;
            std::cout << "body=" << body << std::endl;

            json j1 = json::parse(body);
            std::string action_name = j1.at("action_name").get<std::string>();
            std::cout << "action_name=" << action_name << "\n";            
            std::string table_name = j1.at("table").get<std::string>();
            std::cout << "table_name=" << table_name << "\n";            
            std::string sql = j1.at("sql").get<std::string>();
            std::cout << "sql=" << sql << "\n";            

            std::string outStr = "";
            if (action_name == "select") {
                json j2 = todoDb.selectTableSql(table_name, sql);
                std::string json_str = j2.dump();
                outStr = json_str;
                std::cout << json_str << std::endl;
            } else{
                uuid_t uuid;
                char uuid_str[37];
                uuid_generate(uuid);
                uuid_unparse(uuid, uuid_str);
                std::cout << "UUID: " << uuid_str << std::endl;
                QueItem que;
                que.uuid = uuid_str;
                que.sql = sql;
                VecQue.push_back(que);
                std::cout << "VecQue.size=" << VecQue.size() << std::endl;

                bool success = todoDb.executeSql(sql);
                outStr = body;
                std::cout << "outStr=" << outStr << std::endl;
            }

            // エコー応答（大文字に変換）
            std::string response = outStr;
            
            // データ送信
            int bytes_sent = write(client_fd, response.c_str(), response.length());
            if (bytes_sent < 0) {
                std::cerr << "送信エラー" << std::endl;
                break;
            }
        }
        
        // クライアント切断処理
        close(client_fd);
        auto it = std::find(client_sockets.begin(), client_sockets.end(), client_fd);
        if (it != client_sockets.end()) {
            client_sockets.erase(it);
        }
    }
};

int main(int argc, char* argv[]) {
    int port = 8080;
    if (argc > 1) {
        port = std::atoi(argv[1]);
    }

    TCPServer server(port);
    if (!server.start()) {
        return -1;
    }
    try{    
        std::string content = readFileToString(BACKUP_SQL_PATH);
        std::cout << "--- SQL-FILE-TEXT ---" << std::endl;
        std::cout << content << std::endl;  
        bool ret = todoDb.init_import(content.c_str());  
        std::cout << "todoDb.init_import.ret=" << ret << std::endl;  
    } catch (const std::exception& e) {
        std::cerr << "error: " << e.what() << std::endl;
        return -1;
    }
    
    // メインスレッドを維持
    std::cout << "サーバー実行中... (Ctrl+Cで終了)" << std::endl;
    std::cout << "port=" << port << std::endl;

    while (true) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        backup_save();
    }
    
    return 0;
}