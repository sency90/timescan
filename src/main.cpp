#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <vector>

#include "FileManager.h"
#include "TimeManager.h"

void ValidateArgs(int argc, char **argv) {
    if (argc != 5) {
        std::cerr
            << "Usage: ./timescan <dir> <before|after> <yy/mm/dd> <hh:mm:ss>"
            << std::endl;
        exit(1);
    }

    std::string mode(argv[2]);
    if (mode != "before" && mode != "after") {
        std::cerr << "Mode must be 'before' or 'after'." << std::endl;
        exit(1);
    }
}

time_t GetUserTime(const std::string &date_str, const std::string &time_str) {
    TimeManager &tm = TimeManager::GetInstance();
    try {
        time_t result = tm.ParseDateTime(date_str, time_str);
        return result;
    } catch (std::exception &ex) {
        printf("[EXCEPTION] %s\n", ex.what());
        exit(1);
    }
}

void ParsingArgs(int argc, char **argv, std::string &path, std::string &mode,
                 std::string &date_str, std::string &time_str) {
    path = argv[1];
    mode = argv[2];
    date_str = argv[3];
    time_str = argv[4];
#ifdef _DEBUG
    std::cout << "[debug] path=" << path << " mode=" << mode
              << " date=" << date_str << " time=" << time_str << std::endl;
#endif
}
int main(int argc, char *argv[]) {
    try {
        ValidateArgs(argc, argv);

        std::string path, mode, date_str, time_str;
        ParsingArgs(argc, argv, path, mode, date_str, time_str);

        time_t user_time = GetUserTime(date_str, time_str);

        FileManager fs;
        std::vector<Entry> files, dirs;

        fs.Traverse(path, files, dirs, user_time, mode);

        //std::sort(files.begin(), files.end(), std::greater<Entry>());
        //std::sort(dirs.begin(), dirs.end(), std::greater<Entry>());

        fs.WriteToFile("file_list.txt", files);
        fs.WriteToFile("dir_list.txt", dirs);
    } catch (std::exception &ex) {
        printf("[EXCEPTION] %s\n", ex.what());
    }

    return 0;
}