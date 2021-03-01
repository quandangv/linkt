#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

NAMESPACE(linked_nodes)

constexpr const char excluded_chars[] = "\t \"'=;#[](){}:$\\%";
constexpr const char comment_chars[] = ";#";


void parse_ini(std::istream& is, node::wrapper& root, errorlist& err) {
  string prefix;
  string raw;
  // Iterate through lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    tstring line(raw);
    // skip empty and comment lines
    if (trim(line).empty() || strchr(comment_chars, line.front()))
      continue;
    if (cut_front_back(line, "["_ts, "]"_ts)) {
      // This is a section
      prefix = line + ".";
      continue;
    } if (tstring key; err.extract_key(line, linecount, '=', key)) {
      // This is a key
      try {
        root.add(prefix + key, raw, trim_quotes(line));
      } catch (const std::exception& e) {
        err.report_error(linecount, e.what());
      }
    }
  }
}

void write_ini(std::ostream& os, const node::wrapper& root, const string& prefix) {
  vector<std::pair<string, const node::wrapper*>> wrappers;
  root.iterate_children([&](const string& name, const node::base& child) {
    // The empty key is used as the value of the wrapper, skip it
    if (name.empty())
      return;
    auto ctn = dynamic_cast<const node::wrapper*>(&child);
    if(ctn) {
      // The keys with children will be written after the other keys
      // Otherwise, they will break the section
      wrappers.push_back(std::make_pair(name, ctn));
      if (auto value = child.get(); !value.empty())
        write_key(os, name + " = ", move(value));
    } else
      write_key(os, name + " = ", child.get());
  });
  for(auto pair : wrappers) {
    os << endl << '[' << prefix << pair.first << ']' << endl;
    write_ini(os, *pair.second);
  }
}

NAMESPACE_END
