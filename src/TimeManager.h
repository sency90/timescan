#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
class TimeManager {
 private:
  TimeManager() = default;

 public:
  static TimeManager &GetInstance() {
    static TimeManager tm;
    return tm;
  }

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

  time_t ParseDateTime(const std::string &date_str,
                       const std::string &time_str) {
    std::tm t = {};
    std::istringstream ss(date_str + " " + time_str);
    ss >> std::get_time(&t, "%y/%m/%d %H:%M:%S");
    if (ss.fail()) {
      throw(std::invalid_argument(std::string("Invalid time format: ") +
                                  date_str + " " + time_str));
    }
    return std::mktime(&t);
  }
};
#endif