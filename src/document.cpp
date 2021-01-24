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
  if (auto index = find(section, key); index) {
    auto value = get_ptr(*index);
    if (value)
      return value->get();
  }
  return {};
}

string document::get(const string& section, const string& key, string&& fallback) const {
  if (auto result = get(section, key); result)
    return *result;
  return forward<string>(fallback);
}

string_ref_p& document::get_ptr(size_t index) const {
  auto ptr = values.at(index);
  if (!ptr) throw error("Invalid value at: " + to_string(index));
  return *ptr;
}

void document::add(const string& sec, const string& key, string&& value) {
  tstring ts(value);
  add_key(*this, sec, key, value, ts);
}

string_ref_p2 document::add(const string& sec, const string& key) {
  if (auto res = map[sec].emplace(key, values.size()); !res.second) {
    return values[res.first->second];
  } else {
    return values.emplace_back(make_shared<string_ref_p>());
  }
}

GLOBAL_NAMESPACE_END
