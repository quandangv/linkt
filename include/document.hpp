#pragma once

#include <map>
#include <vector>
#include <string>

namespace lini {
  using std::string;

  using section = std::map<string, string>;
  using document = std::map<string, section>;
  using errorlist = std::vector<std::pair<int, string>>;
  using str_errlist = std::vector<std::pair<string, string>>;

  string to_string(const document&);
  string* find(document&, const string& section, const string& key);
  const string* find(const document&, const string& section, const string& key);
  bool try_get(const document&, const string& section, const string& key, string& result);
  string get(const document&, const string& section, const string& key);
  string get(const document&, const string& section, const string& key, string&& fallback);
}
