// PosixFileSystem.h
#ifndef __POSIXFILESYSTEM_H__
#define __POSIXFILESYSTEM_H__
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include "IFileSystem.h"

class PosixFileSystem : public IFileSystem {
 public:
  bool IsDir(const std::string& path) const override {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
  }

  bool IsFile(const std::string& path) const override {
    struct stat st;
    return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
  }

  bool Stat(const std::string& path, struct stat& st) const override {
    return stat(path.c_str(), &st) == 0;
  }

  bool ReadDir(const std::string& dir_path,
               std::vector<std::string>& entry_names) const override {
    DIR* dir = opendir(dir_path.c_str());
    if (!dir)
      return false;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
      std::string name = entry->d_name;
      if (name != "." && name != "..") {
        entry_names.push_back(name);
      }
    }
    closedir(dir);
    return true;
  }
};
#endif