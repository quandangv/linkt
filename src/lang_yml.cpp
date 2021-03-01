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
      node = &wrp->add(""_ts);
  }
  // Wrap the current node in a node::wrapper if it hasn't been already. Returns the wrapper
  node::wrapper& wrap() {
    if (!wrp) {
      wrp = dynamic_cast<node::wrapper*>(node->get());
      if (!wrp)
        node = &(wrp = &node::wrapper::wrap(*node))->add(""_ts);
    }
    return *wrp;
  }
};

void parse_yml(std::istream& is, node::wrapper& root, errorlist& err) {
  vector<indentpair> records{indentpair(-1, &root)};
  string raw;

  // Iterate the lines
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    tstring line(raw);
    int indent = ltrim(line);
    // Skip empty and comment lines
    if (line.empty() || strchr(comment_chars, line.front()))
      continue;

    // The nodes with larger indentation will be closed
    while (records.back().indent >= indent)
      records.pop_back();
    // Separate the key and the content
    tstring key;
    if (!err.extract_key(line, linecount, ':', key))
      continue;

    try {
      // The next node with smaller indentation becomes the parent
      auto& parent = records.back().wrap();
      if (line.empty()) {
        // Add an empty node and record it as a possible parent
        records.emplace_back(indent, &parent.add(key));
        continue;
      }

      // The character after the colon determines the parsing mode
      auto mode = line.front();
      line.erase_front();
      trim_quotes(line);
      if (mode == '=') {
        // Assign a new value to an existing node
        parent.set(key, line) ? void() :
            err.report_error(linecount, key, "Can't set value of key");
        continue;
      }

      // We are adding a new node
      parent.add(key, node::base_p{});
      // Add a new record for the new node
      auto& record = records.emplace_back(indent, parent.get_child_place(key));
      node::ref_maker make_ref = [&](tstring& ts, const node::base_p& fallback) {
        // Make references relative to the current node
        return record.wrap().make_address_ref(ts, fallback);
      };
      if (mode == ' ') {
        *record.node = node::parse_string(raw, line, make_ref);
      } else if (mode == '$') {
        *record.node = node::parse(raw, line, make_ref);
      } else if (mode == '^') {
        *record.node = node::parse(raw, line, [&](tstring& ts, const node::base_p& fallback) {
          // Make references relative to the parent node
          return parent.make_address_ref(ts, fallback);
        });
      } else {
        err.report_error(linecount, key, "Invalid parse mode: " + mode);
      }
    } catch (const std::exception& e) {
      err.report_error(linecount, key, e.what());
    }
  }
}

void write_yml(std::ostream& os, const node::wrapper& root, int indent) {
  root.iterate_children([&](const string& name, const node::base& child) {
    // The empty key is used as the value of the wrapper, skip it
    if (name.empty())
      return;
    // Indent the line
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
    write_key(os, name + ": ", child.get());
    
    auto ctn = dynamic_cast<const node::wrapper*>(&child);
    if(ctn)
      write_yml(os, *ctn, indent + 2);
  });
}

NAMESPACE_END
