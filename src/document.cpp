#include "document.hpp"
#include "common.hpp"
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
  LG_DBUG("document-get: " << key)
  if (auto index = find(section, key); index) {
    LG_DBUG("document-get: found " << *index)
    auto value = *values.at(*index);
    if (value)
      return value->get();
    LG_DBUG("document-get: key is empty")
  }
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
  LG_DBUG("document-add: key: " << key);
}

document::value_t document::add_empty(const string& sec, const string& key) {
  LG_DBUG("document-add-empty: start: " << key);
  if (auto res = map[sec].emplace(key, values.size()); !res.second) {
    LG_DBUG("document-add-empty: key already exist: " << sec << "." << key << " index: " << res.first->second);
    return values[res.first->second];
  } else {
    LG_DBUG("document-add-empty: key not exist: " << sec << "." << key << ", add index: " << values.size());
    return values.emplace_back(make_shared<string_ref_p>());
  }
}

GLOBAL_NAMESPACE_END
