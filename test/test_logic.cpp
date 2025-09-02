// test/test_logic.cpp
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <utime.h>

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <regex>
#include <string>
#include <vector>

#include "../src/FileManager.h"
#include "../src/TimeManager.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

using std::string;
using std::vector;
using ::testing::HasSubstr;
using namespace ::testing;

// ---------- helpers ----------
class TimeScanTestFixture : public Test {
    private:
    string MakeTempDir() {
        std::string tmpl = "./timescan_test_XXXXXX";
        std::vector<char> buf(tmpl.begin(), tmpl.end());
        buf.push_back('\0');
        char* p = mkdtemp(buf.data());
        if (!p) {
            throw std::runtime_error("mkdtemp failed");
        }

        static char absbuf[PATH_MAX];
        if (!realpath(p, absbuf)) throw std::runtime_error("realpath failed");
        return std::string(absbuf);
    }

    void RemoveAllDir(const std::string & dpath) {
        std::string cmd = "rm -rf '" + dpath + "'";
        int ret = system(cmd.c_str());
        if(ret != 0) {
            throw std::runtime_error("rm -rf failed: " + dpath);
        }
    }

    protected:
    void EnsureDir(const string& path, mode_t mode = 0755) {
        if (mkdir(path.c_str(), mode) != 0) {
            if (errno == EEXIST) return;
            throw std::runtime_error("mkdir failed: " + path +
                                     " err=" + std::to_string(errno));
        }
    }

    void TouchFile(const string& path) {
        int fd = ::open(path.c_str(), O_CREAT | O_WRONLY, 0644);
        if (fd < 0) throw std::runtime_error("open failed: " + path);
        ::close(fd);
    }

    void SetMTime(const string& path, time_t t) {
#if defined(UTIME_OMIT) || defined(UTIME_NOW)
        // POSIX 고정밀 API가 있으면 시도
        struct timespec ts[2];
        ts[0].tv_sec = t;
        ts[0].tv_nsec = 0;  // atime
        ts[1].tv_sec = t;
        ts[1].tv_nsec = 0;  // mtime
        if (utimensat(AT_FDCWD, path.c_str(), ts, 0) != 0) {
            // fallback
            struct utimbuf ub {
                t, t
            };
            if (utime(path.c_str(), &ub) != 0) {
                throw std::runtime_error("utime failed: " + path);
            }
        }
#else
        struct utimbuf ub {
            t, t
        };
        if (utime(path.c_str(), &ub) != 0) {
            throw std::runtime_error("utime failed: " + path);
        }
#endif
    }

    time_t MakeTime(int yy, int mm, int dd, int hh, int mi, int ss) {
        std::tm t{};
        t.tm_year = yy + 100;  // years since 1900 (yy는 00~99 가정)
        t.tm_mon = mm - 1;
        t.tm_mday = dd;
        t.tm_hour = hh;
        t.tm_min = mi;
        t.tm_sec = ss;
        t.tm_isdst = -1;  // DST 알아서
        return std::mktime(&t);
    }

   protected:
    std::string tmp_root;

    void SetUp() override { tmp_root = MakeTempDir(); }

    void TearDown() override {
        if (!tmp_root.empty()) {
            RemoveAllDir(tmp_root);
        }
    }
};  // namespace

// ---------- TimeManager tests ----------

TEST_F(TimeScanTestFixture, TimeManagerTest_ParseDateTime_Valid) {
    TimeManager& tm = TimeManager::GetInstance();
    time_t tt = tm.ParseDateTime("25/09/04", "00:00:00");

    std::string s = tm.FormatTime(tt);
    std::regex re(R"(\[\d{2}/\d{2}/\d{2}\s\d{2}:\d{2}:\d{2}\])");
    EXPECT_TRUE(std::regex_match(s, re));
}

TEST_F(TimeScanTestFixture, TimeManagerTest_ParseDateTime_Invalid) {
    TimeManager& tm = TimeManager::GetInstance();
    EXPECT_THROW(tm.ParseDateTime("25-09-04", "00:00:00"),
                 std::invalid_argument);
    EXPECT_THROW(tm.ParseDateTime("25/13/40", "25:61:61"),
                 std::invalid_argument);
}

TEST_F(TimeScanTestFixture, TimeManagerTest_MatchesTimeFilter) {
    TimeManager& tm = TimeManager::GetInstance();
    time_t base = MakeTime(25, 9, 4, 0, 0, 0);
    time_t beforeT = base - 10;
    time_t afterT = base + 10;

    EXPECT_TRUE(tm.MatchesTimeFilter(beforeT, base, "before"));
    EXPECT_TRUE(tm.MatchesTimeFilter(base, base, "before"));
    EXPECT_FALSE(tm.MatchesTimeFilter(afterT, base, "before"));

    EXPECT_FALSE(tm.MatchesTimeFilter(beforeT, base, "after"));
    EXPECT_TRUE(tm.MatchesTimeFilter(base, base, "after"));
    EXPECT_TRUE(tm.MatchesTimeFilter(afterT, base, "after"));
}

TEST_F(TimeScanTestFixture, TimeManagerTest_FormatTime_RoundTrip) {
    TimeManager tm = TimeManager::GetInstance();
    time_t t = MakeTime(25, 1, 2, 3, 4, 5);  // 2025-01-02 03:04:05
    std::string s = tm.FormatTime(t);
    EXPECT_THAT(s, HasSubstr("[25/01/02 03:04:05]"));
}

