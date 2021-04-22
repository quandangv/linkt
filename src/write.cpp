#include "write.hpp"
#include "common.hpp"
#include <cstring>
#include <vector>

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

