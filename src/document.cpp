#include "document.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "add_key.hpp"

#include <sstream>
#include <iostream>

GLOBAL_NAMESPACE

using namespace std;

bool document::set(const string& section, const string& key, const string& value) {
  if (auto ptr = get_ptr(section, key); ptr)
    if (auto& ref = *ptr; ref)
      if (!ref->readonly())
        return ref->set(value), true;
  return false;
}

optional<string> document::get(const string& section, const string& key) const {
  if (auto ptr = get_ptr(section, key); ptr) {
    if (auto& value = *ptr; value) {
      return value->get();
    } else
      LG_INFO("document-get: failed due to value being null: " << key);
  } else
    LG_INFO("document-get: failed due to key not found: " << key);
  return {};
}

string document::get(const string& section, const string& key, string&& fallback) const {
  if (auto result = get(section, key); result)
    return *result;
  return forward<string>(fallback);
}

string_ref_p2 document::get_ptr(const string& section, const string& key) const {
  if (auto sec_it = map.find(section); sec_it != map.end()) {
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end()) {
      return key_it->second;
    }
  }
  return {};
}

void document::add(const string& sec, const string& key, string&& value) {
  tstring ts(value);
  add_key(*this, sec, key, value, ts);
}

string_ref_p2 document::add(const string& sec, const string& key) {
  auto& ptr = map[sec][key];
  if (!ptr)
    ptr = make_shared<string_ref_p>();
  return ptr;
}

void document::optimize(string_ref_p2 value) {
  if (!value || !*value) return;
  if (auto newval = value->get()->get_optimized(); newval)
    *value = move(newval);
}

GLOBAL_NAMESPACE_END