// ---------- FileManager small integration tests ----------

TEST_F(TimeScanTestFixture, FileManagerTest_Traverse_Before_Filter_Works) {
    string d1 = tmp_root + "/A";
    string d2 = tmp_root + "/A/B";
    string f1 = tmp_root + "/A/file_old.txt";
    string f2 = tmp_root + "/A/B/file_new.txt";

    EnsureDir(d1, 0750);
    EnsureDir(d2, 0750);
    TouchFile(f1);
    TouchFile(f2);

    // 기준 시각
    time_t pivot = MakeTime(25, 9, 4, 0, 0, 0);
    // mtime 설정: f1은 pivot-10, f2는 pivot+10
    SetMTime(f1, pivot - 10);
    SetMTime(f2, pivot + 10);

    // 디렉터리 mtime도 조정 (플랫폼에 따라 재귀적으로 달라질 수 있으나 충분)
    SetMTime(d1, pivot - 20);
    SetMTime(d2, pivot + 20);
    SetMTime(tmp_root, pivot - 30);

    FileManager fm;
    vector<Entry> file_entries, dir_entries;

    // mode = before
    fm.Traverse(tmp_root, file_entries, dir_entries, pivot, "before");

    // 기대:
    // - 루트 디렉터리는 절대경로로 들어감
    // - 하위 디렉터리는 ":" 프리픽스로 이름만 들어감 (코드 구현 특성)
    // - before 이므로 f1만 포함, f2는 제외
    // - d2는 mtime이 pivot+20 → 제외, d1은 포함 가능

    // 파일 검증
    bool has_f1 = false, has_f2 = false;
    for (Entry& e : file_entries) {
        if (e.path_ == f1) has_f1 = true;
        if (e.path_ == f2) has_f2 = true;
    }
    EXPECT_TRUE(has_f1);
    EXPECT_FALSE(has_f2);

    // 디렉터리 엔트리 검증(형식)
    bool has_root_abs = false;
    bool has_d1_name = false;
    bool has_d2_name = false;
    for (Entry& e : dir_entries) {
        if (e.path_ == tmp_root) has_root_abs = true;
        if (e.path_ == ":A") has_d1_name = true;   // level=2에서 ":"
        if (e.path_ == "::B") has_d2_name = true;  // level=3에서 "::"
    }
    EXPECT_TRUE(has_root_abs);
    // d1은 mtime pivot-20 → 포함 가능
    EXPECT_TRUE(has_d1_name);
    // d2는 mtime pivot+20 → before에서는 제외되어야 함
    EXPECT_FALSE(has_d2_name);
}

TEST_F(TimeScanTestFixture, FileManagerTest_Traverse_After_Filter_Works) {
    string d1 = tmp_root + "/X";
    string d2 = tmp_root + "/X/Y";
    string f1 = tmp_root + "/X/old.txt";
    string f2 = tmp_root + "/X/Y/new.txt";

    EnsureDir(d1, 0750);
    EnsureDir(d2, 0750);
    TouchFile(f1);
    TouchFile(f2);

    time_t pivot = MakeTime(25, 9, 4, 0, 0, 0);
    SetMTime(f1, pivot - 10);
    SetMTime(f2, pivot + 10);
    SetMTime(d1, pivot + 5);
    SetMTime(d2, pivot + 20);
    SetMTime(tmp_root, pivot - 30);

    FileManager fm;
    vector<Entry> file_entries, dir_entries;
    fm.Traverse(tmp_root, file_entries, dir_entries, pivot, "after");

    // after → f2만 포함
    bool has_f1 = false, has_f2 = false;
    for (Entry& e : file_entries) {
        if (e.path_ == f1) has_f1 = true;
        if (e.path_ == f2) has_f2 = true;
    }
    EXPECT_FALSE(has_f1);
    EXPECT_TRUE(has_f2);

    // 디렉터리: d1/d2는 after 조건 충족
    bool has_root_abs = false;
    bool has_d1_name = false;
    bool has_d2_name = false;
    for (Entry& e : dir_entries) {
        if (e.path_ == tmp_root) has_root_abs = true;
        if (e.path_ == ":X") has_d1_name = true;
        if (e.path_ == "::Y") has_d2_name = true;
    }
    EXPECT_TRUE(has_root_abs);
    EXPECT_TRUE(has_d1_name);
    EXPECT_TRUE(has_d2_name);
}

TEST_F(TimeScanTestFixture, FileManagerTest_WriteToFile_EmitsExpectedFormat) {
    string out = tmp_root + "/out.txt";

    TimeManager& tm = TimeManager::GetInstance();
    time_t t1 = MakeTime(25, 1, 2, 3, 4, 5);

    vector<Entry> entries = {
        Entry("/dummy/path1", t1),
        Entry("/dummy/path2", t1),
    };

    FileManager fm;
    fm.WriteToFile(out, entries);

    std::ifstream ifs(out);
    ASSERT_TRUE(ifs.good());

    std::string line;
    int count = 0;
    while (std::getline(ifs, line)) {
        EXPECT_THAT(line, HasSubstr("[25/01/02 03:04:05] "));
        ++count;
    }
    EXPECT_EQ(count, 2);
}
