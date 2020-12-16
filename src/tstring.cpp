#include "tstring.hpp"

#include <iostream>
#include <cctype>
#include <algorithm>

#include "logger.hpp"

const char* clone(const char* str, size_t length) {
  char* result = new char[length];
  memcpy(result, str, length*sizeof(char));
  return result;
}

tstring::tstring(const char* data, size_t pos, size_t end_pos) : data(data), pos(pos), end_pos(end_pos) {}

tstring::tstring(const char* data, size_t length) : tstring(data, 0, length) {}

tstring::tstring(const tstring& s) : tstring(s.data, s.pos, s.end_pos) {}

tstring::tstring(const char* data) : tstring(data, strlen(data)) {}

tstring::tstring(const string& s) : tstring(s.c_str(), s.size()) {}

tstring& tstring::erase_front(size_t count) {
  pos += std::min(length(), count);
  return *this;
}

tstring& tstring::erase_back(size_t count) {
  end_pos -= std::min(length(), count);
  return *this;
}

tstring& tstring::ltrim() {
  for(; !empty() && std::isspace(front()); pos++);
  return *this;
}

tstring& tstring::rtrim() {
  for(; !empty() && std::isspace(back()); end_pos--);
  return *this;
}

tstring& tstring::trim() {
  ltrim();
  return rtrim();
}

bool tstring::empty() const {
  return length() == 0;
}

char tstring::front() const {
  return data[pos];
}

char tstring::back() const {
  return data[end_pos - 1];
}

const char* tstring::begin() const {
  return data + pos;
}

const char* tstring::end() const {
  return data + end_pos;
}

size_t tstring::length() const {
  return end_pos - pos;
}

char tstring::operator[](size_t index) const {
  return data[pos + index];
}

tstring tstring::substr(size_t index, size_t len) const {
  if (index >= end_pos)
    return tstring();
  return tstring(data + pos + index, std::min(len, length() - index));
}

size_t tstring::find(char ch) const {
  for(auto p = begin(); p < end(); p++)
    if (*p == ch)
      return p - begin();
  return npos;
}

string tstring::to_string() const {
  return string(begin(), length());
}

template<typename T>
std::strong_ordering tstring::compare(const T& other) const {
  auto len = length();
  if (auto diff = other.length() <=> len; diff != std::strong_ordering::equal)
    return diff;
  for(size_t i = 0; i < len; i++)
    if (auto diff = other[i] <=> (*this)[i]; diff != std::strong_ordering::equal)
      return diff;
  return std::strong_ordering::equal;
}

std::strong_ordering tstring::operator<=>(const tstring& other) const {
  return compare(other);
}

std::strong_ordering tstring::operator<=>(const string& other) const {
  return compare(other);
}

bool tstring::operator==(const tstring& other) const {
  return compare(other) == std::strong_ordering::equal;
}

bool tstring::operator<(const tstring& other) const {
  return compare(other) == std::strong_ordering::less;
}

bool tstring::operator>(const tstring& other) const {
  return compare(other) == std::strong_ordering::greater;
}

string operator+(const tstring& a, const string& b) {
  return a.to_string() + b;
}

bool tstring::cut_front_back(const char* fs, const char* bs) {
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

  pos += fs_length;
  end_pos -= bs_length;
  return true;
}
