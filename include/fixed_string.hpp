#pragma once

#include <string>
#include <cstring>

using std::string;

class fixed_string {
  const char* data;
  size_t off, end_off;

public:
  fixed_string(const char* data, size_t off, size_t end_off) : data(data), off(off), end_off(end_off) {}
  fixed_string(const char* data) : fixed_string(data, 0, off + strlen(data)) {}
  fixed_string(fixed_string& s) : fixed_string(s.data, s.off, s.end_off) {}

  void erase_front(int = 1);
  void erase_back(int = 1);
  void ltrim();
  void rtrim();
  void trim();

  bool empty() const;
  size_t find(char) const;
  char front() const;
  char back() const;
  const char* begin() const;
  const char* end() const;
  fixed_string substr(size_t pos, size_t length) const;

  explicit operator string();
  static constexpr size_t npos = -1;
};
