#include "../src/timescan.cpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "../src/IFileSystem.h"

// Test fixture
class TimeScanTest : public ::testing::Test {
 protected:
  time_t base_time;
  void SetUp() override {
    struct tm t = {};
    strptime("25/08/08 13:00:00", "%y/%m/%d %H:%M:%S", &t);
    base_time = mktime(&t);
  }
};

// Boundary: Exactly equal time
TEST_F(TimeScanTest, MatchesTimeFilter_EqualTime_BeforeMode) {
  EXPECT_TRUE(MatchesTimeFilter(base_time, base_time, "before"));
}

TEST_F(TimeScanTest, MatchesTimeFilter_EqualTime_AfterMode) {
  EXPECT_TRUE(MatchesTimeFilter(base_time, base_time, "after"));
}

// Boundary: Just before
TEST_F(TimeScanTest, MatchesTimeFilter_JustBefore_BeforeMode) {
  EXPECT_TRUE(MatchesTimeFilter(base_time - 1, base_time, "before"));
}

TEST_F(TimeScanTest, MatchesTimeFilter_JustBefore_AfterMode) {
  EXPECT_FALSE(MatchesTimeFilter(base_time - 1, base_time, "after"));
}

// Boundary: Just after
TEST_F(TimeScanTest, MatchesTimeFilter_JustAfter_BeforeMode) {
  EXPECT_FALSE(MatchesTimeFilter(base_time + 1, base_time, "before"));
}

TEST_F(TimeScanTest, MatchesTimeFilter_JustAfter_AfterMode) {
  EXPECT_TRUE(MatchesTimeFilter(base_time + 1, base_time, "after"));
}

// Invalid mode
TEST_F(TimeScanTest, MatchesTimeFilter_InvalidMode) {
  EXPECT_FALSE(MatchesTimeFilter(base_time, base_time, "invalid"));
}

class MockFileSystem : public IFileSystem {
 public:
  MOCK_CONST_METHOD1(IsDir, bool(const std::string& path));
  MOCK_CONST_METHOD1(IsFile, bool(const std::string& path));
  MOCK_CONST_METHOD2(Stat, bool(const std::string& path, struct stat& st));
  MOCK_CONST_METHOD2(ReadDir, bool(const std::string& dir_path,
                                   std::vector<std::string>& entry_names));
};

TEST(TraverseTest, TraverseSingleFile) {
  using ::testing::_;
  using ::testing::DoAll;
  using ::testing::Return;
  using ::testing::SetArgReferee;

  MockFileSystem fs;
  std::vector<Entry> files;
  std::vector<Entry> dirs;

  time_t fake_time = std::time(nullptr);

  EXPECT_CALL(fs, ReadDir("/mock", _))
      .WillOnce(DoAll(SetArgReferee<1>(std::vector<std::string>{"file1.txt"}),
                      Return(true)));

  EXPECT_CALL(fs, Stat("/mock/file1.txt", _))
      .WillOnce(DoAll(
          [](const std::string&, struct stat& st) {
            st.st_mtime = 1234567890;
            return true;
          },
          Return(true)));

  EXPECT_CALL(fs, IsDir("/mock/file1.txt")).WillOnce(Return(false));
  EXPECT_CALL(fs, IsFile("/mock/file1.txt")).WillOnce(Return(true));

  Traverse("/mock", files, dirs, 9999999999, "before", fs);

  ASSERT_EQ(files.size(), 1);
  EXPECT_EQ(files[0].fpath, "/mock/file1.txt");
}
