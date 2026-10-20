//
// Created by PAWEL on 15/10/2026.
//

#ifndef LORIKEET_DATABASE_H
#define LORIKEET_DATABASE_H
#include <functional>
#include <string>
#include <vector>

#include "Json.h"

struct sqlite3; // Forward declaration of native SQLite handle

namespace HTTPServer {
    class Database;

    class TableProxy { // Ergonomic fluent table accessor supporting direct CRUD operations
        public:
            TableProxy(Database& db, std::string tableName);
            [[nodiscard]] Json::Array select(const Json::Object& where = {}) const;
            [[nodiscard]] Json::Array find(const Json::Object& where = {}) const;
            Json::Object insert(const Json::Object& record);
            Json::Array update(const Json::Object& updates, const Json::Object& where = {});
            Json::Array remove(const Json::Object& where = {});
        private:
            Database& _db;
            std::string _table;
    };

    class Database { // High-performance embedded SQLite database engine with WAL concurrency
        public:
            explicit Database(std::string path, bool createIfNotExists = false);
            ~Database();

            Database(const Database&) = delete;
            Database& operator=(const Database&) = delete;

            Database(Database&& other) noexcept;
            Database& operator=(Database&& other) noexcept;

            const std::string& getPath() const;
            bool isMemory() const;
            size_t collectionCount() const;
            bool hasTable(const std::string& tableName) const;
            TableProxy table(std::string tableName);

            // Native SQLite SQL execution primitives (Phase 2)
            void execute(const std::string& sql);
            Json::Array query(const std::string& sql, const std::vector<Json>& params = {}) const;
            int executeUpdate(const std::string& sql, const std::vector<Json>& params = {});
            long long lastInsertId() const;
            void transaction(const std::function<void()>& work);
            sqlite3* getHandle() const;
            void close();

            // Table CRUD & Schema operations
            bool createTable(const std::string& tableName, const Json::Object& scheme);
            Json::Object insert(const std::string& tableName, const Json::Object& record);
            Json::Array select(const std::string& tableName, const Json::Object& where = {}) const;
            Json::Array select(const std::string& tableName, const std::function<bool(const Json::Object&)>& where) const;
            Json::Array update(const std::string& tableName, const Json::Object& updates, const Json::Object& where = {});
            Json::Array update(const std::string& tableName, const Json::Object& updates, const std::function<bool(const Json::Object&)>& where);
            Json::Array remove(const std::string& tableName, const Json::Object& where = {});
            Json::Array remove(const std::string& tableName, const std::function<bool(const Json::Object&)>& where);
            void save() const;

        private:
            std::string _path;
            bool _createIfNotExists;
            bool _isMemory = false;
            sqlite3* _db = nullptr;

            void _initPragmas();
            static void _validateScheme(const Json::Object& scheme);
            static bool _matchesType(const Json& value, const std::string& typeName);
            Json::Object _getTableScheme(const std::string& tableName) const;
            static std::string _mapTypeToSql(const std::string& typeName);
    };
} // HTTPServer

#endif //LORIKEET_DATABASE_H
