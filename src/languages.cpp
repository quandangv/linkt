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

void parse_ini(std::istream& is, node::errorlist& err, node::wrapper_s& root) {
  string prefix;
  string raw;
  node::parse_context context;
  context.parent = context.root = root;
  context.parent_based_ref = true;
  // Iterate through lines
  for (int linecount = 1; std::getline(is, context.raw); linecount++, raw.clear()) {
    tstring line(context.raw);
    // skip empty and comment lines
    if (trim(line).empty() || strchr(comment_chars, line.front()))
      continue;
    if (cut_front_back(line, "["_ts, "]"_ts)) {
      // This is a section
      prefix = line + ".";
      continue;
    } if (tstring key; err.extract_key(line, linecount, '=', key)) {
      // This is a key
      context.current_path = prefix + key;
      try {
        root->add(context.current_path, context, line);
      } catch (const std::exception& e) {
        err.report_error(linecount, e.what());
      }
    }
  }
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
  string path;
  indentpair(int indent, node::wrapper_s node, const string& path)
      : indent(indent), node(node), path(path) {}
};

void parse_yml(std::istream& is, node::errorlist& err, node::wrapper_s& root) {
  vector<indentpair> records{indentpair(-1, root, "")};
  string raw;
  node::parse_context context;
  context.root = root;

  // Iterate the lines
  for (int linecount = 1; std::getline(is, context.raw); linecount++, raw.clear()) {
    tstring line(context.raw);
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
      context.parent = records.back().node ?: (records.back().node = context.get_current());
      context.current_path = records.back().path.empty() ? (string)key : records.back().path + "." + key;
      if (line.empty()) {
        // Add an empty node and record it as a possible parent
        records.emplace_back(indent, node::wrapper_s(), context.current_path);
        context.place = &context.parent->add(key, std::make_shared<node::plain<string>>(""));
        continue;
      }
      auto modes = cut_front(line, ' ');
      
      // Assign a new value to an existing node
      if (find(modes, '=') != tstring::npos) {
        auto child = context.parent->get_child_ptr(key);
        if (auto plain = std::dynamic_pointer_cast<node::plain<string>>(child))
          plain->value = line;
        else err.report_error(linecount, key, !child ? "Key to be set doesn't exist." :
            "Can't set value");
        continue;
      }
      context.parent->add(key);
      context.place = context.parent->get_child_place(key);
      context.current.reset();

      if (find(modes, 'H') != tstring::npos) {
        context.get_current()->map[".hidden"] = std::make_shared<node::plain<string>>("true");
      }

      records.emplace_back(indent, nullptr, context.current_path);
      node::base_s value;
      if (find(modes, '$') == tstring::npos)
        value = node::parse_raw<string>(context, line);
      else if (find(modes, 'i') != tstring::npos)
        value = node::parse_escaped<int>(context, line);
      else if (find(modes, 'f') != tstring::npos)
        value = node::parse_escaped<float>(context, line);
      else
        value = node::parse_escaped<string>(context, line);
      if (value)
        context.get_place() = value;

    } catch (const std::exception& e) {
      err.report_error(linecount, key, e.what());
    }
  }
}

void write_yml(std::ostream& os, const node::wrapper_s& root, int indent) {
  root->iterate_children([&](const string& name, const node::base_s& child) {
    if (!child || name.empty() || name[0] == '.') return;
    if(auto ctn = std::dynamic_pointer_cast<node::wrapper>(child)) {
      if (ctn->map[".hidden"]) {
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

void replace_text(std::istream& is, std::ostream& os, node::wrapper_s& replacements) {
  string raw;
  for (int linecount = 1; std::getline(is, raw); linecount++, raw.clear()) {
    tstring ts(raw);
    size_t start, end;
    while(find_enclosed(ts, raw, "${", "{", "}", start, end)) {
      auto expression = ts.interval(start+2, end-1);
      int offset = -1, length = -1;
      if (auto comp = cut_back(expression, ':'); !comp.untouched()) {
        offset = node::parse<int>(comp.begin(), comp.size());
        if (comp = cut_back(expression, ':'); !comp.untouched()) {
          length = offset;
          offset = node::parse<int>(comp.begin(), comp.size());
        }
      }
      auto _str = replacements->get_child_safe(expression);
      if (_str) {
        auto str = *_str;
        if (offset >= 0)
          str = str.substr(offset, length);
        ts.replace(raw, start, end - start, str);
        end = start + str.size();
      }
      ts.erase_front(end);
    }
    os << raw << std::endl;
  }
}
