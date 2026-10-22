//
// test_database.cpp - Comprehensive automated tests for Keet SQLite Database Engine & KeetScript DSL
//

#include <keet/Database.h>
#include <keet/TestHarness.h>
#include <keet/Testing.h>

#include <filesystem>
#include <thread>
#include <vector>

// Include the AOT-transpiled KeetScript component
#include "DatabaseTest.h"

using namespace HTTPServer;

// =============================================================================
// Test Suite: KeetScript Component Integration
// =============================================================================
KEET_TEST(DatabaseEngine, KeetScriptComponentHarness) {
    auto harness = keet::Testing::TestHarness::mount<Generated::DatabaseTest>();

    KEET_ASSERT_EXISTS(harness.find(".db-test"));
    KEET_ASSERT_EQ(harness.find(".title").text(), "Database KeetScript Test");
    KEET_ASSERT_EQ(harness.find(".user-count").text(), "0");
    KEET_ASSERT_EQ(harness.find(".status-msg").text(), "idle");

    // Click increment button to trigger KeetScript method
    harness.find(".btn-inc").triggerClick();
    KEET_ASSERT_EQ(harness.find(".user-count").text(), "1");
    KEET_ASSERT_EQ(harness.find(".status-msg").text(), "active");
}

// =============================================================================
// Test Suite: In-Memory & File-Backed SQLite Lifecycle
// =============================================================================
KEET_TEST(DatabaseEngine, DatabaseLifecycleAndWal) {
    // 1. In-memory database
    Database memDb(":memory:");
    KEET_ASSERT(memDb.isMemory());
    KEET_ASSERT_EQ(memDb.getPath(), ":memory:");
    KEET_ASSERT_TRUE(memDb.getHandle() != nullptr);

    // 2. File-backed database with automatic directory creation
    std::string testDbPath = "/tmp/keet_test_db_dir/app.db";
    if (std::filesystem::exists(testDbPath)) {
        std::filesystem::remove(testDbPath);
    }

    {
        Database fileDb(testDbPath, true);
        KEET_ASSERT(!fileDb.isMemory());
        KEET_ASSERT_EQ(fileDb.getPath(), testDbPath);

        // Verify WAL mode is active
        Json::Array pragmaRes = fileDb.query("PRAGMA journal_mode;");
        KEET_ASSERT(!pragmaRes.empty());
        const auto& obj = pragmaRes[0].asObject();
        KEET_ASSERT(obj.contains("journal_mode"));
        KEET_ASSERT_EQ(obj.at("journal_mode").asString(), "wal");

        fileDb.execute("CREATE TABLE test_wal (id INTEGER PRIMARY KEY, note TEXT);");
        KEET_ASSERT(fileDb.hasTable("test_wal"));
        fileDb.save();
    }

    // Reopen existing database without createIfNotExists
    {
        Database reopenDb(testDbPath, false);
        KEET_ASSERT(reopenDb.hasTable("test_wal"));
    }

    // Clean up test file
    std::filesystem::remove(testDbPath);
}

// =============================================================================
// Test Suite: Low-Level SQL Primitives (execute, query, executeUpdate)
// =============================================================================
KEET_TEST(DatabaseEngine, RawSqlPrimitives) {
    Database db(":memory:");

    db.execute("CREATE TABLE items (id INTEGER PRIMARY KEY AUTOINCREMENT, name TEXT NOT NULL, score REAL, active INTEGER);");
    KEET_ASSERT(db.hasTable("items"));

    int affected = db.executeUpdate("INSERT INTO items (name, score, active) VALUES (?, ?, ?);",
                                    {Json("Item Alpha"), Json(98.5), Json(true)});
    KEET_ASSERT_EQ(affected, 1);
    KEET_ASSERT_EQ(db.lastInsertId(), 1);

    db.executeUpdate("INSERT INTO items (name, score, active) VALUES (?, ?, ?);",
                     {Json("Item Beta"), Json(42.0), Json(false)});
    KEET_ASSERT_EQ(db.lastInsertId(), 2);

    // Parameterized query
    Json::Array rows = db.query("SELECT * FROM items WHERE score > ? ORDER BY id ASC;", {Json(50.0)});
    KEET_ASSERT_EQ(rows.size(), 1);
    const auto& row = rows[0].asObject();
    KEET_ASSERT_EQ(row.at("name").asString(), "Item Alpha");
    KEET_ASSERT_EQ(row.at("score").asNumber(), 98.5);
    KEET_ASSERT_EQ(row.at("active").asNumber(), 1.0);
}

