#include "database_metadata_manager.h"
#include <boost/filesystem.hpp>
#include <memory>
#include <sstream>
#include <string>
#include "enums.h"
#include "exception_utils.h"
#include "file_info.h"
#include "guard_funcs.h"
#include "index_info_impl.h"
#include "jonoondb_exceptions.h"
#include "path_utils.h"
#include "serializer_utils.h"
#include "sqlite3.h"
#include "sqlite_utils.h"
#include "standard_deleters.h"
#include "string_utils.h"

using namespace boost::filesystem;
using namespace jonoondb_api;

DatabaseMetadataManager::DatabaseMetadataManager(const std::string& dbPath,
                                                 const std::string& dbName,
                                                 bool createDBIfMissing)
    : m_metadataDBConnection(nullptr, GuardFuncs::SQLite3Close) {
  path normalizedPath;
  m_metadataDBConnection = SQLiteUtils::NormalizePathAndCreateDBConnection(
      dbPath, dbName, createDBIfMissing, normalizedPath);

  m_dbPath = normalizedPath.generic_string();
  m_dbName = dbName;

  path pathObj(normalizedPath);

  pathObj += m_dbName;
  pathObj += ".dat";
  m_fullDbPath = pathObj.generic_string();

  CreateTables();
  PrepareStatements();
}

DatabaseMetadataManager::~DatabaseMetadataManager() {
  FinalizeStatements();
}

void DatabaseMetadataManager::CreateTables() {
  // Set DB Pragmas
  int sqliteCode = 0;
  sqliteCode = sqlite3_exec(m_metadataDBConnection.get(),
                            "PRAGMA synchronous = FULL;", 0, 0, 0);
  if (sqliteCode != SQLITE_OK) {
    std::string msg = sqlite3_errstr(sqliteCode);
    throw SQLException(msg, __FILE__, __func__, __LINE__);
  }

  sqliteCode = sqlite3_exec(m_metadataDBConnection.get(),
                            "PRAGMA journal_mode = WAL;", 0, 0, 0);
  if (sqliteCode != SQLITE_OK) {
    std::string msg = sqlite3_errstr(sqliteCode);
    throw SQLException(msg, __FILE__, __func__, __LINE__);
  }

  sqliteCode =
      sqlite3_busy_handler(m_metadataDBConnection.get(),
                           SQLiteUtils::SQLiteGenericBusyHandler, nullptr);
  if (sqliteCode != SQLITE_OK) {
    std::string msg = sqlite3_errstr(sqliteCode);
    throw SQLException(msg, __FILE__, __func__, __LINE__);
  }

  // Create the necessary tables if they do not exist
  std::string sql =
      "CREATE TABLE IF NOT EXISTS Collection ("
      "CollectionName TEXT PRIMARY KEY, "
      "CollectionSchema BLOB, "
      "CollectionSchemaType INT)";
  sqliteCode =
      sqlite3_exec(m_metadataDBConnection.get(), sql.c_str(), NULL, NULL, NULL);
  if (sqliteCode != SQLITE_OK) {
    std::string msg = sqlite3_errstr(sqliteCode);
    throw SQLException(msg, __FILE__, __func__, __LINE__);
  }

  sql =
      "CREATE TABLE IF NOT EXISTS CollectionIndex ("
      "CollectionName TEXT, "
      "IndexName TEXT, "
      "IndexType INT, "
      "BinData BLOB, "
      "PRIMARY KEY (CollectionName, IndexName))";
  sqliteCode =
      sqlite3_exec(m_metadataDBConnection.get(), sql.c_str(), NULL, NULL, NULL);
  if (sqliteCode != SQLITE_OK) {
    std::string msg = sqlite3_errstr(sqliteCode);
    throw SQLException(msg, __FILE__, __func__, __LINE__);
  }
}

