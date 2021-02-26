#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

NAMESPACE(lini)

constexpr const char comment_chars[] = ";#";

struct indentpair {
  node::base_p* node;
  int indent;
  node::wrapper* wrp;
  indentpair(int indent, node::wrapper* wrp) : wrp(wrp), indent(indent), node(nullptr) {}
  indentpair(int indent, node::base_p* new_node) : indent(indent), node(new_node) {
    wrp = dynamic_cast<node::wrapper*>(node->get());
    if (wrp)
      node = &wrp->value;
  }
  node::wrapper& wrap() {
    if (!wrp) {
      wrp = dynamic_cast<node::wrapper*>(node->get());
      if (!wrp)
        node = &(wrp = &node::wrapper::wrap(*node))->value;
    }
    return *wrp;
  }
};

node::base_p throw_ref_maker(const tstring&, node::base_p&&) {
  throw std::invalid_argument("Can't make reference to children");
}

void parse_yml(std::istream& is, node::wrapper& root, errorlist& err) {
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
        try {
          auto& parent = nodes.back().wrap();
          if (line.empty())
            continue;

          auto type = line.front();
          line.erase_front();
          trim_quotes(line);
          if (type == '=') {
            parent.set(key, line) ? void() : err.report_error(linecount, key, "Can't set value of key");
            LG_DBUG("Set result: " << *parent.get_child(key));
            continue;
          }

          parent.add(key, node::base_p{});
          auto& node = nodes.emplace_back(indent, &parent.get_child_ref(key));
          node::ref_maker make_ref = [&](tstring& ts, const node::base_p& fallback) {
            return node.wrap().make_address_ref(ts, fallback);
          };
          if (*node.node)
            err.report_error(linecount, key, "Key already have value");
          if (type == ' ') {
            *node.node = node::parse_string(raw, line, make_ref);
          } else if (type == '$') {
            *node.node = node::parse(raw, line, make_ref);
          } else if (type == '^') {
            *node.node = node::parse(raw, line, [&](tstring& ts, const node::base_p& fallback) {
              return parent.make_address_ref(ts, fallback);
            });
          } else {
            err.report_error(linecount, key, "Invalid character: " + type);
          }
        } catch (const std::exception& e) {
          err.report_error(linecount, key, e.what());
        }
      }
    }
  }
}

void write_yml(std::ostream& os, const node::wrapper& root, int indent) {
  root.iterate_children([&](const string& name, const node::base& child) {
    // Indent the line
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
    write_key(os, name + ": ", child.get());
    
    auto ctn = dynamic_cast<const node::wrapper*>(&child);
    if(ctn)
      write_yml(os, *ctn, indent + 2);
  });
}

NAMESPACE_END
