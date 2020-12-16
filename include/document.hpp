#pragma once

#include <map>
#include <string>

#include "error.hpp"

using std::string;

using section = std::map<string, string>;
using document = std::map<string, section>;
using errorlist = std::map<int, string>;
using str_errlist = std::map<string, string>;

DEFINE_ERROR(key_not_found_error)

string to_string(const document&);

inline string* find(document& doc, const string& section, const string& key) {
  if (auto sec_it = doc.find(section); sec_it != doc.end())
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
      return &key_it->second;
  return nullptr;
}

inline const string* find(const document& doc, const string& section, const string& key) {
  if (auto sec_it = doc.find(section); sec_it != doc.end())
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
      return &key_it->second;
  return nullptr;
}

inline bool try_get(const document& doc, const string& section, const string& key, string& result) {
  if (auto result_ptr = find(doc, section, key); result_ptr != nullptr){
    result = *result_ptr;
    return true;
  }
  return false;
}

inline string get(const document& doc, const string& section, const string& key) {
  if (string result; try_get(doc, section, key, result)) return result;
  throw key_not_found_error("Key not found: " + section + "." + key);
}

inline string get(const document& doc, const string& section, const string& key, string&& fallback) {
  if (string result; try_get(doc, section, key, result)) return result;
  return std::forward<string>(fallback);
}
