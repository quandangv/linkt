#include "parse.hpp"

#include <cstring>
#include <iostream>
#include <vector>

#include "fixed_string.hpp"

constexpr const char excluded_chars[] = "\"'=;#[](){}:.$\\%";
constexpr const char comment_chars[] = ";#";
constexpr const char quote_chars[] = "'\"";

using std::move;

// Returns if the name is invalid
inline bool check_name(tmp_fixed_string& name, errorlist& err, int linecount) {
  auto invalid = std::find_if(name.begin(), name.end(), [](unsigned char ch) {
    return std::isspace(ch) || strchr(excluded_chars, ch);
  });
  if (invalid != name.end()) {
    err[linecount] = "Invalid character '" + string{*invalid} + "' in name";
    return true;
  }
  return false;
}

void parse(std::istream& is, document& doc, errorlist& err) {
  auto current_sec = &doc.emplace(fixed_string(""), section{}).first->second;
  std::vector<char> line_space(256);
  string raw;
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    tmp_fixed_string line(raw);

    // trim spaces
    line.trim();

    // skip empty and comment lines
    if (line.empty() || strchr(comment_chars, line.front())) continue;

    // detect section headers
    if (line.front() == '[') {
      if (line.back() != ']') {
        err[linecount] = "Expected ']' at the end of a header line";
        continue;
      }
      line.erase_front();
      line.erase_back();
      if (check_name(line, err, linecount))
        continue;

      current_sec = &doc[fixed_string(line)];
      continue;
    }

    // detect key lines
    if (auto sep = line.find('='); sep != fixed_string::npos) {
      auto key = line.substr(0, sep);
      key.trim();
      if (check_name(key, err, linecount))
        continue;

      line.erase_front(sep + 1);
      line.trim();
      if (strchr(quote_chars, line.front()) && line.front() == line.back()) {
        line.erase_front();
        line.erase_back();
      }
      auto fixed_key = fixed_string(key);
      if (!current_sec->emplace(fixed_key, fixed_string(line)).second) {
        err[linecount] = "Duplicate key: " + key.to_string() + ", Existing value: " + (*current_sec)[fixed_key].to_string();
      }
    }
  }
}
