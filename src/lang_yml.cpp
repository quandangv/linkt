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
          try {
            auto node = new document();
            node->value = node->parse_string(raw, line);
            parents.back()->add(key, string_ref_p(node));
            parents.push_back(node);
            indents.push_back(indent);
          } catch (const std::exception& err) {
            report_err_line(err.what());
          }
        } else report_err_line("Invalid key");
      } else report_err_line("Unparsed line");
    }
  }
  return is;
}

std::ostream& write_yml(std::ostream& os, const container& doc, int indent) {
  doc.iterate_children([&](const string& name, const string_ref& child) {
    auto value = child.get();
    size_t opening = 0;
    while((opening = value.find("${", opening)) != string::npos) {
      value.insert(value.begin() + opening, '\\');
      opening += 2;
    }

    // Indent the line
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
    if (isspace(value.front()) || isspace(value.back()))
      os << name << ": " << '"' << value << '"' << endl;
    else
      os << name << ": " << value << endl;
    
    auto ctn = dynamic_cast<const container*>(&child);
    if(ctn) {
      write_yml(os, *ctn, indent + 2);
    }
  });
  return os;
}

GLOBAL_NAMESPACE_END
