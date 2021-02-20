#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

constexpr const char excluded_chars[] = "\t \"'=;#[](){}:$\\%";
constexpr const char comment_chars[] = ";#";


void parse_ini(std::istream& is, wrapper& root, errorlist& err) {
  string prefix;
  string raw;
  // Iterate through lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    tstring line(raw);
    ltrim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      if (cut_front_back(line, "["_ts, "]"_ts)) {
        prefix = line + ".";
      } else if (tstring key; err.extract_key(line, linecount, '=', key)) {
        try {
          root.add(prefix + key, raw, trim_quotes(line));
        } catch (const std::exception& e) {
          err.report_error(linecount, e.what());
        }
      }
    }
  }
}

void write_ini(std::ostream& os, const container& root, const string& prefix) {
  vector<std::pair<string, const container*>> containers;
  root.iterate_children([&](const string& name, const string_ref& child) {
    auto ctn = dynamic_cast<const container*>(&child);
    if(ctn) {
      containers.push_back(std::make_pair(name, ctn));
      if (auto value = child.get(); !value.empty())
        write_key(os, name + " = ", move(value));
    } else
      write_key(os, name + " = ", child.get());
  });
  for(auto pair : containers) {
    os << endl << '[' << prefix << pair.first << ']' << endl;
    write_ini(os, *pair.second);
  }
}

GLOBAL_NAMESPACE_END
