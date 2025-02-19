#include <boost/filesystem.hpp>
#include "file_info.h"
#include "filename_manager.h"
#include "gtest/gtest.h"
#include "jonoondb_exceptions.h"
#include "test_utils.h"

using namespace std;
using namespace jonoondb_api;
using namespace jonoondb_test;

TEST(FileNameManager, Initialize_MissingDatabaseFile) {
  string dbName = "FileNameManager_Initialize_MissingDatabaseFile";
  string dbPath = g_TestRootDirectory;
  string collectionName = "Collection";
  ASSERT_THROW(
      FileNameManager fileNameManager(dbPath, dbName, collectionName, false),
      MissingDatabaseFileException);
}

TEST(FileNameManager, GetCurrentDataFileInfo) {
  string dbName = "FileNameManager_GetCurrentDataFileInfo";
  string dbPath = g_TestRootDirectory;
  string collectionName = "Collection";
  FileNameManager fileNameManager(dbPath, dbName, collectionName, true);

  FileInfo fileInfo;
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo);
  ASSERT_EQ(fileInfo.fileKey, 0);
  ASSERT_STREQ(fileInfo.fileName.c_str(),
               "FileNameManager_GetCurrentDataFileInfo_Collection.0");
  std::string completePath =
      dbPath + "/FileNameManager_GetCurrentDataFileInfo_Collection.0";
  ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(), completePath.c_str());

  // Again get the same thing and make sure we get the same file
  FileInfo fileInfo2;
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo2);
  ASSERT_EQ(fileInfo.fileKey, fileInfo2.fileKey);
  ASSERT_STREQ(fileInfo.fileName.c_str(), fileInfo2.fileName.c_str());
  ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(),
               fileInfo2.fileNameWithPath.c_str());
}

TEST(FileNameManager, GetNextDataFileInfo) {
  string dbName = "FileNameManager_GetNextDataFileInfo";
  string dbPath = g_TestRootDirectory;
  string collectionName = "Collection";
  FileNameManager fileNameManager(dbPath, dbName, collectionName, true);

  FileInfo fileInfo;
  string fileNamePattern = "FileNameManager_GetNextDataFileInfo_Collection.";
  string fileNameWithPathPattern =
      (boost::filesystem::path(dbPath) / fileNamePattern).generic_string();
  string fileName;
  string fileNameWithPath;

  // Add the first FileInfo record
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo);

  int count = 5;

  // Get next file in a loop
  for (int i = 1; i < count; i++) {
    fileNameManager.GetNextDataFileInfo(fileInfo);

    ASSERT_EQ(fileInfo.fileKey, i);

    fileName = fileNamePattern + std::to_string(i);
    fileNameWithPath = fileNameWithPathPattern + std::to_string(i);

    ASSERT_STREQ(fileInfo.fileName.c_str(), fileName.c_str());
    ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(), fileNameWithPath.c_str());
  }
}

TEST(FileNameManager, GetCurrentAndNextFileInfoCombined) {
  string dbName = "FileNameManager_GetCurrentAndNextFileInfoCombined";
  string dbPath = g_TestRootDirectory;
  string collectionName = "Collection";
  FileNameManager fileNameManager(dbPath, dbName, collectionName, true);

  FileInfo fileInfo;
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo);

  ASSERT_EQ(fileInfo.fileKey, 0);
  ASSERT_STREQ(
      fileInfo.fileName.c_str(),
      "FileNameManager_GetCurrentAndNextFileInfoCombined_Collection.0");
  boost::filesystem::path completePath(dbPath);
  completePath /=
      "FileNameManager_GetCurrentAndNextFileInfoCombined_Collection.0";
  ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(),
               completePath.generic_string().c_str());

  // Again get the same thing and make sure we get the same file
  FileInfo fileInfo2;
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo2);
  ASSERT_EQ(fileInfo.fileKey, fileInfo2.fileKey);
  ASSERT_STREQ(fileInfo.fileName.c_str(), fileInfo2.fileName.c_str());
  ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(),
               fileInfo2.fileNameWithPath.c_str());

  string fileNamePattern =
      "FileNameManager_GetCurrentAndNextFileInfoCombined_Collection.";
  string fileNameWithPathPattern =
      (completePath.parent_path() / fileNamePattern).generic_string();
  string fileName;
  string fileNameWithPath;
  // Get next file in a loop
  for (int i = 1; i < 11; i++) {
    fileNameManager.GetNextDataFileInfo(fileInfo);

    ASSERT_EQ(fileInfo.fileKey, i);
    fileName = fileNamePattern + std::to_string(i);
    fileNameWithPath = fileNameWithPathPattern + std::to_string(i);

    ASSERT_STREQ(fileInfo.fileName.c_str(), fileName.c_str());
    ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(), fileNameWithPath.c_str());
  }
}

