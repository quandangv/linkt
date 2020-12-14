#pragma once

#include <map>
#include <string>

#include "error.hpp"
#include "fixed_string.hpp"

using std::string;

using section = std::map<string, fixed_string>;
using document = std::map<string, section>;
using errorlist = std::map<int, string>;

DEFINE_ERROR(key_not_found_error)

string to_string(const document&);

inline bool try_get(const document& doc, const fixed_string& section, const fixed_string& key, fixed_string& result) {
  if (auto sec_it = doc.find(section); sec_it != doc.end()) {
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
      result = key_it->second;
      return true;
  }
  return false;
}

inline fixed_string get(const document& doc, const fixed_string& section, const fixed_string& key) {
  if (fixed_string result; try_get(doc, section, key, result)) return result;
  throw key_not_found_error("Key not found: " + section.to_string() + "." + key.to_string());
}

inline fixed_string get(const document& doc, const fixed_string& section, const fixed_string& key, fixed_string&& fallback) {
  if (fixed_string result; try_get(doc, section, key, result)) return result;
  return std::forward<fixed_string>(fallback);
}
