#include "database.h"
#include "gtest/gtest.h"

using namespace jonoondb_api;

TEST(Options, Ctor_Deafult) {
  Options opt;
  ASSERT_TRUE(opt.GetCreateDBIfMissing());
  ASSERT_EQ(opt.GetMemoryCleanupThreshold(), 1024LL * 1024LL * 1024LL * 4LL);
}

TEST(Options, Ctor_Params) {
  Options opt(false, 1024, 1024 * 1024);
  ASSERT_FALSE(opt.GetCreateDBIfMissing());
  ASSERT_EQ(opt.GetMaxDataFileSize(), 1024);
  ASSERT_EQ(opt.GetMemoryCleanupThreshold(), 1024 * 1024);
}

TEST(Options, Copy_Ctor) {
  Options opt1;
  opt1.SetMaxDataFileSize(12345);
  opt1.SetMemoryCleanupThreshold(1024);
  Options opt2(opt1);
  ASSERT_EQ(opt1.GetCreateDBIfMissing(), opt2.GetCreateDBIfMissing());
  ASSERT_EQ(opt1.GetMaxDataFileSize(), opt2.GetMaxDataFileSize());
  ASSERT_EQ(opt1.GetMemoryCleanupThreshold(), opt2.GetMemoryCleanupThreshold());
}

TEST(Options, Copy_Assignment) {
  Options opt1;
  opt1.SetMaxDataFileSize(12345);
  opt1.SetMemoryCleanupThreshold(1024);
  Options opt2;
  opt2 = opt1;
  ASSERT_EQ(opt1.GetCreateDBIfMissing(), opt2.GetCreateDBIfMissing());
  ASSERT_EQ(opt1.GetMaxDataFileSize(), opt2.GetMaxDataFileSize());
  ASSERT_EQ(opt1.GetMemoryCleanupThreshold(), opt2.GetMemoryCleanupThreshold());
}

TEST(Options, Move_Ctor) {
  Options opt1;
  opt1.SetMaxDataFileSize(12345);
  opt1.SetMemoryCleanupThreshold(1024);
  auto creeateDB = opt1.GetCreateDBIfMissing();
  auto maxSize = opt1.GetMaxDataFileSize();
  auto memThreshold = opt1.GetMemoryCleanupThreshold();

  Options opt2(std::move(opt1));
  ASSERT_EQ(opt2.GetCreateDBIfMissing(), creeateDB);
  ASSERT_EQ(opt2.GetMaxDataFileSize(), maxSize);
  ASSERT_EQ(opt2.GetMemoryCleanupThreshold(), memThreshold);
}

TEST(Options, Move_Assignment) {
  Options opt1;
  opt1.SetMaxDataFileSize(12345);
  auto creeateDB = opt1.GetCreateDBIfMissing();
  auto maxSize = opt1.GetMaxDataFileSize();
  auto memThreshold = opt1.GetMemoryCleanupThreshold();

  Options opt2;
  opt2 = std::move(opt1);
  ASSERT_EQ(opt2.GetCreateDBIfMissing(), creeateDB);
  ASSERT_EQ(opt2.GetMaxDataFileSize(), maxSize);
  ASSERT_EQ(opt2.GetMemoryCleanupThreshold(), memThreshold);
}