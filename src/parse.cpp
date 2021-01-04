#include "parse_delink.hpp"
#include "logger.hpp"

#include <cstring>
#include <iostream>
#include <vector>

#include "tstring.hpp"

GLOBAL_NAMESPACE

constexpr const char excluded_chars[] = "\"'=;#[](){}:.$\\%";
constexpr const char comment_chars[] = ";#";

using namespace std;

std::istream& parse(std::istream& is, document& doc, errorlist& err, const string& initial_section, char line_separator) {
  auto current_sec = &doc.map.emplace(initial_section, document::sec_map{}).first->second;
  string raw;
  for (int linecount = 1; std::getline(is, raw, line_separator); linecount++, raw.clear()) {
    tstring line(raw);
    // Determines if the name is invalid
    auto check_name = [&](const tstring& name) {
      auto invalid = std::find_if(name.begin(), name.end(), [](unsigned char ch) {
        return std::isspace(ch) || strchr(excluded_chars, ch);
      });
      if (invalid != name.end()) {
        err.emplace_back(linecount, "Invalid character '" + string{*invalid} + "' in name");
        return true;
      }
      return false;
    };
    line.trim();

    // skip empty and comment lines
    if (line.empty() || strchr(comment_chars, line.front())) continue;

    // detect section headers
    if (line.cut_front_back("[", "]")) {
      if (check_name(line))
        continue;
      current_sec = &doc.map[line];
      continue;
    }

    // detect key lines
    if (auto sep = line.find('='); sep != tstring::npos) {
      auto key = line.substr(0, sep);
      key.trim();
      if (check_name(key))
        continue;

      line.erase_front(sep + 1);
      line.trim_quotes();
      if (current_sec->emplace(key, doc.values.size()).second) {
        doc.values.emplace_back(make_unique<onetime_string>(line));
      } else {
        err.emplace_back(linecount, "Duplicate key: " + key.to_string() + ", Existing value: " + doc.values[(*current_sec)[key]]->get());
      }
      continue;
    }
    err.emplace_back(linecount, "Unparsed line");
  }
  return is;
}

GLOBAL_NAMESPACE_END
