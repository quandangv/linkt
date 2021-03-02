#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"

#include <vector>
#include <cstring>
#include <iostream>

constexpr const char comment_chars[] = ";#";

// Escape the value and write it and the prefix to the stream
void write_key(std::ostream& os, const string& prefix, string&& value) {
  size_t opening = 0;
  while((opening = value.find("${", opening)) != string::npos) {
    value.insert(value.begin() + opening, '\\');
    opening += 2;
  }
  if (value.empty())
    os << prefix << endl;
  else if (isspace(value.front()) || isspace(value.back()))
    os << prefix << " \"" << value << '"' << endl;
  else
    os << prefix << ' ' << value << endl;
}

void parse_ini(std::istream& is, node::wrapper& root, node::errorlist& err) {
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
        write_key(os, name + " =", move(value));
    } else
      write_key(os, name + " =", child.get());
  });
  for(auto pair : wrappers) {
    os << endl << '[' << prefix << pair.first << ']' << endl;
    write_ini(os, *pair.second);
  }
}

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

void parse_yml(std::istream& is, node::wrapper& root, node::errorlist& err) {
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
      indentpair* record = nullptr;
      auto modes = cut_front(line, ' ');
      
      node::parse_func parser = find(modes, '$') != tstring::npos ?
          node::parse_escaped : node::parse_raw;
      auto rmaker = find(modes, '^') != tstring::npos ?
          (node::ref_maker) [&](tstring& ts, const node::base_p& fallback) {
            // Make references relative to the parent node
            return parent.make_address_ref(ts, fallback);
          } : (node::ref_maker) [&](tstring& ts, const node::base_p& fallback) {
            // Make references relative to the current node
            return record->wrap().make_address_ref(ts, fallback);
          };
      // Assign a new value to an existing node
      if (find(modes, '=') != tstring::npos && !parent.set(key, line)) {
        auto ptr = parent.get_child_place(key);
        err.report_error(linecount, key, !ptr || !*ptr ? "Key to be set doesn't exist." :
            ("Can't set value of type: "s + typeid(**ptr).name()));
      }
      parent.add(key, node::base_p{});
      // Add a new record for the new node
      record = &records.emplace_back(indent, parent.get_child_place(key));
      *record->node = parser(raw, trim_quotes(line), rmaker);
    } catch (const std::exception& e) {
      err.report_error(linecount, key, e.what());
    }
    nextline:;
  }
}

void write_yml(std::ostream& os, const node::wrapper& root, int indent) {
  root.iterate_children([&](const string& name, const node::base& child) {
    // The empty key is used as the value of the wrapper, skip it
    if (name.empty())
      return;
    // Indent the line
    std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
    write_key(os, name + ":", child.get());
    
    auto ctn = dynamic_cast<const node::wrapper*>(&child);
    if(ctn)
      write_yml(os, *ctn, indent + 2);
  });
}
