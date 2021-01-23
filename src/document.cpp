#include "document.hpp"
#include "logger.hpp"
#include "tstring.hpp"
#include "add_key.hpp"

#include <sstream>
#include <iostream>

GLOBAL_NAMESPACE

using namespace std;

optional<size_t> document::find(const string& section, const string& key) const {
  if (auto sec_it = map.find(section); sec_it != map.end())
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
      return key_it->second;
  return {};
}

optional<string> document::get(const string& section, const string& key) const {
  if (auto index = find(section, key); index)
    return values.at(*index)->get();
  return {};
}

string document::get(const string& section, const string& key, string&& fallback) const {
  if (auto result = get(section, key); result)
    return *result;
  return forward<string>(fallback);
}

void document::add(const string& sec, const string& key, string&& value) {
  tstring ts(value);
  add_key(*this, sec, key, value, ts);
}

GLOBAL_NAMESPACE_END
