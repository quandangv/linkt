#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

constexpr const char excluded_chars[] = "\t \"'=;#[](){}:$\\%";
constexpr const char comment_chars[] = ";#";


std::istream& parse_ini(std::istream& is, document& doc, errorlist& err) {
  string prefix;
  string raw;
  // Iterate through lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    LG_DBUG("parse: line: " << raw);
    tstring line(raw);
    auto report_err_line = [&](const string& msg) {
      err.emplace_back("line " + std::to_string(linecount), msg);
    };
    // Determines if the name is valid
    auto check_name = [&](const tstring& name) {
      for(char c : name)
        if (auto invalid = strchr(excluded_chars, c); invalid)
          return report_err_line("Invalid character '" + string{*invalid} + "' in name"), false;
      return true;
    };
    ltrim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      if (cut_front_back(line, "[", "]")) {
        // detected section header
        if (check_name(line)) {
          prefix = line + ".";
        }
      } else if (auto key = cut_front(line, '='); !key.untouched()) {
        // detected key line
        trim(key);
        if (check_name(key)) {
          trim_quotes(line);
          try {
            doc.add(prefix + key, raw, line);
          } catch (const std::exception& err) {
            LG_DBUG("parse: key error: " << err.what());
            report_err_line(err.what());
          }
        } else report_err_line("Invalid key");
      } else report_err_line("Unparsed line");
    }
  }
  return is;
}

std::ostream& write_ini(std::ostream& os, const container& doc, const string& prefix) {
  vector<std::pair<string, const container*>> containers;
  doc.iterate_children([&](const string& name, const string_ref& child) {
    auto ctn = dynamic_cast<const container*>(&child);
    if(ctn) {
      containers.push_back(std::make_pair(name, ctn));
      return;
    }
    auto value = child.get();
    size_t opening = 0;
    while((opening = value.find("${", opening)) != string::npos) {
      value.insert(value.begin() + opening, '\\');
      opening += 2;
    }
    if (isspace(value.front()) || isspace(value.back()))
      os << name << " = " << '"' << value << '"' << endl;
    else
      os << name << " = " << value << endl;
  });
  for(auto pair : containers) {
    os << endl << '[' << prefix << pair.first << ']' << endl;
    write_ini(os, *pair.second);
  }
  return os;
}

GLOBAL_NAMESPACE_END
