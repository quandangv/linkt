#include "tstring.hpp"

#include <iostream>
#include <cctype>
#include <algorithm>

using namespace std;

const char* clone(const char* str, size_t length) {
  char* result = new char[length];
  memcpy(result, str, length*sizeof(char));
  return result;
}

tstring& tstring::set(const string& s) {
  this->data = s.data();
  pos = 0;
  end_pos = s.length();
  return *this;
}

tstring& tstring::erase_front(size_t count) {
  pos += min(length(), count);
  return *this;
}

tstring& tstring::erase_back(size_t count) {
  end_pos -= min(length(), count);
  return *this;
}

tstring& tstring::set_length(size_t length) {
  end_pos = pos + min(size(), length);
  return *this;
}

tstring& ltrim(tstring& ts, const char* trim_char) {
  auto ptr = ts.begin(), end = ts.end();
  for(; ptr != end && strchr(trim_char, *ptr) != nullptr; ptr++);
  ts.erase_front(ptr - ts.begin());
  return ts;
}

tstring& rtrim(tstring& ts, const char* trim_char) {
  auto ptr = ts.rbegin(), end = ts.rend();
  for(; ptr != end && strchr(trim_char, *ptr) != nullptr; ptr++);
  ts.erase_back(ptr - ts.rbegin());
  return ts;
}

tstring& trim_quotes(tstring& ts) {
  trim(ts);
  cut_front_back(ts, "'", "'");
  cut_front_back(ts, "\"", "\"");
  return ts;
}

bool cut_front(tstring& ts, const char* front) {
  auto lfront = strlen(front);
  if (lfront > ts.length() || !std::equal(front, front + lfront, ts.begin()))
    return false;
  ts.erase_front(lfront);
  return true;
}

tstring cut_front(tstring& ts, char limit) {
  if (auto lim = find(ts, limit); lim == tstring::npos) {
    return tstring();
  } else {
    auto result = ts.interval(0, lim);
    ts.erase_front(lim + 1);
    return result;
  }
}

bool cut_back(tstring& ts, const char* back) {
  auto lback = strlen(back);
  if (lback > ts.length() || !std::equal(back, back + lback, ts.end() - lback))
    return false;
  ts.erase_back(lback);
  return true;
}

tstring cut_back(tstring& ts, char limit) {
  if (auto lim = rfind(ts, limit); lim == tstring::npos) {
    return tstring();
  } else {
    auto result = ts.interval(lim + 1);
    ts.set_length(lim);
    return result;
  }
}

bool cut_front_back(tstring& ts, const char* front, const char* back) {
  auto lfront = strlen(front);
  auto lback = strlen(back);
  if (lfront + lback > ts.length() || !std::equal(front, front + lfront, ts.begin()) || !std::equal(back, back + lback, ts.end() - lback))
    return false;
  ts.erase_front(lfront);
  ts.erase_back(lback);
  return true;
}

bool tstring::empty() const {
  return length() == 0;
}

bool tstring::untouched() const {
  return data == nullptr;
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

reverse_iterator<const char*> tstring::rbegin() const {
  return reverse_iterator<const char*>(data + end_pos);
}

reverse_iterator<const char*> tstring::rend() const {
  return reverse_iterator<const char*>(data + pos);
}

size_t tstring::length() const {
  return end_pos - pos;
}

char tstring::operator[](size_t index) const {
  return data[pos + index];
}

tstring tstring::interval(size_t start, size_t end) const {
  if (start >= end_pos)
    return tstring(data, end_pos, end_pos);
  return tstring(data, pos + start, min(end_pos, pos + end));
}

tstring substr(const tstring& ts, size_t offset, size_t len) {
  return ts.interval(offset, offset + len);
}

size_t find(const tstring& ts, char ch) {
  auto result = std::find(ts.begin(), ts.end(), ch);
  return result == ts.end() ? tstring::npos : result - ts.begin();
}

size_t rfind(const tstring& ts, char ch) {
  auto result = std::find(ts.rbegin(), ts.rend(), ch);
  return result == ts.rend() ? tstring::npos : result.base() - ts.begin() - 1;
}

tstring& tstring::erase(string& source, size_t off, size_t length) {
  auto s = size();
  // Try to use linear-time methods first
  if (off < s) {
    if (off == 0)
      erase_front(length);
    else if (length + off > s) {
      set_length(off);
    } else {
      source.erase(pos + off, length);
      end_pos -= length;
      data = source.data();
    }
  }
  return *this;
}

template<typename T>
strong_ordering tstring::compare(const T& other) const {
  auto len = length();
  if (auto diff = other.length() <=> len; diff != 0)
    return diff;
  for(size_t i = 0; i < len; i++)
    if (auto diff = other[i] <=> (*this)[i]; diff != 0)
      return diff;
  return strong_ordering::equal;
}

tstring::operator string() const {
  return string(&*begin(), size());
}

bool find_enclosed(tstring& str, string& src,
                   const string& start_group, const string& end_group,
                   size_t& start, size_t& end) {
  size_t opening_count = 0;
  auto ptr = str.begin(),
       start_limit = str.end() + 1 - std::max(start_group.size(), end_group.size()),
       end_limit = str.end() + 1 - end_group.size();
  for(; ptr < end_limit; ptr++) {
    if (ptr < start_limit && std::equal(start_group.data(), start_group.data() + start_group.size(), ptr)) {
      if (ptr > str.begin() && ptr[-1] == '\\') {
        str.erase(src, --ptr - str.begin(), 1);
      } else if (opening_count++ == 0)
        start = ptr - str.begin();
      ptr += start_group.size() - 1;
    } else if (std::equal(end_group.data(), end_group.data() + end_group.size(), ptr)) {
      ptr += end_group.size() - 1;
      if (opening_count > 0 && --opening_count == 0) {
        end = ptr - str.begin() + 1;
        return true;
      }
    }
  }
  return false;
}
