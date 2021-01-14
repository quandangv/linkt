#include "parse_delink.hpp"
#include "logger.hpp"
#include "tstring.hpp"

#include <cstring>
#include <iostream>
#include <vector>

GLOBAL_NAMESPACE

using namespace std;

constexpr const char excluded_chars[] = "\t \"'=;#[](){}:.$\\%";
constexpr const char comment_chars[] = ";#";

std::istream& parse(std::istream& is, document& doc, errorlist& err, const string& initial_section, char line_separator) {
  auto current_sec = &doc.map.emplace(initial_section, document::sec_map{}).first->second;
  string raw;
  for (int linecount = 1; std::getline(is, raw, line_separator); linecount++, raw.clear()) {
    tstring line(raw);
    // Determines if the name is valid
    auto check_name = [&](const tstring& name) {
      auto invalid = std::find_if(name.begin(), name.end(), [](char ch) { return strchr(excluded_chars, ch); });
      if (invalid == name.end())
        return true;
      err.emplace_back(linecount, "Invalid character '" + string{*invalid} + "' in name");
      return false;
    };
    trim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      if (cut_front_back(line, "[", "]")) {
        // detected section header
        if (check_name(line))
          current_sec = &doc.map[line];
      } else if (auto key = cut_front(line, '='); !key.untouched()) {
        // detected key line
        trim(key);
        if (check_name(key)) {
          trim_quotes(line);
          if (current_sec->emplace(key, doc.values.size()).second) {
            doc.values.emplace_back(make_unique<onetime_ref>(line));
          } else {
            err.emplace_back(linecount, "Duplicate key: " + key + ", Existing value: " + doc.values[(*current_sec)[key]]->get());
          }
        }
      } else err.emplace_back(linecount, "Unparsed line");
    }
  }
  return is;
}

GLOBAL_NAMESPACE_END