void DatabaseMetadataManager::PrepareStatements() {
  int sqliteCode = sqlite3_prepare_v2(
      m_metadataDBConnection.get(),
      "INSERT INTO CollectionIndex (CollectionName, IndexName, IndexType, "
      "BinData) VALUES (?, ?, ?, ?)",  // stmt
      -1,  // If greater than zero, then stmt is read up to the first null
           // terminator
      &m_insertCollectionIndexStmt,  // Statement that is to be prepared
      0                              // Pointer to unused portion of stmt
  );

  if (sqliteCode != SQLITE_OK) {
    std::string msg = sqlite3_errstr(sqliteCode);
    throw SQLException(msg, __FILE__, __func__, __LINE__);
  }

  sqliteCode = sqlite3_prepare_v2(
      m_metadataDBConnection.get(),
      "INSERT INTO Collection (CollectionName, CollectionSchema, "
      "CollectionSchemaType) VALUES (?, ?, ?)",  // stmt
      -1,  // If greater than zero, then stmt is read up to the first null
           // terminator
      &m_insertCollectionSchemaStmt,  // Statement that is to be prepared
      0                               // Pointer to unused portion of stmt
  );

  if (sqliteCode != SQLITE_OK) {
    std::string msg = sqlite3_errstr(sqliteCode);
    throw SQLException(msg, __FILE__, __func__, __LINE__);
  }
}

void DatabaseMetadataManager::AddCollection(
    const std::string& name, SchemaType schemaType, const std::string& schema,
    const std::vector<IndexInfoImpl*>& indexes) {
  // statement guard will make sure that the statement is cleared and reset when
  // statementGuard object goes out of scope
  std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> statementGuard(
      m_insertCollectionSchemaStmt, SQLiteUtils::ClearAndResetStatement);

  // 1. Prepare stmt to add data in Collection table
  int code =
      sqlite3_bind_text(m_insertCollectionSchemaStmt, 1,  // Index of wildcard
                        name.c_str(),                     // CollectionName
                        -1,  // length of the string is the number of bytes up
                             // to the first zero terminator
                        SQLITE_STATIC);

  if (code != SQLITE_OK) {
    throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
  }

  code =
      sqlite3_bind_blob(m_insertCollectionSchemaStmt, 2,  // Index of wildcard
                        schema.data(), schema.size(), SQLITE_STATIC);
  if (code != SQLITE_OK) {
    throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
  }

  code = sqlite3_bind_int(m_insertCollectionSchemaStmt, 3,  // Index of wildcard
                          static_cast<int>(schemaType));
  if (code != SQLITE_OK) {
    throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
  }

  // 2. Start Transaction before issuing insert
  code = sqlite3_exec(m_metadataDBConnection.get(), "BEGIN", 0, 0, 0);
  if (code != SQLITE_OK) {
    throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
  }

  code = sqlite3_step(m_insertCollectionSchemaStmt);
  if (code != SQLITE_DONE) {
    if (code == SQLITE_CONSTRAINT) {
      // Key already exists
      std::ostringstream ss;
      ss << "Collection with name \"" << name << "\" already exists.";
      std::string errorMsg = ss.str();
      sqlite3_exec(m_metadataDBConnection.get(), "ROLLBACK", 0, 0, 0);
      throw CollectionAlreadyExistException(ss.str(), __FILE__, __func__,
                                            __LINE__);
    } else {
      sqlite3_exec(m_metadataDBConnection.get(), "ROLLBACK", 0, 0, 0);
      throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
    }
  }

  // 3. Add all the collection indexes
  try {
    for (int i = 0; i < indexes.size(); i++) {
      CreateIndex(name.c_str(), *indexes[i]);
    }
  } catch (...) {
    sqlite3_exec(m_metadataDBConnection.get(), "ROLLBACK", 0, 0, 0);
    throw;
  }

  code = sqlite3_exec(m_metadataDBConnection.get(), "COMMIT", 0, 0, 0);
  if (code != SQLITE_OK) {
    // Comment copied from sqlite documentation. It is recommended that
    // applications respond to the errors listed above by explicitly issuing
    // a ROLLBACK command.If the transaction has already been rolled back
    // automatically by the error response, then the ROLLBACK command will
    // fail with an error, but no harm is caused by this.
    sqlite3_exec(m_metadataDBConnection.get(), "ROLLBACK", 0, 0, 0);
    throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
  }
}

const std::string& DatabaseMetadataManager::GetDBPath() const {
  return m_dbPath;
}

const std::string& DatabaseMetadataManager::GetDBName() const {
  return m_dbName;
}

