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
    return values.at(*index)->get();
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
  //LG_DBUG("doc-add: " << values.back()->get());
  LG_DBUG("doc-add: " << (bool)get(sec, key));
}

string_ref_p& document::add_empty(const string& sec, const string& key) {
  LG_DBUG("add-empty: start: " << key);
  if (auto res = map[sec].emplace(key, values.size()); !res.second) {
    LG_DBUG("add-empty: key already exist: " << sec << "." << key << values[res.first->second].get());
    return values[res.first->second];
  } else {
    LG_DBUG("add-empty: key not exist: " << sec << "." << key);
    return values.emplace_back();
  }
}

GLOBAL_NAMESPACE_END
