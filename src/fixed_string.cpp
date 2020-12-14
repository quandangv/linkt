#include "fixed_string.hpp"

#include <iostream>
#include <cctype>
#include <algorithm>

#include "logger.hpp"

tmp_fixed_string::tmp_fixed_string(const char* data, size_t off, size_t end_off) : data(data), off(off), end_off(end_off) {}

tmp_fixed_string::tmp_fixed_string(const char* data, size_t length) : off(0), end_off(length), data(data) {}

tmp_fixed_string::tmp_fixed_string(const tmp_fixed_string& s) : tmp_fixed_string(s.data + s.off, s.end_off - s.off) {}

tmp_fixed_string::tmp_fixed_string(const char* data) : tmp_fixed_string(data, strlen(data)) {}

tmp_fixed_string::tmp_fixed_string(const string& s) : tmp_fixed_string(s.c_str(), s.size()) {}

tmp_fixed_string& tmp_fixed_string::erase_front(size_t count) {
  off += std::min(length(), count);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::erase_back(size_t count) {
  end_off -= std::min(length(), count);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::ltrim() {
  for(; !empty() && std::isspace(front()); off++);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::rtrim() {
  for(; !empty() && std::isspace(back()); end_off--);
  return *this;
}

tmp_fixed_string& tmp_fixed_string::trim() {
  ltrim();
  rtrim();
  return *this;
}

bool tmp_fixed_string::empty() const {
  return length() == 0;
}

char tmp_fixed_string::front() const {
  return data[off];
}

char tmp_fixed_string::back() const {
  return data[end_off - 1];
}

const char* tmp_fixed_string::begin() const {
  return data + off;
}

const char* tmp_fixed_string::end() const {
  return data + end_off;
}

size_t tmp_fixed_string::length() const {
  return end_off - off;
}

char tmp_fixed_string::operator[](size_t index) const {
  return data[index + off];
}

tmp_fixed_string tmp_fixed_string::substr(size_t pos, size_t length) const {
  return tmp_fixed_string(data + off + pos, length);
}

size_t tmp_fixed_string::find(char ch) const {
  for(size_t i = off; i < end_off; i++)
    if (data[i] == ch)
      return i - off;
  return npos;
}

string tmp_fixed_string::to_string() const {
  return string(data + off, length());
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

constexpr char copy_scope[] = "copy";
const char* clone(const char* str, size_t length) {
  logger::debug("COPY\n");
  char* result = new char[length];
  memcpy(result, str, length*sizeof(char));
  return result;
}

fixed_string::fixed_string(const char* _data, size_t _length) : tmp_fixed_string(clone(_data, _length), _length) {}

fixed_string::fixed_string(fixed_string&& s) : tmp_fixed_string(s.data, s.off, s.end_off) {
  s.data = nullptr;
  s.off = 0;
  s.end_off = 0;
}

fixed_string& fixed_string::operator=(const fixed_string& other) {
  if (data) delete[] data;
  end_off = other.length();
  data = clone(other.data + other.off, end_off);
  off = 0;
  return *this;
}

fixed_string& fixed_string::operator=(fixed_string&& other) {
  if (data) delete[] data;
  end_off = other.end_off;
  data = other.data;
  off = other.off;
  other.data = nullptr;
  other.off = 0;
  other.end_off = 0;
  return *this;
}

fixed_string::~fixed_string() {
  delete[] data;
}

