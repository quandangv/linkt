// C string wrappers that provides constant-time erase front/back and substr operations
#pragma once

#include <string>
#include <cstring>

using std::string;

// This class doesn't copy string data on construction
// Will be invalidated as soon as the original string go out of scope
class tmp_fixed_string {
protected:
  const char* _begin, *_end;
  tmp_fixed_string(const char*, const char*);

public:
  // const time operations
  tmp_fixed_string() : tmp_fixed_string(nullptr, nullptr) {}
  tmp_fixed_string(const char* data, size_t length);
  explicit tmp_fixed_string(const tmp_fixed_string&);
  explicit tmp_fixed_string(const char*);
  explicit tmp_fixed_string(const string&);

  size_t length() const;
  const char* c_str() const { return _begin; }
  tmp_fixed_string& erase_front(size_t = 1);
  tmp_fixed_string& erase_back(size_t = 1);
  tmp_fixed_string& ltrim();
  tmp_fixed_string& rtrim();
  tmp_fixed_string& trim();
  bool cut_front_back(const char* front, const char* back = "");
  bool empty() const;
  char front() const;
  char back() const;
  const char* begin() const;
  const char* end() const;
  tmp_fixed_string substr(size_t pos, size_t length) const;
  char operator[](size_t) const;

  // linear time operations
  template<typename T> std::strong_ordering compare(const T&) const;
  std::strong_ordering operator<=>(const tmp_fixed_string& other) const;
  std::strong_ordering operator<=>(const string& other) const;
  bool operator==(const tmp_fixed_string&) const;
  bool operator<(const tmp_fixed_string&) const;
  bool operator>(const tmp_fixed_string&) const;
  static constexpr size_t npos = -1;

  size_t find(char) const;
  string to_string() const;
  operator string() const { return to_string(); }
};

string operator+(const tmp_fixed_string& a, const string& b);

// This class copies its string data on construction
// Doesn't get invalidated
class fixed_string : public tmp_fixed_string {
protected:
  const char* _data;
public:
  fixed_string() : tmp_fixed_string(), _data(nullptr) {}
  fixed_string(const char*, size_t length);
  fixed_string(fixed_string&& s);
  explicit fixed_string(const tmp_fixed_string& s);
  explicit fixed_string(const char* data);
  explicit fixed_string(const string& s);
  ~fixed_string();

  fixed_string& operator=(const fixed_string&);
  fixed_string& operator=(fixed_string&&);

  static int copy_count;
};
