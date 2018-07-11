#ifndef UTIL_H
#define UTIL_H

#include <string>
#include <vector>
#include <random>
#include <cstdio>
#include <cstdarg>

#include <chrono>

inline void SplitString(std::vector<std::string>& res, const std::string &str,
  const std::string &delim) {
  if (str.empty()) return;
  res.clear();
  int offset = 0;
  std::string::size_type find_res = 0;
  while ((find_res = str.find(delim, offset)) != std::string::npos) {
    res.push_back(str.substr(offset, find_res - offset));
    offset = find_res + delim.size();
  }
  res.push_back(str.substr(offset));
}

inline int pow(int a, int b) {
  int res = 1;
  for (int i = 0; i < b; ++i) res *= a;
  return res;
}

template<typename T>
inline std::vector<T> MergeVectors(const std::vector<T> &lhs, const std::vector<T> &rhs) {
  std::vector<T> vec;
  for (auto &i : lhs) vec.push_back(i);
  for (auto &i : rhs) vec.push_back(i);
}

struct Randomer {
  // [lower, upper)
  static int RandInt(int lower, int upper) {
    std::random_device rd;
    return rd() % (upper - lower) + lower;
  }
};

struct Logger {
  void VLog(const char *tag, const char *format, va_list vl) {
    if (to_screen) {
      printf("[%s]\t", tag);
      vprintf(format, vl);
      printf("\n");
    }
    if (to_file) {
      fprintf(fd, "[%s]\t", tag);
      vfprintf(fd, format, vl);
      fprintf(fd, "\n");
    }
  }

  void Debug(const char *format, ...) {
    va_list args;
    va_start(args, format);
    VLog("Debug", format, args);
    va_end(args);
  }

  void Info(const char *format, ...) {
    va_list args;
    va_start(args, format);
    VLog("Info", format, args);
    va_end(args);
  }

  Logger(bool to_screen = true, bool to_file = false, const std::string &filename = "")
    : to_screen(to_screen), to_file(to_file), filename(filename) {
    fd = fopen(filename.c_str(), "a");
  }

  bool to_screen;
  bool to_file;
  const std::string filename;
 private:
  FILE *fd;
};

using namespace std::chrono;
using high_clock = high_resolution_clock;
class TikTok {
  high_clock::time_point t;
  std::string tag;
 public:
  void Tik() {
    t = high_clock::now();
  }
  void Tok() {
    auto dur = high_clock::now() - t;
    printf("[-%s-] %lf\n", tag.c_str(),
      float(std::chrono::duration_cast<milliseconds>(dur).count()) / 1000.0);
  }
  TikTok(const std::string &tag) : tag(tag) {}
};

#endif
