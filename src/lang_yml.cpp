#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

constexpr const char comment_chars[] = ";#";

void parse_yml(std::istream& is, document& doc, errorlist& err) {
  vector<int> indents{-1};
  vector<addable*> parents{&doc};
  string raw;

  // Iterate through lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    LG_DBUG("parse: line: " << raw);
    tstring line(raw);

    int indent = ltrim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      while (indents.back() >= indent) {
        indents.pop_back();
        parents.pop_back();
      }
      if (tstring key; err.extract_key(line, linecount, ':', key)) {
        try {
          auto node = new document();
          node->value = node->parse_string(raw, line);
          parents.back()->add(key, string_ref_p(node));
          parents.push_back(node);
          indents.push_back(indent);
        } catch (const std::exception& e) {
          err.report_error(linecount, e.what());
        }
      }
    }
  }
}

void write_yml(std::ostream& os, const container& doc, int indent) {
  doc.iterate_children([&](const string& name, const string_ref& child) {
    // Indent the line
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
    write_key(os, name + ": ", child.get());
    
    auto ctn = dynamic_cast<const container*>(&child);
    if(ctn) {
      write_yml(os, *ctn, indent + 2);
    }
  });
}

GLOBAL_NAMESPACE_END
