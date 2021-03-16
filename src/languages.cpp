#include "languages.hpp"
#include "common.hpp"
#include "tstring.hpp"
#include "node/parse.hpp"

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

node::wrapper_s parse_ini(std::istream& is, node::errorlist& err) {
  string prefix;
  string raw;
  auto root = std::make_shared<node::wrapper>();
  node::parse_context context;
  context.parent = root;
  context.parent_based_ref = true;
  // Iterate through lines
  for (int linecount = 1; std::getline(is, context.raw); linecount++, raw.clear()) {
    context.value.set(context.raw);
    // skip empty and comment lines
    if (trim(context.value).empty() || strchr(comment_chars, context.value.front()))
      continue;
    if (cut_front_back(context.value, "["_ts, "]"_ts)) {
      // This is a section
      prefix = context.value + ".";
      continue;
    } if (tstring key; err.extract_key(context.value, linecount, '=', key)) {
      // This is a key
      try {
        root->add(prefix + key, context);
      } catch (const std::exception& e) {
        err.report_error(linecount, e.what());
      }
    }
  }
  return root;
}

void write_ini(std::ostream& os, const node::wrapper_s& root, const string& prefix) {
  vector<std::pair<string, const node::wrapper_s>> wrappers;
  root->iterate_children([&](const string& name, const node::base_s& child) {
    if (!child) return;
    // The empty key is used as the value of the wrapper, skip it
    if (name.empty())
      return;
    auto ctn = std::dynamic_pointer_cast<node::wrapper>(child);
    if(ctn) {
      // The keys with children will be written after the other keys
      // Otherwise, they will break the section
      wrappers.push_back(std::make_pair(name, ctn));
      if (auto value = child->get(); !value.empty())
        write_key(os, name + " =", move(value));
    } else
      write_key(os, name + " =", child->get());
  });
  for(auto pair : wrappers) {
    os << endl << '[' << prefix << pair.first << ']' << endl;
    write_ini(os, pair.second);
  }
}

struct indentpair {
  int indent;
  node::wrapper_s node;
  indentpair(int indent, node::wrapper_s node) : indent(indent), node(node) {}
};

node::wrapper_s parse_yml(std::istream& is, node::errorlist& err) {
  auto root = std::make_shared<node::wrapper>();
  vector<indentpair> records{indentpair{-1, root}};
  string raw;
  node::parse_context context;

  // Iterate the lines
  for (int linecount = 1; std::getline(is, context.raw); linecount++, raw.clear()) {
    context.value.set(context.raw);
    int indent = ltrim(context.value);
    // Skip empty and comment lines
    if (context.value.empty() || strchr(comment_chars, context.value.front()))
      continue;
    // The nodes with larger indentation will be closed
    while (records.back().indent >= indent)
      records.pop_back();
    // Separate the key and the content
    tstring key;
    if (!err.extract_key(context.value, linecount, ':', key))
      continue;

    try {
      context.parent = records.back().node ?: (records.back().node = context.get_current());
      if (context.value.empty()) {
        // Add an empty node and record it as a possible parent
        records.emplace_back(indent, node::wrapper_s());
        context.place = &context.parent->add(key);
        continue;
      }
      auto modes = cut_front(context.value, ' ');
      
      // Assign a new value to an existing node
      if (find(modes, '=') != tstring::npos && !context.parent->set(key, context.value)) {
        auto ptr = context.parent->get_child_place(key);
        err.report_error(linecount, key, !ptr || !*ptr ? "Key to be set doesn't exist." :
            ("Can't set value of type: "s + typeid(*ptr).name()));
        continue;
      }
      context.parent->add(key);
      context.place = context.parent->get_child_place(key);
      context.current.reset();

      if (find(modes, 'H') != tstring::npos) {
        context.get_current()->map[".hidden"] = std::make_shared<node::plain<string>>("true");
        LG_DBUG("HIDDEN");
      }

      records.emplace_back(indent, nullptr);
      auto value = find(modes, '$') != tstring::npos ?
          node::parse_escaped<string>(context) : node::parse_raw<string>(context);
      if (value)
        context.get_place() = value;

    } catch (const std::exception& e) {
      err.report_error(linecount, key, e.what());
    }
  }
  return root;
}

void write_yml(std::ostream& os, const node::wrapper_s& root, int indent) {
  root->iterate_children([&](const string& name, const node::base_s& child) {
    if (!child || name.empty() || name[0] == '.') return;
    if(auto ctn = std::dynamic_pointer_cast<node::wrapper>(child)) {
      if (ctn->map[".hidden"]) {
        LG_DBUG("HIDDEN " << name);
        return;
      }
      std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
      write_key(os, name + ":", child->get());
      write_yml(os, ctn, indent + 2);
    } else {
      std::fill_n(std::ostream_iterator<char>(os), indent, ' ');
      write_key(os, name + ":", child->get());
    }
  });
}
