// FileManager.h
#ifndef __POSIXFILESYSTEM_H__
#define __POSIXFILESYSTEM_H__
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

#include <fstream>
#include <unordered_map>
#include <vector>

#include "FileManager.h"
#include "TimeManager.h"

class Entry {
   public:
    std::string path_;
    time_t mtime_;

    Entry() = default;
    Entry(const std::string &path, time_t mtime) : path_(path), mtime_(mtime) {}

    bool operator<(const Entry &rhs) const { return mtime_ < rhs.mtime_; }
    bool operator>(const Entry &rhs) const { return mtime_ > rhs.mtime_; }
};

class FileManager {
   public:
    FileManager() : tm_(TimeManager::GetInstance()) { semicolon_udic_[0] = ""; }

    const std::string &CRefMultipleColonStr(int cnt) {
        if (semicolon_udic_.count(cnt) == 0) {
            return semicolon_udic_[cnt] = CRefMultipleColonStr(cnt - 1) + ":";
        } else
            return semicolon_udic_[cnt];
    }

    void Traverse(const std::string &dpath, std::vector<Entry> &file_entries,
                  std::vector<Entry> &dir_entries, time_t user_time,
                  const std::string &mode, int level = 1) {
        if (realpath(dpath.c_str(), abs_dpath_charr_) == nullptr) {
            throw std::invalid_argument("Failed to get absolute path for: " +
                                        dpath);
        }
        std::string abs_dpath(abs_dpath_charr_);

        if (level == 1) {
            struct stat st;
            if (!Stat(abs_dpath, st)) {
                throw std::invalid_argument("no such dir: " + abs_dpath);
            }
            dir_entries.emplace_back(abs_dpath, st.st_mtime);
        }

        std::vector<std::string> entry_names;
        if (!ReadDir(abs_dpath, entry_names)) {
            std::cerr << "Failed to open directory: " << dpath << "\n";
            return;
        }

        for (const auto &name : entry_names) {
            std::string abs_path = abs_dpath + "/" + name;

            struct stat st;
            if (!Stat(abs_path, st)) continue;
            if (!tm_.MatchesTimeFilter(st.st_mtime, user_time, mode)) continue;

            if (IsDir(abs_path)) {
                dir_entries.emplace_back(CRefMultipleColonStr(level) + name,
                                  st.st_mtime);
                Traverse(abs_path, file_entries, dir_entries, user_time, mode, level + 1);
            } else if (IsFile(abs_path)) {
                file_entries.emplace_back(abs_path, st.st_mtime);
            }
        }
    }

    void WriteToFile(const std::string &output_path,
                     const std::vector<Entry> &entries) {
        std::ofstream ofs(output_path.c_str());
        for (const auto &e : entries) {
            ofs << tm_.FormatTime(e.mtime_) << " " << e.path_ << "\n";
        }
    }

    bool IsDir(const std::string &path) const {
        struct stat st;
        return stat(path.c_str(), &st) == 0 && S_ISDIR(st.st_mode);
    }

    bool IsFile(const std::string &path) const {
        struct stat st;
        return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
    }

    bool Stat(const std::string &path, struct stat &st) const {
        return stat(path.c_str(), &st) == 0;
    }

    bool ReadDir(const std::string &dir_path,
                 std::vector<std::string> &entry_names) const {
        DIR *dir = opendir(dir_path.c_str());
        if (!dir) return false;

        struct dirent *entry;
        while ((entry = readdir(dir)) != nullptr) {
            std::string name = entry->d_name;
            if (name != "." && name != "..") {
                entry_names.push_back(name);
            }
        }
        closedir(dir);
        return true;
    }

   private:
    TimeManager &tm_;
    char abs_dpath_charr_[2048];
    std::unordered_map<int, std::string> semicolon_udic_;
};
#endif