// =============================================================================
// Test Suite: Table Creation with Typed Columns & Constraints
// =============================================================================
KEET_TEST(DatabaseEngine, TableCreationAndSchemaPersistence) {
    Database db(":memory:");

    Json::Object scheme;
    scheme["id"] = Json::Object{{"type", "integer"}, {"primary_key", true}, {"auto_increment", true}};
    scheme["username"] = Json::Object{{"type", "string"}, {"required", true}, {"unique", true}};
    scheme["balance"] = Json::Object{{"type", "float"}, {"required", true}};
    scheme["is_active"] = Json::Object{{"type", "boolean"}, {"required", true}};
    scheme["avatar"] = Json::Object{{"type", "blob"}};
    scheme["preferences"] = Json::Object{{"type", "object"}};
    scheme["roles"] = Json::Object{{"type", "array"}};

    bool created = db.createTable("accounts", scheme);
    KEET_ASSERT(created);
    KEET_ASSERT(db.hasTable("accounts"));
    KEET_ASSERT_EQ(db.collectionCount(), 1);

    // Creating already existing table returns false
    bool recreated = db.createTable("accounts", scheme);
    KEET_ASSERT(!recreated);
}

// =============================================================================
// Test Suite: Parameterized TableProxy CRUD & Constraints
// =============================================================================
KEET_TEST(DatabaseEngine, TableProxyCrudAndTypes) {
    Database db(":memory:");

    Json::Object scheme;
    scheme["id"] = Json::Object{{"type", "integer"}, {"primary_key", true}, {"auto_increment", true}};
    scheme["title"] = Json::Object{{"type", "string"}, {"required", true}, {"unique", true}};
    scheme["rating"] = Json::Object{{"type", "float"}};
    scheme["published"] = Json::Object{{"type", "boolean"}};
    scheme["tags"] = Json::Object{{"type", "array"}};
    scheme["metadata"] = Json::Object{{"type", "object"}};

    db.createTable("articles", scheme);
    auto articles = db.table("articles");

    // 1. Insert records
    Json::Object art1 = articles.insert({
        {"title", "Building Reactive C++ Microservices"},
        {"rating", 4.9},
        {"published", true},
        {"tags", Json::Array{"cpp", "keet", "sqlite"}},
        {"metadata", Json::Object{{"views", 1250.0}, {"author", "Pawel"}}}
    });

    KEET_ASSERT_EQ(art1.at("id").asNumber(), 1.0);
    KEET_ASSERT_EQ(art1.at("title").asString(), "Building Reactive C++ Microservices");
    KEET_ASSERT(art1.at("published").isBool());
    KEET_ASSERT(art1.at("published").asBool());
    KEET_ASSERT(art1.at("tags").isArray());
    KEET_ASSERT_EQ(art1.at("tags").asArray().size(), 3);

    Json::Object art2 = articles.insert({
        {"title", "AOT Compilation with Keety"},
        {"rating", 4.7},
        {"published", false},
        {"tags", Json::Array{"compiler", "dsl"}},
        {"metadata", Json::Object{{"views", 430.0}}}
    });
    KEET_ASSERT_EQ(art2.at("id").asNumber(), 2.0);

    // 2. Count
    KEET_ASSERT_EQ(articles.count(), 2);
    KEET_ASSERT_EQ(articles.count({{"published", true}}), 1);

    // 3. Select with filters
    Json::Array publishedArticles = articles.select({{"published", true}});
    KEET_ASSERT_EQ(publishedArticles.size(), 1);
    KEET_ASSERT_EQ(publishedArticles[0].asObject().at("title").asString(), "Building Reactive C++ Microservices");

    // 4. Select with predicate lambda
    Json::Array highRated = articles.select([](const Json::Object& r) {
        return r.contains("rating") && r.at("rating").asNumber() >= 4.8;
    });
    KEET_ASSERT_EQ(highRated.size(), 1);
    KEET_ASSERT_EQ(highRated[0].asObject().at("id").asNumber(), 1.0);

    // 5. Update
    Json::Array updated = articles.update({{"rating", 5.0}, {"published", true}}, {{"id", 2.0}});
    KEET_ASSERT_EQ(updated.size(), 1);
    KEET_ASSERT_EQ(updated[0].asObject().at("rating").asNumber(), 5.0);
    KEET_ASSERT(updated[0].asObject().at("published").asBool());

    // 6. Unique constraint violation throws invalid_argument
    KEET_ASSERT_THROWS(
        articles.insert({
            {"title", "Building Reactive C++ Microservices"} // Duplicate title
        }),
        std::invalid_argument
    );

    // 7. Auto-increment protection throws invalid_argument
    KEET_ASSERT_THROWS(
        articles.insert({
            {"id", 99.0},
            {"title", "Manual ID Article"}
        }),
        std::invalid_argument
    );

    // 8. Remove
    Json::Array removed = articles.remove({{"id", 1.0}});
    KEET_ASSERT_EQ(removed.size(), 1);
    KEET_ASSERT_EQ(articles.count(), 1);
}