void DatabaseMetadataManager::GetExistingCollections(
    std::vector<CollectionMetadata>& collections) {
  {
    static std::string sqlText =
        "SELECT c.CollectionName, c.CollectionSchema, c.CollectionSchemaType, "
        "ci.IndexName, ci.IndexType, ci.BinData "
        "FROM Collection c LEFT JOIN CollectionIndex ci ON c.CollectionName = "
        "ci.CollectionName "
        "ORDER BY c.CollectionName;";
    collections.clear();
    sqlite3_stmt* sqlStmt = nullptr;

    int code = sqlite3_prepare_v2(m_metadataDBConnection.get(), sqlText.c_str(),
                                  sqlText.size(),
                                  &sqlStmt,  // statement that is to be prepared
                                  nullptr  // pointer to unused portion of stmt
    );

    if (code != SQLITE_OK) {
      throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
    }

    std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> statementGuard(
        sqlStmt, GuardFuncs::SQLite3Finalize);

    code = sqlite3_step(sqlStmt);
    if (code == SQLITE_ROW) {
      do {
        std::string collectionName(
            reinterpret_cast<const char*>(sqlite3_column_text(sqlStmt, 0)),
            sqlite3_column_bytes(sqlStmt, 0));
        if (collections.size() == 0 ||
            collections.back().name != collectionName) {
          // we have a new collection here
          CollectionMetadata metadata;
          metadata.name = collectionName;
          metadata.schema = std::string(
              reinterpret_cast<const char*>(sqlite3_column_blob(sqlStmt, 1)),
              sqlite3_column_bytes(sqlStmt, 1));
          metadata.schemaType =
              static_cast<SchemaType>(sqlite3_column_int(sqlStmt, 2));
          collections.push_back(metadata);
        }

        // see if we have a index record
        std::string indexName(
            reinterpret_cast<const char*>(sqlite3_column_text(sqlStmt, 3)),
            sqlite3_column_bytes(sqlStmt, 3));
        if (indexName.size() > 0) {
          auto binData = sqlite3_column_blob(sqlStmt, 5);
          auto binDataSize = sqlite3_column_bytes(sqlStmt, 5);
          BufferImpl buffer((char*)binData, binDataSize, binDataSize,
                            StandardDeleteNoOp);

          collections.back().indexes.push_back(
              SerializerUtils::BytesToIndexInfo(buffer));
        }
      } while ((code = sqlite3_step(sqlStmt)) == SQLITE_ROW);
    }

    if (code != SQLITE_DONE) {
      // An error occured
      throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
    }
  }

  // Now get the data files
  if (collections.size() > 0) {
    static std::string sqlText =
        "SELECT c.CollectionName, "
        "cdf.FileKey, cdf.FileName, cdf.FileDataLength "
        "FROM Collection c LEFT JOIN CollectionDataFile cdf ON "
        "c.CollectionName = cdf.CollectionName "
        "ORDER BY c.CollectionName, cdf.FileKey;";
    sqlite3_stmt* sqlStmt = nullptr;

    int code = sqlite3_prepare_v2(m_metadataDBConnection.get(), sqlText.c_str(),
                                  sqlText.size(),
                                  &sqlStmt,  // statement that is to be prepared
                                  nullptr  // pointer to unused portion of stmt
    );

    if (code != SQLITE_OK) {
      throw SQLException(sqlite3_errstr(code), __FILE__, __func__, __LINE__);
    }

    std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> statementGuard(
        sqlStmt, GuardFuncs::SQLite3Finalize);

    code = sqlite3_step(sqlStmt);
    int index = 0;
    if (code == SQLITE_ROW) {
      do {
        std::string collectionName(
            reinterpret_cast<const char*>(sqlite3_column_text(sqlStmt, 0)),
            sqlite3_column_bytes(sqlStmt, 0));
        if (collections[index].name == collectionName) {
          auto fileName = std::string(
              reinterpret_cast<const char*>(sqlite3_column_text(sqlStmt, 2)),
              sqlite3_column_bytes(sqlStmt, 2));
          if (fileName.size() > 0) {
            FileInfo fi;
            fi.fileKey = sqlite3_column_int(sqlStmt, 1);
            fi.fileNameWithPath = m_dbPath + fileName;
            fi.dataLength = sqlite3_column_int(sqlStmt, 3);
            fi.fileName = std::move(fileName);
            collections[index].dataFiles.push_back(std::move(fi));
          }
        } else {
          // we are onto the next collection
          index++;
          if (index >= collections.size()) {
            // Todo: remove this check once we have foreign key constraints in
            // sqlite tables This means we have more records in
            // CollectionDataFile table that do not have a corresponding
            // collection record in Collection table. This should never happen.
            std::ostringstream ss;
            ss << "Error occured while trying to read data files for "
                  "collection "
               << collectionName
               << ". CollectionDataFiles have no corresponding collection.";
            throw JonoonDBException(ss.str(), __FILE__, __func__, __LINE__);
          }

          if (collections[index].name != collectionName) {
            // this should never happen because we order the collections vector
            // (the first query) and the current query on CollectionName
            std::ostringstream ss;
            ss << "Error occured while tring to read data files for collection "
               << collections[index].name
               << ". The sort order of vector and sql resultset is not same.";
            throw JonoonDBException(ss.str(), __FILE__, __func__, __LINE__);
          }

          auto fileName = std::string(
              reinterpret_cast<const char*>(sqlite3_column_text(sqlStmt, 2)),
              sqlite3_column_bytes(sqlStmt, 2));
          if (fileName.size() > 0) {
            FileInfo fi;
            fi.fileKey = sqlite3_column_int(sqlStmt, 1);
            fi.fileNameWithPath = m_dbPath + fileName;
            fi.dataLength = sqlite3_column_int(sqlStmt, 3);
            fi.fileName = std::move(fileName);
            collections[index].dataFiles.push_back(std::move(fi));
          }
        }
      } while ((code = sqlite3_step(sqlStmt)) == SQLITE_ROW);
    }
  }
}

