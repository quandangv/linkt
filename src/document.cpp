#include "document.hpp"
#include "logger.hpp"

#include <sstream>
#include <iostream>

GLOBAL_NAMESPACE

using namespace std;

using std::endl;

bool document::add_onetime(const string& section, const string& key, string&& value) {
  auto existing = find(section, key);
  if (existing) {
    values[*existing] = make_unique<onetime_string>(move(value));
    return false;
  }
  map[section][key] = values.size();
  values.emplace_back(make_unique<onetime_string>(move(value)));
  return true;
}

string document::to_string() const {
  stringstream ss;
  auto print_keyval = [&](const sec_map::value_type& keyval) {
    ss << keyval.first << " = ";
    auto value = values[keyval.second]->get();
    if (value.empty())
      ss << endl;
    else if (value.front() == ' ' || value.back() == ' ')
      ss << '"' << value << '"' << endl;
    else 
      ss << value << endl;
  };
  if(map.find("") != map.end())
    for(auto& keyval : map.at(""))
      print_keyval(keyval);

  for(auto& sec : map) {
    if(sec.first.empty()) continue;
    ss << endl << '[' << sec.first << ']' << endl;
    for(auto& keyval : sec.second)
      print_keyval(keyval);
  }
  return ss.str();
}

optional<size_t> document::find(const string& section, const string& key) const {
  if (auto sec_it = map.find(section); sec_it != map.end())
    if (auto key_it = sec_it->second.find(key); key_it != sec_it->second.end())
      return key_it->second;
  return {};
}

opt_str document::get(const string& section, const string& key) const {
  if (auto index = find(section, key); index)
    return values.at(*index)->get();
  return {};
}

string document::get(const string& section, const string& key, string&& fallback) const {
  if (auto result = get(section, key); result)
    return *result;
  return forward<string>(fallback);
}

GLOBAL_NAMESPACE_END
