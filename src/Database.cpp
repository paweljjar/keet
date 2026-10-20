//
// Created by PAWEL on 15/10/2026.
//

#include "keet/Database.h"
#include "keet/sqlite3.h"

#include <cmath>
#include <filesystem>
#include <iostream>
#include <ranges>
#include <set>
#include <sstream>
#include <vector>

namespace HTTPServer {
    namespace {
        void _bindParams(sqlite3_stmt* stmt, const std::vector<Json>& params) {
            for (size_t i = 0; i < params.size(); ++i) {
                int idx = static_cast<int>(i + 1);
                const auto& p = params[i];
                if (p.isNull()) {
                    sqlite3_bind_null(stmt, idx);
                } else if (p.isBool()) {
                    sqlite3_bind_int(stmt, idx, p.asBool() ? 1 : 0);
                } else if (p.isNumber()) {
                    double num = p.asNumber();
                    if (std::isfinite(num) && std::floor(num) == num) {
                        sqlite3_bind_int64(stmt, idx, static_cast<sqlite3_int64>(num));
                    } else {
                        sqlite3_bind_double(stmt, idx, num);
                    }
                } else if (p.isString()) {
                    const std::string& str = p.asString();
                    sqlite3_bind_text(stmt, idx, str.c_str(), static_cast<int>(str.size()), SQLITE_TRANSIENT);
                } else {
                    std::string dump = p.dump();
                    sqlite3_bind_text(stmt, idx, dump.c_str(), static_cast<int>(dump.size()), SQLITE_TRANSIENT);
                }
            }
        }

        struct StmtGuard {
            sqlite3_stmt* stmt = nullptr;
            ~StmtGuard() {
                if (stmt) sqlite3_finalize(stmt);
            }
        };
    }

