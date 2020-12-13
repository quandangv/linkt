#include "fixed_string.hpp"

#include <cctype>

void fixed_string::erase_front(int count) {
  off += count;
}

void fixed_string::erase_back(int count) {
  end_off -= count;
}

void fixed_string::ltrim() {
  for(; !empty() && std::isspace(front()); off++);
}

void fixed_string::rtrim() {
  for(; !empty() && std::isspace(back()); end_off--);
}

void fixed_string::trim() {
  ltrim();
  rtrim();
}

bool fixed_string::empty() const {
  return off == end_off;
}

size_t fixed_string::find(char ch) const {
  for(size_t i = off; i < end_off; i++)
    if (data[i] == ch)
      return i - off;
  return npos;
}

char fixed_string::front() const {
  return data[off];
}

char fixed_string::back() const {
  return data[end_off - 1];
}

const char* fixed_string::begin() const {
  return data + off;
}

const char* fixed_string::end() const {
  return data + end_off;
}

fixed_string fixed_string::substr(size_t pos, size_t length) const {
  return fixed_string(data, off + pos, off + pos + length);
}

fixed_string::operator string() {
  return string(data + off, end_off - off);
}
