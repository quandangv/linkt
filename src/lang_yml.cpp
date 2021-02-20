#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

GLOBAL_NAMESPACE

constexpr const char comment_chars[] = ";#";

struct indentpair {
  string_ref_p* node;
  int indent;
  wrapper* wrp;
  indentpair(int indent, wrapper* wrp) : wrp(wrp), indent(indent), node(nullptr) {}
  indentpair(int indent, string_ref_p* new_node) : indent(indent), node(new_node) {
    wrp = dynamic_cast<wrapper*>(node->get());
    if (wrp)
      node = &wrp->value;
  }
  wrapper& wrap() {
    if (!wrp) {
      wrp = new wrapper();
      if (*node)
        wrp->value = move(*node);
      *node = string_ref_p(wrp);
      node = &wrp->value;
    }
    return *wrp;
  }
};

string_ref_p throw_ref_maker(const tstring&, string_ref_p&&) {
  throw std::invalid_argument("Can't make reference to children");
}

void parse_yml(std::istream& is, wrapper& root, errorlist& err) {
  vector<indentpair> nodes{indentpair(-1, &root)};
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
          auto& parent = nodes.back().wrap();
          local_ref_maker make_parent_ref = [&](tstring& ts, string_ref_p&& fallback) {
            return parent.make_local_ref(ts, move(fallback));
          };
          auto& node = nodes.emplace_back(indent, parent.add(key, string_ref_p{}).get());
          local_ref_maker make_local_ref = [&](tstring& ts, string_ref_p&& fallback) {
            return node.wrap().make_local_ref(ts, move(fallback));
          };
          if (type == ' ') {
            *node.node = parse_string(raw, line, make_local_ref);
          } else if (type == '$') {
            *node.node = parse_ref(raw, line, make_local_ref);
          } else if (type == '^') {
            *node.node = parse_ref(raw, line, make_parent_ref);
          } else {
            err.report_error(linecount, "Invalid character: " + type);
            continue;
          }
        } catch (const std::exception& e) {
          err.report_error(linecount, e.what());
        }
      }
    }
  }
}

void write_yml(std::ostream& os, const container& root, int indent) {
  root.iterate_children([&](const string& name, const base& child) {
    // Indent the line
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
    write_key(os, name + ": ", child.get());
    
    auto ctn = dynamic_cast<const container*>(&child);
    if(ctn)
      write_yml(os, *ctn, indent + 2);
  });
}

GLOBAL_NAMESPACE_END