void DatabaseMetadataManager::CreateIndex(const std::string& collectionName,
                                          const IndexInfoImpl& indexInfo) {
  std::unique_ptr<sqlite3_stmt, void (*)(sqlite3_stmt*)> statementGuard(
      m_insertCollectionIndexStmt, SQLiteUtils::ClearAndResetStatement);

  int sqliteCode = sqlite3_bind_text(
      m_insertCollectionIndexStmt, 1,  // Index of wildcard
      collectionName.c_str(), collectionName.size(), SQLITE_STATIC);
  if (sqliteCode != SQLITE_OK)
    throw SQLException(sqlite3_errstr(sqliteCode), __FILE__, __func__,
                       __LINE__);

  sqliteCode =
      sqlite3_bind_text(m_insertCollectionIndexStmt, 2,  // Index of wildcard
                        indexInfo.GetIndexName().c_str(),
                        indexInfo.GetIndexName().size(), SQLITE_STATIC);
  if (sqliteCode != SQLITE_OK)
    throw SQLException(sqlite3_errstr(sqliteCode), __FILE__, __func__,
                       __LINE__);

  sqliteCode =
      sqlite3_bind_int(m_insertCollectionIndexStmt, 3,  // Index of wildcard
                       static_cast<int>(indexInfo.GetType()));

  if (sqliteCode != SQLITE_OK)
    throw SQLException(sqlite3_errstr(sqliteCode), __FILE__, __func__,
                       __LINE__);

  BufferImpl buffer;
  SerializerUtils::IndexInfoToBytes(indexInfo, buffer);
  sqliteCode =
      sqlite3_bind_blob(m_insertCollectionIndexStmt, 4,  // Index of wildcard
                        buffer.GetData(), buffer.GetLength(), SQLITE_STATIC);
  if (sqliteCode != SQLITE_OK)
    throw SQLException(sqlite3_errstr(sqliteCode), __FILE__, __func__,
                       __LINE__);

  // Now insert the record
  sqliteCode = sqlite3_step(m_insertCollectionIndexStmt);
  if (sqliteCode != SQLITE_DONE) {
    if (sqliteCode == SQLITE_CONSTRAINT) {
      // Key already exists
      std::ostringstream ss;
      ss << "Index with name " << indexInfo.GetIndexName()
         << " already exists.";
      throw IndexAlreadyExistException(ss.str(), __FILE__, __func__, __LINE__);
    } else {
      throw SQLException(sqlite3_errstr(sqliteCode), __FILE__, __func__,
                         __LINE__);
    }
  }
}

void DatabaseMetadataManager::FinalizeStatements() {
  GuardFuncs::SQLite3Finalize(m_insertCollectionIndexStmt);
  GuardFuncs::SQLite3Finalize(m_insertCollectionSchemaStmt);
}