    Database::Database(std::string path, const bool createIfNotExists)
        : _path(std::move(path)), _createIfNotExists(createIfNotExists) {
        if (_path == ":memory:" || _path.empty()) {
            _isMemory = true;
            if (_path.empty()) _path = ":memory:";
            int rc = sqlite3_open_v2(":memory:", &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
            if (rc != SQLITE_OK) {
                std::string err = _db ? sqlite3_errmsg(_db) : "Unknown SQLite error";
                if (_db) { sqlite3_close_v2(_db); _db = nullptr; }
                throw std::runtime_error("Failed to open in-memory database: " + err);
            }
            _initPragmas();
            return;
        }

        _isMemory = false;
        bool exists = std::filesystem::exists(_path);
        if (!exists) {
            if (!_createIfNotExists) {
                throw std::runtime_error("Database file not found: " + _path);
            }
            std::filesystem::path p(_path);
            if (p.has_parent_path()) {
                std::filesystem::create_directories(p.parent_path());
            }
        }

        int rc = sqlite3_open_v2(_path.c_str(), &_db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX, nullptr);
        if (rc != SQLITE_OK) {
            std::string err = _db ? sqlite3_errmsg(_db) : "Unknown SQLite error";
            if (_db) { sqlite3_close_v2(_db); _db = nullptr; }
            throw std::runtime_error("Cannot open database file: " + _path + " (" + err + ")");
        }

        _initPragmas();
    }

    void Database::_initPragmas() {
        if (!_db) return;
        execute("PRAGMA busy_timeout=5000;");
        if (!_isMemory) {
            try {
                execute("PRAGMA journal_mode=WAL;");
            } catch (...) {
                // Ignore busy if another connection already configured WAL
            }
            execute("PRAGMA synchronous=NORMAL;");
        }
        execute("PRAGMA foreign_keys=ON;");
        execute("CREATE TABLE IF NOT EXISTS _keet_schemas (table_name TEXT PRIMARY KEY, schema_json TEXT);");
    }

    void Database::close() {
        if (_db) {
            sqlite3_close_v2(_db);
            _db = nullptr;
        }
    }

    Database::~Database() {
        close();
    }

    Database::Database(Database&& other) noexcept
        : _path(std::move(other._path)),
          _createIfNotExists(other._createIfNotExists),
          _isMemory(other._isMemory),
          _db(other._db) {
        other._db = nullptr;
    }

    Database& Database::operator=(Database&& other) noexcept {
        if (this != &other) {
            close();
            _path = std::move(other._path);
            _createIfNotExists = other._createIfNotExists;
            _isMemory = other._isMemory;
            _db = other._db;
            other._db = nullptr;
        }
        return *this;
    }

    const std::string& Database::getPath() const {
        return _path;
    }

    bool Database::isMemory() const {
        return _isMemory;
    }

    sqlite3* Database::getHandle() const {
        return _db;
    }

    void Database::execute(const std::string& sql) {
        if (!_db) throw std::runtime_error("Database connection is closed");
        char* errMsg = nullptr;
        int rc = sqlite3_exec(_db, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::string err = errMsg ? errMsg : "Unknown SQLite error";
            sqlite3_free(errMsg);
            throw std::runtime_error("SQLite execute error (" + std::to_string(rc) + "): " + err + " [SQL: " + sql + "]");
        }
    }

    Json::Array Database::query(const std::string& sql, const std::vector<Json>& params) const {
        if (!_db) throw std::runtime_error("Database connection is closed");
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("SQLite prepare error (" + std::to_string(rc) + "): " + sqlite3_errmsg(_db) + " [SQL: " + sql + "]");
        }
        StmtGuard guard{stmt};
        _bindParams(stmt, params);

        Json::Array results;
        int colCount = sqlite3_column_count(stmt);
        while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
            Json::Object row;
            for (int c = 0; c < colCount; ++c) {
                const char* colName = sqlite3_column_name(stmt, c);
                std::string key = colName ? colName : "";
                int type = sqlite3_column_type(stmt, c);
                if (type == SQLITE_INTEGER) {
                    row[key] = static_cast<double>(sqlite3_column_int64(stmt, c));
                } else if (type == SQLITE_FLOAT) {
                    row[key] = sqlite3_column_double(stmt, c);
                } else if (type == SQLITE_TEXT) {
                    const char* text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, c));
                    std::string s = text ? text : "";
                    if ((s.starts_with("{") && s.ends_with("}")) || (s.starts_with("[") && s.ends_with("]"))) {
                        try {
                            row[key] = Json::parse(s);
                        } catch (...) {
                            row[key] = s;
                        }
                    } else {
                        row[key] = s;
                    }
                } else if (type == SQLITE_BLOB) {
                    const void* blob = sqlite3_column_blob(stmt, c);
                    int bytes = sqlite3_column_bytes(stmt, c);
                    row[key] = std::string(reinterpret_cast<const char*>(blob), bytes);
                } else {
                    row[key] = nullptr;
                }
            }
            results.push_back(std::move(row));
        }
        if (rc != SQLITE_DONE) {
            throw std::runtime_error("SQLite step error (" + std::to_string(rc) + "): " + sqlite3_errmsg(_db));
        }
        return results;
    }

    int Database::executeUpdate(const std::string& sql, const std::vector<Json>& params) {
        if (!_db) throw std::runtime_error("Database connection is closed");
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(_db, sql.c_str(), static_cast<int>(sql.size()), &stmt, nullptr);
        if (rc != SQLITE_OK) {
            throw std::runtime_error("SQLite prepare error (" + std::to_string(rc) + "): " + sqlite3_errmsg(_db) + " [SQL: " + sql + "]");
        }
        StmtGuard guard{stmt};
        _bindParams(stmt, params);

        rc = sqlite3_step(stmt);
        if (rc != SQLITE_DONE && rc != SQLITE_ROW) {
            throw std::runtime_error("SQLite step error (" + std::to_string(rc) + "): " + sqlite3_errmsg(_db) + " [SQL: " + sql + "]");
        }
        return sqlite3_changes(_db);
    }

    long long Database::lastInsertId() const {
        if (!_db) return 0;
        return static_cast<long long>(sqlite3_last_insert_rowid(_db));
    }

    void Database::transaction(const std::function<void()>& work) {
        if (!work) return;
        execute("BEGIN IMMEDIATE;");
        try {
            work();
            execute("COMMIT;");
        } catch (...) {
            try {
                execute("ROLLBACK;");
            } catch (...) {
                // Rollback failure ignored
            }
            throw;
        }
    }

    bool Database::hasTable(const std::string& tableName) const {
        if (!_db) return false;
        Json::Array res = query("SELECT 1 FROM sqlite_master WHERE type='table' AND name=? LIMIT 1;", {tableName});
        return !res.empty();
    }

    size_t Database::collectionCount() const {
        if (!_db) return 0;
        Json::Array res = query("SELECT count(*) as cnt FROM sqlite_master WHERE type='table' AND name NOT LIKE 'sqlite_%' AND name != '_keet_schemas';");
        if (!res.empty()) {
            const auto& obj = res[0].asObject();
            if (obj.contains("cnt") && obj.at("cnt").isNumber()) {
                return static_cast<size_t>(obj.at("cnt").asNumber());
            }
        }
        return 0;
    }

    void Database::save() const {
        if (_db && !_isMemory) {
            sqlite3_wal_checkpoint_v2(_db, nullptr, SQLITE_CHECKPOINT_PASSIVE, nullptr, nullptr);
        }
    }

    std::string Database::_mapTypeToSql(const std::string& typeName) {
        if (typeName == "integer") return "INTEGER";
        if (typeName == "float") return "REAL";
        if (typeName == "string") return "TEXT";
        if (typeName == "blob") return "BLOB";
        if (typeName == "boolean") return "INTEGER";
        if (typeName == "array") return "TEXT";
        if (typeName == "object") return "TEXT";
        return "TEXT";
    }

    void Database::_validateScheme(const Json::Object& scheme) {
        if (scheme.empty()) {
            throw std::invalid_argument("Table scheme cannot be empty");
        }

        static const std::set<std::string> allowedTypes = {
            "integer", "string", "boolean", "float", "blob", "array", "object"
        };
        static const std::set<std::string> booleanOptions = {
            "required", "primary_key", "auto_increment", "unique"
        };

        for (const auto& [columnName, columnDef] : scheme) {
            if (columnName.empty()) {
                throw std::invalid_argument("Column name cannot be empty");
            }

            if (!columnDef.isObject()) {
                throw std::invalid_argument("Column '" + columnName + "' definition must be a JSON object");
            }

            const Json::Object& colObj = columnDef.asObject();
            auto typeIt = colObj.find("type");
            if (typeIt == colObj.end() || !typeIt->second.isString()) {
                throw std::invalid_argument("Column '" + columnName + "' must have a string 'type' property");
            }

            const std::string& typeName = typeIt->second.asString();
            if (!allowedTypes.contains(typeName)) {
                throw std::invalid_argument("Column '" + columnName + "' has unsupported type '" + typeName + "'");
            }

            for (const auto& [optKey, optVal] : colObj) {
                if (booleanOptions.contains(optKey) && !optVal.isBool()) {
                    throw std::invalid_argument("Option '" + optKey + "' in column '" + columnName + "' must be a boolean");
                }
            }

            if (colObj.contains("auto_increment") && colObj.at("auto_increment").asBool() && typeName != "integer") {
                throw std::invalid_argument("Column '" + columnName + "' with 'auto_increment' must have type 'integer'");
            }
        }
    }

    bool Database::_matchesType(const Json& value, const std::string& typeName) {
        if (typeName == "integer") {
            return value.isNumber() && std::isfinite(value.asNumber()) && std::floor(value.asNumber()) == value.asNumber();
        }
        if (typeName == "float") {
            return value.isNumber() && std::isfinite(value.asNumber());
        }
        if (typeName == "string" || typeName == "blob") {
            return value.isString();
        }
        if (typeName == "boolean") {
            return value.isBool() || (value.isNumber() && (value.asNumber() == 0.0 || value.asNumber() == 1.0));
        }
        if (typeName == "array") {
            return value.isArray();
        }
        if (typeName == "object") {
            return value.isObject();
        }
        return false;
    }

    Json::Object Database::_getTableScheme(const std::string& tableName) const {
        Json::Array res = query("SELECT schema_json FROM _keet_schemas WHERE table_name=? LIMIT 1;", {tableName});
        if (!res.empty()) {
            const auto& obj = res[0].asObject();
            if (obj.contains("schema_json")) {
                const auto& val = obj.at("schema_json");
                if (val.isObject()) {
                    return val.asObject();
                }
                if (val.isString()) {
                    try {
                        Json parsed = Json::parse(val.asString());
                        if (parsed.isObject()) {
                            return parsed.asObject();
                        }
                    } catch (...) {}
                }
            }
        }
        return {};
    }

    Json::Object Database::_formatRowWithScheme(Json::Object row, const Json::Object& scheme) {
        if (scheme.empty()) return row;
        for (const auto& [colName, colDef] : scheme) {
            if (!row.contains(colName)) continue;
            const Json::Object& colObj = colDef.asObject();
            const std::string& typeName = colObj.at("type").asString();
            Json& val = row[colName];
            if (val.isNull()) continue;

            if (typeName == "boolean") {
                if (val.isNumber()) {
                    val = (val.asNumber() != 0.0);
                }
            } else if (typeName == "array" && val.isString()) {
                try {
                    Json parsed = Json::parse(val.asString());
                    if (parsed.isArray()) val = std::move(parsed);
                } catch (...) {}
            } else if (typeName == "object" && val.isString()) {
                try {
                    Json parsed = Json::parse(val.asString());
                    if (parsed.isObject()) val = std::move(parsed);
                } catch (...) {}
            }
        }
        return row;
    }

    bool Database::createTable(const std::string& tableName, const Json::Object& scheme) {
        if (tableName.empty()) {
            throw std::invalid_argument("Table name cannot be empty");
        }

        _validateScheme(scheme);

        if (hasTable(tableName)) {
            return false;
        }

        std::ostringstream sql;
        sql << "CREATE TABLE \"" << tableName << "\" (";
        bool first = true;
        for (const auto& [colName, colDef] : scheme) {
            if (!first) sql << ", ";
            first = false;

            const auto& colObj = colDef.asObject();
            const std::string& typeName = colObj.at("type").asString();
            std::string sqlType = _mapTypeToSql(typeName);

            const bool isAutoInc = colObj.contains("auto_increment") && colObj.at("auto_increment").asBool();
            const bool isPk = colObj.contains("primary_key") && colObj.at("primary_key").asBool();
            const bool isReq = (colObj.contains("required") && colObj.at("required").asBool()) || isPk;
            const bool isUniq = (colObj.contains("unique") && colObj.at("unique").asBool()) || isPk;

            sql << "\"" << colName << "\" " << sqlType;
            if (isPk) {
                sql << " PRIMARY KEY";
                if (isAutoInc) {
                    sql << " AUTOINCREMENT";
                }
            } else {
                if (isReq) sql << " NOT NULL";
                if (isUniq) sql << " UNIQUE";
            }
        }
        sql << ");";

        execute(sql.str());
        executeUpdate("INSERT OR REPLACE INTO _keet_schemas (table_name, schema_json) VALUES (?, ?);", {tableName, Json(scheme).dump()});
        return true;
    }

    Json::Object Database::insert(const std::string& tableName, const Json::Object& record) {
        if (tableName.empty()) {
            throw std::invalid_argument("Table name cannot be empty");
        }
        if (!hasTable(tableName)) {
            throw std::invalid_argument("Table '" + tableName + "' does not exist");
        }

        Json::Object scheme = _getTableScheme(tableName);

        for (const auto& key : record | std::views::keys) {
            if (!scheme.empty() && !scheme.contains(key)) {
                throw std::invalid_argument("Unknown column '" + key + "' in table '" + tableName + "'");
            }
        }

        Json::Object newRecord;
        std::vector<std::string> cols;
        std::vector<Json> params;
        std::string autoIncCol;

        for (const auto& [columnName, columnDef] : scheme) {
            const Json::Object& colObj = columnDef.asObject();
            const std::string& typeName = colObj.at("type").asString();
            const bool isAutoIncrement = colObj.contains("auto_increment") && colObj.at("auto_increment").asBool();
            const bool isPrimaryKey = colObj.contains("primary_key") && colObj.at("primary_key").asBool();
            const bool isRequired = (colObj.contains("required") && colObj.at("required").asBool()) || isPrimaryKey;

            if (isAutoIncrement) {
                if (record.contains(columnName)) {
                    throw std::invalid_argument("Column '" + columnName + "' is auto_increment and cannot be manually specified");
                }
                autoIncCol = columnName;
                continue;
            }

            if (!record.contains(columnName) || record.at(columnName).isNull()) {
                if (isRequired) {
                    throw std::invalid_argument("Missing required column '" + columnName + "' in table '" + tableName + "'");
                }
                if (record.contains(columnName)) {
                    newRecord[columnName] = nullptr;
                    cols.push_back(columnName);
                    params.push_back(Json(nullptr));
                }
                continue;
            }

            const Json& val = record.at(columnName);
            if (!_matchesType(val, typeName)) {
                throw std::invalid_argument("Column '" + columnName + "' expects type '" + typeName + "'");
            }

            newRecord[columnName] = val;
            cols.push_back(columnName);
            params.push_back(val);
        }

        if (scheme.empty()) {
            for (const auto& [colName, val] : record) {
                newRecord[colName] = val;
                cols.push_back(colName);
                params.push_back(val);
            }
        }

        std::ostringstream sql;
        sql << "INSERT INTO \"" << tableName << "\" (";
        for (size_t i = 0; i < cols.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << "\"" << cols[i] << "\"";
        }
        sql << ") VALUES (";
        for (size_t i = 0; i < cols.size(); ++i) {
            if (i > 0) sql << ", ";
            sql << "?";
        }
        sql << ");";

        try {
            executeUpdate(sql.str(), params);
        } catch (const std::exception& e) {
            std::string msg = e.what();
            if (msg.find("UNIQUE") != std::string::npos || msg.find("constraint failed") != std::string::npos) {
                throw std::invalid_argument("Unique constraint violation: " + msg);
            }
            throw;
        }

        if (!autoIncCol.empty()) {
            newRecord[autoIncCol] = static_cast<double>(lastInsertId());
        }

        return _formatRowWithScheme(std::move(newRecord), scheme);
    }

    Json::Array Database::select(const std::string& tableName, const Json::Object& where) const {
        if (tableName.empty()) {
            throw std::invalid_argument("Table name cannot be empty");
        }
        if (!hasTable(tableName)) {
            throw std::invalid_argument("Table '" + tableName + "' does not exist");
        }

        Json::Object scheme = _getTableScheme(tableName);
        if (!where.empty() && !scheme.empty()) {
            for (const auto& [key, val] : where) {
                if (!scheme.contains(key)) {
                    throw std::invalid_argument("Unknown column '" + key + "' in table '" + tableName + "'");
                }
                const std::string& typeName = scheme.at(key).asObject().at("type").asString();
                if (!val.isNull() && !_matchesType(val, typeName)) {
                    throw std::invalid_argument("Filter value for column '" + key + "' expects type '" + typeName + "'");
                }
            }
        }

        std::ostringstream sql;
        sql << "SELECT * FROM \"" << tableName << "\"";
        std::vector<Json> params;
        if (!where.empty()) {
            sql << " WHERE ";
            bool first = true;
            for (const auto& [key, val] : where) {
                if (!first) sql << " AND ";
                first = false;
                if (val.isNull()) {
                    sql << "\"" << key << "\" IS NULL";
                } else {
                    sql << "\"" << key << "\" = ?";
                    params.push_back(val);
                }
            }
        }
        sql << ";";

        Json::Array rows = query(sql.str(), params);
        for (auto& row : rows) {
            row = _formatRowWithScheme(row.asObject(), scheme);
        }
        return rows;
    }

    Json::Array Database::select(const std::string& tableName, const std::function<bool(const Json::Object&)>& where) const {
        if (!where) throw std::invalid_argument("Filter callback cannot be null");
        Json::Array allRows = select(tableName, Json::Object{});
        Json::Array matched;
        for (const auto& row : allRows) {
            if (where(row.asObject())) {
                matched.push_back(row);
            }
        }
        return matched;
    }

    size_t Database::count(const std::string& tableName, const Json::Object& where) const {
        if (tableName.empty()) throw std::invalid_argument("Table name cannot be empty");
        if (!hasTable(tableName)) throw std::invalid_argument("Table '" + tableName + "' does not exist");

        std::ostringstream sql;
        sql << "SELECT count(*) as cnt FROM \"" << tableName << "\"";
        std::vector<Json> params;
        if (!where.empty()) {
            sql << " WHERE ";
            bool first = true;
            for (const auto& [key, val] : where) {
                if (!first) sql << " AND ";
                first = false;
                if (val.isNull()) {
                    sql << "\"" << key << "\" IS NULL";
                } else {
                    sql << "\"" << key << "\" = ?";
                    params.push_back(val);
                }
            }
        }
        sql << ";";

        Json::Array res = query(sql.str(), params);
        if (!res.empty()) {
            const auto& obj = res[0].asObject();
            if (obj.contains("cnt") && obj.at("cnt").isNumber()) {
                return static_cast<size_t>(obj.at("cnt").asNumber());
            }
        }
        return 0;
    }

    Json::Array Database::update(const std::string& tableName, const Json::Object& updates, const Json::Object& where) {
        if (tableName.empty()) throw std::invalid_argument("Table name cannot be empty");
        if (!hasTable(tableName)) throw std::invalid_argument("Table '" + tableName + "' does not exist");
        if (updates.empty()) throw std::invalid_argument("Update data cannot be empty");

        Json::Object scheme = _getTableScheme(tableName);

        for (const auto& [colName, newVal] : updates) {
            if (!scheme.empty() && !scheme.contains(colName)) {
                throw std::invalid_argument("Unknown column '" + colName + "' in table '" + tableName + "'");
            }
            if (!scheme.empty()) {
                const Json::Object& colObj = scheme.at(colName).asObject();
                if (colObj.contains("primary_key") && colObj.at("primary_key").asBool()) {
                    throw std::invalid_argument("Column '" + colName + "' is a primary_key and cannot be updated");
                }
                if (colObj.contains("auto_increment") && colObj.at("auto_increment").asBool()) {
                    throw std::invalid_argument("Column '" + colName + "' is auto_increment and cannot be updated");
                }
                const bool isRequired = colObj.contains("required") && colObj.at("required").asBool();
                if (newVal.isNull() && isRequired) {
                    throw std::invalid_argument("Required column '" + colName + "' in table '" + tableName + "' cannot be set to null");
                }
                if (!newVal.isNull()) {
                    const std::string& typeName = colObj.at("type").asString();
                    if (!_matchesType(newVal, typeName)) {
                        throw std::invalid_argument("Column '" + colName + "' expects type '" + typeName + "'");
                    }
                }
            }
        }

        Json::Array matched = select(tableName, where);
        if (matched.empty()) return {};

        std::ostringstream sql;
        sql << "UPDATE \"" << tableName << "\" SET ";
        std::vector<Json> params;
        bool first = true;
        for (const auto& [col, val] : updates) {
            if (!first) sql << ", ";
            first = false;
            sql << "\"" << col << "\" = ?";
            params.push_back(val);
        }
        if (!where.empty()) {
            sql << " WHERE ";
            bool firstWhere = true;
            for (const auto& [k, v] : where) {
                if (!firstWhere) sql << " AND ";
                firstWhere = false;
                if (v.isNull()) {
                    sql << "\"" << k << "\" IS NULL";
                } else {
                    sql << "\"" << k << "\" = ?";
                    params.push_back(v);
                }
            }
        }
        sql << ";";

        try {
            executeUpdate(sql.str(), params);
        } catch (const std::exception& e) {
            std::string msg = e.what();
            if (msg.find("UNIQUE") != std::string::npos || msg.find("constraint failed") != std::string::npos) {
                throw std::invalid_argument("Unique constraint violation: " + msg);
            }
            throw;
        }

        Json::Array updatedRecords;
        for (auto& row : matched) {
            Json::Object updated = row.asObject();
            for (const auto& [col, val] : updates) {
                updated[col] = val;
            }
            updatedRecords.push_back(_formatRowWithScheme(std::move(updated), scheme));
        }
        return updatedRecords;
    }

    Json::Array Database::update(const std::string& tableName, const Json::Object& updates, const std::function<bool(const Json::Object&)>& where) {
        if (!where) throw std::invalid_argument("Filter callback cannot be null");
        Json::Array allRows = select(tableName, Json::Object{});
        Json::Array updatedRecords;
        for (const auto& row : allRows) {
            if (where(row.asObject())) {
                const auto& rowObj = row.asObject();
                Json::Object rowWhere;
                if (rowObj.contains("id")) {
                    rowWhere["id"] = rowObj.at("id");
                } else {
                    rowWhere = rowObj;
                }
                Json::Array res = update(tableName, updates, rowWhere);
                for (const auto& r : res) {
                    updatedRecords.push_back(r);
                }
            }
        }
        return updatedRecords;
    }

    Json::Array Database::remove(const std::string& tableName, const Json::Object& where) {
        if (tableName.empty()) throw std::invalid_argument("Table name cannot be empty");
        if (!hasTable(tableName)) throw std::invalid_argument("Table '" + tableName + "' does not exist");

        Json::Array matched = select(tableName, where);
        if (matched.empty()) return {};

        std::ostringstream sql;
        sql << "DELETE FROM \"" << tableName << "\"";
        std::vector<Json> params;
        if (!where.empty()) {
            sql << " WHERE ";
            bool first = true;
            for (const auto& [k, v] : where) {
                if (!first) sql << " AND ";
                first = false;
                if (v.isNull()) {
                    sql << "\"" << k << "\" IS NULL";
                } else {
                    sql << "\"" << k << "\" = ?";
                    params.push_back(v);
                }
            }
        }
        sql << ";";

        executeUpdate(sql.str(), params);
        return matched;
    }

    Json::Array Database::remove(const std::string& tableName, const std::function<bool(const Json::Object&)>& where) {
        if (!where) throw std::invalid_argument("Filter callback cannot be null");
        Json::Array allRows = select(tableName, Json::Object{});
        Json::Array deletedRecords;
        for (const auto& row : allRows) {
            if (where(row.asObject())) {
                const auto& rowObj = row.asObject();
                Json::Object rowWhere;
                if (rowObj.contains("id")) {
                    rowWhere["id"] = rowObj.at("id");
                } else {
                    rowWhere = rowObj;
                }
                Json::Array res = remove(tableName, rowWhere);
                for (const auto& r : res) {
                    deletedRecords.push_back(r);
                }
            }
        }
        return deletedRecords;
    }

    TableProxy Database::table(std::string tableName) {
        return TableProxy(*this, std::move(tableName));
    }

    TableProxy::TableProxy(Database& db, std::string tableName)
        : _db(db), _table(std::move(tableName)) {}

    Json::Array TableProxy::select(const Json::Object& where) const {
        return _db.select(_table, where);
    }

    Json::Array TableProxy::select(const std::function<bool(const Json::Object&)>& where) const {
        return _db.select(_table, where);
    }

    Json::Array TableProxy::find(const Json::Object& where) const {
        return _db.select(_table, where);
    }

    Json::Array TableProxy::find(const std::function<bool(const Json::Object&)>& where) const {
        return _db.select(_table, where);
    }

    size_t TableProxy::count(const Json::Object& where) const {
        return _db.count(_table, where);
    }

    Json::Object TableProxy::insert(const Json::Object& record) {
        return _db.insert(_table, record);
    }

    Json::Array TableProxy::update(const Json::Object& updates, const Json::Object& where) {
        return _db.update(_table, updates, where);
    }

    Json::Array TableProxy::update(const Json::Object& updates, const std::function<bool(const Json::Object&)>& where) {
        return _db.update(_table, updates, where);
    }

    Json::Array TableProxy::remove(const Json::Object& where) {
        return _db.remove(_table, where);
    }

    Json::Array TableProxy::remove(const std::function<bool(const Json::Object&)>& where) {
        return _db.remove(_table, where);
    }
} // HTTPServer
