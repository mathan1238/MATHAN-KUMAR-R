#pragma once

#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>
#include <libpq-fe.h>

namespace mathan_mart {

class Database {
public:
    static Database& getInstance();

    // Prevent copy/move
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    bool initialize(const std::string& connStr = "");
    void close();

    PGconn* getDbHandle();
    std::mutex& getMutex();

    bool execute(const std::string& sql);

    // Transaction support
    bool beginTransaction();
    bool commit();
    bool rollback();

    // Not used with PG (use RETURNING id instead), kept for interface compat
    int64_t getLastInsertId() { return 0; }

private:
    Database();
    ~Database();

    void createSchemaIfNotExist();
    void seedDataIfEmpty();

    PGconn*     conn_    = nullptr;
    std::mutex  dbMutex_;
    std::string connStr_;
};

} // namespace mathan_mart
