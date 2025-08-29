// FileManager.h
#ifndef __POSIXFILESYSTEM_H__
#define __POSIXFILESYSTEM_H__
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <unordered_map>

#include <fstream>
#include <vector>

#include "FileManager.h"
#include "TimeManager.h"

class Entry {
   public:
    std::string path;
    time_t mtime;

    Entry()=default;
    Entry(const std::string & path, time_t mtime):path(path), mtime(mtime) {}

    bool operator<(const Entry &rhs) const { return mtime < rhs.mtime; }
    bool operator>(const Entry &rhs) const { return mtime > rhs.mtime; }
};

class FileManager {
   public:
    FileManager() : tm(TimeManager::GetInstance()) {
        semicolon_udic[0] = "";
    }

    const std::string & CRefMultipleColonStr(int cnt) {
        if(semicolon_udic.count(cnt)==0) {
            return semicolon_udic[cnt] = CRefMultipleColonStr(cnt-1)+":";
        }
        else return semicolon_udic[cnt];
    }

    void Traverse(const std::string &dir_path, std::vector<Entry> &files,
                  std::vector<Entry> &dirs, time_t user_time,
                  const std::string &mode, int level=1) {
        if (realpath(dir_path.c_str(), abs_path) == nullptr) {
            throw std::invalid_argument("Failed to get absolute path for: " +
                                        dir_path);
        }
        if(level==1) {
            struct stat st;
            Stat(abs_path, st);
            dirs.emplace_back(abs_path, st.st_mtime);
        }
        std::string abs_path_str(abs_path);

        std::vector<std::string> entry_names;
        if (!ReadDir(abs_path_str, entry_names)) {
            std::cerr << "Failed to open directory: " << dir_path << "\n";
            return;
        }

        for (const auto &name : entry_names) {
            std::string full_path = abs_path_str + "/" + name;

            struct stat st;
            if (!Stat(full_path, st)) continue;
            if (!tm.MatchesTimeFilter(st.st_mtime, user_time, mode)) continue;

            if (IsDir(full_path)) {
                dirs.emplace_back(CRefMultipleColonStr(level)+name, st.st_mtime);
                Traverse(full_path, files, dirs, user_time, mode, level+1);
            } else if (IsFile(full_path)) {
                files.push_back({full_path, st.st_mtime});
            }
        }
    }

    void WriteToFile(const std::string &output_path,
                     const std::vector<Entry> &entries) {
        std::ofstream ofs(output_path.c_str());
        for (const auto &e : entries) {
            ofs << tm.FormatTime(e.mtime) << " " << e.path << "\n";
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
    TimeManager &tm;
    char abs_path[2048];
    std::unordered_map<int, std::string> semicolon_udic;
};
#endif