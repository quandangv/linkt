#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

constexpr const char comment_chars[] = ";#";

struct indentpair {
  int indent;
  addable& node;
  indentpair(int indent, addable& node) : node(node), indent(indent) {}
};

void parse_yml(std::istream& is, document& doc, errorlist& err) {
  vector<indentpair> nodes{indentpair(-1, doc)};
  string raw;

  // Iterate through lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    LG_DBUG("parse: line: " << raw);
    tstring line(raw);

    int indent = ltrim(line);
    // skip empty and comment lines
    if (!line.empty() && !strchr(comment_chars, line.front())) {
      while (nodes.back().indent >= indent)
        nodes.pop_back();
      if (tstring key; err.extract_key(line, linecount, ':', key)) {
        auto type = line.front();
        line.erase_front();
        trim_quotes(line);
        try {
          auto& parent = nodes.back().node;
          document* node;
          auto existing = parent.get_child_ptr(key);
          if (!existing || !*existing) {
            node = new document();
            parent.add(key, string_ref_p(node));
          } else {
            node = dynamic_cast<document*>(existing->get());
            if (!node || node->value) {
              err.report_error(linecount, "Duplicate key");
              continue;
            }
          }
          switch(type) {
          case ' ':
            node->value = node->parse_string(raw, line); break;
          case '$':
            node->value = node->parse_ref(raw, line); break;
          case '^':
            node->value = parent.parse_ref(raw, line); break;
          default:
            err.report_error(linecount, "Colons ':' are followed by a whitespace");
            continue;
          }
          nodes.emplace_back(indent, *node);
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
    if(ctn)
      write_yml(os, *ctn, indent + 2);
  });
}

GLOBAL_NAMESPACE_END
