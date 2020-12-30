#include "document.hpp"
#include "logger.hpp"

#include <sstream>
#include <iostream>

GLOBAL_NAMESPACE

DEFINE_ERROR(document_error)

using std::endl;

string to_string(const document& doc) {
  std::stringstream ss;
  auto print_keyval = [&](const section::value_type& keyval) {
    ss << keyval.first << " = ";
    if (keyval.second.empty())
      ss << endl;
    else if (keyval.second.front() == ' ' || keyval.second.back() == ' ')
      ss << '"' << keyval.second << '"' << endl;
    else 
      ss << keyval.second << endl;
  };
  if(doc.find("") != doc.end())
    for(auto& keyval : doc.at(""))
      print_keyval(keyval);

  for(auto& sec : doc) {
    if(sec.first.empty()) continue;
    ss << endl << '[' << sec.first << ']' << endl;
    for(auto& keyval : sec.second)
      print_keyval(keyval);
  }
  return ss.str();
}

string* find(document& doc, const string& section, const string& key) {
  if (auto sec_it = doc.find(section); sec_it != doc.end())
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
      return &key_it->second;
  return nullptr;
}

const string* find(const document& doc, const string& section, const string& key) {
  if (auto sec_it = doc.find(section); sec_it != doc.end())
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
      return &key_it->second;
  return nullptr;
}

bool try_get(const document& doc, const string& section, const string& key, string& result) {
  if (auto result_ptr = find(doc, section, key); result_ptr != nullptr){
    result = *result_ptr;
    return true;
  }
  return false;
}

string get(const document& doc, const string& section, const string& key) {
  if (string result; try_get(doc, section, key, result)) return result;
  throw document_error("Key not found: " + section + "." + key);
}

string get(const document& doc, const string& section, const string& key, string&& fallback) {
  if (string result; try_get(doc, section, key, result)) return result;
  return std::forward<string>(fallback);
}
GLOBAL_NAMESPACE_END