TEST(FileNameManager, GetCurrentAndNextFileInfoCombined_Slash) {
  string dbName = "FileNameManager_GetCurrentAndNextFileInfoCombined_Slash";
  std::string dbPath;
  if (g_TestRootDirectory.at(g_TestRootDirectory.size() - 1) == '/') {
    dbPath = g_TestRootDirectory.substr(0, g_TestRootDirectory.size() - 1);
  } else {
    dbPath = g_TestRootDirectory;
    dbPath.append("/");
  }
  string collectionName = "Collection";
  FileNameManager fileNameManager(dbPath, dbName, collectionName, true);

  FileInfo fileInfo;
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo);

  ASSERT_EQ(fileInfo.fileKey, 0);
  ASSERT_STREQ(
      fileInfo.fileName.c_str(),
      "FileNameManager_GetCurrentAndNextFileInfoCombined_Slash_Collection.0");
  boost::filesystem::path completePath(dbPath);
  if (dbPath.at(dbPath.size() - 1) != '/') {
    completePath /= "/";
  }
  completePath /=
      "FileNameManager_GetCurrentAndNextFileInfoCombined_Slash_Collection.0";
  ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(),
               completePath.generic_string().c_str());

  // Again get the same thing and make sure we get the same file
  FileInfo fileInfo2;
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo2);
  ASSERT_EQ(fileInfo.fileKey, fileInfo2.fileKey);
  ASSERT_STREQ(fileInfo.fileName.c_str(), fileInfo2.fileName.c_str());
  ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(),
               fileInfo2.fileNameWithPath.c_str());

  string fileNamePattern =
      "FileNameManager_GetCurrentAndNextFileInfoCombined_Slash_Collection.";
  string fileNameWithPathPattern =
      (completePath.parent_path() / fileNamePattern).generic_string();
  string fileName;
  string fileNameWithPath;

  // Get next file in a loop
  for (int i = 1; i < 11; i++) {
    fileNameManager.GetNextDataFileInfo(fileInfo);

    ASSERT_EQ(fileInfo.fileKey, i);

    fileName = fileNamePattern + std::to_string(i);
    fileNameWithPath = fileNameWithPathPattern + std::to_string(i);

    ASSERT_STREQ(fileInfo.fileName.c_str(), fileName.c_str());
    ASSERT_STREQ(fileInfo.fileNameWithPath.c_str(), fileNameWithPath.c_str());
  }
}

TEST(FileNameManager, GetDataFileInfo) {
  string dbName = "FileNameManager_GetDataFileInfo";
  string dbPath = g_TestRootDirectory;
  string collectionName = "Collection";
  FileNameManager fileNameManager(dbPath, dbName, collectionName, true);

  FileInfo fileInfo;
  string fileNamePattern = "FileNameManager_GetDataFileInfo_Collection.";
  string fileNameWithPathPattern =
      (boost::filesystem::path(dbPath) / fileNamePattern).generic_string();
  string fileName;
  string fileNameWithPath;

  // Add the first FileInfo record
  fileNameManager.GetCurrentDataFileInfo(true, fileInfo);

  int count = 5;

  // Insert some data
  for (int i = 1; i < count; i++) {
    fileNameManager.GetNextDataFileInfo(fileInfo);
  }

  // Now retrieve these file info objects
  shared_ptr<FileInfo> fileInfoOut(new FileInfo());
  for (int i = 0; i < count; i++) {
    fileNameManager.GetFileInfo(i, fileInfoOut);

    ASSERT_EQ(fileInfoOut->fileKey, i);

    fileName = fileNamePattern + std::to_string(i);
    fileNameWithPath = fileNameWithPathPattern + std::to_string(i);

    ASSERT_STREQ(fileInfoOut->fileName.c_str(), fileName.c_str());
    ASSERT_STREQ(fileInfoOut->fileNameWithPath.c_str(),
                 fileNameWithPath.c_str());
  }
}