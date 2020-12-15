#include "fixed_string.hpp"

#include <iostream>
#include <cctype>
#include <algorithm>

#include "logger.hpp"

tmp_fixed_string::tmp_fixed_string(const char* begin, const char* end) : _begin(begin), _end(end) {}

tmp_fixed_string::tmp_fixed_string(const char* data, size_t length) : tmp_fixed_string(data, data + length) {}

tmp_fixed_string::tmp_fixed_string(const tmp_fixed_string& s) : tmp_fixed_string(s._begin, s._end) {}

tmp_fixed_string::tmp_fixed_string(const char* data) : tmp_fixed_string(data, strlen(data)) {}

tmp_fixed_string::tmp_fixed_string(const string& s) : tmp_fixed_string(s.c_str(), s.size()) {}

tmp_fixed_string& tmp_fixed_string::erase_front(size_t count) {
  _begin += std::min(length(), count);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::erase_back(size_t count) {
  _end -= std::min(length(), count);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::ltrim() {
  for(; !empty() && std::isspace(front()); _begin++);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::rtrim() {
  for(; !empty() && std::isspace(back()); _end--);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::trim() {
  ltrim();
  return rtrim();
}

bool tmp_fixed_string::empty() const {
  return length() == 0;
}

char tmp_fixed_string::front() const {
  return *_begin;
}

char tmp_fixed_string::back() const {
  return *(_end -1);
}

const char* tmp_fixed_string::begin() const {
  return _begin;
}

const char* tmp_fixed_string::end() const {
  return _end;
}

size_t tmp_fixed_string::length() const {
  return _end - _begin;
}

char tmp_fixed_string::operator[](size_t index) const {
  return _begin[index];
}

tmp_fixed_string tmp_fixed_string::substr(size_t pos, size_t length) const {
  return tmp_fixed_string(_begin + pos, length);
}

size_t tmp_fixed_string::find(char ch) const {
  for(auto p = _begin; p < _end; p++)
    if (*p == ch)
      return p - _begin;
  return npos;
}

string tmp_fixed_string::to_string() const {
  return string(_begin, length());
}

template<typename T>
std::strong_ordering tmp_fixed_string::compare(const T& other) const {
  auto len = length();
  if (auto diff = other.length() <=> len; diff != std::strong_ordering::equal)
    return diff;
  for(size_t i = 0; i < len; i++)
    if (auto diff = other[i] <=> (*this)[i]; diff != std::strong_ordering::equal)
      return diff;
  return std::strong_ordering::equal;
}

std::strong_ordering tmp_fixed_string::operator<=>(const tmp_fixed_string& other) const {
  return compare(other);
}

std::strong_ordering tmp_fixed_string::operator<=>(const string& other) const {
  return compare(other);
}

bool tmp_fixed_string::operator==(const tmp_fixed_string& other) const {
  return compare(other) == std::strong_ordering::equal;
}

bool tmp_fixed_string::operator<(const tmp_fixed_string& other) const {
  return compare(other) == std::strong_ordering::less;
}

bool tmp_fixed_string::operator>(const tmp_fixed_string& other) const {
  return compare(other) == std::strong_ordering::greater;
}

string operator+(const tmp_fixed_string& a, const string& b) {
  return a.to_string() + b;
}

int fixed_string::copy_count = 0;
const char* clone(const char* str, size_t length) {
  if constexpr(logger::has_scope<copy_scope>()) {
    fixed_string::copy_count++;
  }
  char* result = new char[length];
  memcpy(result, str, length*sizeof(char));
  return result;
}

fixed_string::fixed_string(const char* data, size_t length) : tmp_fixed_string(clone(data, length), length), _data(_begin) {}

fixed_string::fixed_string(fixed_string&& s) : tmp_fixed_string(s._begin, s._end), _data(s._data) {
  s._data = s._begin = s._end = nullptr;
}

fixed_string::fixed_string(const tmp_fixed_string& s) : fixed_string(s.begin(), s.length()) {}
fixed_string::fixed_string(const char* data) : fixed_string(data, strlen(data)) {}
fixed_string::fixed_string(const string& s) : fixed_string(s.c_str(), s.size()) {}

fixed_string& fixed_string::operator=(const fixed_string& other) {
  if (_data != nullptr) delete[] _data;
  _data = clone(other._begin, other.length());
  _begin = _data;
  _end = _begin + other.length();
  return *this;
}

fixed_string& fixed_string::operator=(fixed_string&& other) {
  if (_data != nullptr) delete[] _data;
  _data = other._data;
  _begin = other._begin;
  _end = other._end;
  other._data = other._begin = other._end = nullptr;
  return *this;
}

fixed_string::~fixed_string() {
  if (_data != nullptr) delete[] _data;
}

bool tmp_fixed_string::cut_front_back(const char* fs, const char* bs) {
  const char* fp = begin(), *bp = end();
  auto fs_length = strlen(fs);
  if (fs_length > length()) return false;
  for(; *fs; fp++, fs++)
    if(*fs != *fp)
      return false;

  auto bs_length = strlen(bs);
  bp -= bs_length;
  if (fp > bp) return false;
  for(; *bs; bp++, bs++)
    if(*bs != *bp)
      return false;
  erase_front(fs_length);
  erase_back(bs_length);
  return true;
}
