#include "parse.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "add_key.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

using namespace std;

constexpr const char excluded_chars[] = "\t \"'=;#[](){}:.$\\%";
constexpr const char comment_chars[] = ";#";


std::istream& parse(std::istream& is, document& doc, errorlist& err) {
  auto report_err_key = [&](const string& section, const string& key, const string& msg) {
    err.emplace_back("key " + section + "." + key, msg);
  };
  string section;
  auto add_section = [&](const tstring& name) {
    section = name;
    doc.add(section, make_unique<document>(), true)->get();
  };
  // Add initial section
  add_section(tstring());
  string raw;
  // Iterate through lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    LG_DBUG("parse: line: " << raw);
    tstring line(raw);
    auto report_err_line = [&](const string& msg) {
      err.emplace_back("line " + to_string(linecount), msg);
    };
    auto check_name = [&](const tstring& name) {
      // Determines if the name is valid
      auto invalid = std::find_if(name.begin(), name.end(), [](char ch) { return strchr(excluded_chars, ch); });
      if (invalid == name.end())
        return true;
      report_err_line("Invalid character '" + string{*invalid} + "' in name");
      return false;
    };
    ltrim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      if (cut_front_back(line, "[", "]")) {
        // detected section header
        if (check_name(line)) {
          add_section(line);
        }
      } else if (auto key = cut_front(line, '='); !key.untouched()) {
        // detected key line
        trim(key);
        if (check_name(key)) {
          trim_quotes(line);
          try {
            doc.add(section + "." + key, raw, line);
          } catch (const exception& err) {
            LG_DBUG("parse: key error: " << err.what());
            report_err_line(err.what());
          }
        } else report_err_line("Invalid key");
      } else report_err_line("Unparsed line");
    }
  }
  return is;
}
//
//ostream& write(ostream& os, const document& doc) {
//  auto print_key_pair = [&](const document::sec_map::value_type& key_pair) {
//    auto& ptr = key_pair.second;
//    if (!ptr || !*ptr) return;
//    auto value = (*key_pair.second)->get();
//    os << key_pair.first << " = ";
//    if (value.empty())
//      os << endl;
//    else if (value.front() == ' ' || value.back() == ' ')
//      os << '"' << value << '"' << endl;
//    else {
//      if (auto pos = value.find("${"); pos != string::npos)
//        value.insert(value.begin() + pos, '\\');
//      os << value << endl;
//    }
//  };
//  for(auto& sec : doc.map) {
//    if(!sec.first.empty())
//      os << endl << '[' << sec.first << ']' << endl;
//    for(auto& key_pair : sec.second)
//      print_key_pair(key_pair);
//  }
//  return os;
//}
//
GLOBAL_NAMESPACE_END