// =============================================================================
// Test Suite: ACID Transactions (Commit & Rollback)
// =============================================================================
KEET_TEST(DatabaseEngine, AcidTransactions) {
    Database db(":memory:");

    Json::Object scheme;
    scheme["id"] = Json::Object{{"type", "integer"}, {"primary_key", true}, {"auto_increment", true}};
    scheme["name"] = Json::Object{{"type", "string"}, {"required", true}};
    db.createTable("users", scheme);
    auto users = db.table("users");

    // 1. Successful transaction commits
    db.transaction([&]() {
        users.insert({{"name", "Alice"}});
        users.insert({{"name", "Bob"}});
    });
    KEET_ASSERT_EQ(users.count(), 2);

    // 2. Failing transaction automatically rolls back
    bool caughtException = false;
    try {
        db.transaction([&]() {
            users.insert({{"name", "Charlie"}});
            users.insert({{"name", "Dave"}});
            throw std::runtime_error("Simulated catastrophic failure inside transaction");
        });
    } catch (const std::runtime_error&) {
        caughtException = true;
    }

    KEET_ASSERT(caughtException);
    // Charlie and Dave must be rolled back!
    KEET_ASSERT_EQ(users.count(), 2);
    KEET_ASSERT_EQ(users.select({{"name", "Charlie"}}).size(), 0);
    KEET_ASSERT_EQ(users.select({{"name", "Dave"}}).size(), 0);
}

// =============================================================================
// Test Suite: Multi-Threaded Concurrency
// =============================================================================
KEET_TEST(DatabaseEngine, ConcurrentWorkerThreads) {
    std::string dbFile = "/tmp/keet_concurrent_test.db";
    if (std::filesystem::exists(dbFile)) {
        std::filesystem::remove(dbFile);
    }

    {
        Database initDb(dbFile, true);
        Json::Object scheme;
        scheme["id"] = Json::Object{{"type", "integer"}, {"primary_key", true}, {"auto_increment", true}};
        scheme["thread_id"] = Json::Object{{"type", "integer"}};
        scheme["message"] = Json::Object{{"type", "string"}};
        initDb.createTable("logs", scheme);
    }

    constexpr int NUM_THREADS = 4;
    constexpr int INSERTS_PER_THREAD = 25;
    std::vector<std::thread> workers;

    for (int t = 0; t < NUM_THREADS; ++t) {
        workers.emplace_back([dbFile, t]() {
            Database db(dbFile, false);
            auto logs = db.table("logs");
            for (int i = 0; i < INSERTS_PER_THREAD; ++i) {
                logs.insert({
                    {"thread_id", static_cast<double>(t)},
                    {"message", "Message #" + std::to_string(i)}
                });
            }
        });
    }

    for (auto& w : workers) {
        w.join();
    }

    {
        Database verifyDb(dbFile, false);
        auto logs = verifyDb.table("logs");
        KEET_ASSERT_EQ(logs.count(), NUM_THREADS * INSERTS_PER_THREAD);
    }

    std::filesystem::remove(dbFile);
}

KEET_TEST_MAIN()
