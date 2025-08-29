// #include "../src/timescan.cpp"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "../src/FileManager.h"
using namespace testing;
using std::pair;
using std::string;
using std::vector;

// Test fixture
class TimeManagerTest : public Test {
protected:
  TimeManager &tm = TimeManager::GetInstance();
  time_t base_time;
  void SetUp() override {
    struct tm t = {};
    strptime("25/08/08 13:00:00", "%y/%m/%d %H:%M:%S", &t);
    base_time = mktime(&t);
  }
};

// Boundary: Exactly equal time
TEST_F(TimeManagerTest, MatchesTimeFilter_EqualTime_BeforeMode) {
  EXPECT_TRUE(tm.MatchesTimeFilter(base_time, base_time, "before"));
}

TEST_F(TimeManagerTest, MatchesTimeFilter_EqualTime_AfterMode) {
  EXPECT_TRUE(tm.MatchesTimeFilter(base_time, base_time, "after"));
}

// Boundary: Just before
TEST_F(TimeManagerTest, MatchesTimeFilter_JustBefore_BeforeMode) {
  EXPECT_TRUE(tm.MatchesTimeFilter(base_time - 1, base_time, "before"));
}

TEST_F(TimeManagerTest, MatchesTimeFilter_JustBefore_AfterMode) {
  EXPECT_FALSE(tm.MatchesTimeFilter(base_time - 1, base_time, "after"));
}

// Boundary: Just after
TEST_F(TimeManagerTest, MatchesTimeFilter_JustAfter_BeforeMode) {
  EXPECT_FALSE(tm.MatchesTimeFilter(base_time + 1, base_time, "before"));
}

TEST_F(TimeManagerTest, MatchesTimeFilter_JustAfter_AfterMode) {
  EXPECT_TRUE(tm.MatchesTimeFilter(base_time + 1, base_time, "after"));
}

// Invalid mode
TEST_F(TimeManagerTest, MatchesTimeFilter_InvalidMode) {
  EXPECT_FALSE(tm.MatchesTimeFilter(base_time, base_time, "invalid"));
}

TEST(FileManagerTest, TraverseSingleFile) {
  vector<Entry> file_entries;
  vector<Entry> dir_entries;

  time_t cur_time = std::time(nullptr);

  vector<string> dir_paths = {"filemanager_test_dir2", "filemanager_test_dir1",
                              "filemanager_test_dir2/subdir",
                              "filemanager_test_dir1/subdir",
                              "filemanager_test_dir3"};
  vector<string> file_paths = {"filemanager_test_dir3/333_2.txt",
                               "filemanager_test_dir1/111_2.txt",
                               "filemanager_test_dir1/111_1.txt",
                               "filemanager_test_dir2/subdir/222_1_1.txt",
                               "filemanager_test_dir3/333_1.txt",
                               "filemanager_test_dir2/222_1.txt",
                               "filemanager_test_dir1/subdir/111_1_1.txt"};

  char cur_dir_charr[2048];
  getcwd(cur_dir_charr, 2047);
  string cur_dir_str(cur_dir_charr);

  try {
    for (auto &path : dir_paths) {
      path = cur_dir_str + "/" + path;
    }

    for (auto &path : file_paths) {
      path = cur_dir_str + "/" + path;
    }

    string cmd;
    for (int i = 0; i < dir_paths.size(); i++) {
      cmd = "mkdir -p " + dir_paths[i];
      system(cmd.c_str());
    }
    for (int i = 0; i < file_paths.size(); i++) {
      cmd = "touch " + file_paths[i];
      system(cmd.c_str());
    }

    FileManager fm;
    fm.Traverse("./", file_entries, dir_entries, cur_time, "after");

    EXPECT_EQ(dir_entries.size(), dir_paths.size()+1);
    EXPECT_EQ(file_entries.size(), file_paths.size());

    fm.WriteToFile("dir_list.txt", dir_entries);
    fm.WriteToFile("file_list.txt", file_entries);
  } catch (std::exception &ex) {
    printf("[EXCEPTION] %s\n", ex.what());
  }

  std::string cmd;
  for (int i = 0; i < dir_paths.size(); i++) {
    cmd = "rm -rf " + dir_paths[i];
    system(cmd.c_str());
  }
}
