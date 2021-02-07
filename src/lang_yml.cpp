#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

constexpr const char excluded_chars[] = "\t \"'=;#[](){}:$\\%";
constexpr const char comment_chars[] = ";#";

std::istream& parse_yml(std::istream& is, document& doc, errorlist& err) {
  vector<int> indents{-1};
  vector<addable*> parents{&doc};
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
    int indent = ltrim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      LG_DBUG("Indent: " << indent << " back: " << indents.back() << " result: " << int(indents.back() >= indent))
      while (indents.back() >= indent) {
        indents.pop_back();
        parents.pop_back();
      }
      LG_DBUG("Level: " << indents.size())
      if (!parents.back()) {
        report_err_line("Parent can't take children");
      } else if (auto key = cut_front(line, ':'); !key.untouched()) {
        trim(key);
        if (check_name(key)) {
          trim_quotes(line);
          LG_DBUG("Add")
          try {
            auto node = std::make_unique<document>();
            node->value = node->parse_string(raw, line);
            auto item = parents.back()->add(key, move(node));
            auto newparent = item ? dynamic_cast<addable*>(item->get()) : nullptr;
            parents.push_back(newparent);
            indents.push_back(indent);
          } catch (const std::exception& err) {
            LG_DBUG("parse: key error: " << err.what());
            report_err_line(err.what());
          }
          LG_DBUG("Add done")
        } else report_err_line("Invalid key");
      } else report_err_line("Unparsed line");
    }
  }
  return is;
}

//std::ostream& write_yml(std::ostream& os, const container& doc, const string& prefix) {
//}

GLOBAL_NAMESPACE_END
