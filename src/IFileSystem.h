// IFileSystem.h
#ifndef __IFILESYSTEM_H__
#define __IFILESYSTEM_H__

#include <sys/stat.h>

#include <string>
#include <vector>

struct DirEntry {
  std::string name;
  bool is_dir;
};

class IFileSystem {
 public:
  virtual ~IFileSystem() {}

  virtual bool IsDir(const std::string& path) const = 0;
  virtual bool IsFile(const std::string& path) const = 0;
  virtual bool Stat(const std::string& path, struct stat& st) const = 0;
  virtual bool ReadDir(const std::string& dir_path,
                       std::vector<std::string>& entry_names) const = 0;
};
#endif