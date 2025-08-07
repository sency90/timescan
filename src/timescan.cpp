#include <dirent.h>
#include <gmock/gmock.h>
#include <sys/stat.h>
#include <unistd.h>

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

#include "IFileSystem.h"
#include "PosixFileSystem.h"

struct Entry {
  std::string fpath;
  time_t mtime;

  bool operator<(const Entry &rhs) const { return mtime < rhs.mtime; }
};

std::string FormatTime(time_t raw_time) {
  std::tm *t = std::localtime(&raw_time);
  std::ostringstream oss;
  oss << std::setfill('0') << "[" << std::setw(2) << (t->tm_year % 100) << "/"
      << std::setw(2) << (t->tm_mon + 1) << "/" << std::setw(2) << t->tm_mday
      << " " << std::setw(2) << t->tm_hour << ":" << std::setw(2) << t->tm_min
      << ":" << std::setw(2) << t->tm_sec << "]";
  return oss.str();
}

bool MatchesTimeFilter(time_t file_time, time_t user_time,
                       const std::string &mode) {
  if (mode == "before") {
    return file_time <= user_time;
  } else if (mode == "after") {
    return file_time >= user_time;
  } else {
    return false;
  }
}

void Traverse(const std::string &dir_path, std::vector<Entry> &files,
              std::vector<Entry> &dirs, time_t user_time,
              const std::string &mode, const IFileSystem &fs) {
  std::vector<std::string> entry_names;
  if (!fs.ReadDir(dir_path, entry_names)) {
    std::cerr << "Failed to open directory: " << dir_path << "\n";
    return;
  }

  for (const auto &name : entry_names) {
    std::string full_path = dir_path + "/" + name;

    struct stat st;
    if (!fs.Stat(full_path, st))
      continue;
    if (!MatchesTimeFilter(st.st_mtime, user_time, mode))
      continue;

    if (fs.IsDir(full_path)) {
      dirs.push_back({full_path, st.st_mtime});
      Traverse(full_path, files, dirs, user_time, mode, fs);
    } else if (fs.IsFile(full_path)) {
      files.push_back({full_path, st.st_mtime});
    }
  }
}

void WriteToFile(const std::string &output_path,
                 const std::vector<Entry> &entries) {
  std::ofstream ofs(output_path.c_str());
  for (const auto &e : entries) {
    ofs << FormatTime(e.mtime) << " " << e.fpath << "\n";
  }
}

time_t ParseDateTime(const std::string &date_str, const std::string &time_str) {
  std::tm t = {};
  std::istringstream ss(date_str + " " + time_str);
  ss >> std::get_time(&t, "%y/%m/%d %H:%M:%S");
  if (ss.fail()) {
    std::cerr << "Invalid time format: " << date_str << " " << time_str
              << std::endl;
    return static_cast<time_t>(-1);
  }
  return std::mktime(&t);
}

int main(int argc, char *argv[]) {
  ::testing::InitGoogleMock();
  if (argc != 5) {
    std::cerr << "Usage: ./timescan <dir> <before|after> <yy/mm/dd> <hh:mm:ss>"
              << std::endl;
    return 1;
  }

  std::string path = argv[1];
  std::string mode = argv[2];
  std::string date_str = argv[3];
  std::string time_str = argv[4];

  std::cout << "[debug] path=" << path << " mode=" << mode
            << " date=" << date_str << " time=" << time_str << std::endl;

  if (mode != "before" && mode != "after") {
    std::cerr << "Mode must be 'before' or 'after'." << std::endl;
    return 1;
  }

  time_t user_time = ParseDateTime(date_str, time_str);
  if (user_time == static_cast<time_t>(-1)) {
    return 1;
  }

  std::vector<Entry> files;
  std::vector<Entry> dirs;

  PosixFileSystem fs;
  Traverse(path, files, dirs, user_time, mode, fs);

  std::sort(files.begin(), files.end());
  std::sort(dirs.begin(), dirs.end());

  WriteToFile("file_list.txt", files);
  WriteToFile("dir_list.txt", dirs);

  return RUN_ALL_TESTS();
}

TEST(TS1, TC1) { EXPECT_EQ(true, true); }